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
  const blocks = [makePre('\na <= b\n   '), makePre('a = b')];
  const listeners = {};
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
          '=': { href: 'equals.html', label: 'Operator', title: 'equals' }
        },
        keywords: ['SELECT']
      },
      clearTimeout() {},
      setTimeout() {}
    }
  };

  vm.runInNewContext(fs.readFileSync('doc/html/example-blocks.js', 'utf8'), sandbox,
    { filename: 'example-blocks.js' });
  listeners.DOMContentLoaded();
  await Promise.resolve();
  await Promise.resolve();

  assert.strictEqual(blocks[0].textContent, 'a <= b');
  assert.strictEqual(blocks[0].innerHTML, 'a &lt;= b');
  assert.doesNotMatch(blocks[0].innerHTML, /<a\b/);
  assert.match(blocks[1].innerHTML, /<a\b[^>]*>\=<\/a>/);

  const wrapper = { querySelector: () => blocks[0] };
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
}

main().then(function () {
  console.log('ok');
}).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
