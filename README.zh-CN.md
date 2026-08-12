# Asset Tab Groups

[English README](README.md)

> 面向 Unreal Engine 4/5 的 Editor 插件，用于管理 **Asset Editor 标签页**、**工作区会话** 和 **Content Browser 资产分组**。

Asset Tab Groups 可以把当前正在编辑的资产保存为可复用的命名分组，之后重新打开，并通过 **Open Group** 或 **Focus Group** 快速切换工作上下文。它类似浏览器 Tab Groups 或 IDE Workspace，但专门面向 Unreal Editor 的资产编辑流程。

## 解决什么问题？

在 Unreal Engine 项目中，经常需要同时打开 Blueprint、Material、Data Asset、Animation 等一组相关资产。这个插件可以把临时打开的资产编辑器状态保存为工作区，不修改资产本身，也不需要手动重新寻找和打开文件。

## 当前功能

- **Asset Editor 标签页分组**：创建、重命名、删除、设置颜色、添加备注和排序 Group。
- **六点拖拽排序**：拖拽 Group 左侧的六点 handle 调整顺序，并自动持久化。
- **保存当前打开资产**：保存全部打开资产、选择部分打开资产，或从 Content Browser 选择资产。
- **Content Browser 集成**：从资产选择窗口添加文件，或直接添加当前 Content Browser 选中的资产。
- **Open Group**：打开 Group 中仍然可用的资产，并聚焦 Group 的活动资产。
- **Focus Group（安全聚焦）**：打开 Group，并尝试关闭 Group 外部未修改的资产编辑器。
- **List / Tile 视图**：使用类似 Content Browser Columns 的列表，或缩略图 Tile 视图浏览成员。
- **Tile 多选**：普通点击单选，`Ctrl` 点击切换选择，`Shift` 点击范围选择。
- **批量操作**：右键批量打开选中的 Tile，或从当前 Group 中删除选中的资产引用。
- **缺失资产追踪**：恢复失败时记录具体原因，可检查或批量移除失效条目。
- **工作区持久化**：Group 元数据保存在 `<Project>/Saved/AssetTabGroups/Workspace.json`。
- **Editor 原生 Slate UI**：使用插件本地样式，并兼容 UE4/UE5 编辑器主题。

这是一个 Editor-only 源码插件，不需要创建 UMG Widget 或额外的插件资产。

## 支持的 Unreal Engine 版本

- **Unreal Engine 4.26**：通过 `ENGINE_MAJOR_VERSION` 兼容分支支持。
- **Unreal Engine 5**：支持，具体小版本表现取决于项目使用的引擎版本。

## 安装

1. 将插件复制或克隆到项目目录：

   ```text
   YourProject/Plugins/AssetTabGroups/
   ```

2. 重新生成项目文件并编译 Editor Target。
3. 如果插件尚未启用，在 **Edit → Plugins → Editor** 中启用 **Asset Tab Groups**。
4. 通过 **Window → Asset Tab Groups** 打开面板。

源码编译可以使用项目常规配置。`DebugGame Editor` 示例：

```bat
Engine\Build\BatchFiles\Build.bat YourProjectEditor Win64 DebugGame -Project="YourProject.uproject"
```

插件模块名为 **AssetTabGroups**，模块类型为 **Editor**。

## 快速开始

### 保存当前资产编辑工作区

1. 打开需要同时处理的资产编辑器。
2. 打开 **Window → Asset Tab Groups**。
3. 选择：
   - **Save All Open**：保存所有可识别的已打开资产编辑器。
   - **Save Selected**：选择需要保存的已打开资产编辑器。
   - **New Group**：创建空 Group，之后再添加文件。
4. 使用 Group 左侧的六点 handle 调整顺序。

### 恢复工作区

- **Open**：打开 Group 中仍然可用的资产，并聚焦活动资产。
- **Focus**：打开 Group，并尝试关闭 Group 外部可以安全关闭的未修改资产编辑器。

Focus 模式会保留已修改资产、World 相关资产、PIE/Simulation 相关编辑器以及临时资产编辑器。

### 管理 Group 文件

- 在 **List** 和 **Tile** 视图之间切换。
- Tile 模式下使用 `Ctrl`、`Shift` 进行多选。
- 双击资产打开对应编辑器。
- 右键已选 Tile，可以批量打开资产或从 Group 中删除资产引用。
- 在详情区域空白处右键，可以添加文件、添加 Content Browser 当前选中资产、打开 Group 或 Focus Group。

## 缺失资产处理

如果资产无法恢复，插件会保留 Group 条目并记录失败原因。缺失资产会在 Group 和成员视图中标记出来，可以：

- 查看缺失资产路径和失败原因；
- 修复或恢复资产后重新执行 Group 恢复；
- 从 Group 菜单中移除已记录的缺失资产。

## 数据与隐私

Group 元数据保存在项目本地：

```text
<Project>/Saved/AssetTabGroups/Workspace.json
```

插件不使用网络访问。删除 Group 只会删除保存的 Group 元数据，不会删除或修改被引用的资产。

## 当前限制

- 不恢复 Blueprint 节点位置、Graph 缩放、节点选择和视图状态。
- 当前不提供用于团队共享的公开 JSON 导入/导出流程。
- 插件管理的是 Editor 侧的资产引用，不替代 Content Browser Collections，也不会修改资产包。

## 常见问题

### 它会替代 Content Browser Collection 吗？

不会。Collection 适合长期资产分类；Asset Tab Groups 更适合保存当前任务中临时打开的一组资产编辑器。

### 需要创建 UMG 资产吗？

不需要。界面使用 Slate 构建，包含在 Editor 插件模块中。

### 可以和其他开发者共享工作区吗？

工作区是项目本地元数据。目前没有稳定的公共 JSON 导入/导出格式，因此暂不把该文件作为团队共享格式。

### 这是 Runtime 插件吗？

不是。它是 Unreal Editor-only 插件，不会被打包进运行时 Cook 结果。

## 项目结构

```text
AssetTabGroups/
├── AssetTabGroups.uplugin
├── LICENSE
├── README.md
├── README.zh-CN.md
└── Source/
    └── AssetTabGroups/
        ├── AssetTabGroups.Build.cs
        ├── Public/
        └── Private/
            ├── UI/                 # Slate 面板
            ├── Commands/           # 用户操作
            ├── Repository/         # JSON 持久化
            ├── Session/            # 资产编辑器集成
            └── AssetTabGroupsCompatibility.h
```

## License

MIT License，详见 [LICENSE](LICENSE)。

## 贡献

欢迎提交 Issue 和 Pull Request：

[github.com/Whalesea-Gustav/AssetTabGroups](https://github.com/Whalesea-Gustav/AssetTabGroups)
