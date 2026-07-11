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
    makePre('SELECT (a + (b))\nFROM things)')
  ];
  const output = makePre('wide output row\nsecond row');
  output.attributes = {};
  const trailingOutput = makePre('first row\nsecond row\n');
  trailingOutput.attributes = {};
  const listeners = {};
  const windowListeners = {};
  const wrapClasses = [new Set(), new Set()];
  const observedBlocks = [];
  let resizeObserverCallback = null;
  const layoutLines = wrapClasses.map(function (classes, index) {
    return {
      owner: index === 0 ? output : blocks[0],
      scrollHeight: 16,
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
      if (selector === '.postgis-example-block pre.programlisting, .postgis-example-block pre.screen') {
        return blocks.concat([output, trailingOutput]);
      }
      if (selector === '.postgis-example-block pre[data-postgis-lines="true"] > .line') {
        return layoutLines;
      }
      return [];
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
      location: { pathname: '/manual/ST_Test.html' },
      POSTGIS_REF_INDEX: {
        functions: {},
        operators: {
          '=': { href: 'equals.html', label: 'Operator', title: 'equals' },
          '==': { href: 'same.html', label: 'Operator', title: 'same' }
        },
        keywords: ['SELECT']
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

  const parserCases = [
    ['POINT(1 2)', 'POINT', 1],
    ['LINESTRING(0 0,1 1)', 'LINESTRING', 2],
    ['POLYGON((0 0,4 0,4 4,0 0))', 'POLYGON', 4],
    ['MULTIPOINT((0 0),(1 1))', 'MULTIPOINT', 2],
    ['MULTILINESTRING((0 0,1 1),(2 2,3 3))', 'MULTILINESTRING', 4],
    ['MULTIPOLYGON(((0 0,2 0,0 2,0 0)),((3 3,4 3,3 4,3 3)))', 'MULTIPOLYGON', 8],
    ['GEOMETRYCOLLECTION(POINT(0 0),LINESTRING(0 0,1 1))', 'GEOMETRYCOLLECTION', 3],
    ['BOX2D(1 2,5 6)', 'BOX2D', 2],
    ['BOX3D(1 2 3,5 6 7)', 'BOX3D', 2]
  ];
  parserCases.forEach(function ([text, type, vertices]) {
    const parsed = geometry.parseWkt(text);
    assert(parsed, text);
    assert.strictEqual(parsed.type, type);
    assert.strictEqual(parsed.vertices, vertices);
  });

  assert.deepStrictEqual(Array.from(geometry.parseWkt('SRID=4326;POINT Z (1 2 9)').primitives[0].points[0]),
    [1, 2]);
  assert.deepStrictEqual(Array.from(geometry.parseWkt('POINT M (3 4 99)').primitives[0].points[0]),
    [3, 4]);
  assert.deepStrictEqual(Array.from(geometry.parseWkt('POINT ZM (5 6 7 8)').primitives[0].points[0]),
    [5, 6]);
  assert.strictEqual(geometry.parseWkt('POINT Z (1 2)'), null);
  assert.strictEqual(geometry.parseWkt('BOX2D(1 2 3,4 5 6)'), null);
  assert.strictEqual(geometry.parseWkt('BOX3D(1 2,4 5)'), null);
  assert.strictEqual(geometry.parseWkt('POINT EMPTY'), null);
  assert.strictEqual(geometry.parseWkt('POINT(1)'), null);
  assert.strictEqual(geometry.parseWkt('LINESTRING(0 0)'), null);
  assert.strictEqual(geometry.parseWkt('LINESTRING(0 0,1 1 2)'), null);
  assert.strictEqual(geometry.parseWkt('POLYGON((0 0,1 0,0 1))'), null);
  assert.strictEqual(geometry.parseWkt('POLYGON((0 0,1 0'), null);
  assert.strictEqual(geometry.parseWkt('CIRCULARSTRING(0 0,1 1,2 0)'), null);
  assert.strictEqual(geometry.parseWkt('TIN(((0 0,1 0,0 1,0 0)))'), null);
  assert.strictEqual(geometry.scanGeometryLiterals('XPOINT(1 2)').length, 0);
  assert.strictEqual(geometry.scanGeometryLiterals('POINT(1 2)junk').length, 0);
  assert.strictEqual(geometry.scanGeometryLiterals("SELECT 'POINT EMPTY'").length, 0);
  assert.strictEqual(geometry.scanGeometryLiterals("SELECT 'POINT(1 2)', 'LINESTRING(0 0,1 1)'").length, 2);
  assert.strictEqual(geometry.scanGeometryLiterals('SELECT POINT(1 2)', true).length, 0);
  assert.strictEqual(geometry.scanGeometryLiterals("SELECT 'POINT(1 2)'", true).length, 1);
  assert.strictEqual(geometry.scanGeometryLiterals('SELECT $$POINT(1 2)$$', true).length, 1);
  assert.strictEqual(geometry.scanGeometryLiterals('SELECT $wkt$POINT(1 2)$wkt$', true).length, 1);
  assert.strictEqual(geometry.scanGeometryLiterals("-- 'POINT(1 2)'", true).length, 0);
  assert.strictEqual(geometry.scanGeometryLiterals("/* 'POINT(1 2)' */", true).length, 0);
  assert.strictEqual(geometry.scanGeometryLiterals('SELECT "POINT(1 2)"', true).length, 0);

  const tooManyPoints = 'LINESTRING(' + Array.from({ length: 2001 }, (_, index) =>
    index + ' ' + index).join(',') + ')';
  assert.strictEqual(geometry.parseWkt(tooManyPoints), null);

  const extent = geometry.geometryExtent([
    { geometry: geometry.parseWkt('POINT(0 0)') },
    { geometry: geometry.parseWkt('LINESTRING(10 5,20 15)') }
  ]);
  assert.deepStrictEqual(
    [extent.minX, extent.minY, extent.maxX, extent.maxY].map((value) => Number(value.toFixed(2))),
    [-1.6, -1.2, 21.6, 16.2]
  );
  const pointExtent = geometry.geometryExtent([{ geometry: geometry.parseWkt('POINT(2 3)') }]);
  assert(pointExtent.minX < 2 && pointExtent.maxX > 2 && pointExtent.minY < 3 && pointExtent.maxY > 3);
  assert.strictEqual(geometry.gridStep(83, 8), 20);
  assert.strictEqual(geometry.gridStep(0.0083, 8), 0.002);
  assert.strictEqual(geometry.gridStep(0, 8), 1);

  const literalOriginal = "SELECT 'POINT(1 2)'";
  const literalPre = makeLiteralTree(literalOriginal, document);
  geometry.wrapLiteralRange(literalPre, { start: 8, end: 18 }, 'postgis-geometry-99');
  assert.strictEqual(literalPre.textContent, literalOriginal);
  assert.strictEqual(geometry.codeText(literalPre), literalOriginal);
  const literalSpan = literalPre.children[1];
  assert.strictEqual(literalSpan.textContent, 'POINT(1 2)');
  assert.strictEqual(literalSpan.id, 'postgis-geometry-99-literal');
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
  assert.strictEqual(output.innerHTML,
    '<span class="line">wide output row</span><span class="line">second row</span>');
  assert.strictEqual(output.textContent, 'wide output rowsecond row');
  assert.strictEqual(trailingOutput.innerHTML,
    '<span class="line">first row</span><span class="line">second row</span>');
  assert.strictEqual((trailingOutput.innerHTML.match(/class="line"/g) || []).length, 2);
  assert(!trailingOutput.innerHTML.includes('<span class="line"></span>'));
  assert(observedBlocks.includes(output));
  assert(observedBlocks.includes(trailingOutput));
  assert(!wrapClasses[0].has('line-wrapped'));
  assert(!wrapClasses[1].has('line-wrapped'));
  layoutLines[0].scrollHeight = 32;
  resizeObserverCallback([{ target: output }]);
  assert.strictEqual(layoutLines[0].owner, output);
  assert(wrapClasses[0].has('line-wrapped'));

  const pairIds = Array.from(blocks[3].innerHTML.matchAll(/data-postgis-paren-pair="(\d+)"/g),
    (match) => match[1]);
  assert.strictEqual(pairIds.length, 4);
  assert.deepStrictEqual(Array.from(new Set(pairIds)).sort(), ['1', '2']);
  assert.deepStrictEqual(
    Array.from(blocks[3].innerHTML.matchAll(/data-postgis-paren-depth="(\d+)"/g), (match) => match[1]),
    ['0', '1', '1', '0', '0']
  );
  assert.match(blocks[3].innerHTML, /postgis-sql-paren paren-unmatched/);

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
