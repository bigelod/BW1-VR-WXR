# What this fork is

Black & White (2001) in VR on a standalone headset. The game runs as a plain
32 bit Windows program inside WinlatorXR on a Quest 3, and the mod gives it a
head, two hands and a stereo picture. The game itself stays the game: its
villagers, its Creature, its quests and its advisors are its own.

This fork starts from the PCVR mod, which puts the same game in VR on a
Windows PC through OpenXR. The game code hooks, the God view, the Hand and the
input design all come from there and all stay. What changes is everything
between the mod and the headset.

Read this file first, then `PROJECT_PLAN.md` for the plan and
`CURRENT_STATUS.md` for where the work stands. The general guidance for a
WinlatorXR port is in `Docs/general_wxr/`, and it is not specific to this
game. The rules of the repo are in the root `CLAUDE.md`.

## The stack

Nothing of the mod runs on Android. The mod is a Windows DLL inside Wine, and
it speaks Win32. The only Android things it touches are a UDP socket and a few
files.

```
  Quest 3 (Android, arm64)
  └── WinlatorXR APK
      ├── Box86            x86 instruction translation
      ├── Wine             the Windows API, on Linux, on Android
      │   └── a container  a virtual Windows machine with a fixed display size
      │       └── runblack.exe (v1.42)
      │           └── ddraw.dll   the mod
      └── the XR layer     captures the game window, reads the red sync pixel,
                           sends poses over UDP, presents to the compositor
```

The protocol between the mod and the runtime is XrApi: two UDP sockets that
carry one line of ASCII each way, a handshake through files under
`Z:\tmp\xr`, and one red square in the top left corner of the picture that
the game draws. That is the whole surface. `Docs/general_wxr/XRAPI_V0_4_DESC.md`
has every field.

## How this fork differs from the PCVR mod

| Part | The PCVR mod | This fork |
| --- | --- | --- |
| Headset API | The OpenXR loader, a session, swapchains, action sets | XrApi: UDP port 7872 in, port 7278 out, files under `Z:\tmp\xr` |
| The way the picture leaves | Eye pictures stay on the GPU and go into OpenXR swapchains | The runtime captures the game window. The picture never leaves the window |
| The bridge to Direct3D 11 | `gpu_bridge.cpp` copies Direct3D 9 textures into a Direct3D 11 device for the runtime | None. There is no runtime to hand a texture to |
| The renderer | dxwrapper turns Direct3D 7 into Direct3D 9Ex, or the mod makes DirectDraw 7 and Direct3D 7 itself on Vulkan | Plain DirectDraw 7 and Direct3D 7. The container handles the upgrade above the game |
| The frame clock | `xrWaitFrame` paces the game | The red sync pixel. The mod echoes the frame id of the last packet into the top left corner |
| Stereo | Two eye pictures, one swapchain for each eye | Two halves of one window buffer, side by side. Mono and alternate eye rendering are the other two choices |
| The cards and the panels | Quad layers of OpenXR, each with its own swapchain | One video channel only. The mod composites each card into the frame of the game |
| The cut scene window | A quad for each eye and four black walls, as layers | The same picture, composited into the frame |
| The controllers | One OpenXR action set, synced once a frame | 28 floats and 19 characters of `T` and `F` in the inbound packet |
| Haptics | `xrApplyHapticFeedback` | A duration in frames, per hand, in the outbound packet |
| The field of view | The runtime gives an asymmetric frustum for each eye | Two degrees in the outbound packet, tuned by eye |
| Supersampling | A resolution scale in the launcher | The resolution of the container. It needs no code |
| The settings | A launcher program and an ini file | A menu inside the game |
| Steam Frame | An x64 host program and a bridge to it | Dropped |
| How a frame cost is measured | Timers inside the mod | Android and WinlatorXR tooling, on the device |

## What stays

- The mod is still a `ddraw.dll` proxy with MinHook hooks in `runblack.exe`,
  unofficial patch v1.42 only. The build is still 32 bit.
- The God view is still the main mode: the land at the height of the waist,
  and the player reaches in.
- The hands are still the Hand of the game, 1:1 in place and size. A pick up,
  a throw and a miracle still go through the code of the game.
- The game still transforms every vertex on the CPU and submits screen space
  vertices. The mod still walks the scene one time for each eye and changes
  the camera between the two walks.
- The mod still adds no file of the game to the repo and still never touches a
  saved game, a profile or the registry.

## Why the differences are simplifications

The PCVR mod carries dxwrapper, a Direct3D 11 device, shared textures, a
render thread and the OpenXR loader because an OpenXR runtime wants a GPU
texture in a swapchain. WinlatorXR wants none of that. It reads the window.

So the parts of the mod that exist to move a picture from the game to a
runtime have no job in this fork. What takes their place is small: a UDP
parse, a pose conversion, a red square, and a composite of the two eye
pictures into the halves of one buffer.

The cost is on the other side. Every layer that OpenXR composited for free is
now the mod's own work, because there is one picture and the mod draws all of
it.

## The hard parts, named early

- **The mod's own drawing has no Direct3D 9 device.** The hands, the
  highlights, the arcs and the walls of the cut scene window are drawn by a
  renderer of the mod that targets Direct3D 9. With plain DirectDraw 7 that
  device does not exist. The route is the game's own path: the mod transforms
  its geometry on the CPU and submits screen space vertices through the
  Direct3D 7 device of the game, as every draw of the game does.
- **The cards must be composited.** The mod already draws each card on the
  CPU, as BGRA pixels. The surfaces of the game also live in the memory of the
  CPU. So a card goes into the frame as a copy of pixels, in both halves under
  side by side.
- **Three readers of the pointer become four.** The pointer has readers in
  window pixels, in render pixels and in the screen of the game camera. Side
  by side in one buffer adds the half that a pixel belongs to.
- **Nothing is proven on the device yet.** No part of this fork has run in a
  container. Each item of the plan says what a desktop test can show and what
  only the headset can.
