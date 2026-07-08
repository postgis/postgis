(function () {
  'use strict';

  var resetTimers = new WeakMap();

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
