# BW1-VR -> BW1-WXR (WinlatorXR) Port - VS2022 Project

This drop contains:
- `BW1WXR.vcxproj` - VS2022 Win32 DLL project configured for WinlatorXR XrAPI 0.5
- `src/vr/wxr/WinXrApi.h/cpp` - IVR replacement that talks UDP
- `src/vr/wxr/WinXrApiUDP.h/cpp` - Winsock2 listener thread (port 7872 in, 7278 out)
- `src/vr/wxr/fov.txt` - FOV mapping, copy to `VR/` in game folder

## How to integrate into existing BW1-VR repo

1. Clone BW1-VR: https://github.com/BW1-VR/BW1-VR
2. Copy `src/vr/wxr/*` into `bwvr/src/vr/wxr/` in that repo
3. Replace `bwvr/bwvr.vcxproj` with the provided `BW1WXR.vcxproj` OR merge the changes:
   - Remove from build: `src/vr/openxr/*`, `src/vr/openvr/*`, `openxr_loader.dll` references
   - Add: `src/vr/wxr/WinXrApi.cpp`, `src/vr/wxr/WinXrApiUDP.cpp`
   - Link: `ws2_32.lib`
   - Define: `BWVR_WINLATORXR;XRAPI_VERSION=\"0.5\"`
4. In your VR backend factory (likely `VRSystem::Create()` or `vr::CreateBackend()`), return `new WinXrApi()` when `Z:\tmp\xr\system` exists or `D:\xrtemp\system` exists.
5. Build Win32 Release. Output is `ddraw.dll` (or `bwvr.dll` depending on your original).

## Runtime behavior (matches HaloCEWXR / COD4 WXR)

- On Init: writes `Z:/tmp/xr/vr` containing "VR" and `Z:/tmp/xr/version` containing "0.5" [HaloCEWXR docs]
- Fallback: `D:/xrtemp/vr` and `D:/xrtemp/version` for desktop testing
- Binds UDP INADDR_ANY:7872, parses 28 floats + frameID + 21 T/F button string
- Sends haptics+FOV to 127.0.0.1:7278 as `<leftVib> <rightVib> <MODE_VR> <MODE_3D> <fovW> <fovH>`
- Rendering: game draws both eyes side-by-side into its own window, WinlatorXR shows left half to left eye and right half to right eye
- Frame pacing: draws 10x10 red block at top-left with ARGB(255, FrameID, 0, EyeValue) so WinlatorXR can scrape pixel for sync
- Window forced fullscreen borderless to cover display

## Testing without headset

Use SixDOFinator minimal project's UDP Test app to send fake packets to localhost:7872, or build debug WinlatorXR that streams to your PC IP via udp_debug folder.

## Notes for Black & White specifics

- Black & White is 32-bit DirectX 7/8 era with a ddraw wrapper. The SBS rendering is done before final `Present`/`Flip`.
- Original mod has land at waist height - keep that scaling. HMD altitude is floor-relative in XrAPI 0.5, so subtract eye height offset like COD4 does.
- XrAPI controller rotation is 180deg rolled vs OpenXR aim - corrected in WinXrApi.cpp.
