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
  // Inline WKT figures
  // ---------------------------------------------------------------------------

  var geometryTypes = {
    POINT: true,
    LINESTRING: true,
    POLYGON: true,
    MULTIPOINT: true,
    MULTILINESTRING: true,
    MULTIPOLYGON: true,
    GEOMETRYCOLLECTION: true,
    BOX2D: true,
    BOX3D: true
  };
  var geometryPalettes = {
    Code: ['#2878b8', '#59a4d8', '#0f5f9c'],
    Output: ['#d95f3d', '#ef8a47', '#b83b2d'],
    Additional: ['#4a9b62', '#d1a62c']
  };
  var geometrySerial = 0;

  function WktReader(text) {
    this.text = text;
    this.index = 0;
    this.vertices = 0;
    this.coordinateDimensions = null;
  }

  WktReader.prototype.skipSpace = function () {
    while (/\s/.test(this.text.charAt(this.index))) {
      this.index += 1;
    }
  };

  WktReader.prototype.word = function () {
    this.skipSpace();
    var match = /^[A-Za-z][A-Za-z0-9]*/.exec(this.text.slice(this.index));
    if (!match) {
      return '';
    }
    this.index += match[0].length;
    return match[0].toUpperCase();
  };

  WktReader.prototype.take = function (character) {
    this.skipSpace();
    if (this.text.charAt(this.index) !== character) {
      throw new Error('unexpected WKT token');
    }
    this.index += 1;
  };

  WktReader.prototype.number = function () {
    this.skipSpace();
    var match = /^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?/.exec(this.text.slice(this.index));
    if (!match) {
      throw new Error('invalid WKT number');
    }
    this.index += match[0].length;
    var value = Number(match[0]);
    if (!isFinite(value)) {
      throw new Error('non-finite WKT number');
    }
    return value;
  };

  WktReader.prototype.coordinate = function (expectedDimensions) {
    var ordinates = [];
    this.skipSpace();
    while (/[+\-.0-9]/.test(this.text.charAt(this.index))) {
      ordinates.push(this.number());
      this.skipSpace();
    }
    expectedDimensions = expectedDimensions || this.coordinateDimensions;
    if (ordinates.length < 2 || ordinates.length > 4 ||
        (expectedDimensions && ordinates.length !== expectedDimensions)) {
      throw new Error('invalid WKT coordinate');
    }
    if (!this.coordinateDimensions && !expectedDimensions) {
      this.coordinateDimensions = ordinates.length;
    }
    this.vertices += 1;
    if (this.vertices > 2000) {
      throw new Error('WKT vertex limit');
    }
    return [ordinates[0], ordinates[1]];
  };

  WktReader.prototype.coordinateSequence = function () {
    var coordinates = [];
    this.take('(');
    coordinates.push(this.coordinate());
    this.skipSpace();
    while (this.text.charAt(this.index) === ',') {
      this.index += 1;
      coordinates.push(this.coordinate());
      this.skipSpace();
    }
    this.take(')');
    return coordinates;
  };

  WktReader.prototype.sequenceList = function () {
    var sequences = [];
    this.take('(');
    sequences.push(this.coordinateSequence());
    this.skipSpace();
    while (this.text.charAt(this.index) === ',') {
      this.index += 1;
      sequences.push(this.coordinateSequence());
      this.skipSpace();
    }
    this.take(')');
    return sequences;
  };

  WktReader.prototype.geometry = function (allowSrid) {
    this.skipSpace();
    if (allowSrid && this.text.slice(this.index).toUpperCase().indexOf('SRID=') === 0) {
      this.index += 5;
      this.skipSpace();
      if (!/^\d+/.test(this.text.slice(this.index))) {
        throw new Error('invalid SRID');
      }
      this.index += /^\d+/.exec(this.text.slice(this.index))[0].length;
      this.take(';');
    }
    var type = this.word();
    if (!geometryTypes[type]) {
      throw new Error('unsupported WKT type');
    }
    var saved = this.index;
    var inheritedDimensions = this.coordinateDimensions;
    var marker = this.word();
    if (marker !== 'Z' && marker !== 'M' && marker !== 'ZM') {
      this.index = saved;
      marker = '';
    }
    this.coordinateDimensions = marker === 'ZM' ? 4 :
      (marker === 'Z' || marker === 'M' ? 3 : inheritedDimensions);
    saved = this.index;
    if (this.word() === 'EMPTY') {
      throw new Error('empty WKT');
    }
    this.index = saved;

    var primitives = [];
    var i;
    var j;
    var sequences;
    if (type === 'POINT') {
      this.take('(');
      primitives.push({ kind: 'point', points: [this.coordinate()] });
      this.take(')');
    } else if (type === 'LINESTRING') {
      primitives.push({ kind: 'line', points: this.coordinateSequence() });
    } else if (type === 'POLYGON') {
      sequences = this.sequenceList();
      for (i = 0; i < sequences.length; i += 1) {
        primitives.push({ kind: 'ring', points: sequences[i] });
      }
    } else if (type === 'MULTIPOINT') {
      this.take('(');
      this.skipSpace();
      while (true) {
        if (this.text.charAt(this.index) === '(') {
          this.take('(');
          primitives.push({ kind: 'point', points: [this.coordinate()] });
          this.take(')');
        } else {
          primitives.push({ kind: 'point', points: [this.coordinate()] });
        }
        this.skipSpace();
        if (this.text.charAt(this.index) !== ',') {
          break;
        }
        this.index += 1;
      }
      this.take(')');
    } else if (type === 'MULTILINESTRING') {
      sequences = this.sequenceList();
      for (i = 0; i < sequences.length; i += 1) {
        primitives.push({ kind: 'line', points: sequences[i] });
      }
    } else if (type === 'MULTIPOLYGON') {
      this.take('(');
      while (true) {
        sequences = this.sequenceList();
        for (i = 0; i < sequences.length; i += 1) {
          primitives.push({ kind: 'ring', points: sequences[i] });
        }
        this.skipSpace();
        if (this.text.charAt(this.index) !== ',') {
          break;
        }
        this.index += 1;
      }
      this.take(')');
    } else if (type === 'GEOMETRYCOLLECTION') {
      this.take('(');
      while (true) {
        var child = this.geometry(false);
        for (i = 0; i < child.primitives.length; i += 1) {
          primitives.push(child.primitives[i]);
        }
        this.skipSpace();
        if (this.text.charAt(this.index) !== ',') {
          break;
        }
        this.index += 1;
      }
      this.take(')');
    } else {
      this.take('(');
      var boxDimensions = type === 'BOX3D' ? 3 : 2;
      var first = this.coordinate(boxDimensions);
      this.take(',');
      var second = this.coordinate(boxDimensions);
      this.take(')');
      var minX = Math.min(first[0], second[0]);
      var maxX = Math.max(first[0], second[0]);
      var minY = Math.min(first[1], second[1]);
      var maxY = Math.max(first[1], second[1]);
      primitives.push({ kind: 'ring', points: [
        [minX, minY], [maxX, minY], [maxX, maxY], [minX, maxY], [minX, minY]
      ] });
    }
    for (i = 0; i < primitives.length; i += 1) {
      if (primitives[i].kind === 'line' && primitives[i].points.length < 2) {
        throw new Error('short WKT line');
      }
      if (primitives[i].kind === 'ring' &&
          (primitives[i].points.length < 4 ||
           primitives[i].points[0][0] !== primitives[i].points[primitives[i].points.length - 1][0] ||
           primitives[i].points[0][1] !== primitives[i].points[primitives[i].points.length - 1][1])) {
        throw new Error('invalid WKT ring');
      }
      for (j = 0; j < primitives[i].points.length; j += 1) {
        if (!isFinite(primitives[i].points[j][0]) || !isFinite(primitives[i].points[j][1])) {
          throw new Error('invalid WKT ordinate');
        }
      }
    }
    this.coordinateDimensions = inheritedDimensions;
    return { type: type, primitives: primitives, vertices: this.vertices };
  };

  function parseWkt(text) {
    try {
      var reader = new WktReader(String(text).trim());
      var geometry = reader.geometry(true);
      reader.skipSpace();
      if (reader.index !== reader.text.length) {
        return null;
      }
      geometry.wkt = reader.text;
      return geometry;
    } catch (error) {
      return null;
    }
  }

  function closingParenthesis(text, opening) {
    var depth = 0;
    for (var i = opening; i < text.length; i += 1) {
      if (text.charAt(i) === '(') {
        depth += 1;
      } else if (text.charAt(i) === ')') {
        depth -= 1;
        if (depth === 0) {
          return i;
        }
      }
    }
    return -1;
  }

  function quotedRanges(text) {
    var ranges = [];
    for (var i = 0; i < text.length; i += 1) {
      if (text.slice(i, i + 2) === '--') {
        i = text.indexOf('\n', i + 2);
        if (i < 0) {
          break;
        }
        continue;
      }
      if (text.slice(i, i + 2) === '/*') {
        i = text.indexOf('*/', i + 2);
        if (i < 0) {
          break;
        }
        i += 1;
        continue;
      }
      var quote = text.charAt(i);
      if (quote === '$') {
        var delimiterMatch = /^\$(?:[A-Za-z_][A-Za-z0-9_]*)?\$/.exec(text.slice(i));
        if (!delimiterMatch) {
          continue;
        }
        var delimiter = delimiterMatch[0];
        var dollarStart = i + delimiter.length;
        var dollarEnd = text.indexOf(delimiter, dollarStart);
        if (dollarEnd < 0) {
          break;
        }
        ranges.push({ start: dollarStart, end: dollarEnd });
        i = dollarEnd + delimiter.length - 1;
        continue;
      }
      if (quote !== "'") {
        continue;
      }
      var start = i + 1;
      i += 1;
      while (i < text.length) {
        if (text.charAt(i) === quote) {
          if (text.charAt(i + 1) === quote) {
            i += 2;
            continue;
          }
          ranges.push({ start: start, end: i });
          break;
        }
        i += 1;
      }
    }
    return ranges;
  }

  function rangeIsQuoted(ranges, start, end) {
    for (var i = 0; i < ranges.length; i += 1) {
      if (start >= ranges[i].start && end <= ranges[i].end) {
        return true;
      }
    }
    return false;
  }

  function scanGeometryLiterals(text, quotedOnly) {
    var pattern = /(?:SRID\s*=\s*\d+\s*;\s*)?(?:POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION|BOX2D|BOX3D)(?:\s+(?:ZM|Z|M))?\s*(?:EMPTY|\()/ig;
    var matches = [];
    var ranges = quotedOnly ? quotedRanges(text) : [];
    var match;
    while ((match = pattern.exec(text)) !== null) {
      if (match.index > 0 && /[A-Za-z0-9_]/.test(text.charAt(match.index - 1))) {
        continue;
      }
      var opening = text.indexOf('(', match.index);
      if (/EMPTY\s*$/i.test(match[0])) {
        continue;
      }
      var closing = closingParenthesis(text, opening);
      if (closing < 0) {
        continue;
      }
      if (/[A-Za-z0-9_]/.test(text.charAt(closing + 1)) ||
          (quotedOnly && !rangeIsQuoted(ranges, match.index, closing + 1))) {
        pattern.lastIndex = closing + 1;
        continue;
      }
      var literal = text.slice(match.index, closing + 1);
      var geometry = parseWkt(literal);
      if (geometry) {
        matches.push({ start: match.index, end: closing + 1, text: literal, geometry: geometry });
      }
      pattern.lastIndex = closing + 1;
    }
    return matches;
  }

  function geometryExtent(geometries) {
    var extent = { minX: Infinity, minY: Infinity, maxX: -Infinity, maxY: -Infinity };
    for (var i = 0; i < geometries.length; i += 1) {
      var primitives = geometries[i].geometry.primitives;
      for (var j = 0; j < primitives.length; j += 1) {
        for (var k = 0; k < primitives[j].points.length; k += 1) {
          var point = primitives[j].points[k];
          extent.minX = Math.min(extent.minX, point[0]);
          extent.minY = Math.min(extent.minY, point[1]);
          extent.maxX = Math.max(extent.maxX, point[0]);
          extent.maxY = Math.max(extent.maxY, point[1]);
        }
      }
    }
    if (!isFinite(extent.minX)) {
      return null;
    }
    var width = extent.maxX - extent.minX;
    var height = extent.maxY - extent.minY;
    var fallback = Math.max(width, height, 1);
    if (width === 0) {
      extent.minX -= fallback * 0.5;
      extent.maxX += fallback * 0.5;
    }
    if (height === 0) {
      extent.minY -= fallback * 0.5;
      extent.maxY += fallback * 0.5;
    }
    width = extent.maxX - extent.minX;
    height = extent.maxY - extent.minY;
    extent.minX -= width * 0.08;
    extent.maxX += width * 0.08;
    extent.minY -= height * 0.08;
    extent.maxY += height * 0.08;
    return extent;
  }

  function gridStep(span, targetLines) {
    if (!(span > 0) || !(targetLines > 0)) {
      return 1;
    }
    var rough = span / targetLines;
    var power = Math.pow(10, Math.floor(Math.log(rough) / Math.LN10));
    var normalized = rough / power;
    var nice = normalized <= 1 ? 1 : normalized <= 2 ? 2 : normalized <= 5 ? 5 : 10;
    return nice * power;
  }

  function geometryColor(source, sourceIndex) {
    var palette = geometryPalettes[source] || geometryPalettes.Additional;
    if (sourceIndex < palette.length) {
      return palette[sourceIndex];
    }
    return geometryPalettes.Additional[(sourceIndex - palette.length) % geometryPalettes.Additional.length];
  }

  function wrapLiteralRange(pre, match, id) {
    if (!document.createTreeWalker || !document.createElement) {
      return;
    }
    var walker = document.createTreeWalker(pre, 4);
    var nodes = [];
    var node;
    var consumed = 0;
    while ((node = walker.nextNode())) {
      nodes.push({ node: node, start: consumed, end: consumed + node.nodeValue.length });
      consumed += node.nodeValue.length;
    }
    var wrapped = [];
    for (var i = nodes.length - 1; i >= 0; i -= 1) {
      var item = nodes[i];
      var start = Math.max(match.start, item.start);
      var end = Math.min(match.end, item.end);
      if (start >= end) {
        continue;
      }
      var target = item.node;
      if (end < item.end) {
        target.splitText(end - item.start);
      }
      if (start > item.start) {
        target = target.splitText(start - item.start);
      }
      var span = document.createElement('span');
      span.className = 'postgis-geometry-literal';
      span.setAttribute('data-postgis-geometry-id', id);
      target.parentNode.insertBefore(span, target);
      span.appendChild(target);
      wrapped.push(span);
    }
    if (wrapped.length > 0) {
      wrapped[wrapped.length - 1].id = id + '-literal';
    }
  }

  function svgElement(name, attributes) {
    var element = document.createElementNS('http://www.w3.org/2000/svg', name);
    Object.keys(attributes || {}).forEach(function (key) {
      element.setAttribute(key, attributes[key]);
    });
    return element;
  }

  function pointProjector(extent) {
    var plot = { left: 20, top: 12, width: 760, height: 236 };
    var scale = Math.min(plot.width / (extent.maxX - extent.minX), plot.height / (extent.maxY - extent.minY));
    var usedWidth = (extent.maxX - extent.minX) * scale;
    var usedHeight = (extent.maxY - extent.minY) * scale;
    var left = plot.left + (plot.width - usedWidth) / 2;
    var top = plot.top + (plot.height - usedHeight) / 2;
    return function (point) {
      return [left + (point[0] - extent.minX) * scale,
        top + usedHeight - (point[1] - extent.minY) * scale];
    };
  }

  function appendGrid(svg, extent, project) {
    var group = svgElement('g', { 'class': 'postgis-geometry-grid', 'aria-hidden': 'true' });
    var step = gridStep(Math.max(extent.maxX - extent.minX, extent.maxY - extent.minY), 8);
    var value;
    for (value = Math.ceil(extent.minX / step) * step; value <= extent.maxX; value += step) {
      var verticalStart = project([value, extent.minY]);
      var verticalEnd = project([value, extent.maxY]);
      group.appendChild(svgElement('line', { x1: verticalStart[0], y1: verticalStart[1],
        x2: verticalEnd[0], y2: verticalEnd[1] }));
    }
    for (value = Math.ceil(extent.minY / step) * step; value <= extent.maxY; value += step) {
      var horizontalStart = project([extent.minX, value]);
      var horizontalEnd = project([extent.maxX, value]);
      group.appendChild(svgElement('line', { x1: horizontalStart[0], y1: horizontalStart[1],
        x2: horizontalEnd[0], y2: horizontalEnd[1] }));
    }
    svg.appendChild(group);
  }

  function appendGeometry(svg, entry, project) {
    var group = svgElement('g', {
      'class': 'postgis-geometry-shape postgis-geometry-target',
      'data-postgis-geometry-id': entry.id,
      'style': '--postgis-geometry-color:' + entry.color
    });
    var fillData = [];
    for (var i = 0; i < entry.geometry.primitives.length; i += 1) {
      var primitive = entry.geometry.primitives[i];
      var projected = primitive.points.map(project);
      if (primitive.kind === 'point') {
        group.appendChild(svgElement('circle', { 'class': 'postgis-geometry-point',
          cx: projected[0][0], cy: projected[0][1], r: 4 }));
      } else {
        var data = projected.map(function (point, index) {
          return (index === 0 ? 'M' : 'L') + point[0].toFixed(2) + ' ' + point[1].toFixed(2);
        }).join(' ');
        if (primitive.kind === 'ring') {
          fillData.push(data + ' Z');
        } else {
          group.appendChild(svgElement('path', { 'class': 'postgis-geometry-edge', d: data }));
        }
      }
      for (var j = 0; j < projected.length; j += 1) {
        group.appendChild(svgElement('circle', { 'class': 'postgis-geometry-vertex',
          cx: projected[j][0], cy: projected[j][1], r: 2.2 }));
      }
    }
    if (fillData.length > 0) {
      group.insertBefore(svgElement('path', { 'class': 'postgis-geometry-edge postgis-geometry-fill',
        d: fillData.join(' '), 'fill-rule': 'evenodd' }), group.firstChild);
    }
    svg.appendChild(group);
  }

  function legendHead(text) {
    var compact = text.replace(/\s+/g, ' ').trim();
    return compact.length <= 44 ? compact : compact.slice(0, 42) + ' …';
  }

  function createGeometryFigure(entries, extent) {
    var figure = document.createElement('figure');
    figure.className = 'postgis-geometry-figure';
    figure.setAttribute('aria-label', 'Geometry figure');
    var toggle = document.createElement('button');
    toggle.type = 'button';
    toggle.className = 'postgis-geometry-toggle';
    toggle.textContent = 'hide figure';
    toggle.setAttribute('aria-expanded', 'true');
    figure.appendChild(toggle);
    var body = document.createElement('div');
    body.className = 'postgis-geometry-figure-body';
    body.id = 'postgis-geometry-figure-' + entries[0].id.replace(/\D/g, '');
    toggle.setAttribute('aria-controls', body.id);
    var svg = svgElement('svg', { viewBox: '0 0 800 260', role: 'img',
      'aria-label': 'Shared XY projection of example geometries' });
    var project = pointProjector(extent);
    appendGrid(svg, extent, project);
    for (var i = 0; i < entries.length; i += 1) {
      appendGeometry(svg, entries[i], project);
    }
    body.appendChild(svg);
    var legend = document.createElement('figcaption');
    legend.className = 'postgis-geometry-legend';
    for (i = 0; i < entries.length; i += 1) {
      var row = document.createElement('span');
      row.className = 'postgis-geometry-legend-row postgis-geometry-target';
      row.setAttribute('data-postgis-geometry-id', entries[i].id);
      var chip = document.createElement('span');
      chip.className = 'postgis-geometry-chip';
      chip.style.backgroundColor = entries[i].color;
      row.appendChild(chip);
      row.appendChild(document.createTextNode(legendHead(entries[i].text) + ' — ' + entries[i].source));
      legend.appendChild(row);
    }
    body.appendChild(legend);
    figure.appendChild(body);
    return figure;
  }

  function scanBlock(pre, source, sourceCounts, entries) {
    var matches = scanGeometryLiterals(pre.textContent, source === 'Code');
    for (var i = 0; i < matches.length; i += 1) {
      geometrySerial += 1;
      var id = 'postgis-geometry-' + geometrySerial;
      var sourceIndex = sourceCounts[source] || 0;
      sourceCounts[source] = sourceIndex + 1;
      var entry = {
        id: id,
        source: source,
        text: matches[i].text,
        geometry: matches[i].geometry,
        color: geometryColor(source, sourceIndex)
      };
      entries.push(entry);
      entry.pre = pre;
      entry.match = matches[i];
    }
  }

  function applyGeometryFigures() {
    if (!document.createElementNS || !document.createElement) {
      return;
    }
    var codeBlocks = document.querySelectorAll('.postgis-example-code');
    for (var i = 0; i < codeBlocks.length; i += 1) {
      var code = codeBlocks[i];
      if (code.getAttribute('data-postgis-geometry-scanned') === 'true') {
        continue;
      }
      code.setAttribute('data-postgis-geometry-scanned', 'true');
      var entries = [];
      var sourceCounts = {};
      var codePre = code.querySelector('pre.programlisting');
      if (codePre) {
        scanBlock(codePre, 'Code', sourceCounts, entries);
      }
      var last = code;
      var next = code.nextElementSibling;
      while (next && next.matches && next.matches('.postgis-example-output')) {
        var outputPre = next.querySelector('pre.screen');
        if (outputPre) {
          scanBlock(outputPre, 'Output', sourceCounts, entries);
        }
        last = next;
        next = next.nextElementSibling;
      }
      if (entries.length === 0 || (entries.length === 1 && entries[0].geometry.type === 'POINT')) {
        continue;
      }
      var extent = geometryExtent(entries);
      if (extent) {
        for (var entryIndex = 0; entryIndex < entries.length; entryIndex += 1) {
          wrapLiteralRange(entries[entryIndex].pre, entries[entryIndex].match, entries[entryIndex].id);
        }
        last.parentNode.insertBefore(createGeometryFigure(entries, extent), last.nextSibling);
      }
    }
  }

  function setGeometryHighlight(event, highlighted) {
    var target = event.target.closest && event.target.closest('.postgis-geometry-target');
    if (!target) {
      return;
    }
    if (event.relatedTarget && target.contains && target.contains(event.relatedTarget)) {
      return;
    }
    var id = target.getAttribute('data-postgis-geometry-id');
    if (!/^postgis-geometry-\d+$/.test(id)) {
      return;
    }
    var related = document.querySelectorAll('[data-postgis-geometry-id="' + id + '"]');
    for (var i = 0; i < related.length; i += 1) {
      related[i].classList.toggle('postgis-geometry-active', highlighted);
    }
  }

  function handleFigureToggle(event) {
    var button = event.target.closest && event.target.closest('.postgis-geometry-toggle');
    if (!button) {
      return false;
    }
    var figure = button.closest('.postgis-geometry-figure');
    var collapsed = !figure.classList.contains('postgis-geometry-collapsed');
    figure.classList.toggle('postgis-geometry-collapsed', collapsed);
    button.textContent = collapsed ? 'show figure' : 'hide figure';
    button.setAttribute('aria-expanded', collapsed ? 'false' : 'true');
    return true;
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
    if (handleFigureToggle(event)) {
      return;
    }
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
      applyGeometryFigures();
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
  document.addEventListener('mouseover', function (event) { setGeometryHighlight(event, true); });
  document.addEventListener('mouseout', function (event) { setGeometryHighlight(event, false); });
  window.addEventListener('resize', handleResize);

  window.POSTGIS_EXAMPLE_BLOCKS_TEST = {
    parseWkt: parseWkt,
    scanGeometryLiterals: scanGeometryLiterals,
    geometryExtent: geometryExtent,
    gridStep: gridStep,
    legendHead: legendHead,
    wrapLiteralRange: wrapLiteralRange,
    codeText: codeText
  };
}());
