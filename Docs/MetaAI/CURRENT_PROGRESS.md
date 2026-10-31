# CURRENT_PROGRESS.md - BW1-WXR In-Progress Features

Status as of 2026-10-30 - alpha scaffold phase.

## Completed (scaffold)

- [x] VS2022 project scaffold `BW1WXR.vcxproj` + `BW1WXR.sln` created (Win32, v143, C++17)
- [x] `WinXrApiUDP` implementation: WSAStartup, bind 7872, recv thread, C locale `istringstream` parsing, `TryGetLatest` / blocking `GetLatest` with condition_variable
- [x] `WinXrApi` handshake: detects `Z:/` vs `D:/xrtemp`, creates marker dirs via `std::filesystem`, writes `vr` and `version` = `0.5`
- [x] FOV calibration file loader: reads 6 floats from `VR/fov.txt` (A,B,C,D,E,F) and applies `FOVH = A*inH + B`
- [x] FOV + haptics sender to 7278
- [x] Controller 180-degree roll fixup placeholder (`XMQuaternionRotationRollPitchYaw(0,0,PI)`)
- [x] Fullscreen borderless helper stripping `WS_THICKFRAME|WS_BORDER|WS_CAPTION`
- [x] Red pixel marker D3D9 implementation stub: 10x10 quad at (0,0) with ARGB(255, FrameID, 0, EyeValue) - needs port to B&W's DDraw/D3D7 path

## In-Progress (active work)

- [ ] **Render path conversion** - BW1-VR currently submits to OpenXR swapchain. Needs conversion to SBS viewport rendering:
  - Left viewport (0,0,W/2,H) + right viewport (W/2,0,W/2,H) into same backbuffer
  - Disable mirror device creation (currently commented out in HaloCEWXR)
  - Ensure `StretchRect` blit path used by Halo (`Game.cpp:664-665`) equivalent exists for B&W
  - Need to verify B&W's original present is DDraw Flip vs Blt

- [ ] **VR Backend factory integration** - Find BW1-VR's `CreateVRBackend()` / `VRSystem::Init()` and inject:
  ```cpp
  if (PathFileExists("Z:/tmp/xr/system") || PathFileExists("D:/xrtemp"))
    return new WinXrApi();
  ```
  - Determine exact interface: does BW1-VR use `IVR` like Halo or `OpenXRBackend` class?

- [ ] **Input mapping for Black & White hands**
  - Right grip = close hand/take/throw (needs velocity estimation from successive packets)
  - Left grip = grab land and pull
  - Both grips = turn/zoom
  - Left stick = walk (with comfort toggle), right stick = zoom/turn
  - A = teleport aim, B = miracle gesture
  - Y = ring menu, Menu = game menu
  - Currently only bool mapping done, analog trigger/grip values missing

- [ ] **Locale fix verification** - UDP parser must use `locale::classic()` for '.' decimals. Packet examples from WinlatorXR use '.' even on EU locales

- [ ] **Frame pacing sync** - `OpenXRFrameWait` logic: when incoming FrameID repeats, set wait flag and drop packet, early-return from `UpdateInputs()`/`UpdateCamera()` - prevents stutter

- [ ] **32-bit build verification** - Ensure `vcruntime140.dll`, `msvcp140.dll` present in container's SysWow64 - HaloCEWXR required manual copy

## Partially Working (needs device testing)

- Marker file creation on Quest storage (Z: vs D: detection untested on actual WinlatorXR container)
- UDP receive loop timeout handling (100ms SO_RCVTIMEO)
- Red pixel location - top-left 10x10 may be occluded by B&W's UI

## Next Steps This Week

1. Dump actual `bwvr/src/vr/` file list from BW1-VR repo to confirm exclusion list
2. Port `DrawRedPixelMarker()` from D3D9 example to D3D7 `DrawPrimitiveUP` equivalent B&W uses
3. Test with SixDOFinator UDP Test program on PC - validate FrameID increments
4. Build Release and test in WinlatorXR container on Quest 3
