# 贡献指南

MySongPlayer 使用单主线、短主题分支、自审 Pull Request 和 Squash Merge 的轻量工作流。`main` 是唯一集成与发布主线，应始终保持可构建、可测试。

## 分支

从最新的 `main` 创建主题分支：

```bash
git switch main
git pull --ff-only origin main
git switch -c feat/batch-import
```

按改动目的选择前缀：

- `feat/<slug>`：用户功能。
- `fix/<slug>`：普通缺陷。
- `refactor/<slug>`：不夹带独立功能的重构。
- `docs/<slug>`、`build/<slug>`、`ci/<slug>`：文档、构建和自动化。
- `hotfix/vX.Y.Z-<slug>`：已发布版本的临时紧急修复。

主题分支通常应在 1～3 天内完成，最长不超过 7 天。合并后删除本地和远端分支。不要创建长期 `develop`、`release` 或 `hotfix` 分支。

个人主题分支使用 Rebase 同步主线：

```bash
git fetch origin
git rebase origin/main
git push --force-with-lease
```

仅已推送且由自己独占的主题分支可以使用 `--force-with-lease`。共享分支、`main`、维护分支和 Tag 不得重写历史。

## 提交

提交和 PR 标题采用轻量 Conventional Commits：

```text
<type>(<scope>): <简洁说明>
```

常用类型为 `feat`、`fix`、`refactor`、`test`、`build`、`ci`、`docs`、`perf` 和 `chore`。Scope 可使用 `core`、`qml`、`import`、`player`、`playlist`、`storage`、`lyrics` 或 `release`。

示例：

```text
feat(import): 支持批量导入进度反馈
fix(qml): 修复音量滑块点击定位错误
refactor(core): 分离可移植播放列表规则
test(storage): 覆盖播放列表往返持久化
```

一个提交只表达一个原因明确的改动。不要使用 `fix 03`、`update something`、`.` 或 `try again`。Draft 分支可以暂存 WIP 或 `fixup!` 提交，但 WIP 不得进入 `main`。

## 本地验证

只改动标准库核心时至少运行：

```bash
cmake --preset core-only
cmake --build --preset core-only
ctest --preset core-only
```

涉及 Qt、QML、存储、导入或应用集成时运行完整验证：

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

界面和播放行为仍需进行与改动相匹配的人工验证。

## Pull Request

- 所有变更都通过目标为 `main` 的 PR 合并。
- 一个 PR 解决一个问题，并包含所需测试和文档。
- 推荐不超过约 400 行有效代码和 10 个实现文件；大型机械迁移需在描述中说明。
- 超过一天、用户可见缺陷或需要讨论的功能应关联 Issue；小型文档或局部维护可写 `N/A`。
- QML/UI 改动附截图；性能或异步行为声明附复现方法和数据。
- CI Job `core` 必须通过。涉及 Qt 层时在 PR 中记录完整 `dev` 测试结果。
- 当前维护者可以在 CI 通过并完成自审后自行合并；外部贡献必须由维护者审核。

仓库只使用 Squash Merge。最终 Squash Commit 标题应与符合 Conventional Commits 的 PR 标题一致。合并后以该提交作为回滚单元。

## 发布

正式版本只从经过验证的 `main` 创建，Tag 使用注解形式 `vX.Y.Z`，并与 CMake 项目版本一致：

```bash
git tag -a v1.1.0 -m "MySongPlayer v1.1.0"
git push origin v1.1.0
```

不得移动或复用已发布 Tag。当前不维护历史发布线；只有明确需要并行支持旧版本时才创建 `maint/X.Y`。

## 敏感信息和二进制文件

不要提交令牌、密码、私钥、个人数据、构建产物或媒体演示文件。敏感信息一旦推送，应先撤销或轮换凭据，再处理 Git 历史。大型演示资产应上传到 GitHub Release 或外部存储；只有确实需要版本化的大型二进制资源才考虑 Git LFS。
