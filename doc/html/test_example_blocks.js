const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

function plainText(html) {
  return html.replace(/<[^>]*>/g, '').replace(/&lt;/g, '<').replace(/&gt;/g, '>')
    .replace(/&quot;/g, '"').replace(/&amp;/g, '&');
}

function makePre(text) {
  let innerHTML = '';
  return {
    textContent: text,
    children: [],
    attributes: { 'data-postgis-language': 'sql' },
    get innerHTML() {
      return innerHTML;
    },
    set innerHTML(html) {
      innerHTML = html;
      this.textContent = plainText(html);
    },
    getAttribute(name) {
      return this.attributes[name] || null;
    },
    setAttribute(name, value) {
      this.attributes[name] = value;
    },
    removeAttribute(name) {
      delete this.attributes[name];
    },
    cloneNode() {
      return {
        textContent: this.textContent,
        querySelectorAll() {
          return [];
        }
      };
    }
  };
}

function makeLiteralTree(text, document) {
  function removeFromParent(node) {
    if (node.parentNode) {
      const index = node.parentNode.children.indexOf(node);
      if (index >= 0) {
        node.parentNode.children.splice(index, 1);
      }
    }
  }

  function makeElement(tagName) {
    return {
      tagName,
      children: [],
      attributes: {},
      className: '',
      id: '',
      style: {},
      get textContent() {
        return this.children.map((child) => child.nodeValue === undefined ? child.textContent : child.nodeValue)
          .join('');
      },
      setAttribute(name, value) {
        this.attributes[name] = value;
      },
      appendChild(node) {
        removeFromParent(node);
        this.children.push(node);
        node.parentNode = this;
      },
      remove() {
        removeFromParent(this);
      }
    };
  }

  function makeText(value) {
    return {
      nodeValue: value,
      parentNode: null,
      splitText(offset) {
        const tail = makeText(this.nodeValue.slice(offset));
        this.nodeValue = this.nodeValue.slice(0, offset);
        const index = this.parentNode.children.indexOf(this);
        this.parentNode.children.splice(index + 1, 0, tail);
        tail.parentNode = this.parentNode;
        return tail;
      }
    };
  }

  const root = makeElement('pre');
  root.insertBefore = function (node, target) {
    removeFromParent(node);
    const index = this.children.indexOf(target);
    this.children.splice(index, 0, node);
    node.parentNode = this;
  };
  root.cloneNode = function () {
    return { textContent: this.textContent, querySelectorAll: () => [] };
  };
  root.appendChild(makeText(text));
  document.createElement = makeElement;
  document.createTreeWalker = function (tree) {
    const nodes = [];
    (function collect(node) {
      if (node.nodeValue !== undefined) {
        nodes.push(node);
      } else {
        node.children.forEach(collect);
      }
    }(tree));
    let index = 0;
    return { nextNode: () => nodes[index++] || null };
  };
  return root;
}

async function main() {
  const blocks = [
    makePre('\na <= b\n \n   '),
    makePre('a = b'),
    makePre('a == b'),
    makePre('SELECT (a + (b))\nFROM things)'),
    makePre('SELECT st_buffer(g, 1), ST_Buffer(g, 2), ST_MakePoint(1, 2)'),
    makePre('SET postgis.gdal_datapath TO default;')
  ];
  const shellSource = 'DB=[yourdatabase]\n' +
    'SCRIPTSDIR=`pg_config --sharedir`/contrib/postgis-3.6/\n\n' +
    '# Core objects\n' +
    'psql -d ${DB} -f "${SCRIPTSDIR}/postgis.sql" # OPTIONAL';
  const shellBlock = makePre(shellSource);
  shellBlock.attributes['data-postgis-language'] = 'bash';
  const batchSource = 'REM In a .bat file, use %%F. At an interactive cmd.exe prompt, use %F.\n' +
    'for %%F in ("C:\\data\\shapefiles\\*.shp") do (\n' +
    '  shp2pgsql -d -D -s 4269 -I -W UTF-8 "%%~fF" "myschema.%%~nF" | psql -d roadsdb\n' +
    ')';
  const batchBlock = makePre(batchSource);
  batchBlock.attributes['data-postgis-language'] = 'cmd';
  const htmlSource = '<script type="text/javascript">\n' +
    'const path = google.maps.geometry.encoding.decodePath("encoded");\n' +
    '</script>';
  const htmlBlock = makePre(htmlSource);
  htmlBlock.attributes['data-postgis-language'] = 'html';
  const javascriptSource = 'const route = decodePath(encoded); // polyline5';
  const javascriptBlock = makePre(javascriptSource);
  javascriptBlock.attributes['data-postgis-language'] = 'javascript';
  const output = makePre('wide output row\nsecond row');
  output.attributes = {};
  const trailingOutput = makePre('first row\nsecond row\n');
  trailingOutput.attributes = {};
  const psqlOutput = makePre(' track_id |       dist\n' +
    '----------+------------------\n' +
    '      395 | 0.576496831518066\n' +
    '      380 | 5.06797130410151\n' +
    '(2 rows)');
  psqlOutput.attributes = {};
  const listeners = {};
  const windowListeners = {};
  let geometryRelated = [];
  let geometryObjects = [];
  const wrapClasses = [new Set(), new Set()];
  const observedBlocks = [];
  let resizeObserverCallback = null;
  const layoutLines = wrapClasses.map(function (classes, index) {
    return {
      owner: index === 0 ? output : blocks[0],
      scrollHeight: 16,
      children: [],
      appendChild(node) {
        this.children.push(node);
        node.parentNode = this;
      },
      querySelectorAll(selector) {
        if (selector === ':scope > .postgis-wrap-indicator') {
          return this.children.filter((child) => child.className === 'postgis-wrap-indicator');
        }
        return [];
      },
      classList: {
        toggle(name, enabled) {
          if (enabled) {
            classes.add(name);
          } else {
            classes.delete(name);
          }
        }
      }
    };
  });
  let copiedText = null;
  let refnameText = '';
  const document = {
    currentScript: { src: 'file:///tmp/example-blocks.js' },
    readyState: 'loading',
    addEventListener(name, handler) {
      const previous = listeners[name];
      listeners[name] = previous ? function (event) {
        previous(event);
        handler(event);
      } : handler;
    },
    querySelectorAll(selector) {
      if (selector === '.postgis-example-code pre.programlisting[data-postgis-language="sql"]') {
        return blocks;
      }
      if (selector === '.postgis-example-code pre.programlisting[data-postgis-language="bash"], ' +
          '.postgis-example-code pre.programlisting[data-postgis-language="sh"], ' +
          '.postgis-example-code pre.programlisting[data-postgis-language="shell"]') {
        return [shellBlock];
      }
      if (selector === '.postgis-example-code pre.programlisting[data-postgis-language="bat"], ' +
          '.postgis-example-code pre.programlisting[data-postgis-language="batch"], ' +
          '.postgis-example-code pre.programlisting[data-postgis-language="cmd"]') {
        return [batchBlock];
      }
      if (selector === '.postgis-example-code pre.programlisting[data-postgis-language="html"], ' +
          '.postgis-example-code pre.programlisting[data-postgis-language="xml"]') {
        return [htmlBlock];
      }
      if (selector === '.postgis-example-code pre.programlisting[data-postgis-language="javascript"], ' +
          '.postgis-example-code pre.programlisting[data-postgis-language="js"]') {
        return [javascriptBlock];
      }
      if (selector === '.postgis-example-block pre.programlisting, .postgis-example-block pre.screen') {
        return blocks.concat([shellBlock, batchBlock, htmlBlock, javascriptBlock,
          output, trailingOutput, psqlOutput]);
      }
      if (selector === '.postgis-example-block pre[data-postgis-lines="true"] > .line') {
        return layoutLines;
      }
      if (selector.startsWith('[data-postgis-geometry-id=')) {
        return geometryRelated;
      }
      if (selector === '.postgis-geometry-figure-body') {
        return geometryObjects;
      }
      return [];
    },
    querySelector(selector) {
      if (selector === '.refnamediv .refname, .refnamediv p' && refnameText) {
        return { textContent: refnameText };
      }
      return null;
    },
    getElementById() {
      return null;
    }
  };
  const sandbox = {
    Promise,
    URL,
    WeakMap,
    document,
    navigator: {
      clipboard: {
        writeText(text) {
          copiedText = text;
          return Promise.resolve();
        }
      }
    },
    window: {
      location: { pathname: '/manual/st_buffer.HTML' },
      POSTGIS_REF_INDEX: {
        commands: {
          postgis_restore: {
            id: 'hard_upgrade',
            href: 'postgis_administration.html#hard_upgrade',
            label: 'postgis_restore',
            title: 'Hard upgrade'
          }
        },
        functions: {
          ST_BUFFER: { href: 'ST_Buffer.html', label: 'ST_Buffer', title: 'buffer geometry' },
          ST_MAKEPOINT: { href: 'ST_MakePoint.html', label: 'ST_MakePoint', title: 'make a point' }
        },
        operators: {
          '=': { href: 'equals.html', label: 'Operator', title: 'equals' },
          '==': { href: 'same.html', label: 'Operator', title: 'same' }
        },
        keywords: [
          'ANALYZE', 'BEGIN', 'DECLARE', 'DO', 'END', 'EXECUTE', 'FOR', 'IF',
          'LOOP', 'SELECT', 'SET', 'THEN', 'VACUUM'
        ]
      },
      clearTimeout() {},
      setTimeout() {},
      requestAnimationFrame(callback) {
        callback();
      },
      getComputedStyle() {
        return { lineHeight: '16px' };
      },
      addEventListener(name, handler) {
        windowListeners[name] = handler;
      },
      ResizeObserver: function (handler) {
        assert.strictEqual(typeof handler, 'function');
        resizeObserverCallback = handler;
        this.observe = function (block) {
          observedBlocks.push(block);
        };
      }
    }
  };

  vm.runInNewContext(fs.readFileSync('doc/html/example-blocks.js', 'utf8'), sandbox,
    { filename: 'example-blocks.js' });
  const geometry = sandbox.window.POSTGIS_EXAMPLE_BLOCKS_TEST;

  const stackCode = { tagName: 'DIV', textContent: 'code', children: [] };
  const stackOutput = { tagName: 'DIV', textContent: 'output', children: [], previousElementSibling: stackCode };
  const emptyStackParagraph = {
    tagName: 'P', textContent: '  ', children: [], previousElementSibling: stackOutput
  };
  const stackFigure = { previousElementSibling: emptyStackParagraph };
  const stackParent = {
    removeChild(node) {
      assert.strictEqual(node, emptyStackParagraph);
      stackFigure.previousElementSibling = node.previousElementSibling;
    }
  };
  emptyStackParagraph.parentNode = stackParent;
  assert.strictEqual(geometry.previousVisualStackSibling(stackFigure), stackOutput);
  assert.strictEqual(geometry.previousVisualStackSibling(stackOutput), stackCode);

  function geometryTexts(text, quotedOnly) {
    return geometry.scanGeometryCandidates(text, quotedOnly).map((match) => match.text);
  }

  assert.deepStrictEqual(Array.from(geometryTexts(
    "SELECT 'POINT(1 2)', 'CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 0))'", true
  )), ['POINT(1 2)', 'CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 0))']);
  assert.deepStrictEqual(Array.from(geometryTexts(
    'SELECT $wkt$NURBSCURVE(DEGREE 2,CONTROLPOINTS(NURBSPOINT(WEIGHTEDPOINT(0 0),WEIGHT 1)))$wkt$',
    true
  )), ['NURBSCURVE(DEGREE 2,CONTROLPOINTS(NURBSPOINT(WEIGHTEDPOINT(0 0),WEIGHT 1)))']);
  assert.strictEqual(geometryTexts('SELECT POINT(1 2)', true).length, 0);
  assert.strictEqual(geometryTexts("-- 'POINT(1 2)'", true).length, 0);
  assert.strictEqual(geometryTexts("/* 'POINT(1 2)' */", true).length, 0);
  assert.strictEqual(geometryTexts('SELECT "POINT(1 2)"', true).length, 0);
  assert.strictEqual(geometryTexts('XPOINT(1 2)', false).length, 0);
  assert.strictEqual(geometryTexts('POINT(1 2)junk', false).length, 0);
  assert.deepStrictEqual(Array.from(geometryTexts(
    'GEOMETRYCOLLECTION(TRIANGLE((0 0,2 0,0 2,0 0)),POINT(1 1))', false
  )), ['GEOMETRYCOLLECTION(TRIANGLE((0 0,2 0,0 2,0 0)),POINT(1 1))']);
  assert.deepStrictEqual(Array.from(geometryTexts(
    "SELECT f('LINESTRING(0 0,10 10)', ST_MakeEnvelope(2, 3, 8, 9, 4326))", true
  )), ['LINESTRING(0 0,10 10)', 'ST_MakeEnvelope(2, 3, 8, 9, 4326)']);

  const outputState = new Set();
  const outputBlock = {
    classList: {
      toggle(name, enabled) {
        if (enabled) outputState.add(name); else outputState.delete(name);
      }
    }
  };
  const outputButton = {
    textContent: '',
    attributes: {},
    setAttribute(name, value) { this.attributes[name] = value; }
  };
  geometry.setOutputTextCollapsed(outputBlock, outputButton, true);
  assert(outputState.has('postgis-output-text-collapsed'));
  assert.strictEqual(outputButton.textContent, 'Show text');
  assert.strictEqual(outputButton.attributes['aria-expanded'], 'false');
  assert.strictEqual(outputButton.attributes['aria-label'], 'Show output text');
  geometry.setOutputTextCollapsed(outputBlock, outputButton, false);
  assert(!outputState.has('postgis-output-text-collapsed'));
  assert.strictEqual(outputButton.textContent, 'Hide text');
  assert.strictEqual(outputButton.attributes['aria-expanded'], 'true');
  assert.strictEqual(outputButton.attributes['aria-label'], 'Hide output text');

  const localizedOutputBlock = {
    classList: {
      toggle(name, enabled) {
        if (enabled) outputState.add(name); else outputState.delete(name);
      }
    },
    getAttribute(name) {
      return {
        'data-postgis-show-text-label': 'Показать текст',
        'data-postgis-hide-text-label': 'Скрыть текст',
        'data-postgis-show-output-text-label': 'Показать текстовый вывод',
        'data-postgis-hide-output-text-label': 'Скрыть текстовый вывод'
      }[name] || '';
    }
  };
  geometry.setOutputTextCollapsed(localizedOutputBlock, outputButton, true);
  assert.strictEqual(outputButton.textContent, 'Показать текст');
  assert.strictEqual(outputButton.attributes['aria-label'], 'Показать текстовый вывод');

  const readableOutput = makePre('POINT(1 2)');
  const nativeOutput = makePre('0101000000000000000000F03F0000000000000040');
  readableOutput.hidden = false;
  nativeOutput.hidden = true;
  const representationState = new Set(['postgis-output-text-collapsed']);
  const representationTextButton = {
    textContent: 'Show text',
    attributes: { 'aria-expanded': 'false' },
    setAttribute(name, value) { this.attributes[name] = value; }
  };
  const representationBlock = {
    classList: {
      contains(name) { return representationState.has(name); },
      toggle(name, enabled) {
        if (enabled) representationState.add(name); else representationState.delete(name);
      }
    },
    getAttribute(name) {
      return {
        'data-postgis-show-hexewkb-label': 'Показать HEXEWKB',
        'data-postgis-show-native-hexewkb-label': 'Показать native HEXEWKB',
        'data-postgis-show-ewkt-label': 'Показать EWKT',
        'data-postgis-show-readable-ewkt-label': 'Показать readable EWKT',
        'data-postgis-show-text-label': 'Показать текст',
        'data-postgis-hide-text-label': 'Скрыть текст',
        'data-postgis-show-output-text-label': 'Показать текстовый вывод',
        'data-postgis-hide-output-text-label': 'Скрыть текстовый вывод'
      }[name] || '';
    },
    querySelector(selector) {
      if (selector === 'pre.screen:not(.postgis-native-output)') return readableOutput;
      if (selector === 'pre.postgis-native-output') return nativeOutput;
      if (selector === '.postgis-output-toggle') return representationTextButton;
      return null;
    }
  };
  const representationButton = {
    textContent: '',
    attributes: {},
    setAttribute(name, value) { this.attributes[name] = value; }
  };
  geometry.setOutputRepresentation(representationBlock, representationButton, true);
  assert.strictEqual(readableOutput.hidden, true);
  assert.strictEqual(nativeOutput.hidden, false);
  assert(!representationState.has('postgis-output-text-collapsed'));
  assert.strictEqual(representationTextButton.textContent, 'Скрыть текст');
  assert.strictEqual(representationTextButton.attributes['aria-expanded'], 'true');
  assert.strictEqual(representationButton.textContent, 'Показать EWKT');
  assert.strictEqual(representationButton.attributes['aria-expanded'], 'true');
  geometry.setOutputRepresentation(representationBlock, representationButton, false);
  assert.strictEqual(readableOutput.hidden, false);
  assert.strictEqual(nativeOutput.hidden, true);
  assert.strictEqual(representationButton.textContent, 'Показать HEXEWKB');

  const literalOriginal = "SELECT 'POINT(1 2)'";
  const literalPre = makeLiteralTree(literalOriginal, document);
  geometry.wrapLiteralRange(literalPre, { start: 8, end: 18 }, 'postgis-geometry-99');
  assert.strictEqual(literalPre.textContent, literalOriginal);
  assert.strictEqual(geometry.codeText(literalPre), literalOriginal);
  const literalSpan = literalPre.children[1];
  assert.strictEqual(literalSpan.textContent, 'POINT(1 2)');
  assert.strictEqual(literalSpan.attributes['data-postgis-geometry-id'], 'postgis-geometry-99');

  listeners.DOMContentLoaded();
  await Promise.resolve();
  await Promise.resolve();

  assert.strictEqual(blocks[0].textContent, 'a <= b');
  assert.strictEqual(blocks[0].innerHTML, '<span class="line">a &lt;= b</span>');
  assert.doesNotMatch(blocks[0].innerHTML, /<a\b/);
  assert.match(blocks[1].innerHTML, /<a\b[^>]*>\=<\/a>/);
  assert.match(blocks[2].innerHTML, /<a\b[^>]*>==<\/a>/);
  assert.match(blocks[3].innerHTML,
    /^<span class="line"><span class="postgis-sql-keyword">SELECT<\/span> /);
  assert.strictEqual((blocks[3].innerHTML.match(/class="line"/g) || []).length, 2);
  assert.strictEqual(blocks[3].attributes['data-postgis-multiline'], 'true');
  assert.strictEqual(blocks[3].textContent, 'SELECT (a + (b))FROM things)');
  assert.doesNotMatch(blocks[3].innerHTML, /<\/span>\n<span class="line">/);
  assert.match(blocks[5].innerHTML,
    /^<span class="line"><span class="postgis-sql-keyword">SET<\/span> /);
  assert.strictEqual(output.innerHTML,
    '<span class="line">wide output row</span><span class="line">second row</span>');
  assert.strictEqual(output.textContent, 'wide output rowsecond row');
  assert.strictEqual(trailingOutput.innerHTML,
    '<span class="line">first row</span><span class="line">second row</span>');
  assert.strictEqual((trailingOutput.innerHTML.match(/class="line"/g) || []).length, 2);
  assert(!trailingOutput.innerHTML.includes('<span class="line"></span>'));
  assert.match(psqlOutput.innerHTML, /track_id │       dist/);
  assert.match(psqlOutput.innerHTML, /──────────┼──────────────────/);
  assert.doesNotMatch(psqlOutput.innerHTML, /---\+---/);
  assert(observedBlocks.includes(output));
  assert(observedBlocks.includes(trailingOutput));
  assert(observedBlocks.includes(psqlOutput));
  assert(!wrapClasses[0].has('line-wrapped'));
  assert(!wrapClasses[1].has('line-wrapped'));
  layoutLines[0].scrollHeight = 32;
  resizeObserverCallback([{ target: output }]);
  assert.strictEqual(layoutLines[0].owner, output);
  assert(wrapClasses[0].has('line-wrapped'));
  assert.strictEqual(layoutLines[0].children.length, 1);
  assert.strictEqual(layoutLines[0].children[0].attributes['aria-hidden'], 'true');
  assert.strictEqual(layoutLines[0].children[0].style.top, '16px');
  layoutLines[0].scrollHeight = 48;
  resizeObserverCallback([{ target: output }]);
  assert.strictEqual(layoutLines[0].children.length, 2);
  assert.deepStrictEqual(layoutLines[0].children.map((indicator) => indicator.style.top), ['16px', '32px']);
  layoutLines[0].scrollHeight = 16;
  resizeObserverCallback([{ target: output }]);
  assert(!wrapClasses[0].has('line-wrapped'));
  assert.strictEqual(layoutLines[0].children.length, 0);

  const relatedClasses = new Set();
  const layerClasses = new Set(['geometry-layer']);
  const toggleClass = (classes) => ({
    contains: (name) => classes.has(name),
    toggle(name, enabled) {
      if (enabled) classes.add(name);
      else classes.delete(name);
    }
  });
  const related = { classList: toggleClass(relatedClasses) };
  const layer = {
    classList: toggleClass(layerClasses)
  };
  geometryRelated = [related];
  geometryObjects = [{
    contentDocument: {
      querySelectorAll() { return [layer]; }
    }
  }];
  geometry.setGeometryActive('example-code-1', true);
  geometry.setGeometryActive('example-code-1', true);
  assert(relatedClasses.has('postgis-geometry-active'));
  assert(layerClasses.has('active'));
  geometry.setGeometryActive('example-code-1', false);
  geometry.setGeometryActive('example-code-1', true);

  let pendingGeometryDeactivate = null;
  sandbox.window.setTimeout = function (callback) {
    pendingGeometryDeactivate = callback;
    return callback;
  };
  sandbox.window.clearTimeout = function (callback) {
    if (pendingGeometryDeactivate === callback) pendingGeometryDeactivate = null;
  };
  geometry.setGeometryHovered('example-code-1', false);
  assert(pendingGeometryDeactivate);
  geometry.setGeometryHovered('example-code-1', true);
  assert.strictEqual(pendingGeometryDeactivate, null);
  assert(layerClasses.has('active'));
  geometry.setGeometryHovered('example-code-1', false);
  pendingGeometryDeactivate();
  assert(!relatedClasses.has('postgis-geometry-active'));
  assert(!layerClasses.has('active'));

  const pairIds = Array.from(blocks[3].innerHTML.matchAll(/data-postgis-paren-pair="(\d+)"/g),
    (match) => match[1]);
  assert.strictEqual(pairIds.length, 4);
  assert.deepStrictEqual(Array.from(new Set(pairIds)).sort(), ['1', '2']);
  assert.deepStrictEqual(
    Array.from(blocks[3].innerHTML.matchAll(/data-postgis-paren-depth="(\d+)"/g), (match) => match[1]),
    ['0', '1', '1', '0', '0']
  );
  assert.match(blocks[3].innerHTML, /postgis-sql-paren paren-unmatched/);
  assert.strictEqual((blocks[4].innerHTML.match(/class="current-fn"/g) || []).length, 2);
  assert.match(blocks[4].innerHTML, /<span class="current-fn">st_buffer<\/span>/);
  assert.match(blocks[4].innerHTML, /<span class="current-fn">ST_Buffer<\/span>/);
  assert.doesNotMatch(blocks[4].innerHTML, /<a\b[^>]*>[^<]*ST_Buffer<\/a>/i);
  assert.match(blocks[4].innerHTML,
    /<a\b[^>]*href="ST_MakePoint\.html"[^>]*>ST_MakePoint<\/a>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-variable">DB<\/span>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-variable">\$\{DB\}<\/span>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-command">pg_config<\/span>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-command">psql<\/span>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-option">--sharedir<\/span>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-option">-d<\/span>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-comment"># Core objects<\/span>/);
  assert.match(shellBlock.innerHTML, /postgis-shell-string">"\$\{SCRIPTSDIR\}\/postgis\.sql"<\/span>/);
  assert.strictEqual(geometry.codeText(shellBlock), shellSource);
  const shellRedirections = geometry.highlightShell('echo ok > out\n< input cat\ncat 2>> log');
  assert.strictEqual((shellRedirections.match(/postgis-shell-command/g) || []).length, 3);
  assert.match(shellRedirections, /postgis-shell-command">echo<\/span>/);
  assert.strictEqual((shellRedirections.match(/postgis-shell-command">cat<\/span>/g) || []).length, 2);
  assert.doesNotMatch(shellRedirections, /postgis-shell-command">(?:out|input|log)<\/span>/);
  assert.match(shellRedirections, /postgis-shell-operator">2&gt;&gt;<\/span>/);
  const linkedPipeline = geometry.highlightShell(
    'postgis_restore old.backup | psql -d newdb',
    geometry.normalizeRefIndex(sandbox.window.POSTGIS_REF_INDEX)
  );
  assert.match(linkedPipeline,
    /<a class="postgis-shell-command postgis-shell-link" href="postgis_administration\.html#hard_upgrade"[^>]*>postgis_restore<\/a>/);
  assert.match(linkedPipeline, /postgis-shell-operator">\|<\/span>/);
  assert.match(linkedPipeline, /<span class="postgis-shell-command">psql<\/span>/);
  assert.doesNotMatch(linkedPipeline, /<a[^>]*>psql<\/a>/);
  assert.match(batchBlock.innerHTML, /postgis-shell-comment">REM In a \.bat file/);
  assert.match(batchBlock.innerHTML, /postgis-shell-keyword">for<\/span>/);
  assert.match(batchBlock.innerHTML, /postgis-shell-variable">%%F<\/span>/);
  assert.match(batchBlock.innerHTML, /postgis-shell-string">"%%~fF"<\/span>/);
  assert.match(batchBlock.innerHTML, /postgis-shell-string">"myschema\.%%~nF"<\/span>/);
  assert.match(batchBlock.innerHTML, /postgis-shell-command">shp2pgsql<\/span>/);
  assert.match(batchBlock.innerHTML, /postgis-shell-option">-d<\/span>/);
  assert.match(batchBlock.innerHTML, /postgis-shell-operator">\|<\/span>/);
  assert.strictEqual(geometry.codeText(batchBlock), batchSource);
  const batchRedirections = geometry.highlightBatch('echo ok > out\npsql -d roadsdb 2> errors.txt');
  assert.match(batchRedirections, /postgis-shell-keyword">echo<\/span>/);
  assert.match(batchRedirections, /postgis-shell-command">psql<\/span>/);
  assert.match(batchRedirections, /postgis-shell-operator">2&gt;<\/span>/);
  assert.match(htmlBlock.innerHTML, /postgis-html-tag">script<\/span>/);
  assert.match(htmlBlock.innerHTML, /postgis-html-attribute">type<\/span>/);
  assert.match(htmlBlock.innerHTML, /postgis-html-string">"text\/javascript"<\/span>/);
  assert.match(htmlBlock.innerHTML, /postgis-js-keyword">const<\/span>/);
  assert.match(htmlBlock.innerHTML, /postgis-js-function">decodePath<\/span>/);
  assert.strictEqual(geometry.codeText(htmlBlock), htmlSource);
  assert.match(javascriptBlock.innerHTML, /postgis-js-keyword">const<\/span>/);
  assert.match(javascriptBlock.innerHTML, /postgis-js-function">decodePath<\/span>/);
  assert.match(javascriptBlock.innerHTML, /postgis-js-comment">\/\/ polyline5<\/span>/);
  assert.strictEqual(geometry.codeText(javascriptBlock), javascriptSource);

  const refIndex = geometry.normalizeRefIndex(sandbox.window.POSTGIS_REF_INDEX);
  sandbox.window.location.pathname = '/manual/not-in-index.html';
  refnameText = 'ST_MakePoint — Creates a point.';
  assert.strictEqual(geometry.currentFunctionName(refIndex), 'ST_MAKEPOINT');
  sandbox.window.location.pathname = '';
  assert.strictEqual(geometry.currentFunctionName(refIndex), 'ST_MAKEPOINT');
  refnameText = '';
  assert.strictEqual(geometry.currentFunctionName(refIndex), null);
  assert.match(geometry.highlightSql('ST_Buffer(g, 1)', refIndex),
    /<a\b[^>]*href="ST_Buffer\.html"[^>]*>ST_Buffer<\/a>/);
  assert.match(geometry.highlightSql('VACUUM ANALYZE table_name;', refIndex),
    /<span class="postgis-sql-keyword">VACUUM<\/span> <span class="postgis-sql-keyword">ANALYZE<\/span>/);
  const doBlock = geometry.highlightSql(
    "DO $$\nDECLARE\n  invalid_count bigint;\nBEGIN\n  IF invalid_count > 0 THEN\n    EXECUTE format('SELECT 1');\n  END IF;\nEND;\n$$;",
    refIndex
  );
  assert.match(doBlock, /postgis-sql-string">\$\$<\/span>/);
  assert.match(doBlock, /postgis-sql-keyword">DECLARE<\/span>/);
  assert.match(doBlock, /postgis-sql-keyword">BEGIN<\/span>/);
  assert.match(doBlock, /postgis-sql-keyword">IF<\/span>/);
  assert.match(doBlock, /postgis-sql-keyword">EXECUTE<\/span>/);
  assert.doesNotMatch(doBlock, /postgis-sql-string">\$\$\nDECLARE/);
  assert.strictEqual(
    geometry.unicodePsqlOutput(' a | b\n---+---\n 1 | 2'),
    ' a │ b\n───┼───\n 1 │ 2'
  );
  assert.strictEqual(
    geometry.unicodePsqlOutput('GEOMETRYCOLLECTION(POINT(0 0)|POINT(1 1))'),
    'GEOMETRYCOLLECTION(POINT(0 0)|POINT(1 1))'
  );

  const matchedClasses = [new Set(), new Set()];
  const pairPre = {
    querySelectorAll() {
      return pairElements;
    }
  };
  const pairElements = matchedClasses.map(function (classes) {
    return {
      closest(selector) {
        if (selector === '.postgis-sql-paren[data-postgis-paren-pair]') {
          return this;
        }
        return selector === 'pre.programlisting' ? pairPre : null;
      },
      getAttribute() {
        return '7';
      },
      classList: {
        toggle(name, enabled) {
          if (enabled) {
            classes.add(name);
          } else {
            classes.delete(name);
          }
        }
      }
    };
  });
  listeners.mouseover({ target: pairElements[0] });
  assert(matchedClasses.every((classes) => classes.has('paren-match')));
  listeners.mouseout({ target: pairElements[0] });
  assert(matchedClasses.every((classes) => !classes.has('paren-match')));
  assert.strictEqual(typeof windowListeners.resize, 'function');

  let copiedPre = blocks[0];
  const wrapper = { querySelector: () => copiedPre };
  const button = {
    closest(selector) {
      return selector === '.postgis-example-code' ? wrapper : null;
    },
    getAttribute() {
      return null;
    },
    setAttribute() {}
  };
  listeners.click({
    target: {
      closest(selector) {
        return selector === '.postgis-copy-button' ? button : null;
      }
    }
  });
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(copiedText, 'a <= b');

  copiedPre = blocks[3];
  listeners.click({
    target: {
      closest(selector) {
        return selector === '.postgis-copy-button' ? button : null;
      }
    }
  });
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(copiedText, 'SELECT (a + (b))\nFROM things)');
}

main().then(function () {
  console.log('ok');
}).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
