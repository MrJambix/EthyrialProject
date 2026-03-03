# Ethyrial Injector Pro

![alt text](image.png)
A modern, user-friendly DLL injector and buff tracker toolkit for **Ethyrial: Echoes of Yore**.

## Planned
### Telegraph

+0x30  RPGLibrary.SpellTargetType  TargetType      <-- spell shape type
+0x34  System.Boolean              SelfTarget
+0x58  System.Single               MaxRange         <-- how far it reaches
+0x5C  System.Single               CastTime         <-- wind-up duration
+0x64  System.Single               ChannelTime
+0xA0  RPGLibrary.ProgressInfo     CurrentChannelProgress
+0xA8  RPGLibrary.SpellPattern     Pattern          <-- the shape data!

+0x134  RPGLibrary.Directions      _direction       <-- (inherited from Entity) facing
+0x138  RPGLibrary.Position        _position        <-- (inherited from Entity) world pos
+0x248  Dictionary<String,Spell>   spellsDict       <-- all spells
+0x250  List<Spell>                spells           <-- all spells list
+0x258  DictionaryList<ProgressInfoType,ProgressInfo>  progresses  <-- active casts
+0x34C  Boolean                    IsBoss
+0x34D  Boolean                    IsElite
OnProgressStarted(params=2)                          <-- hook target
OnProgressEnded(params=2)                            <-- hook target

+0x20  MeshFilter       _meshFilter
+0x28  MeshRenderer     _meshRenderer
+0x30  Mesh             _mesh
+0x38  Vector3[]        _vertices
+0x40  Color32[]        _colors
+0x48  EntityModel      _entityModel
+0x5C  Single           _outerRadius
+0x60  Single           _midRadius
+0x64  Single           _innerRadius
+0x68  Vector2          _patternOffset     <-- interesting!
+0x70  Boolean          _dirty
+0x74  HitboxDisplay.Type  _currentType
SetType(params=1)
SetSize(params=1)
RebuildColors()

+0x38  Entity           _entity
+0x138 HitboxDisplay    HitboxDisplay      <-- DIRECT REFERENCE to the hitbox mesh!

+0x98  UnityEngine.Camera  myCamera        <-- for world-to-screen
+0x58  RPGLibrary.Position MyWorldPosition
Static: Instance

WorldToScreenPoint(params=1)  native=00007FF82DB359B0
WorldToScreenPoint(params=2)  native=00007FF82DB356B0
get_main(params=0)            native=00007FF82DB36410

## Features

### Buff Tracker Overlay
- **Real-time buff/debuff monitoring** with color-coded timer bars
- Permanent buffs shown with ∞ symbol
- Stack count display on multi-stack effects
- Draggable overlay window positioned anywhere on screen
- Auto-resizes based on active buff count

### Stacks Tracker Overlay
- **Tracks multi-stack effects** separately with peak tracking
- Shows current stacks, peak stacks, and remaining duration
- Fade-out animation when stacks drop off
- Color indicators: green = at peak, yellow = active, red = dropped

### Raid Tracker Overlay
- **Monitor party/raid members' buffs in real-time**
- Green dot = nearby, Red dot = out of range
- Shows each tracked player's active effects with timers
- Add players by name or pick from nearby entity list

### Auto Loot
- **Automatically loots all items** from open corpse loot windows
- Hooks into `ContainerWindow.Update` on the Unity main thread for stability
- Calls `LootAll` via `runtime_invoke` — same path as clicking the button in-game
- Smart item detection: waits for container details to arrive from the server before looting
- Grace period ensures the loot window is fully initialized before triggering
- Re-checks item count after looting and retries if items remain
- Skips empty containers and non-loot windows (inventory, bank, etc.)
- Auto-disables after repeated failures to prevent crashes
- Toggle via **Settings > Misc** tab
- Zero work on background threads — runs entirely on the Unity main thread

### Quick Cast
- **Instantly casts spells** without requiring a target confirmation click
- Hooks into the spell casting system to skip the targeting step
- Toggle with `CTRL+Q`
- Can be enabled/disabled from **Settings > Misc** tab

### Settings GUI (F11)
- **Full dark-themed custom-painted settings window**
- 4 tabs: Options | Buffs | Raid | Entities

#### Options Tab
- Auto-Find Player or Pick from Entity List
- Toggle tracking on/off with live status indicator
- Show/hide individual overlays (Buff, Stacks, Raid)
- Reset player, clear stacks history, save all configs

#### Buffs Tab
- Checkbox list of all discovered buffs
- Filter modes: Show All, Whitelist (only checked), Blacklist (hide unchecked)
- All ON / All OFF buttons
- Save config to file

#### Raid Tab
- Tracked players list with add/remove
- Nearby players list with refresh and one-click add
- Type-to-add text input with Enter support
- Save raid config to file

#### Entities Tab
- **Visual entity picker** — no more console number typing
- Shows all living entities with name, effect count, and status
- Color coded: green = current player, cyan = has effects, gray = mobs
- Click to select, click "Select This Entity" to start tracking
- Sorted: current player first, then by effect count

### Console (Advanced)
- Debug console with color-coded logging
- All features also accessible via keyboard shortcuts
- Crash handler with error code reporting

### Keyboard Shortcuts
| Key | Action |
|-----|--------|
| `F1` | Find player (auto-detect, falls back to entity picker) |
| `F2` | Start/Stop tracking |
| `F3` | Toggle Buff overlay |
| `F4` | Toggle Stacks overlay |
| `F5` | Buff filter config (console) |
| `F6` | List entities (console) |
| `F7` | Reset player |
| `F8` | Clear stacks history |
| `F9` | Raid config (console) |
| `F11` | **Settings Window (GUI)** |
| `CTRL+Q` | **Toggle Quick Cast** |
| `END` | Save all configs + Unload |

### Persistent Configuration
- `ethyrial_buffs.cfg` — Buff filter settings and enabled/disabled states
- `ethyrial_raid.cfg` — Raid tracker player list
- Auto-loads on startup, manual save from GUI or on unload

### Technical Details
- IL2CPP runtime resolver for Unity game internals
- GDI+ rendered overlays with transparency and anti-aliasing
- Multi-threaded: separate overlay, data, and input threads
- Main-thread hooks for Auto Loot and Quick Cast (Unity-safe)
- Safe memory reading with `IsBadReadPtr` validation and SEH wrappers
- `runtime_invoke` for managed method calls — proper IL2CPP calling convention
- DLL injection via standard injector

## How to Use

1. Launch **Ethyrial: Echoes of Yore**
2. Run the injector and inject the DLL
3. Wait for the console to show "READY"
4. Press `F1` to find your player (or `F11` → Entity Picker)
5. Press `F11` to open Settings and configure overlays, buffs, raid tracking, Auto Loot, and Quick Cast
6. Press `CTRL+Q` to toggle Quick Cast on/off
7. All overlays are draggable — position them wherever you like

## Credits

- Created by **MrJambix**
---

**RYAN IS AWARE OF THIS PROJECT.**