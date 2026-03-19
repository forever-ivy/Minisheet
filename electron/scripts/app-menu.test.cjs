const test = require('node:test');
const assert = require('node:assert/strict');

const {
  buildAppMenuTemplate,
} = require('./app-menu.cjs');

test('uses Chinese labels for the standard desktop menu sections on Windows and Linux', () => {
  const template = buildAppMenuTemplate('win32', 'Minisheet');

  assert.deepEqual(
    template.map((item) => item.label),
    ['文件', '编辑', '视图', '窗口', '帮助'],
  );
});

test('keeps the app menu on macOS and translates the standard sections to Chinese', () => {
  const template = buildAppMenuTemplate('darwin', 'Minisheet');

  assert.equal(template[0].label, 'Minisheet');
  assert.deepEqual(
    template.slice(1).map((item) => item.label),
    ['文件', '编辑', '视图', '窗口', '帮助'],
  );
});
