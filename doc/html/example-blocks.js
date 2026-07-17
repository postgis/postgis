(function () {
  'use strict';

  var resetTimers = new WeakMap();
  var geometryDeactivateTimers = new Map();
  var originalBlockText = new WeakMap();
  var scriptElement = document.currentScript;
  var emptyRefIndex = { commands: {}, functions: {}, operators: {}, keywords: [] };
  var resizeTimer = null;
  var wrapObserver = null;
  var numberPattern = '[-+]?(?:\\d+(?:\\.\\d*)?|\\.\\d+)(?:[eE][-+]?\\d+)?';
  var makeEnvelopePattern = new RegExp(
    '\\bST_MakeEnvelope\\s*\\(\\s*' + numberPattern + '\\s*,\\s*' +
    numberPattern + '\\s*,\\s*' + numberPattern + '\\s*,\\s*' + numberPattern +
    '(?:\\s*,\\s*-?\\d+)?\\s*\\)', 'gi'
  );

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
      commands: refIndex.commands || {},
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
    if (/^#[A-Za-z0-9_.:-]+$/.test(href) ||
        /^[A-Za-z0-9_.:-]+\.html(?:#[A-Za-z0-9_.:-]+)?$/.test(href)) {
      return href;
    }
    return '';
  }

  function currentPageId() {
    var pathname = window.location && window.location.pathname || '';
    var filename = pathname.split('/').pop() || '';
    try {
      filename = decodeURIComponent(filename);
    } catch (error) {
      // Keep the literal basename when the URL contains invalid escapes.
    }
    return filename.replace(/\.html$/i, '');
  }

  function indexedFunctionName(candidate, refIndex) {
    var upper = String(candidate || '').toUpperCase();
    var names;
    if (!upper) {
      return null;
    }
    names = Object.keys(refIndex.functions || {});
    for (var i = 0; i < names.length; i += 1) {
      if (names[i].toUpperCase() === upper) {
        return names[i];
      }
    }
    return null;
  }

  function currentFunctionName(refIndex) {
    var name = indexedFunctionName(currentPageId(), refIndex);
    var refname;
    var match;
    if (name) {
      return name;
    }
    refname = document.querySelector('.refnamediv .refname, .refnamediv p');
    match = refname && /^\s*([A-Za-z_][A-Za-z0-9_]*)/.exec(refname.textContent);
    return match ? indexedFunctionName(match[1], refIndex) : null;
  }

  function refHref(ref) {
    if (ref.id && document.getElementById(ref.id)) {
      return '#' + ref.id;
    }
    return ref.href || '';
  }

  function linkedToken(token, tokenClass, ref, linkClass) {
    var href = safeHref(refHref(ref));
    var title = ref.title ? ref.label + ': ' + ref.title : ref.label;
    linkClass = linkClass || 'postgis-sql-link';
    if (!href) {
      return '<span class="' + tokenClass + '">' + escapeHtml(token) + '</span>';
    }
    return '<a class="' + tokenClass + ' ' + linkClass + '" href="' + escapeAttribute(href) + '" title="' +
      escapeAttribute(title) + '" aria-label="' + escapeAttribute(title) + '">' + escapeHtml(token) + '</a>';
  }

  function lookupFunction(token, refIndex) {
    return refIndex.functions && refIndex.functions[token.toUpperCase()];
  }

  function lookupCommand(token, refIndex) {
    var basename = token.split('/').pop();
    return refIndex.commands && refIndex.commands[basename];
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
    if (token.charAt(0) === '\'' || token.charAt(0) === '"' ||
        (token.charAt(0) === '$' && dollarQuoteDelimiter(token))) {
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
      if (refIndex.currentFunction &&
          token.toUpperCase() === refIndex.currentFunction.toUpperCase()) {
        return '<span class="current-fn">' + escapeHtml(displayedText) + '</span>';
      }
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

  function dollarQuoteDelimiter(token) {
    var match = /^\$[A-Za-z_][A-Za-z0-9_]*\$|^\$\$/.exec(token);
    return match ? match[0] : null;
  }

  function isPlpgsqlDollarQuote(text, startIndex, endIndex) {
    var prefix = text.slice(0, startIndex);
    var suffix = text.slice(endIndex);
    var statementStart = Math.max(prefix.lastIndexOf(';'), prefix.lastIndexOf('\n/\n'));
    var statementPrefix = prefix.slice(statementStart + 1);

    if (/\bDO\s*(?:LANGUAGE\s+['"]?plpgsql['"]?)?\s*$/i.test(statementPrefix)) {
      return true;
    }
    if (/\bAS\s*$/i.test(statementPrefix) &&
        /^\s*LANGUAGE\s+['"]?plpgsql['"]?/i.test(suffix)) {
      return true;
    }
    return false;
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
    if (lines.length > 1 && lines[lines.length - 1] === '' &&
        segments.length > 0 && /\n$/.test(segments[segments.length - 1].text)) {
      lines.pop();
    }
    return lines.map(function (line) {
      return '<span class="line">' + line + '</span>';
    }).join('');
  }

  function sqlSegments(text, refIndex) {
    var operators = operatorPattern(refIndex);
    var parts = [
      '--[^\\n\\r]*',
      '\\/\\*[\\s\\S]*?\\*\\/',
      '\\$\\$[\\s\\S]*?\\$\\$',
      '\\$[A-Za-z_][A-Za-z0-9_]*\\$[\\s\\S]*?\\$[A-Za-z_][A-Za-z0-9_]*\\$',
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
      } else if (match[0].charAt(0) === '$' &&
          isPlpgsqlDollarQuote(text, match.index, tokenPattern.lastIndex)) {
        var delimiter = dollarQuoteDelimiter(match[0]);
        if (delimiter && match[0].slice(-delimiter.length) === delimiter) {
          segments.push({ text: delimiter, token: true, refIndex: refIndex });
          segments = segments.concat(sqlSegments(
            match[0].slice(delimiter.length, -delimiter.length), refIndex));
          segments.push({ text: delimiter, token: true, refIndex: refIndex });
        } else {
          segments.push({ text: match[0], token: true, refIndex: refIndex });
        }
      } else {
        segments.push({ text: match[0], token: true, refIndex: refIndex });
      }
      lastIndex = tokenPattern.lastIndex;
    }
    if (lastIndex < text.length) {
      segments.push({ text: text.slice(lastIndex) });
    }
    return segments;
  }

  function highlightSql(text, refIndex) {
    var segments = sqlSegments(text, refIndex);
    return renderLogicalLines(segments);
  }

  // ---------------------------------------------------------------------------
  // HTML and JavaScript tokenizers
  // ---------------------------------------------------------------------------

  function renderSourceLines(segments) {
    var lines = [''];
    for (var i = 0; i < segments.length; i += 1) {
      var pieces = segments[i].text.split('\n');
      for (var pieceIndex = 0; pieceIndex < pieces.length; pieceIndex += 1) {
        if (pieceIndex > 0) {
          lines.push('');
        }
        var piece = escapeHtml(pieces[pieceIndex]);
        if (segments[i].tokenClass && piece !== '') {
          piece = '<span class="' + segments[i].tokenClass + '">' + piece + '</span>';
        }
        lines[lines.length - 1] += piece;
      }
    }
    if (lines.length > 1 && lines[lines.length - 1] === '' &&
        segments.length > 0 && /\n$/.test(segments[segments.length - 1].text)) {
      lines.pop();
    }
    return lines.map(function (line) {
      return '<span class="line">' + line + '</span>';
    }).join('');
  }

  var javascriptKeywords = {
    'async': true, 'await': true, 'break': true, 'case': true, 'catch': true,
    'class': true, 'const': true, 'continue': true, 'debugger': true,
    'default': true, 'delete': true, 'do': true, 'else': true, 'export': true,
    'extends': true, 'finally': true, 'for': true, 'from': true, 'function': true,
    'get': true, 'if': true, 'import': true, 'in': true, 'instanceof': true,
    'let': true, 'new': true, 'of': true, 'return': true, 'set': true,
    'static': true, 'super': true, 'switch': true, 'this': true, 'throw': true,
    'try': true, 'typeof': true, 'var': true, 'void': true, 'while': true,
    'with': true, 'yield': true
  };

  var javascriptLiterals = {
    'false': true, 'Infinity': true, 'NaN': true, 'null': true,
    'true': true, 'undefined': true
  };

  function javascriptSegments(text) {
    var tokenPattern = /(\/\*[\s\S]*?\*\/|\/\/[^\n\r]*|`(?:\\[\s\S]|[^`])*`|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|\b\d+(?:\.\d+)?\b|\b[A-Za-z_$][A-Za-z0-9_$]*\b|===|!==|=>|==|!=|<=|>=|\+\+|--|&&|\|\||\?\?|\+=|-=|\*=|\/=|%=|[+*\/%<>=!&|?:-])/g;
    var segments = [];
    var lastIndex = 0;
    var match;
    while ((match = tokenPattern.exec(text)) !== null) {
      if (match.index > lastIndex) {
        segments.push({ text: text.slice(lastIndex, match.index) });
      }
      var token = match[0];
      var tokenClass = '';
      if (token.slice(0, 2) === '//' || token.slice(0, 2) === '/*') {
        tokenClass = 'postgis-js-comment';
      } else if (/^["'`]/.test(token)) {
        tokenClass = 'postgis-js-string';
      } else if (/^\d/.test(token)) {
        tokenClass = 'postgis-js-number';
      } else if (javascriptKeywords[token]) {
        tokenClass = 'postgis-js-keyword';
      } else if (javascriptLiterals[token]) {
        tokenClass = 'postgis-js-literal';
      } else if (/^[A-Za-z_$]/.test(token) && /^\s*\(/.test(text.slice(tokenPattern.lastIndex))) {
        tokenClass = 'postgis-js-function';
      } else if (!/^[A-Za-z_$]/.test(token)) {
        tokenClass = 'postgis-js-operator';
      }
      segments.push({ text: token, tokenClass: tokenClass });
      lastIndex = tokenPattern.lastIndex;
    }
    if (lastIndex < text.length) {
      segments.push({ text: text.slice(lastIndex) });
    }
    return segments;
  }

  function highlightJavaScript(text) {
    return renderSourceLines(javascriptSegments(text));
  }

  function htmlTagEnd(text, start) {
    var quote = '';
    for (var i = start + 1; i < text.length; i += 1) {
      var character = text.charAt(i);
      if (quote) {
        if (character === quote && text.charAt(i - 1) !== '\\') {
          quote = '';
        }
      } else if (character === '"' || character === "'") {
        quote = character;
      } else if (character === '>') {
        return i + 1;
      }
    }
    return -1;
  }

  function htmlTagSegments(tag) {
    if (tag.slice(0, 4) === '<!--') {
      return [{ text: tag, tokenClass: 'postgis-html-comment' }];
    }
    var match = /^(<\/?)([A-Za-z][A-Za-z0-9:.-]*)([\s\S]*?)(\/?>)$/.exec(tag);
    if (!match) {
      return [{ text: tag }];
    }
    var segments = [
      { text: match[1], tokenClass: 'postgis-html-punctuation' },
      { text: match[2], tokenClass: 'postgis-html-tag' }
    ];
    var attributes = match[3];
    var attributePattern = /(\s+)|([A-Za-z_:][A-Za-z0-9_.:-]*)(?=\s*=)|("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|(=)|([^\s]+)/g;
    var lastIndex = 0;
    var attribute;
    while ((attribute = attributePattern.exec(attributes)) !== null) {
      if (attribute.index > lastIndex) {
        segments.push({ text: attributes.slice(lastIndex, attribute.index) });
      }
      var tokenClass = '';
      if (attribute[2]) {
        tokenClass = 'postgis-html-attribute';
      } else if (attribute[3]) {
        tokenClass = 'postgis-html-string';
      } else if (attribute[4]) {
        tokenClass = 'postgis-html-punctuation';
      }
      segments.push({ text: attribute[0], tokenClass: tokenClass });
      lastIndex = attributePattern.lastIndex;
    }
    if (lastIndex < attributes.length) {
      segments.push({ text: attributes.slice(lastIndex) });
    }
    segments.push({ text: match[4], tokenClass: 'postgis-html-punctuation' });
    return segments;
  }

  function highlightHtml(text) {
    var segments = [];
    var lower = text.toLowerCase();
    var index = 0;
    var inScript = false;
    while (index < text.length) {
      if (inScript) {
        var closingScript = lower.indexOf('</script', index);
        if (closingScript === -1) {
          segments = segments.concat(javascriptSegments(text.slice(index)));
          break;
        }
        segments = segments.concat(javascriptSegments(text.slice(index, closingScript)));
        index = closingScript;
      }

      var opening = text.indexOf('<', index);
      if (opening === -1) {
        segments.push({ text: text.slice(index) });
        break;
      }
      if (opening > index) {
        segments.push({ text: text.slice(index, opening) });
      }
      if (text.slice(opening, opening + 4) === '<!--') {
        var commentEnd = text.indexOf('-->', opening + 4);
        commentEnd = commentEnd === -1 ? text.length : commentEnd + 3;
        segments.push({ text: text.slice(opening, commentEnd), tokenClass: 'postgis-html-comment' });
        index = commentEnd;
        continue;
      }
      if (!/^<\/?[A-Za-z]/.test(text.slice(opening))) {
        segments.push({ text: '<' });
        index = opening + 1;
        continue;
      }
      var end = htmlTagEnd(text, opening);
      if (end === -1) {
        segments.push({ text: text.slice(opening) });
        break;
      }
      var tag = text.slice(opening, end);
      segments = segments.concat(htmlTagSegments(tag));
      if (/^<script\b/i.test(tag)) {
        inScript = true;
      } else if (/^<\/script\b/i.test(tag)) {
        inScript = false;
      }
      index = end;
    }
    return renderSourceLines(segments);
  }

  // ---------------------------------------------------------------------------
  // Shell tokenizer
  // ---------------------------------------------------------------------------

  var shellKeywords = {
    'case': true, 'do': true, 'done': true, 'elif': true, 'else': true,
    'esac': true, 'fi': true, 'for': true, 'function': true, 'if': true,
    'in': true, 'select': true, 'then': true, 'time': true, 'until': true,
    'while': true
  };
  var shellCommandPrefixes = {
    'do': true, 'elif': true, 'else': true, 'if': true, 'then': true,
    'time': true, 'until': true, 'while': true
  };

  function shellToken(tokenClass, text) {
    if (!tokenClass) {
      return escapeHtml(text);
    }
    return '<span class="postgis-shell-' + tokenClass + '">' + escapeHtml(text) + '</span>';
  }

  function highlightShellLine(line, refIndex) {
    var html = '';
    var index = 0;
    var commandExpected = true;
    var assignmentValue = false;
    var redirectionExpected = false;
    var atWordStart = true;
    var inBacktick = false;
    var savedAssignmentValue = false;
    var savedCommandExpected = true;

    while (index < line.length) {
      var character = line.charAt(index);
      var remainder = line.slice(index);
      var match;
      var end;
      var lower;
      var tokenClass = '';

      if (/\s/.test(character)) {
        html += escapeHtml(character);
        index += 1;
        atWordStart = true;
        if (!inBacktick) {
          assignmentValue = false;
        }
        continue;
      }

      if (character === '#' && atWordStart) {
        html += shellToken('comment', remainder);
        break;
      }

      if (character === '\'' || character === '"') {
        end = index + 1;
        while (end < line.length) {
          if (character === '"' && line.charAt(end) === '\\') {
            end += 2;
            continue;
          }
          if (line.charAt(end) === character) {
            end += 1;
            break;
          }
          end += 1;
        }
        html += shellToken('string', line.slice(index, end));
        index = end;
        atWordStart = false;
        if (redirectionExpected) {
          redirectionExpected = false;
        } else if (!assignmentValue) {
          commandExpected = false;
        }
        continue;
      }

      if (character === '`') {
        html += shellToken('operator', character);
        if (inBacktick) {
          inBacktick = false;
          assignmentValue = savedAssignmentValue;
          commandExpected = savedCommandExpected;
          atWordStart = false;
        } else {
          inBacktick = true;
          savedAssignmentValue = assignmentValue;
          savedCommandExpected = commandExpected;
          assignmentValue = false;
          commandExpected = true;
          atWordStart = true;
        }
        index += 1;
        continue;
      }

      if (character === '$') {
        match = /^(\$\{[^}\n]*\}|\$[A-Za-z_][A-Za-z0-9_]*|\$[0-9@*#?$!_-])/.exec(remainder);
        if (match) {
          html += shellToken('variable', match[0]);
          index += match[0].length;
          atWordStart = false;
          if (redirectionExpected) {
            redirectionExpected = false;
          }
          continue;
        }
      }

      if (commandExpected && !assignmentValue && !redirectionExpected && atWordStart) {
        match = /^([A-Za-z_][A-Za-z0-9_]*)(\+?=)/.exec(remainder);
        if (match) {
          html += shellToken('variable', match[1]);
          html += shellToken('operator', match[2]);
          index += match[0].length;
          assignmentValue = true;
          atWordStart = false;
          continue;
        }
      }

      match = /^(\d*(?:<<-?|>>|<>|>&|<&|[<>])|&&|\|\||[|;&()])/.exec(remainder);
      if (match) {
        html += shellToken('operator', match[0]);
        index += match[0].length;
        if (/^\d*(?:<<-?|>>|<>|>&|<&|[<>])$/.test(match[0])) {
          redirectionExpected = true;
        } else {
          commandExpected = match[0] !== ')';
          redirectionExpected = false;
        }
        assignmentValue = false;
        atWordStart = true;
        continue;
      }

      end = index;
      while (end < line.length && !/[\s#'"`$|&;()<>]/.test(line.charAt(end))) {
        end += 1;
      }
      if (end === index) {
        html += escapeHtml(character);
        index += 1;
        atWordStart = false;
        continue;
      }

      match = [line.slice(index, end)];
      lower = match[0].toLowerCase();
      if (redirectionExpected) {
        redirectionExpected = false;
      } else if (shellKeywords[lower]) {
        tokenClass = 'keyword';
        commandExpected = Boolean(shellCommandPrefixes[lower]);
      } else if (!commandExpected && /^--?[A-Za-z0-9]/.test(match[0])) {
        tokenClass = 'option';
      } else if (commandExpected && !assignmentValue) {
        tokenClass = 'command';
        commandExpected = false;
      }
      var commandRef = tokenClass === 'command' ? lookupCommand(match[0], refIndex) : null;
      if (commandRef) {
        html += linkedToken(match[0], 'postgis-shell-command', commandRef, 'postgis-shell-link');
      } else {
        html += shellToken(tokenClass, match[0]);
      }
      index = end;
      atWordStart = false;
    }
    return html;
  }

  function highlightShell(text, refIndex) {
    refIndex = refIndex || normalizeRefIndex(emptyRefIndex);
    var lines = text.split('\n');
    if (lines.length > 1 && lines[lines.length - 1] === '' && /\n$/.test(text)) {
      lines.pop();
    }
    return lines.map(function (line) {
      return '<span class="line">' + highlightShellLine(line, refIndex) + '</span>';
    }).join('');
  }

  var batchKeywords = {
    'call': true, 'do': true, 'echo': true, 'else': true, 'exit': true,
    'for': true, 'goto': true, 'if': true, 'in': true, 'not': true,
    'pause': true, 'rem': true, 'set': true, 'shift': true
  };
  var batchCommandPrefixes = {
    'do': true, 'else': true, 'if': true
  };

  function highlightBatchLine(line, refIndex) {
    var html = '';
    var index = 0;
    var commandExpected = true;
    var redirectionExpected = false;
    var atWordStart = true;
    var match;
    var lower;
    var end;
    var tokenClass;

    if (/^\s*(?:rem\b|::)/i.test(line)) {
      return shellToken('comment', line);
    }

    while (index < line.length) {
      var character = line.charAt(index);
      var remainder = line.slice(index);

      if (/\s/.test(character)) {
        html += escapeHtml(character);
        index += 1;
        atWordStart = true;
        continue;
      }

      if (character === '"') {
        end = index + 1;
        while (end < line.length) {
          if (line.charAt(end) === '"') {
            end += 1;
            break;
          }
          end += 1;
        }
        html += shellToken('string', line.slice(index, end));
        index = end;
        atWordStart = false;
        if (redirectionExpected) {
          redirectionExpected = false;
        } else {
          commandExpected = false;
        }
        continue;
      }

      if (character === '%') {
        match = /^(%%?~[A-Za-z]+[A-Za-z0-9]|%%?[A-Za-z0-9]|%%?[A-Za-z0-9_]+%%?|%[A-Za-z0-9_]+%)/.exec(remainder);
        if (match) {
          html += shellToken('variable', match[0]);
          index += match[0].length;
          atWordStart = false;
          if (redirectionExpected) {
            redirectionExpected = false;
          }
          continue;
        }
      }

      match = /^(\d*(?:>>|>&|<&|[<>])|&&|\|\||[|&()])/.exec(remainder);
      if (match) {
        html += shellToken('operator', match[0]);
        index += match[0].length;
        if (/^\d*(?:>>|>&|<&|[<>])$/.test(match[0])) {
          redirectionExpected = true;
        } else {
          commandExpected = match[0] !== ')';
          redirectionExpected = false;
        }
        atWordStart = true;
        continue;
      }

      end = index;
      while (end < line.length && !/[\s"%|&()<>]/.test(line.charAt(end))) {
        end += 1;
      }
      if (end === index) {
        html += escapeHtml(character);
        index += 1;
        atWordStart = false;
        continue;
      }

      match = [line.slice(index, end)];
      lower = match[0].toLowerCase();
      tokenClass = '';
      if (redirectionExpected) {
        redirectionExpected = false;
      } else if (batchKeywords[lower]) {
        tokenClass = 'keyword';
        commandExpected = Boolean(batchCommandPrefixes[lower]);
      } else if (!commandExpected && /^[-/][A-Za-z0-9]/.test(match[0])) {
        tokenClass = 'option';
      } else if (commandExpected && atWordStart) {
        tokenClass = 'command';
        commandExpected = false;
      }

      var commandRef = tokenClass === 'command' ? lookupCommand(match[0], refIndex) : null;
      if (commandRef) {
        html += linkedToken(match[0], 'postgis-shell-command', commandRef, 'postgis-shell-link');
      } else {
        html += shellToken(tokenClass, match[0]);
      }
      index = end;
      atWordStart = false;
    }
    return html;
  }

  function highlightBatch(text, refIndex) {
    refIndex = refIndex || normalizeRefIndex(emptyRefIndex);
    var lines = text.split('\n');
    if (lines.length > 1 && lines[lines.length - 1] === '' && /\n$/.test(text)) {
      lines.pop();
    }
    return lines.map(function (line) {
      return '<span class="line">' + highlightBatchLine(line, refIndex) + '</span>';
    }).join('');
  }

  function setLineMetadata(pre, count) {
    pre.setAttribute('data-postgis-lines', 'true');
    if (count > 1) {
      pre.setAttribute('data-postgis-multiline', 'true');
    } else {
      pre.removeAttribute('data-postgis-multiline');
    }
  }

  function unicodePsqlOutput(text) {
    var lines = String(text).split('\n');
    var hasAsciiTable = lines.some(function (line) {
      return line.indexOf('+') >= 0 && line.indexOf('-') >= 0 && /^[\s+-]+$/.test(line);
    });
    if (!hasAsciiTable) {
      return text;
    }
    return lines.map(function (line) {
      if (line.indexOf('+') >= 0 && line.indexOf('-') >= 0 && /^[\s+-]+$/.test(line)) {
        return line.replace(/-/g, '─').replace(/\+/g, '┼');
      }
      if (line.indexOf('|') >= 0) {
        return line.replace(/\|/g, '│');
      }
      return line;
    }).join('\n');
  }

  function wrapPlainText(pre) {
    var text = unicodePsqlOutput(pre.textContent);
    var lines = text.split('\n');
    if (lines.length > 1 && lines[lines.length - 1] === '' && /\n$/.test(text)) {
      lines.pop();
    }
    pre.textContent = lines.join('\n');
    pre.innerHTML = lines.map(function (line) {
      return '<span class="line">' + escapeHtml(line) + '</span>';
    }).join('');
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
    if (starts.length > 1 && starts[starts.length - 1] === text.length) {
      starts.pop();
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
    }
    pre.replaceChildren(fragment);
    setLineMetadata(pre, starts.length);
  }

  function wrapLogicalLines(pre) {
    if (pre.getAttribute('data-postgis-lines') === 'true') {
      return;
    }
    if (!originalBlockText.has(pre)) {
      originalBlockText.set(pre, normalizeDisplayedSql(pre.textContent));
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
    blocks = document.querySelectorAll(
      '.postgis-example-code pre.programlisting[data-postgis-language="bash"], ' +
      '.postgis-example-code pre.programlisting[data-postgis-language="sh"], ' +
      '.postgis-example-code pre.programlisting[data-postgis-language="shell"]'
    );
    for (var shellIndex = 0; shellIndex < blocks.length; shellIndex += 1) {
      if (blocks[shellIndex].getAttribute('data-postgis-highlighted') === 'shell' ||
          blocks[shellIndex].children.length !== 0) {
        continue;
      }
      text = normalizeDisplayedSql(blocks[shellIndex].textContent);
      originalBlockText.set(blocks[shellIndex], text);
      blocks[shellIndex].textContent = text;
      blocks[shellIndex].innerHTML = highlightShell(text, refIndex);
      setLineMetadata(blocks[shellIndex], text.split('\n').length);
      blocks[shellIndex].setAttribute('data-postgis-highlighted', 'shell');
    }

    blocks = document.querySelectorAll(
      '.postgis-example-code pre.programlisting[data-postgis-language="bat"], ' +
      '.postgis-example-code pre.programlisting[data-postgis-language="batch"], ' +
      '.postgis-example-code pre.programlisting[data-postgis-language="cmd"]'
    );
    for (var batchIndex = 0; batchIndex < blocks.length; batchIndex += 1) {
      if (blocks[batchIndex].getAttribute('data-postgis-highlighted') === 'batch' ||
          blocks[batchIndex].children.length !== 0) {
        continue;
      }
      text = normalizeDisplayedSql(blocks[batchIndex].textContent);
      originalBlockText.set(blocks[batchIndex], text);
      blocks[batchIndex].textContent = text;
      blocks[batchIndex].innerHTML = highlightBatch(text, refIndex);
      setLineMetadata(blocks[batchIndex], text.split('\n').length);
      blocks[batchIndex].setAttribute('data-postgis-highlighted', 'batch');
    }

    blocks = document.querySelectorAll(
      '.postgis-example-code pre.programlisting[data-postgis-language="html"], ' +
      '.postgis-example-code pre.programlisting[data-postgis-language="xml"]'
    );
    for (var htmlIndex = 0; htmlIndex < blocks.length; htmlIndex += 1) {
      if (blocks[htmlIndex].getAttribute('data-postgis-highlighted') === 'html' ||
          blocks[htmlIndex].children.length !== 0) {
        continue;
      }
      text = normalizeDisplayedSql(blocks[htmlIndex].textContent);
      originalBlockText.set(blocks[htmlIndex], text);
      blocks[htmlIndex].textContent = text;
      blocks[htmlIndex].innerHTML = highlightHtml(text);
      setLineMetadata(blocks[htmlIndex], text.split('\n').length);
      blocks[htmlIndex].setAttribute('data-postgis-highlighted', 'html');
    }

    blocks = document.querySelectorAll(
      '.postgis-example-code pre.programlisting[data-postgis-language="javascript"], ' +
      '.postgis-example-code pre.programlisting[data-postgis-language="js"]'
    );
    for (var javascriptIndex = 0; javascriptIndex < blocks.length; javascriptIndex += 1) {
      if (blocks[javascriptIndex].getAttribute('data-postgis-highlighted') === 'javascript' ||
          blocks[javascriptIndex].children.length !== 0) {
        continue;
      }
      text = normalizeDisplayedSql(blocks[javascriptIndex].textContent);
      originalBlockText.set(blocks[javascriptIndex], text);
      blocks[javascriptIndex].textContent = text;
      blocks[javascriptIndex].innerHTML = highlightJavaScript(text);
      setLineMetadata(blocks[javascriptIndex], text.split('\n').length);
      blocks[javascriptIndex].setAttribute('data-postgis-highlighted', 'javascript');
    }

    if (!refIndex.keywords.length) return;
    blocks = document.querySelectorAll('.postgis-example-code pre.programlisting[data-postgis-language="sql"]');
    for (var i = 0; i < blocks.length; i += 1) {
      if (blocks[i].getAttribute('data-postgis-highlighted') === 'sql') {
        continue;
      }
      if (blocks[i].children.length !== 0) {
        continue;
      }
      text = normalizeDisplayedSql(blocks[i].textContent);
      originalBlockText.set(blocks[i], text);
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
      var oldIndicators = lines[i].querySelectorAll(':scope > .postgis-wrap-indicator');
      for (var oldIndex = 0; oldIndex < oldIndicators.length; oldIndex += 1) {
        oldIndicators[oldIndex].remove();
      }
      var style = window.getComputedStyle(lines[i]);
      var lineHeight = parseFloat(style.lineHeight);
      var visualLineCount = lineHeight > 0 ? Math.max(1, Math.round(lines[i].scrollHeight / lineHeight)) : 1;
      var wrapped = visualLineCount > 1;
      lines[i].classList.toggle('line-wrapped', wrapped);
      for (var visualLine = 1; visualLine < visualLineCount; visualLine += 1) {
        var indicator = document.createElement('span');
        indicator.className = 'postgis-wrap-indicator';
        indicator.setAttribute('aria-hidden', 'true');
        indicator.style.top = (visualLine * lineHeight) + 'px';
        lines[i].appendChild(indicator);
      }
    }
  }

  function scheduleWrapIndicatorUpdate() {
    if (window.requestAnimationFrame) {
      window.requestAnimationFrame(updateWrapIndicators);
    } else {
      updateWrapIndicators();
    }
  }

  function observeLineBlocks() {
    if (!window.ResizeObserver) {
      return;
    }
    wrapObserver = new window.ResizeObserver(scheduleWrapIndicatorUpdate);
    var blocks = document.querySelectorAll('.postgis-example-block pre.programlisting, ' +
      '.postgis-example-block pre.screen');
    for (var i = 0; i < blocks.length; i += 1) {
      wrapObserver.observe(blocks[i]);
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
  // Build-time geometry figure interaction
  // ---------------------------------------------------------------------------

  var geometrySerial = 0;
  var geometryPattern = /(?:SRID\s*=\s*\d+\s*;\s*)?(?:POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION|CIRCULARSTRING|COMPOUNDCURVE|CURVEPOLYGON|MULTICURVE|MULTISURFACE|TRIANGLE|TIN|POLYHEDRALSURFACE|NURBSCURVE)(?:\s+(?:ZM|Z|M))?\s*\(/ig;

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
        if (i < 0) break;
        continue;
      }
      if (text.slice(i, i + 2) === '/*') {
        i = text.indexOf('*/', i + 2);
        if (i < 0) break;
        i += 1;
        continue;
      }
      if (text.charAt(i) === '$') {
        var delimiterMatch = /^\$(?:[A-Za-z_][A-Za-z0-9_]*)?\$/.exec(text.slice(i));
        if (!delimiterMatch) continue;
        var delimiter = delimiterMatch[0];
        var dollarStart = i + delimiter.length;
        var dollarEnd = text.indexOf(delimiter, dollarStart);
        if (dollarEnd < 0) break;
        ranges.push({ start: dollarStart, end: dollarEnd });
        i = dollarEnd + delimiter.length - 1;
        continue;
      }
      if (text.charAt(i) !== "'") continue;
      var start = i + 1;
      i += 1;
      while (i < text.length) {
        if (text.charAt(i) === "'") {
          if (text.charAt(i + 1) === "'") {
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

  function scanGeometryCandidates(text, quotedOnly) {
    var matches = [];
    var ranges = quotedOnly ? quotedRanges(text) : [];
    var match;
    geometryPattern.lastIndex = 0;
    while ((match = geometryPattern.exec(text)) !== null) {
      if (match.index > 0 && /[A-Za-z0-9_]/.test(text.charAt(match.index - 1))) continue;
      var opening = text.indexOf('(', match.index);
      var closing = closingParenthesis(text, opening);
      if (closing < 0) break;
      if (/[A-Za-z0-9_]/.test(text.charAt(closing + 1))) {
        geometryPattern.lastIndex = closing + 1;
        continue;
      }
      if (quotedOnly && !ranges.some(function (range) {
        return match.index >= range.start && closing + 1 <= range.end;
      })) {
        geometryPattern.lastIndex = closing + 1;
        continue;
      }
      matches.push({ start: match.index, end: closing + 1, text: text.slice(match.index, closing + 1) });
      geometryPattern.lastIndex = closing + 1;
    }
    if (quotedOnly) {
      makeEnvelopePattern.lastIndex = 0;
      while ((match = makeEnvelopePattern.exec(text)) !== null) {
        matches.push({ start: match.index, end: makeEnvelopePattern.lastIndex, text: match[0] });
      }
    }
    return matches.sort(function (left, right) { return left.start - right.start; });
  }

  function wrapLiteralRange(pre, match, id) {
    if (!document.createTreeWalker || !document.createElement) return;
    var walker = document.createTreeWalker(pre, 4);
    var nodes = [];
    var node;
    var consumed = 0;
    while ((node = walker.nextNode())) {
      nodes.push({ node: node, start: consumed, end: consumed + node.nodeValue.length });
      consumed += node.nodeValue.length;
    }
    for (var i = nodes.length - 1; i >= 0; i -= 1) {
      var item = nodes[i];
      var start = Math.max(match.start, item.start);
      var end = Math.min(match.end, item.end);
      if (start >= end) continue;
      var target = item.node;
      if (end < item.end) target.splitText(end - item.start);
      if (start > item.start) target = target.splitText(start - item.start);
      var span = document.createElement('span');
      span.className = 'postgis-geometry-literal postgis-geometry-target';
      span.setAttribute('data-postgis-geometry-id', id);
      target.parentNode.insertBefore(span, target);
      span.appendChild(target);
    }
  }

  function setOutputTextCollapsed(block, button, collapsed) {
    if (!block || !button) return;
    var text = collapsed
      ? blockLabel(block, 'data-postgis-show-text-label', 'Show text')
      : blockLabel(block, 'data-postgis-hide-text-label', 'Hide text');
    var aria = collapsed
      ? blockLabel(block, 'data-postgis-show-output-text-label', 'Show output text')
      : blockLabel(block, 'data-postgis-hide-output-text-label', 'Hide output text');
    block.classList.toggle('postgis-output-text-collapsed', collapsed);
    button.textContent = text;
    button.setAttribute('aria-label', aria);
    button.setAttribute('title', aria);
    button.setAttribute('aria-expanded', collapsed ? 'false' : 'true');
  }

  function blockLabel(block, attribute, fallback) {
    if (!block || !block.getAttribute) return fallback;
    return block.getAttribute(attribute) || fallback;
  }

  function revealOutputText(block) {
    var textButton = block && block.querySelector('.postgis-output-toggle');
    if (block && block.classList && block.classList.contains('postgis-output-text-collapsed')) {
      setOutputTextCollapsed(block, textButton, false);
    }
  }

  function setOutputRepresentation(block, button, nativeOutput) {
    var readable = block && block.querySelector('pre.screen:not(.postgis-native-output)');
    var native = block && block.querySelector('pre.postgis-native-output');
    if (!readable || !native || !button) return;
    revealOutputText(block);
    readable.hidden = nativeOutput;
    readable.setAttribute('aria-hidden', nativeOutput ? 'true' : 'false');
    native.hidden = !nativeOutput;
    native.setAttribute('aria-hidden', nativeOutput ? 'false' : 'true');
    var text = nativeOutput
      ? blockLabel(block, 'data-postgis-show-ewkt-label', 'Show EWKT')
      : blockLabel(block, 'data-postgis-show-hexewkb-label', 'Show HEXEWKB');
    var aria = nativeOutput
      ? blockLabel(block, 'data-postgis-show-readable-ewkt-label', 'Show readable EWKT')
      : blockLabel(block, 'data-postgis-show-native-hexewkb-label', 'Show native HEXEWKB');
    button.textContent = text;
    button.setAttribute('aria-label', aria);
    button.setAttribute('title', aria);
    button.setAttribute('aria-expanded', nativeOutput ? 'true' : 'false');
  }

  function preferVisualOutput(block) {
    if (!block || block.querySelector('.postgis-output-toggle')) return;
    var pre = block.querySelector('pre.screen');
    var header = block.querySelector('.postgis-example-header');
    if (!pre || !header) return;
    if (!pre.id) pre.id = 'postgis-output-text-' + (++geometrySerial);
    var button = document.createElement('button');
    button.type = 'button';
    button.className = 'postgis-output-toggle';
    button.setAttribute('aria-controls', pre.id);
    header.appendChild(button);
    setOutputTextCollapsed(block, button, true);
  }

  function setGeometryActive(id, active) {
    var related = document.querySelectorAll('[data-postgis-geometry-id="' + id + '"]');
    for (var i = 0; i < related.length; i += 1) {
      related[i].classList.toggle('postgis-geometry-active', active);
    }
    var objects = document.querySelectorAll('.postgis-geometry-figure-body');
    for (i = 0; i < objects.length; i += 1) {
      var svgDocument = objects[i].contentDocument;
      if (!svgDocument) continue;
      var svgTargets = svgDocument.querySelectorAll('[data-postgis-geometry-id="' + id + '"]');
      for (var j = 0; j < svgTargets.length; j += 1) {
        svgTargets[j].classList.toggle('active', active);
      }
    }
  }

  function setGeometryHovered(id, active) {
    var pending = geometryDeactivateTimers.get(id);
    if (pending) {
      window.clearTimeout(pending);
      geometryDeactivateTimers.delete(id);
    }
    if (active) {
      setGeometryActive(id, true);
      return;
    }
    geometryDeactivateTimers.set(id, window.setTimeout(function () {
      geometryDeactivateTimers.delete(id);
      setGeometryActive(id, false);
    }, 60));
  }

  function bindSvgTargets(object) {
    var svgDocument = object.contentDocument;
    var svgRoot = svgDocument && svgDocument.documentElement;
    if (!svgRoot || svgRoot.getAttribute('data-postgis-svg-bound') === 'true') return;
    svgRoot.setAttribute('data-postgis-svg-bound', 'true');
    svgDocument.addEventListener('mouseover', function (event) {
      var target = event.target.closest && event.target.closest('[data-postgis-geometry-id]');
      if (target) setGeometryHovered(target.getAttribute('data-postgis-geometry-id'), true);
    });
    svgDocument.addEventListener('mouseout', function (event) {
      var target = event.target.closest && event.target.closest('[data-postgis-geometry-id]');
      if (target && (!event.relatedTarget || !target.contains(event.relatedTarget))) {
        setGeometryHovered(target.getAttribute('data-postgis-geometry-id'), false);
      }
    });
  }

  function previousVisualStackSibling(node) {
    var sibling = node && node.previousElementSibling;
    while (sibling && sibling.tagName === 'P' && !sibling.textContent.trim() && !sibling.children.length) {
      var emptyParagraph = sibling;
      sibling = sibling.previousElementSibling;
      emptyParagraph.parentNode.removeChild(emptyParagraph);
    }
    return sibling;
  }

  function enhanceBuiltGeometryFigures() {
    var figures = document.querySelectorAll('[data-postgis-built-geometry="true"]');
    for (var i = 0; i < figures.length; i += 1) {
      var figure = figures[i];
      if (figure.getAttribute('data-postgis-geometry-enhanced') === 'true') continue;
      figure.setAttribute('data-postgis-geometry-enhanced', 'true');
      var visualId = figure.getAttribute('data-postgis-visual-id');
      var object = figure.querySelector('object.postgis-geometry-figure-body');
      if (!visualId || !object) continue;

      var output = previousVisualStackSibling(figure);
      var code = previousVisualStackSibling(output);
      if (!output || !output.matches('.postgis-example-output') ||
          !code || !code.matches('.postgis-example-code') ||
          output.getAttribute('data-postgis-visual-id') !== visualId ||
          code.getAttribute('data-postgis-visual-id') !== visualId) continue;

      var codePre = code.querySelector('pre.programlisting');
      var outputPre = output.querySelector('pre.screen');
      var sources = [
        { pre: codePre, name: 'code', quoted: true },
        { pre: outputPre, name: 'output', quoted: false }
      ];
      for (var sourceIndex = 0; sourceIndex < sources.length; sourceIndex += 1) {
        if (!sources[sourceIndex].pre) continue;
        var matches = scanGeometryCandidates(sources[sourceIndex].pre.textContent, sources[sourceIndex].quoted);
        for (var matchIndex = 0; matchIndex < matches.length; matchIndex += 1) {
          wrapLiteralRange(
            sources[sourceIndex].pre,
            matches[matchIndex],
            visualId + '-' + sources[sourceIndex].name + '-' + (matchIndex + 1)
          );
        }
      }

      var toggle = document.createElement('button');
      toggle.type = 'button';
      toggle.className = 'postgis-geometry-toggle';
      toggle.textContent = 'Hide figure';
      toggle.setAttribute('aria-label', 'Hide figure');
      toggle.setAttribute('title', 'Hide figure');
      toggle.setAttribute('aria-expanded', 'true');
      if (!object.id) object.id = visualId + '-figure';
      toggle.setAttribute('aria-controls', object.id);
      figure.querySelector('.postgis-example-header').appendChild(toggle);
      if (output.getAttribute('data-postgis-output-preference') === 'visual') {
        preferVisualOutput(output);
      }
      object.addEventListener('load', function (event) { bindSvgTargets(event.currentTarget); });
      if (object.contentDocument) bindSvgTargets(object);
    }
  }

  function handleFigureToggle(event) {
    var button = event.target.closest && event.target.closest('.postgis-geometry-toggle');
    if (!button) return false;
    var figure = button.closest('.postgis-geometry-figure');
    var collapsed = !figure.classList.contains('postgis-geometry-collapsed');
    figure.classList.toggle('postgis-geometry-collapsed', collapsed);
    button.textContent = collapsed ? 'Show figure' : 'Hide figure';
    button.setAttribute('aria-label', collapsed ? 'Show figure' : 'Hide figure');
    button.setAttribute('title', collapsed ? 'Show figure' : 'Hide figure');
    button.setAttribute('aria-expanded', collapsed ? 'false' : 'true');
    return true;
  }

  function handleOutputToggle(event) {
    var button = event.target.closest && event.target.closest('.postgis-output-toggle');
    if (!button) return false;
    var block = button.closest('.postgis-example-output');
    setOutputTextCollapsed(block, button, !block.classList.contains('postgis-output-text-collapsed'));
    return true;
  }

  function handleOutputRepresentationToggle(event) {
    var button = event.target.closest && event.target.closest('.postgis-output-representation-toggle');
    if (!button) return false;
    var block = button.closest('.postgis-example-output');
    setOutputRepresentation(block, button, button.getAttribute('aria-expanded') !== 'true');
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
    if (originalBlockText.has(pre)) {
      return originalBlockText.get(pre);
    }
    var clone = pre.cloneNode(true);
    var ignored = clone.querySelectorAll(
      '.linenumber, .line-number, .ln, .co, .callout, .callout-bug, .postgis-wrap-indicator'
    );
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
    if (handleFigureToggle(event) || handleOutputToggle(event) || handleOutputRepresentationToggle(event)) {
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
      refIndex.currentFunction = currentFunctionName(refIndex);
      applySyntaxHighlighting(refIndex);
      applyLineWrappers();
      enhanceBuiltGeometryFigures();
      observeLineBlocks();
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
  document.addEventListener('mouseover', function (event) {
    var target = event.target.closest && event.target.closest('[data-postgis-geometry-id]');
    if (target) setGeometryHovered(target.getAttribute('data-postgis-geometry-id'), true);
  });
  document.addEventListener('mouseout', function (event) {
    var target = event.target.closest && event.target.closest('[data-postgis-geometry-id]');
    if (target && (!event.relatedTarget || !target.contains(event.relatedTarget))) {
      setGeometryHovered(target.getAttribute('data-postgis-geometry-id'), false);
    }
  });
  window.addEventListener('resize', handleResize);

  window.POSTGIS_EXAMPLE_BLOCKS_TEST = {
    scanGeometryCandidates: scanGeometryCandidates,
    wrapLiteralRange: wrapLiteralRange,
    setOutputTextCollapsed: setOutputTextCollapsed,
    setOutputRepresentation: setOutputRepresentation,
    normalizeDisplayedSql: normalizeDisplayedSql,
    codeText: codeText,
    normalizeRefIndex: normalizeRefIndex,
    unicodePsqlOutput: unicodePsqlOutput,
    highlightSql: highlightSql,
    highlightShell: highlightShell,
    highlightBatch: highlightBatch,
    highlightHtml: highlightHtml,
    highlightJavaScript: highlightJavaScript,
    currentFunctionName: currentFunctionName,
    previousVisualStackSibling: previousVisualStackSibling,
    setGeometryActive: setGeometryActive,
    setGeometryHovered: setGeometryHovered,
    updateWrapIndicators: updateWrapIndicators
  };
}());
