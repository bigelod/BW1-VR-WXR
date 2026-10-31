# PROJECT_PLAN.md - BW1-VR -> BW1-WXR (WinlatorXR)

## Goal
Port the existing PC VR mod for Black & White (2001) - https://github.com/BW1-VR/BW1-VR - from standard OpenXR/OpenVR to WinlatorXR's XrAPI, so it runs natively inside the WinlatorXR Android container on a Meta Quest without a PC.

Reference pattern: Halo CE WXR - https://github.com/bigelod/HaloCEWXR

## Why a separate WinlatorXR build?
Standard PC VR mods talk to a local OpenXR or OpenVR compositor (SteamVR, VDXR). WinlatorXR has no OpenXR runtime inside the container. Instead, the WinlatorXR runtime on the Quest broadcasts 6DOF tracking over loopback UDP and expects the game to render both eyes side-by-side into its own window. WinlatorXR then shows left half to left eye, right half to right eye.

To make this work we keep all Black & White specific code (Hand physics, land deformation, creature interaction, miracle gestures, ring menu, renderer hooks for DDraw/D3D) and replace only the VR backend.

## Architecture Comparison

**Parent (PC) BW1-VR:**
Game -> bwvr.dll (ddraw wrapper) -> IVR -> OpenXR backend -> openxr_loader.dll -> SteamVR/VDXR compositor -> Submit()

**WXR (Quest) BW1-WXR:**
Game -> ddraw.dll (same wrapper) -> IVR -> WinXrApi backend -> WinXrApiUDP (Winsock2) -> UDP 127.0.0.1:7872 IN / 127.0.0.1:7278 OUT -> WinlatorXR renderer.c (scrapes red pixel for frame pacing)

## XrAPI 0.5 Spec Implemented (from COD4 WXR / HaloCEWXR)

1. **Activation handshake:**
   - On Init, if `Z:/` exists, create `Z:/tmp/xr/` and write file `vr` containing string `VR`
   - Write file `version` containing `0.5` (0.3 = Non-VR, 0.2 = old Halo)
   - Fallback for desktop testing: `D:/xrtemp/` when `Z:/` not present
   - Read `Z:/tmp/xr/system` for HMD make/model for debug logging

2. **Tracking in:**
   - `WinXrApiUDP` binds `INADDR_ANY:7872` with a detached thread and `recvfrom()`
   - Wire format: ASCII space-separated, C locale, 28 floats:
     - L quat[0-3], L stick[4-5], L pos[6-8]
     - R quat[9-12], R stick[13-14], R pos[15-17]
     - HMD quat[18-21], HMD pos[22-24], IPD[25], FOV_H[26], FOV_V[27]
     - int FrameID, then 21x T/F button string
   - Button order: L_GRIP, L_MENU, L_THUMB_PRESS, L_THUMB_L,R,U,D, L_TRIG, L_X, L_Y, R_A, R_B, R_GRIP, R_THUMB_PRESS, R_THUMB_L,R,U,D, R_TRIG, IS_IMMERSIVE, IS_SBS

3. **Haptics/FOV out:**
   - Send ASCII to `127.0.0.1:7278`: `<leftVib> <rightVib> <MODE_VR> <MODE_3D> <fovW> <fovH>`
   - MODE_VR: 1=VR, 2=Non-VR, MODE_3D: 1=SBS, 2=AER

4. **Rendering / Frame pacing:**
   - Game window stripped of `WS_THICKFRAME|WS_BORDER|WS_CAPTION` to fullscreen borderless - the container window *is* the VR viewport
   - Game renders left eye to left half, right eye to right half of same backbuffer (SBS)
   - Before Present(), draws 10x10 px quad at top-left (0,0) coloured `D3DCOLOR_ARGB(255, OpenXRFrameID & 0xFF, 0, RenderEyeValue)` - WinlatorXR's `renderer.c` scrapes this pixel to detect new frame and eye
   - `Update()` blocks up to 20ms (`KISAK_VR_WINLATORXR_SYNC_WAIT_MS`) waiting for new pose - mimics `WaitGetPoses`

5. **Project changes:**
   - New VS2022 solution `BW1WXR.sln` / `BW1WXR.vcxproj` (Win32, v143, C++17, `BWVR_WINLATORXR`, `XRAPI_VERSION="0.5"`, link `ws2_32.lib`)
   - `src/vr/wxr/WinXrApi.h/cpp`, `WinXrApiUDP.h/cpp`, `fov.txt`
   - Exclude from build: `src/vr/openxr/*.cpp`, `src/vr/openvr/*.cpp`, `openxr_loader.dll`, `openvr_api.dll`, SteamVR manifests, mirror device, compositor submit
   - Keep: Hand/land/creature/miracle/ring menu logic, DDraw hooks

## Build / Test Flow
1. Build Win32 Release -> `build/Release/ddraw.dll`
2. Copy to Black & White folder inside WinlatorXR container (`D:\Black & White\` maps to Quest storage)
3. Copy `VR/fov.txt` alongside
4. Desktop test without headset: SixDOFinator UDP Test app sends fake packets to localhost:7872
5. On Quest: WinlatorXR writes tracking to container's 7872 automatically when game writes marker file

## Success Criteria
- Game launches in container, writes `Z:/tmp/xr/vr`
- UDP packets flow (log shows FrameID incrementing)
- Red pixel visible in top-left on PC capture
- Both eyes render SBS without double vision
- Left grip = grab land, right grip = grab object/throw, A = teleport, B = miracle gesture, Y = ring menu
