(function () {
  'use strict';

  var resetTimers = new WeakMap();
  var scriptElement = document.currentScript;
  var emptyRefIndex = { functions: {}, operators: {} };
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

  function escapeAttribute(text) {
    return String(text).replace(/[&<>"]/g, function (character) {
      return {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;'
      }[character];
    });
  }

  function escapeRegExp(text) {
    return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  }

  function safeHref(href) {
    if (/^#[A-Za-z0-9_.:-]+$/.test(href) || /^[A-Za-z0-9_.:-]+\.html$/.test(href)) {
      return href;
    }
    return '';
  }

  function currentPageId() {
    var filename = window.location.pathname.split('/').pop() || '';
    return filename.replace(/\.html$/, '');
  }

  function refHref(ref) {
    if (ref.id && document.getElementById(ref.id)) {
      return '#' + ref.id;
    }
    return ref.href || '';
  }

  function linkedToken(token, tokenClass, ref) {
    var href = safeHref(refHref(ref));
    var title = ref.title ? ref.label + ': ' + ref.title : ref.label;
    if (!href) {
      return '<span class="' + tokenClass + '">' + escapeHtml(token) + '</span>';
    }
    return '<a class="' + tokenClass + ' postgis-sql-link" href="' + escapeAttribute(href) + '" title="' +
      escapeAttribute(title) + '" aria-label="' + escapeAttribute(title) + '">' + escapeHtml(token) + '</a>';
  }

  function lookupFunction(token, refIndex) {
    return refIndex.functions && refIndex.functions[token.toUpperCase()];
  }

  function lookupOperator(token, refIndex) {
    var entries = refIndex.operators && refIndex.operators[token];
    var pageId = currentPageId();
    var i;
    if (!entries) {
      return null;
    }
    if (!Array.isArray(entries)) {
      return entries;
    }
    for (i = 0; i < entries.length; i += 1) {
      if (entries[i].id === pageId) {
        return entries[i];
      }
    }
    for (i = 0; i < entries.length; i += 1) {
      if (entries[i].id && document.getElementById(entries[i].id)) {
        return entries[i];
      }
    }
    return entries[0];
  }

  function operatorPattern(refIndex) {
    var operators = Object.keys(refIndex.operators || {}).filter(function (operator) {
      return operator.length > 0;
    });
    if (operators.length === 0) {
      return '';
    }
    operators.sort(function (left, right) {
      return right.length - left.length || left.localeCompare(right);
    });
    return operators.map(escapeRegExp).join('|');
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

  function highlightedToken(token, refIndex) {
    var tokenClass = sqlTokenClass(token);
    var ref = lookupFunction(token, refIndex);
    if (ref) {
      return linkedToken(token, 'postgis-sql-function', ref);
    }
    ref = lookupOperator(token, refIndex);
    if (ref) {
      return linkedToken(token, 'postgis-sql-operator', ref);
    }
    if (tokenClass) {
      return '<span class="' + tokenClass + '">' + escapeHtml(token) + '</span>';
    }
    return escapeHtml(token);
  }

  function highlightSql(text, refIndex) {
    var operators = operatorPattern(refIndex);
    var parts = [
      '--[^\\n\\r]*',
      '\\/\\*[\\s\\S]*?\\*\\/',
      '\\$\\$[\\s\\S]*?\\$\\$',
      "\'(?:\'\'|[^\'])*\'",
      '"(?:""|[^"])*"'
    ];
    if (operators) {
      parts.push(operators);
    }
    parts.push('\\b[A-Za-z_][A-Za-z0-9_]*\\b');
    parts.push('\\b\\d+(?:\\.\\d+)?\\b');

    var tokenPattern = new RegExp('(' + parts.join('|') + ')', 'g');
    var highlighted = '';
    var lastIndex = 0;
    var match;

    while ((match = tokenPattern.exec(text)) !== null) {
      highlighted += escapeHtml(text.slice(lastIndex, match.index));
      highlighted += highlightedToken(match[0], refIndex);
      lastIndex = tokenPattern.lastIndex;
    }
    highlighted += escapeHtml(text.slice(lastIndex));
    return highlighted;
  }

  function refIndexUrl() {
    if (!scriptElement || !scriptElement.src) {
      return null;
    }
    return new URL('postgis-ref-index.json', scriptElement.src).toString();
  }

  function loadRefIndex() {
    var url = refIndexUrl();
    if (!url || !window.fetch) {
      return Promise.resolve(emptyRefIndex);
    }
    return window.fetch(url, { credentials: 'same-origin' }).then(function (response) {
      if (!response.ok) {
        return emptyRefIndex;
      }
      return response.json();
    }).catch(function () {
      return emptyRefIndex;
    });
  }

  function applySyntaxHighlighting(refIndex) {
    var blocks = document.querySelectorAll('.postgis-example-code pre.programlisting[data-postgis-language="sql"]');
    for (var i = 0; i < blocks.length; i += 1) {
      if (blocks[i].getAttribute('data-postgis-highlighted') === 'sql') {
        continue;
      }
      if (blocks[i].children.length !== 0) {
        continue;
      }
      blocks[i].innerHTML = highlightSql(blocks[i].textContent, refIndex || emptyRefIndex);
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

  function highlightWhenReady() {
    loadRefIndex().then(applySyntaxHighlighting);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', highlightWhenReady);
  } else {
    highlightWhenReady();
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
