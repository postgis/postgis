(function () {
  'use strict';

  var resetTimers = new WeakMap();
  var scriptElement = document.currentScript;
  var emptyRefIndex = { functions: {}, operators: {}, keywords: [] };
  var resizeTimer = null;

  // ---------------------------------------------------------------------------
  // Reference-index loading
  // ---------------------------------------------------------------------------

  function refIndexUrl() {
    if (!scriptElement || !scriptElement.src) {
      return null;
    }
    return new URL('postgis-ref-index.json', scriptElement.src).toString();
  }

  function keywordMap(keywords) {
    var map = {};
    if (!Array.isArray(keywords)) {
      return map;
    }
    for (var i = 0; i < keywords.length; i += 1) {
      map[String(keywords[i]).toUpperCase()] = true;
    }
    return map;
  }

  function normalizeRefIndex(refIndex) {
    refIndex = refIndex || emptyRefIndex;
    return {
      functions: refIndex.functions || {},
      operators: refIndex.operators || {},
      keywords: Array.isArray(refIndex.keywords) ? refIndex.keywords : [],
      keywordMap: keywordMap(refIndex.keywords)
    };
  }

  function loadRefIndex() {
    if (window.POSTGIS_REF_INDEX) {
      return Promise.resolve(normalizeRefIndex(window.POSTGIS_REF_INDEX));
    }
    var url = refIndexUrl();
    if (!url || !window.fetch) {
      return Promise.resolve(normalizeRefIndex(emptyRefIndex));
    }
    return window.fetch(url, { credentials: 'same-origin' }).then(function (response) {
      if (!response.ok) {
        return emptyRefIndex;
      }
      return response.json();
    }).catch(function () {
      return emptyRefIndex;
    }).then(normalizeRefIndex);
  }

  // ---------------------------------------------------------------------------
  // SQL tokenizer, highlighting, and reference linking
  // ---------------------------------------------------------------------------

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

  function normalizeDisplayedSql(text) {
    return String(text).replace(/^\r?\n/, '').replace(/\r?\n[ \t\r\n]*$/, '');
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

  function isOperatorCharacter(character) {
    return /[<>=!~@&|+*/%^#-]/.test(character);
  }

  function sqlTokenClass(token, refIndex) {
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
    if (refIndex.keywordMap[upper]) {
      return 'postgis-sql-keyword';
    }
    return '';
  }

  function highlightedToken(token, refIndex, displayedText) {
    var tokenClass = sqlTokenClass(token, refIndex);
    var ref = lookupFunction(token, refIndex);
    displayedText = displayedText === undefined ? token : displayedText;
    if (ref) {
      return linkedToken(displayedText, 'postgis-sql-function', ref);
    }
    ref = lookupOperator(token, refIndex);
    if (ref) {
      return linkedToken(displayedText, 'postgis-sql-operator', ref);
    }
    if (tokenClass) {
      return '<span class="' + tokenClass + '">' + escapeHtml(displayedText) + '</span>';
    }
    return escapeHtml(displayedText);
  }

  function parenToken(segment, text) {
    var classes = 'postgis-sql-paren';
    var attributes = ' data-postgis-paren-depth="' + segment.depth + '"';
    if (segment.pairId !== null) {
      attributes += ' data-postgis-paren-pair="' + segment.pairId + '"';
    } else {
      classes += ' paren-unmatched';
    }
    return '<span class="' + classes + '"' + attributes + '>' + escapeHtml(text) + '</span>';
  }

  function renderLogicalLines(segments) {
    var lines = [''];
    for (var i = 0; i < segments.length; i += 1) {
      var pieces = segments[i].text.split('\n');
      for (var pieceIndex = 0; pieceIndex < pieces.length; pieceIndex += 1) {
        if (pieceIndex > 0) {
          lines.push('');
        }
        if (segments[i].paren) {
          lines[lines.length - 1] += parenToken(segments[i], pieces[pieceIndex]);
        } else if (segments[i].token) {
          lines[lines.length - 1] += highlightedToken(segments[i].text, segments[i].refIndex,
            pieces[pieceIndex]);
        } else {
          lines[lines.length - 1] += escapeHtml(pieces[pieceIndex]);
        }
      }
    }
    return lines.map(function (line) {
      return '<span class="line">' + line + '</span>';
    }).join('\n');
  }

  function highlightSql(text, refIndex) {
    var operators = operatorPattern(refIndex);
    var parts = [
      '--[^\\n\\r]*',
      '\\/\\*[\\s\\S]*?\\*\\/',
      '\\$\\$[\\s\\S]*?\\$\\$',
      "'(?:''|[^'])*'",
      '"(?:""|[^"])*"'
    ];
    if (operators) {
      parts.push(operators);
    }
    parts.push('\\b[A-Za-z_][A-Za-z0-9_]*\\b');
    parts.push('\\b\\d+(?:\\.\\d+)?\\b');
    parts.push('[()]');

    var tokenPattern = new RegExp('(' + parts.join('|') + ')', 'g');
    var segments = [];
    var parenStack = [];
    var nextPairId = 1;
    var lastIndex = 0;
    var match;

    while ((match = tokenPattern.exec(text)) !== null) {
      if (lookupOperator(match[0], refIndex) &&
          (isOperatorCharacter(text.charAt(match.index - 1)) ||
           isOperatorCharacter(text.charAt(tokenPattern.lastIndex)))) {
        continue;
      }
      if (match.index > lastIndex) {
        segments.push({ text: text.slice(lastIndex, match.index) });
      }
      if (match[0] === '(') {
        segments.push({ text: match[0], paren: true, depth: parenStack.length, pairId: null });
        parenStack.push(segments.length - 1);
      } else if (match[0] === ')') {
        var openingIndex = parenStack.pop();
        var closing = { text: match[0], paren: true, depth: parenStack.length, pairId: null };
        if (openingIndex !== undefined) {
          segments[openingIndex].pairId = nextPairId;
          closing.depth = segments[openingIndex].depth;
          closing.pairId = nextPairId;
          nextPairId += 1;
        }
        segments.push(closing);
      } else {
        segments.push({ text: match[0], token: true, refIndex: refIndex });
      }
      lastIndex = tokenPattern.lastIndex;
    }
    if (lastIndex < text.length) {
      segments.push({ text: text.slice(lastIndex) });
    }
    return renderLogicalLines(segments);
  }

  function setLineMetadata(pre, count) {
    pre.setAttribute('data-postgis-lines', 'true');
    if (count > 1) {
      pre.setAttribute('data-postgis-multiline', 'true');
    } else {
      pre.removeAttribute('data-postgis-multiline');
    }
  }

  function wrapPlainText(pre) {
    var text = pre.textContent;
    var lines = text.split('\n');
    pre.innerHTML = lines.map(function (line) {
      return '<span class="line">' + escapeHtml(line) + '</span>';
    }).join('\n');
    setLineMetadata(pre, lines.length);
  }

  function textBoundary(textNodes, offset) {
    var consumed = 0;
    for (var i = 0; i < textNodes.length; i += 1) {
      var length = textNodes[i].nodeValue.length;
      if (offset <= consumed + length) {
        return { node: textNodes[i], offset: offset - consumed };
      }
      consumed += length;
    }
    return { node: textNodes[textNodes.length - 1], offset: textNodes[textNodes.length - 1].nodeValue.length };
  }

  function wrapExistingMarkup(pre) {
    var text = pre.textContent;
    var walker = document.createTreeWalker(pre, 4);
    var textNodes = [];
    var node;
    while ((node = walker.nextNode())) {
      textNodes.push(node);
    }
    if (textNodes.length === 0) {
      wrapPlainText(pre);
      return;
    }

    var fragment = document.createDocumentFragment();
    var starts = [0];
    for (var i = 0; i < text.length; i += 1) {
      if (text.charAt(i) === '\n') {
        starts.push(i + 1);
      }
    }
    for (var lineIndex = 0; lineIndex < starts.length; lineIndex += 1) {
      var end = lineIndex + 1 < starts.length ? starts[lineIndex + 1] - 1 : text.length;
      var startBoundary = textBoundary(textNodes, starts[lineIndex]);
      var endBoundary = textBoundary(textNodes, end);
      var range = document.createRange();
      var line = document.createElement('span');
      line.className = 'line';
      range.setStart(startBoundary.node, startBoundary.offset);
      range.setEnd(endBoundary.node, endBoundary.offset);
      line.appendChild(range.cloneContents());
      fragment.appendChild(line);
      if (lineIndex + 1 < starts.length) {
        fragment.appendChild(document.createTextNode('\n'));
      }
    }
    pre.replaceChildren(fragment);
    setLineMetadata(pre, starts.length);
  }

  function wrapLogicalLines(pre) {
    if (pre.getAttribute('data-postgis-lines') === 'true') {
      return;
    }
    if (pre.children.length === 0) {
      wrapPlainText(pre);
    } else {
      wrapExistingMarkup(pre);
    }
  }

  function applySyntaxHighlighting(refIndex) {
    var blocks;
    var text;
    if (!refIndex.keywords.length) {
      return;
    }
    blocks = document.querySelectorAll('.postgis-example-code pre.programlisting[data-postgis-language="sql"]');
    for (var i = 0; i < blocks.length; i += 1) {
      if (blocks[i].getAttribute('data-postgis-highlighted') === 'sql') {
        continue;
      }
      if (blocks[i].children.length !== 0) {
        continue;
      }
      text = normalizeDisplayedSql(blocks[i].textContent);
      blocks[i].textContent = text;
      blocks[i].innerHTML = highlightSql(text, refIndex);
      setLineMetadata(blocks[i], text.split('\n').length);
      blocks[i].setAttribute('data-postgis-highlighted', 'sql');
    }
  }

  function applyLineWrappers() {
    var blocks = document.querySelectorAll('.postgis-example-block pre.programlisting, ' +
      '.postgis-example-block pre.screen');
    for (var i = 0; i < blocks.length; i += 1) {
      wrapLogicalLines(blocks[i]);
    }
  }

  function updateWrapIndicators() {
    var lines = document.querySelectorAll('.postgis-example-block pre[data-postgis-lines="true"] > .line');
    for (var i = 0; i < lines.length; i += 1) {
      var style = window.getComputedStyle(lines[i]);
      var lineHeight = parseFloat(style.lineHeight);
      var wrapped = lineHeight > 0 && lines[i].scrollHeight > lineHeight * 1.5;
      lines[i].classList.toggle('line-wrapped', wrapped);
    }
  }

  function scheduleWrapIndicatorUpdate() {
    if (window.requestAnimationFrame) {
      window.requestAnimationFrame(updateWrapIndicators);
    } else {
      updateWrapIndicators();
    }
  }

  function handleResize() {
    if (resizeTimer) {
      window.clearTimeout(resizeTimer);
    }
    resizeTimer = window.setTimeout(function () {
      resizeTimer = null;
      updateWrapIndicators();
    }, 120);
  }

  function parenFromEvent(event) {
    return event.target.closest && event.target.closest('.postgis-sql-paren[data-postgis-paren-pair]');
  }

  function setParenMatch(event, matched) {
    var paren = parenFromEvent(event);
    var pre;
    var pairId;
    var pair;
    if (!paren) {
      return;
    }
    pre = paren.closest('pre.programlisting');
    pairId = paren.getAttribute('data-postgis-paren-pair');
    if (!pre || !/^\d+$/.test(pairId)) {
      return;
    }
    pair = pre.querySelectorAll('.postgis-sql-paren[data-postgis-paren-pair="' + pairId + '"]');
    for (var i = 0; i < pair.length; i += 1) {
      pair[i].classList.toggle('paren-match', matched);
    }
  }

  // ---------------------------------------------------------------------------
  // Copy button
  // ---------------------------------------------------------------------------

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
    return normalizeDisplayedSql(clone.textContent);
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

  function handleCopyClick(event) {
    var button = event.target.closest && event.target.closest('.postgis-copy-button');
    var block;
    var pre;
    if (!button) {
      return;
    }

    block = button.closest('.postgis-example-code');
    pre = block && block.querySelector('pre.programlisting');
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
  }

  // ---------------------------------------------------------------------------
  // Bootstrapping
  // ---------------------------------------------------------------------------

  function highlightWhenReady() {
    loadRefIndex().then(function (refIndex) {
      applySyntaxHighlighting(refIndex);
      applyLineWrappers();
      scheduleWrapIndicatorUpdate();
    });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', highlightWhenReady);
  } else {
    highlightWhenReady();
  }

  document.addEventListener('click', handleCopyClick);
  document.addEventListener('mouseover', function (event) { setParenMatch(event, true); });
  document.addEventListener('mouseout', function (event) { setParenMatch(event, false); });
  window.addEventListener('resize', handleResize);
}());
