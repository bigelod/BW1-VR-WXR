# FUTURE_GOALS.md - BW1-WXR Remaining Features (Not Yet Started)

Features planned but not yet started - post alpha.

## Input & Interaction Polish

- [ ] **Velocity estimation** - Controller velocity derived from successive packets for realistic throw (currently no velocity, only pos). HaloCEWXR estimates velocity; COD4 WXR notes "Controller velocity is estimated from successive packets"
- [ ] **Miracle gesture recognition** - B Hold + draw gesture in air. Need 6DOF stroke recorder + recognizer tuned for WinlatorXR tracking jitter
- [ ] **Point, grip, pull** - Bring distant object to hand. Requires raycast from controller aim pose, currently only near grab implemented
- [ ] **Leash control** - Right trigger to send creature. Need proper aim vs head-aim toggle
- [ ] **Two-handed land manipulation** - Both grips to turn and zoom land mass - needs relative hand delta logic
- [ ] **Seated mode** - Optional Non-VR mode (like Halo's `MODE_VR=2`) that plays like flat game but with 6DOF motion tracking, for low-comfort users

## UI / UX

- [ ] **Virtual screen for menus** - Menus and cinematics have no compositor layer in WXR, so draw on virtual screen 2m in front (like COD4 does: menus from eye they were drawn into 4:3, cinematics 16:9 anchored when appears)
- [ ] **Ring menu depth** - Y hold ring menu needs 3D placement at hand, not screen-locked
- [ ] **HUD safe zone** - Lower HUD for tall Quest lens (COD4 lowers to 0.60) - B&W's advisor text currently may be cut off
- [ ] **Recenter (F11)** - Long press menu button to recenter. Currently Halo notes "No recentre when pressing and holding menu button (use HMD reset instead)" - need proper implementation syncing player relative to game, not tethered to real device
- [ ] **ConfTool / Launcher** - External config tool like Halo's `HWXR_ConfTool` for remapping, haptics strength, FOV override, comfort toggles

## Rendering

- [ ] **AER stereo vs SBS toggle** - Currently forced SBS (MODE_3D=1). Add AER support (MODE_3D=2) using blue channel of red pixel for experimental 3D
- [ ] **Resolution scaling** - Launcher "Resolution scale" option needs mapping to WinlatorXR virtual screen resolution (Quest 3 eye aspect ~1.105:1, e.g., 2388x1080 total)
- [ ] **MSAA / DXVK compatibility** - WinlatorXR uses DXVK 1.10.1 + Turnip driver. Need to test DDraw -> DXVK translation for B&W's old pipeline
- [ ] **Dedicated scope panel** - COD4 notes "No dedicated physical-scope panel. The window has no room for it" - similar for B&W's miracle zoom

## Haptics & Audio

- [ ] **Per-action haptics** - Map grab, throw, miracle cast, creature slap to left/right vibration amplitudes from `config.txt` floats (like Halo's `PlasmaPistolHapticStrength`)
- [ ] **3D audio** - B&W uses headphones with world-placed sound. Verify audio not causing massive slowdown (Halo notes "Investigate ways to enable audio without massive slowdown (known issue of Halo CE on WinlatorXR)")
- [ ] **Rumble via gesture** - Melee via physical gesture based on positional data only, adjustable sensitivity (Halo done)

## Performance & Compatibility

- [ ] **Boundless mode** - Ability to disable thumbstick move and enable virtual body move to catch up to HMD movements, scalable IRL vs in-game
- [ ] **Pico 4 / Quest 2 controller flip** - Auto-enable extra 180-degree controller flip on Quest 2, Quest Pro and Pico (HWXR reference does)
- [ ] **Multiplayer / Creature Isle** - Original BW1-VR targets v1.42 without Creature Isle. Future: support Creature Isle expansion
- [ ] **Save system in VR** - Quick save in ring menu under "More" needs controller-accessible file dialog

## WinlatorXR Integration

- [ ] **Installer package** - Quest installer like `Setup-BFVR-Quest.bat` that stages game + `ddraw.dll` + `VR/fov.txt` + shortcut creation on WinlatorXR desktop
- [ ] **UDP debug forwarding** - Support debug build that streams to PC IP via `udp_debug` folder + file named after dev server IP
- [ ] **System file parsing** - Read `Z:/tmp/xr/system` two lines (make/model) to select controller orientation quirks per headset
- [ ] **Version negotiation** - Support both 0.2 (old) and 0.5 (current) API versions based on what container reports

## Stretch Goals

- [ ] Left/right hand flipping for movement sticks
- [ ] Hand tracking (no controllers) experimental
- [ ] Mixed Reality passthrough for table-top mode (land on real table)
