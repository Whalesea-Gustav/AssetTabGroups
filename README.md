# Asset Tab Groups

Unreal Editor plugin for organizing and restoring groups of open asset editor tabs.

Save the assets you are working on into named groups, reopen them later, and switch context with **Open** or **Focus** workflows—similar to browser tab groups or IDE session layouts, but for the Content Browser asset editor ecosystem.

## Features

- **Group management** — create, rename, delete, reorder, color-tag, and annotate groups
- **Capture open editors** — save all open assets, selected open assets, or pick from Content Browser
- **Restore sessions** — **Open Group** opens all members; **Focus Group** opens the group and safely closes clean editors outside it
- **List and tile views** — browse group members with thumbnails in tile mode
- **Persistent workspace** — groups stored under `<Project>/Saved/AssetTabGroups/Workspace.json`
- **Missing asset tracking** — failed restores are recorded so you can review or remove stale entries
- **Editor-native UI** — styling follows `FAppStyle` (UE5) / `FEditorStyle` (UE4)

## Supported Engine Versions

| Engine | Status |
|--------|--------|
| **Unreal Engine 5.4+** | Primary target, tested in development |
| **Unreal Engine 4.26** | Supported via `ENGINE_MAJOR_VERSION` compatibility shims |

## Installation

1. Clone this repository into your project's `Plugins` folder:

   ```text
   YourProject/Plugins/AssetTabGroups/
   ```

2. Regenerate project files and compile the editor target.

3. Enable **Asset Tab Groups** in **Edit → Plugins → Editor** if it is not already enabled.

4. Open the panel from **Window → Asset Tab Groups** (or use the Window menu entry registered by the plugin).

## Usage

### Create a group from open assets

1. Open the asset editors you want to track (materials, blueprints, etc.).
2. Open **Asset Tab Groups**.
3. Click **Save All Open** or **Save Selected** and name the group.

### Restore a group

- **Open** — opens all available members and focuses the group's active asset.
- **Focus (Safe)** — same as Open, then attempts to close *clean* asset editors that are not in the group. Dirty, world, PIE/simulation, and transient assets are kept.

### Manage members

Right-click in the group file list (or an empty group area) to:

- Add files from Content Browser
- Add currently selected Content Browser assets
- Remove individual members

Group-level actions (rename, note, color, delete) are available from the group row context menu or the `...` menu in the details header.

## Data & privacy

Group metadata is stored locally in your project:

```text
<Saved>/AssetTabGroups/Workspace.json
```

No network access. No assets are modified when deleting a group—only the group metadata is removed.

## Project layout

```text
AssetTabGroups/
├── AssetTabGroups.uplugin
├── LICENSE
├── README.md
└── Source/
    └── AssetTabGroups/
        ├── AssetTabGroups.Build.cs
        ├── Public/
        └── Private/
            ├── UI/                 # Slate panel
            ├── Commands/           # User actions
            ├── Repository/         # JSON persistence
            ├── Session/            # AssetEditorSubsystem integration
            └── AssetTabGroupsCompatibility.h
```

## Building from source

Compile your editor target as usual, for example:

```bat
Engine\Build\BatchFiles\Build.bat YourProjectEditor Win64 Development -Project="YourProject.uproject"
```

The plugin module name is **AssetTabGroups** (Editor type).

## License

MIT License — see [LICENSE](LICENSE).

## Contributing

Issues and pull requests are welcome at [github.com/Whalesea-Gustav/AssetTabGroups](https://github.com/Whalesea-Gustav/AssetTabGroups).
