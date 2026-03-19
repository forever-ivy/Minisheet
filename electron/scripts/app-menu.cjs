function buildAppMenuTemplate(platform = process.platform, appName = 'Minisheet') {
  const template = [];

  if (platform === 'darwin') {
    template.push({
      label: appName,
      submenu: [
        { role: 'about', label: `关于 ${appName}` },
        { type: 'separator' },
        { role: 'services', label: '服务' },
        { type: 'separator' },
        { role: 'hide', label: `隐藏 ${appName}` },
        { role: 'hideOthers', label: '隐藏其他' },
        { role: 'unhide', label: '全部显示' },
        { type: 'separator' },
        { role: 'quit', label: '退出' },
      ],
    });
  }

  template.push(
    {
      label: '文件',
      submenu: [
        platform === 'darwin'
          ? { role: 'close', label: '关闭窗口' }
          : { role: 'quit', label: '退出' },
      ],
    },
    {
      label: '编辑',
      submenu: [
        { role: 'undo', label: '撤销' },
        { role: 'redo', label: '重做' },
        { type: 'separator' },
        { role: 'cut', label: '剪切' },
        { role: 'copy', label: '复制' },
        { role: 'paste', label: '粘贴' },
        { role: 'selectAll', label: '全选' },
      ],
    },
    {
      label: '视图',
      submenu: [
        { role: 'reload', label: '重新加载' },
        { role: 'forceReload', label: '强制重新加载' },
        { role: 'toggleDevTools', label: '切换开发者工具' },
        { type: 'separator' },
        { role: 'resetZoom', label: '实际大小' },
        { role: 'zoomIn', label: '放大' },
        { role: 'zoomOut', label: '缩小' },
        { type: 'separator' },
        { role: 'togglefullscreen', label: '切换全屏' },
      ],
    },
    {
      label: '窗口',
      submenu:
        platform === 'darwin'
          ? [
              { role: 'minimize', label: '最小化' },
              { role: 'zoom', label: '缩放' },
              { type: 'separator' },
              { role: 'front', label: '移到最前' },
            ]
          : [
              { role: 'minimize', label: '最小化' },
              { role: 'close', label: '关闭窗口' },
            ],
    },
    {
      label: '帮助',
      submenu: [
        {
          label: 'Minisheet 帮助',
          enabled: false,
        },
      ],
    },
  );

  return template;
}

module.exports = {
  buildAppMenuTemplate,
};
