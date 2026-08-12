# Asset Tab Groups

[中文说明](README.zh-CN.md)

> Unreal Engine 4/5 Editor plugin for managing **asset editor tabs**, **workspace sessions**, and **Content Browser asset groups**.

Asset Tab Groups lets you save the assets you are working on into named groups, reopen them later, and switch editing context with **Open Group** or **Focus Group** workflows. It is similar to browser tab groups or IDE workspaces, but designed for the Unreal Editor asset-editing workflow.

## Why use it?

Unreal Editor projects often require several related assets to be open at the same time: Blueprints, Materials, Data Assets, Animation assets, and more. This plugin turns that temporary editor state into reusable **asset editor workspaces** without modifying the assets themselves.

## Features

- **Asset editor tab groups** — create, rename, delete, color-tag, annotate, and reorder groups.
- **Drag-to-reorder Groups** — use the six-dot handle to move a Group and persist its order.
- **Capture open editors** — save all open assets, selected open assets, or assets chosen through Content Browser.
- **Content Browser integration** — add files from an asset picker or add the assets currently selected in Content Browser.
- **Open Group** — open all available members and focus the Group's active asset.
- **Focus Group (Safe)** — open a Group and safely close clean asset editors outside it.
- **List and Tile views** — browse members with Content Browser-style columns or thumbnails.
- **Tile multi-selection** — use normal click, `Ctrl` click, and `Shift` click to select multiple assets.
- **Batch asset actions** — open selected Tile assets or remove selected asset references from a Group.
- **Missing asset tracking** — failed restores record a reason so stale entries can be reviewed or removed.
- **Persistent workspace data** — store Group metadata under `<Project>/Saved/AssetTabGroups/Workspace.json`.
- **Editor-native Slate UI** — use plugin-local styles with UE4/UE5 editor theme compatibility.

## Supported Unreal Engine versions

- **Unreal Engine 4.26** — supported through `ENGINE_MAJOR_VERSION` compatibility branches.
- **Unreal Engine 5** — supported; the exact minor-version result depends on the target project's engine version.

This is an editor-only source plugin. It does not require UMG widgets or plugin content assets.

## Installation

1. Copy or clone the plugin into your project:

   ```text
   YourProject/Plugins/AssetTabGroups/
   ```

2. Regenerate project files and compile your editor target.
3. Enable **Asset Tab Groups** under **Edit → Plugins → Editor** if it is not already enabled.
4. Open the panel from **Window → Asset Tab Groups**.

For source builds, compile the editor target with your project's normal configuration. A DebugGame Editor example is:

```bat
Engine\Build\BatchFiles\Build.bat YourProjectEditor Win64 DebugGame -Project="YourProject.uproject"
```

The plugin module name is **AssetTabGroups** and its module type is **Editor**.

## Quick start

### Save the current asset editor workspace

1. Open the asset editors you want to track.
2. Open **Window → Asset Tab Groups**.
3. Choose one of:
   - **Save All Open** — capture every compatible open asset editor.
   - **Save Selected** — choose which open asset editors to capture.
   - **New Group** — create an empty Group and add files later.
4. Use the six-dot handle to adjust the Group order.

### Restore a workspace

- **Open** opens all available Group members and focuses the active asset.
- **Focus** opens the Group and attempts to close clean asset editors that are not part of it.

Focus mode keeps dirty editors, world assets, PIE/simulation-related editors, and transient editors when it cannot safely close them.

### Work with Group members

- Switch between **List** and **Tile** views.
- In Tile view, use `Ctrl` and `Shift` for multi-selection.
- Double-click an asset to open it.
- Right-click selected Tiles to open all selected assets or remove their references from the Group.
- Right-click empty detail space to add files, add selected Content Browser assets, open the Group, or focus the Group.

## Missing assets

If an asset cannot be restored, the plugin keeps the Group entry and records the failure reason. Missing entries are marked in the Group and member views so you can:

- inspect the missing asset path and reason;
- retry the Group after fixing or restoring the asset;
- remove recorded missing entries in the Group menu.

## Data and privacy

Group metadata is stored locally in the project:

```text
<Project>/Saved/AssetTabGroups/Workspace.json
```

The plugin does not use network access. Deleting a Group removes only the saved Group metadata; it does not delete or modify the referenced assets.

## Current limitations

- Blueprint node positions, graph zoom, selection, and viewport state are not restored.
- JSON import/export is not currently provided as a public sharing workflow.
- The plugin manages editor-side asset references; it does not replace Content Browser collections or modify asset packages.

## FAQ

### Does this replace Content Browser Collections?

No. Collections are still useful for long-term asset categorization. Asset Tab Groups are focused on the temporary set of asset editors you have open during a task.

### Does the plugin create UMG assets?

No. The interface is built with Slate and is contained in the editor plugin module.

### Can I share a workspace with another developer?

The workspace file is local project metadata. A public JSON import/export workflow is not currently included, so sharing is not treated as a stable interchange format.

### Is this a runtime plugin?

No. It is an Unreal Editor-only plugin and is not included in cooked runtime builds.

## Project layout

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
            ├── UI/                 # Slate panel
            ├── Commands/           # User actions
            ├── Repository/         # JSON persistence
            ├── Session/            # Asset editor integration
            └── AssetTabGroupsCompatibility.h
```

## License

MIT License — see [LICENSE](LICENSE).

## Contributing

Issues and pull requests are welcome at [github.com/Whalesea-Gustav/AssetTabGroups](https://github.com/Whalesea-Gustav/AssetTabGroups).
