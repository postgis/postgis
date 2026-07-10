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

async function main() {
  const blocks = [
    makePre('\na <= b\n \n   '),
    makePre('a = b'),
    makePre('a == b'),
    makePre('SELECT (a + (b))\nFROM things)')
  ];
  const output = makePre('wide output row\nsecond row');
  output.attributes = {};
  const listeners = {};
  const windowListeners = {};
  const wrapClasses = [new Set(), new Set()];
  const layoutLines = wrapClasses.map(function (classes, index) {
    return {
      scrollHeight: index === 0 ? 32 : 16,
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
      listeners[name] = handler;
    },
    querySelectorAll(selector) {
      if (selector === '.postgis-example-code pre.programlisting[data-postgis-language="sql"]') {
        return blocks;
      }
      if (selector === '.postgis-example-block pre.programlisting, .postgis-example-block pre.screen') {
        return blocks.concat([output]);
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
      }
    }
  };

  vm.runInNewContext(fs.readFileSync('doc/html/example-blocks.js', 'utf8'), sandbox,
    { filename: 'example-blocks.js' });
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
  assert.strictEqual(blocks[3].textContent, 'SELECT (a + (b))\nFROM things)');
  assert.strictEqual(output.innerHTML,
    '<span class="line">wide output row</span>\n<span class="line">second row</span>');
  assert.strictEqual(output.textContent, 'wide output row\nsecond row');
  assert(wrapClasses[0].has('line-wrapped'));
  assert(!wrapClasses[1].has('line-wrapped'));

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
        return selector === '.postgis-sql-paren[data-postgis-paren-pair]' ? this : pairPre;
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
