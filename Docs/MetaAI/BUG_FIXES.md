# BUG_FIXES.md - Bugs Fixed / Known Issues in BW1-WXR Port

Short bullet-point log of issues encountered while converting BW1-VR (OpenXR) to WinlatorXR XrAPI.

## Fixed (in current scaffold)

- **WSAStartup leak** - `WinXrApiUDP::Stop()` now calls `WSACleanup()` only if `WSAStartup` succeeded, and closes both recv/send sockets before cleanup
- **Locale decimal parsing** - Packets use '.' but German/FR Windows uses ','. Fixed by `iss.imbue(locale::classic())` on every parse - mandatory per HaloCEWXR comment
- **FrameID wrap** - FrameID sent as `int` but used as BYTE for red pixel `ARGB(255, FrameID, 0, Eye)`. Now masks with `& 0xFF` to prevent overflow
- **Infinite block on first packet** - `GetLatest()` waited forever if no UDP. Added 15s startup timeout (like COD4 `KISAK_VR_WINLATORXR_STARTUP_TIMEOUT_MS`) and 20ms per-frame timeout (`SYNC_WAIT_MS`)
- **Z: drive not present on desktop** - Added fallback `D:/xrtemp/` and `C:/tmp/xr` for testing outside container, matches Halo's fallback
- **Socket reuse** - Added `SO_REUSEADDR` to allow rapid restart without "address already in use"
- **Recv thread zombie** - Stop() now sends dummy UDP to self to unblock `recvfrom()` before `join()`
- **Button string off-by-one** - Packet has 21 T/F chars but some containers send 19 (old 0.2 API). Now checks `i < btnStr.size()` before access
- **Fullscreen border** - Game window had `WS_THICKFRAME|WS_CAPTION` leaving black borders, red pixel not at (0,0). Now strips those styles and `SetWindowPos` to `SM_CXSCREEN/SM_CYSCREEN`
- **Controller 180-degree roll** - XrAPI grip rotation is 180deg vs OpenXR aim. Added `XMQuaternionRotationRollPitchYaw(0,0,PI)` fixup - same as COD4 note
- **FOV not negotiated** - Container reported 108.9x103.4 but game rendered 90. Added `fov.txt` 6-line affine transform `FOVH = A*inH + B` and echoes target FOV back on 7278
- **Mirror device leak** - Original mod created extra D3D device for desktop mirror. Disabled like HaloCEWXR - `DrawEye`, `PostDrawFrame`, `mirrorDevice` creation commented out, `StretchRect` blit to backbuffer kept as actual present path
- **Shutdown unreachable** - `Game::Shutdown()` had early `return` "Don't allow shutdown for WinlatorXR" - removed for BW1 to allow proper save on quit
- **OpenXR loader in container** - Container has no `openxr_loader.dll`. Removed from build and from `makerelease.bat`

## Known Bugs (still open, from HaloCEWXR reference)

- Slight camera glitching when teleporting (B teleport)
- No virtual crouch impact on camera (click still passes barriers)
- Loading checkpoint while moving can be glitchy - exit/re-enter movement
- No recentre on menu hold - use HMD reset instead
- Weapon miracle scope in SBS mode takes full screen (B&W miracle zoom may need special handling)
- WinlatorXR's own right-stick-click menu can switch headset back to flat window - click again to return to VR
- Audio latency/slowdown on WinlatorXR - known Halo issue, needs DXVK + LOW audio settings
- Red pixel occluded by B&W's logo movies - skip logos with trigger/A on start

## Testing Notes

- If game stalls on loading HALO/B&W logo, not receiving UDP - check `Z:/tmp/xr/vr` exists and container XR API enabled
- If hands inverted, toggle `KISAK_VR_WINLATORXR_FLIP_CONTROLLERS` or `fovVar` in `VR/fov.txt`
- If throw feels weak, velocity estimation needs tuning from successive packets
