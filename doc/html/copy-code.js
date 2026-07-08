(function () {
  'use strict';

  var resetTimers = new WeakMap();
  var sqlKeywords = {
    'ADD': true,
    'ALTER': true,
    'AND': true,
    'AS': true,
    'ASC': true,
    'BEGIN': true,
    'BY': true,
    'CASE': true,
    'CAST': true,
    'COMMIT': true,
    'CREATE': true,
    'CROSS': true,
    'DELETE': true,
    'DESC': true,
    'DISTINCT': true,
    'DO': true,
    'DROP': true,
    'ELSE': true,
    'END': true,
    'EXCEPT': true,
    'EXPLAIN': true,
    'FALSE': true,
    'FROM': true,
    'FULL': true,
    'GROUP': true,
    'HAVING': true,
    'IN': true,
    'INNER': true,
    'INSERT': true,
    'INTERSECT': true,
    'INTO': true,
    'IS': true,
    'JOIN': true,
    'LEFT': true,
    'LIMIT': true,
    'NOT': true,
    'NULL': true,
    'OFFSET': true,
    'ON': true,
    'OR': true,
    'ORDER': true,
    'OUTER': true,
    'OVER': true,
    'RETURNING': true,
    'RIGHT': true,
    'SELECT': true,
    'SET': true,
    'THEN': true,
    'TRUE': true,
    'UNION': true,
    'UPDATE': true,
    'USING': true,
    'VALUES': true,
    'WHEN': true,
    'WHERE': true,
    'WITH': true
  };

  function buttonLabel(button, key, fallback) {
    return button.getAttribute(key) || fallback;
  }

  function setButtonState(button, label) {
    button.textContent = label;
    button.setAttribute('aria-label', label);
    button.setAttribute('title', label);
  }

  function resetButtonLater(button) {
    var oldTimer = resetTimers.get(button);
    if (oldTimer) {
      window.clearTimeout(oldTimer);
    }
    var timer = window.setTimeout(function () {
      setButtonState(button, buttonLabel(button, 'data-copy-label', 'Copy'));
      resetTimers.delete(button);
    }, 1800);
    resetTimers.set(button, timer);
  }

  function codeText(pre) {
    var clone = pre.cloneNode(true);
    var ignored = clone.querySelectorAll('.linenumber, .line-number, .ln, .co, .callout, .callout-bug');
    for (var i = 0; i < ignored.length; i += 1) {
      ignored[i].remove();
    }
    return clone.textContent;
  }

  function escapeHtml(text) {
    return text.replace(/[&<>]/g, function (character) {
      return {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;'
      }[character];
    });
  }

  function sqlTokenClass(token) {
    var upper = token.toUpperCase();
    if (token.slice(0, 2) === '--' || token.slice(0, 2) === '/*') {
      return 'postgis-sql-comment';
    }
    if (token.charAt(0) === '\'' || token.charAt(0) === '"' || token.slice(0, 2) === '$$') {
      return 'postgis-sql-string';
    }
    if (/^\d/.test(token)) {
      return 'postgis-sql-number';
    }
    if (/^ST_[A-Z0-9_]+$/i.test(token)) {
      return 'postgis-sql-function';
    }
    if (sqlKeywords[upper]) {
      return 'postgis-sql-keyword';
    }
    return '';
  }

  function highlightSql(text) {
    var tokenPattern = /(--[^\n\r]*|\/\*[\s\S]*?\*\/|\$\$[\s\S]*?\$\$|'(?:''|[^'])*'|"(?:""|[^"])*"|\bST_[A-Za-z0-9_]+\b|\b[A-Za-z_][A-Za-z0-9_]*\b|\b\d+(?:\.\d+)?\b)/g;
    var highlighted = '';
    var lastIndex = 0;
    var match;
    var tokenClass;

    while ((match = tokenPattern.exec(text)) !== null) {
      highlighted += escapeHtml(text.slice(lastIndex, match.index));
      tokenClass = sqlTokenClass(match[0]);
      if (tokenClass) {
        highlighted += '<span class="' + tokenClass + '">' + escapeHtml(match[0]) + '</span>';
      } else {
        highlighted += escapeHtml(match[0]);
      }
      lastIndex = tokenPattern.lastIndex;
    }
    highlighted += escapeHtml(text.slice(lastIndex));
    return highlighted;
  }

  function applySyntaxHighlighting() {
    var blocks = document.querySelectorAll('.postgis-example-code pre.programlisting[data-postgis-language="sql"]');
    for (var i = 0; i < blocks.length; i += 1) {
      if (blocks[i].getAttribute('data-postgis-highlighted') === 'sql') {
        continue;
      }
      if (blocks[i].children.length !== 0) {
        continue;
      }
      blocks[i].innerHTML = highlightSql(blocks[i].textContent);
      blocks[i].setAttribute('data-postgis-highlighted', 'sql');
    }
  }

  function fallbackCopy(text) {
    var textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.setAttribute('readonly', 'readonly');
    textarea.style.position = 'fixed';
    textarea.style.left = '-9999px';
    textarea.style.top = '0';
    document.body.appendChild(textarea);
    textarea.focus();
    textarea.select();
    var copied = false;
    try {
      copied = document.execCommand('copy');
    } finally {
      document.body.removeChild(textarea);
    }
    return copied ? Promise.resolve() : Promise.reject(new Error('copy command failed'));
  }

  function copy(text) {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      return navigator.clipboard.writeText(text).catch(function () {
        return fallbackCopy(text);
      });
    }
    return fallbackCopy(text);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', applySyntaxHighlighting);
  } else {
    applySyntaxHighlighting();
  }

  document.addEventListener('click', function (event) {
    var button = event.target.closest && event.target.closest('.postgis-copy-button');
    if (!button) {
      return;
    }

    var block = button.closest('.postgis-example-code');
    var pre = block && block.querySelector('pre.programlisting');
    if (!pre) {
      return;
    }

    copy(codeText(pre)).then(function () {
      setButtonState(button, buttonLabel(button, 'data-copied-label', 'Copied'));
      resetButtonLater(button);
    }).catch(function () {
      setButtonState(button, buttonLabel(button, 'data-copy-failed-label', 'Failed'));
      resetButtonLater(button);
    });
  });
}());
