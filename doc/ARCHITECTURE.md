# MySongPlayer 架构与演进约束

**基线：** C++23、Qt 6.11、TagLib 2.x
**原则：** Qt 是 UI 与平台适配框架，不是业务领域模型。

## 1. 依赖方向

```text
QML UI
  │
  ▼
Qt Presentation（Controller / QAbstractItemModel）
  │                         │
  │                         ├──► Qt Infrastructure（Multimedia / Network / SQL）
  │                         │
  ▼                         └──► TagLib adapter
Portable C++ Core  ◄────────────────────┘
```

外层可以依赖内层，内层不能依赖外层。基础设施实现核心/应用层定义的端口，并由组合根装配；核心不得认识 `QObject`、QML、Qt SQL、Qt URL 或 TagLib。

## 2. 当前由构建系统强制的边界

| Target | 职责 | 允许依赖 |
|---|---|---|
| `MySongPlayerCore` | 实体、值类型、规则、算法和标准库端口 | C++ 标准库 |
| `MySongPlayerMetadata` | `IAudioMetadataReader` 的 TagLib 实现、封面缓存 | Core、TagLib；禁止 Qt |
| `MySongPlayerAppCore` | Qt 应用编排、展示模型、平台服务和 QML module | Core、Metadata、所需 Qt modules |
| `appMySongPlayer` | 进程入口与最终组合 | AppCore、Qt Gui/Quick |

`MYSONGPLAYER_BUILD_UI=OFF` 时不会查找 Qt 或 TagLib，可单独配置、构建并测试 Core。核心 target 关闭 Qt AUTOGEN，不链接任何框架；CMake 会扫描全部核心源文件，拒绝 Qt、TagLib 和外层目录 include。

## 3. 目录职责

| 目录 | 职责 |
|---|---|
| `src/core`、`src/include/core` | 纯 C++23 领域类型、端口与算法 |
| `src/infrastructure` | 第三方库实现；对上返回核心值类型 |
| `src/adapters` | Qt 值类型与标准库/核心类型的单点转换 |
| `src/models` | QML 可观察投影；所有对象只在 GUI 线程修改 |
| `src/controllers` | 面向 QML 的薄外观和用例入口 |
| `src/coordinators` | Qt 展示用例编排，不放文件/网络/SQL 实现 |
| `src/services`、`src/storage` | Qt 平台能力与持久化适配 |
| `qml` | 展示、绑定和用户交互，不承载业务规则 |

## 4. 本地音频导入协议

本地导入采用“一个有界后台任务 + GUI 线程提交结果”，而不是给每个文件创建无界线程：

```text
QML / GUI thread                         import pool (max 1 thread)
      │                                            │
      ├─ QList<QUrl> → std::filesystem::path       │
      ├─ validate batch / publish busy state       │
      ├──────── AudioImportRequest values ────────►│
      │                                            ├─ filesystem validation
      │                                            ├─ TagLib metadata read
      │                                            ├─ atomic cover-cache write
      │◄──── std::expected<ImportedAudio, Error> ──┤
      ├─ QFutureWatcher queued delivery            │
      ├─ update PlaylistModel (GUI only)            │
      └─ publish progress / terminal summary        │
```

必须保持以下不变量：

1. Worker 只接收拥有所有权的标准库值，不捕获 Controller、Model 或其他 QObject。
2. 元数据读取仍按用户选择顺序串行执行，避免磁盘争用并保证首曲语义确定。
3. 同一时间只允许一个 batch；第二个请求会被明确拒绝。
4. 取消是协作式的：已提交结果保留，当前 TagLib 调用结束后停止剩余文件，终态只发送一次。
5. `QFutureWatcher` 属于 GUI 线程；只有它的结果回调可以创建/修改 `AudioInfo` 和 `PlaylistModel`。
6. 导入期间禁止清空或加载其他播放列表，避免迟到结果进入错误上下文。
7. 导入期间暂停自动保存，终态后仅合并调度一次保存，避免逐首重写 SQLite。
8. Importer 析构时先取消并等待受控线程池结束，不允许后台访问已销毁对象。
9. 同名歌曲的封面缓存键包含完整源路径的确定性散列；先写临时文件，再原子 rename。

## 5. C++23 与 Qt 使用规则

- 跨层成功/失败使用 `std::expected`，可选值使用 `std::optional`，路径使用 `std::filesystem::path`。
- 核心集合、字符串和算法使用标准库；Qt 容器只留在 QML/Qt API 边界。
- QObject 所有权使用父子关系；非 QObject 依赖使用 RAII/智能指针，禁止裸 owning pointer。
- 不把 QObject、`QList<AudioInfo*>`、SQL query 或 Qt connection 传到工作线程。
- Qt 长任务使用有界 executor，并提供 busy、进度、错误、取消和唯一终态；不得通过 `moveToThread()` 后直接调用对象方法来伪装异步。
- 新业务规则先写 Core 测试，再增加 Qt adapter；QML 不实现去重、播放顺序、校验或持久化规则。

## 6. 架构验收

标准验证流程：

```bash
cmake --preset core-only
cmake --build --preset core-only
ctest --preset core-only

cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

验收项：

- Core-only 构建在禁用 Qt/TagLib package lookup 时通过。
- GCC/Clang 的 C++23 + warnings-as-errors 构建通过。
- Core、SQLite 和异步导入测试通过；QML lint 已纳入完整 CTest。
- 慢速 fake reader 执行时 GUI heartbeat 继续运行；reader 不在 GUI 线程，模型回调在 GUI 线程。
- 取消、并发请求、进度和终态汇总行为有确定性测试。
- 新核心文件即使忘记加入 target，也会进入架构依赖扫描。

## 7. 后续迁移债务

外围 Qt 代码仍在渐进拆分，新增代码不得扩大以下债务：

1. 将现有 `IPlaylist*`/`ICurrentSongManager` 明确为 Qt presentation interfaces，随后用标准库 `PlaylistSnapshot` 和 repository port 替换其中的 `QString`、`AudioInfo*` 与具体 StorageService。
2. 把 `MySongPlayerAppCore` 继续拆为 Qt Presentation、Multimedia、Network 和 SQL targets，让链接图约束依赖方向。
3. 让 `PlaylistModel` 只投影一个权威 Core playlist，移除 Qt/Core 双集合手动同步。
4. 把歌词文件查找、网络搜索映射和数据库快照迁移为标准库端口；各自的 Qt 实现留在 infrastructure。

这些项目是有记录、可测试的迁移队列，不是新功能继续耦合到 Qt 单体的理由。
