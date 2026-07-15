## 目的

<!-- 解决什么问题，为什么现在要改。 -->
关联 Issue：Closes #___ / N/A

## 主要变更

-
-

## 验证

- [ ] `cmake --preset core-only`
- [ ] `cmake --build --preset core-only`
- [ ] `ctest --preset core-only`
- [ ] 涉及 Qt/QML/存储/导入时已运行 `dev` 构建和 `ctest --preset dev`
- [ ] 已进行必要的人工播放或界面验证
- [ ] 文档已同步，或本次不需要更新文档

## 风险与回滚

风险：

回滚方式：Revert 本 PR 的 Squash Commit

## 界面或性能证据

<!-- UI 改动附截图；性能改动附测试方法与前后数据；不适用写 N/A。 -->

## 自审清单

- [ ] PR 只处理一个问题
- [ ] 没有无关格式化或重构
- [ ] 没有 WIP、敏感信息、构建产物或大文件
- [ ] PR 标题符合 Conventional Commits
