# Plan: Black & White to standalone VR through WinlatorXR

The goal, in plain words, is in `PROJECT_OVERVIEW.md`. Where the work stands is
in `CURRENT_STATUS.md`. This file is the plan and the decisions.

## 1. Intake

### The game

| Question | Answer |
| --- | --- |
| Game, version, patch level | Black & White (2001), official patch 1.10, then 1.20, then the unofficial patch v1.42. No other patch and no other mod |
| Distribution | The owner's own copy. No store sells the game |
| Install path | `E:\tmp\Q3Mods\BnW` on the PC, a copy of the install that runs on the Quest 3. A contributor's own path is kept in `local\game-path.txt` by `.\dev.ps1 setup` |
| Bitness | 32 bit |
| Renderer | DirectDraw 7 and Direct3D 7. Every draw path submits screen space vertices, FVF `0x01C4` |
| Engine | Lionhead's own (`LH3DRender`, `LH3DTech`) |
| Source available | No. The addresses are in `bwvr/src/common/bw_addresses.h`, and the names come from the public project `bw1-decomp` |
| Mod API | None. The mod is a `ddraw.dll` proxy |
| A VR build already exists | Yes, the PCVR mod that this fork starts from. It is an OpenXR mod with dxwrapper and a Direct3D 11 bridge |
| An existing PCVR mod to retarget | The same one. This fork is that retarget |
| Modding community and documentation | bwgame.net for the game, `bw1-decomp` for the names, and `bwvr/docs/` for the reverse engineering notes of each area |
| Native head tracking support | None |

### The port

| Question | Answer |
| --- | --- |
| Real VR port or injection port | A real VR port. XrApi, 6DOF, poses straight into the game camera and the Hand |
| XrApi version | 0.5. It is 0.4 plus the InputAPI listener at no cost, it is readable in a log, and the XrAPI UDP Tester simulates it |
| Target headset | Quest 3 through WinlatorXR. Other headsets can work and are untested |
| Presentation mode | Mono VR (`MODE_VR=1`, `MODE_3D=0`). One image to both eyes, with the head driving the camera. The owner's choice, for the frame cost: the port does no stereo |
| Ship a non-VR mode | To decide after the first measurement. It needs no camera work, so it stays available as a comfort and performance choice |
| Container resolution | 1400x1400 is the size to plan for, and a 4:3 size of about the same height where the game refuses a square one. 1280x720 is the default of WinlatorXR, and the plain game runs at it with no change to any container setting. The mod must still read the render size each frame and work at any size, because the resolution belongs to the player and is the supersampling control |
| Framing | The God view: the land at the height of the waist. Hands in view, 1:1 |
| Hands in view | Yes. Both. The hands are the Hand of the game |
| Mouse motion needed from outside the process | No. The mod never moves the real mouse. It writes the state of the game's own `LHMouse` from inside the process |
| Multiplayer in scope | No |

### Constraints

| Question | Answer |
| --- | --- |
| May the result be published | Yes, as the parent mod is. Nothing is pushed or published until the owner asks |
| May any of the game's source appear in the repo | The game has no source release. No file of the game and nothing made from one ever enters the repo. `bwvr/tools/check_repo.ps1` stops a commit that holds one |
| Release owned files | TBD. Name them before the first automated packaging step |
| Who verifies on hardware | The owner, on a Quest 3 through WinlatorXR, or on the PC with the XrAPI UDP Tester |
| Prior work to inherit | The whole PCVR mod, about 146000 lines. The game hooks, the God view, the Hand, the grab, the throw, the miracles, the village and the audio all stay. The OpenXR transport, dxwrapper, the Direct3D 11 bridge, the render thread and the Steam Frame host all go |
| An earlier pass to inherit or drop | `Docs/MetaAI/` holds notes and a standalone scaffold of an XrApi client (`MetaAITestProj/src/vr/wxr/`, about 620 lines). It is not in the build and it has not run. TBD whether Feature 1 starts from it or from the reference clients. Its claims are listed in `BUG_FIXES.md` as things to verify |

## 2. Hard rules

The rules of the root `CLAUDE.md` hold without change: no file of the game in
the repo, no load or change of a saved game or a profile, no change to the
registry, no start of the game outside `bwvr/tools/desk/desk_run.ps1` in a
test, no directory link to `third_party`, and no push or publish unless the
owner asks.

The standing choices of `Docs/general_wxr/CLAUDE.md` hold too, and they are
not to be relitigated: XrApi 0.4 or 0.5, mod the game directly, keep the
game's own renderer and keep it 32 bit, measure performance on the device, and
a higher version number is not a goal.

Two of its non-negotiables decide the order of the work. The sync pixel comes
before the camera. The window must be borderless and fill the display, and the
mod must re-apply that every frame.

## 3. What exists today

The repo is the PCVR mod, unchanged, plus `Docs/general_wxr/` and these files.
No line of the port is written. `CURRENT_STATUS.md` has the detail.

The blocker is Stage 1: nobody has run the plain game in a WinlatorXR
container. Everything after it rests on what that run shows.

## 4. Approach

### The injection vector

The `ddraw.dll` proxy that the mod already uses. The game loads `ddraw.dll`
from its own folder by name, so the mod is in the process with no patch of the
executable. MinHook then hooks the functions of the game.

One thing about it needs an early device test: a hook under Box86 and Wine is
not the same as a hook on Windows. Prove that MinHook patches the game inside
a container before any feature rests on it. That is Stage 2.

### The renderer

The game keeps plain DirectDraw 7 and Direct3D 7 (`backend=system`). The
container upgrades it above the game, outside the mod. So this fork carries no
dxwrapper, no Direct3D 9, no Direct3D 11 and no Vulkan renderer of its own.

The consequence is the main piece of engineering in the port. The mod's own
renderer targets Direct3D 9, and that device is gone. The mod's drawing moves
to the game's own path: transform on the CPU and submit screen space vertices
through the Direct3D 7 device of the game (`LH3DRender::Direct3DDevice7`, a
global at `0x00ECA638`), as every draw of the game does. The cards are already
drawn on the CPU as BGRA pixels, and the surfaces of the game are in the
memory of the CPU, so a card reaches the frame as a copy of pixels.

### What a resolution costs, and what a second eye costs

Two measurements of the parent mod decide where the frame time goes, and they
point the same way.

The drawing of the game does not grow with the resolution. It measured 4.33 to
6.52 ms at every size from 1280x720 to 2560x1440. The scene walk is nearly all
of a frame, and a saving has to come from a cheaper walk and not from pixels.
So a square resolution of 1400x1400 costs the game almost nothing against the
default, and the player gets the quality.

The cost that did grow with the resolution was the readback: a lock of the
DirectDraw target and a write into an upload texture, 0.41 ms at 1280x720 and
1.34 ms at 2560x1440, for each eye. This fork has no readback. The picture
stays in the window that the runtime captures, so that cost is gone.

What is left is the scene walk, 2 to 11 ms of a frame on this game. A second
walk for a second eye is the largest cost the port could carry, and it is the
reason the port is mono.

### Decisions taken deliberately

Record these so no session relitigates them.

| Decision | Choice | Why |
| --- | --- | --- |
| Bitness | 32 bit | There is no OpenXR runtime to load, so there is no bitness boundary to cross. The mod stays in the process of the game |
| Renderer | The game's own DirectDraw 7 and Direct3D 7 | Known good, and the container handles the upgrade above the game |
| Wrappers | None. No dxwrapper, no dgVoodoo, no ReShade | Each is a layer inside Box86 and Wine. WinlatorXR can inject ReShade itself if a player wants it |
| Process model | In the process of the game | No IPC, no second program to ship, start and reap |
| Transport | XrApi 0.5, direct | OXRWXR and OpenComposite would carry the PCVR mod with no new code, and they cost heavily in performance for content that already needs a desktop GPU. A direct mod pays nothing for generality |
| Presentation | Mono. No stereo | The owner's choice, for the frame cost. A second eye is a second walk of the scene, which is the largest cost of a frame. Mono also keeps a camera fault apart from a stereo fault |
| The square resolution | 1400x1400 fits mono as it is | One picture at the full size of the render, and the same picture in both eyes |
| Supersampling | The resolution of the container | The runtime filters when it samples the window down. It needs no code |
| Settings | A menu inside the game | Editing an ini on a headset is painful. `debug_menu.cpp` is the start |
| The interface of the mod | On the screen, in about the middle third across and down | XrApi has no layer, and the mod does not build a head up display of its own. A card in the room was an OpenXR quad layer, and that is gone |
| Frame cost | Measured on the device with Android and WinlatorXR tooling | The mod cannot see the compositor, the translation layers or the GPU, and that is where the time goes |
| Feature scope of the first build | The full God view stays live | The owner's choice. The transport changes under a mod that works, so a fault in a feature is a fault in the transport until proven otherwise |

On that last row, one caution to hold rather than to act on now. The general
guidance is one change for each deploy, and a full feature set makes the first
fault on the device harder to attribute. The mitigation is already in the mod:
each change of behavior has an ini key with the old behavior as a value, and a
test key for a live A/B. So a feature can be turned off for one deploy when it
is the suspect, with no build.

### The subtraction list

These parts of the PCVR mod have no job in this fork. Each one goes, and the
doc of its area then says what the mod does now.

| Part | Files |
| --- | --- |
| The OpenXR session, the layers and the input | `src/vr/xr_screen.cpp`. Out of the build. `src/vr/xr_screen_wxr.cpp` takes its place and speaks the XrApi |
| The standalone test of the session | `src/xrtest/`. Out of the build: it showed a test pattern through an OpenXR session |
| The bridge to Direct3D 11 and the shared textures | `src/mod/gpu_bridge.cpp`, `src/mod/frame_send.cpp` |
| The Direct3D 9 backend and dxwrapper | `src/mod/d3d9_backend.cpp`, `bwvr/config/dxwrapper.reference.ini` |
| The Vulkan renderer of the mod | `src/mod/vk_ddraw.cpp`, `vk_window.cpp`, `vk_fallback.cpp` |
| The render thread and the thread of Direct3D 7 | `src/mod/render_thread.cpp`, `d3d7_thread.cpp` |
| The merge of draw calls | `src/mod/draw_merge.cpp` |
| The x64 host for Steam Frame | `src/vr/bridge/` |
| The launcher | `src/launcher/` |

`XrScreen` is the seam to keep, and the swap is done. Its header states what
the mod needs from a headset: an `EyeView` for each eye, a `HandInput` for
each hand, the cards, the panels and the cut scene window. Only 12 files of
the mod name it, so one implementation carries the whole change and the rest
of the mod does not learn that OpenXR is gone.

`xr_screen_wxr.cpp` is that implementation. The methods that carry a picture
to a runtime, the swapchains, the textures, the layers and the Direct3D 11
device, do nothing in it and are marked as such. They are not a design: the
picture reaches the headset because the runtime captures the window. Each one
goes away with its callers as the mod learns to draw its cards into the frame
of the game, at stage 7.

## 5. Milestones

The stages in order. Each one leaves something testable, so a fault belongs to
the stage that caused it. Each one ends at a checkpoint that only the owner,
the device or the hardware can answer. `CURRENT_STATUS.md` says which of these
are still open, and it is the only place that says so.

| Stage | What it leaves | The checkpoint |
| --- | --- | --- |
| **0** | The intake and this plan | The owner agrees |
| **1** | The plain game runs in a container | The owner says what it needed to run. Each workaround is a thing an installer or a readme has to carry |
| **2** | The mod loads in the container and writes a log | The first line of the log is the build stamp that was just built, and a MinHook hook holds under Box86 and Wine |
| **3** | The window is confirmed to fill the display with no chrome | The game already runs full screen in the container, so this is a check and not work. No title bar, and the pixel at the top left corner belongs to the game |
| **4** | The XrApi client | One raw packet captured on the device, compared field by field with the tester. The owner presses each button and the log names the index that moved |
| **5** | The sync pixel, last, 10 by 10 | Nothing to see. It is judged at stage 6 |
| **6** | Mono 6DOF head tracking | It feels locked to the head. This is the big one, and nothing goes on top of it until it is right |
| **7** | The interface of the mod on the screen, in the middle third | It is readable in the headset, and nothing of it floats in the room |
| **8** | The hands and the full input map | The owner confirms the map before it is built, and again on the device |
| **9** | The settings menu inside the game, and the comfort settings | A tester changes a setting without leaving the headset |

### What mono gives

Mono is one picture at the full size of the render, in both eyes.

- One sync square, not one for each half.
- The cards and the interface at the full render width, so a card of 768
  pixels fits and nothing has to be drawn twice.
- One field of view, the game's own for the shape of the render.
- One scene walk a frame, so the frame cost of the port is the frame cost of
  the plain game.
- 1400x1400 fits as it is.

What it gives up is depth. The land at the waist reads as a solid scene from
the scale, the motion of the head and the hands at 1:1, and the frame cost of
a second walk buys depth that this hardware cannot pay for.

## 6. Features

One subsection for each feature, written before it is built. Keep the
acceptance of each one honest: at least one line of it is an observation by
the owner on the device.

### Feature 1 - The XrApi client

**Goal.** The mod knows where the head and the hands are, which buttons are
down, the interpupillary distance, the field of view that the runtime wants
and the frame id.

**Where it starts.** Adapt the client of the Shogo port,
`game/XrApiUDP.cpp` in `Shogo-VR-WXR`. It is C++, 32 bit, in the process of
the game, and it is proven on a Quest 3. It is the closest of the reference
ports to this one. The scaffold in `Docs/MetaAI/` is a second reading of the
same protocol and not the start.

**What must be proven first.** That a packet arrives at all inside a
container. The `version` file decides it, it is read one time at startup, and
an unrecognised value gives total silence and no error.

**Approach.**
1. Write `version` with `0.5` and `vr` with `VR` into `Z:\tmp\xr`, after the
   runtime is up, because the runtime wipes that directory at its own startup.
   Fall back to a local folder when `Z:\` does not exist, so one binary runs
   on the desktop and on the device. Delete `vr` at a clean shutdown.
2. Read `system`. It has seven lines. Line 1 is the manufacturer and line 2 is
   a product codename, so `EUREKA` is a Quest 3. Default to `META` and
   `QUEST 3` when the file is missing rather than failing. The file is also
   the test for "am I on a headset": the mod selects this backend when
   `Z:\tmp\xr\system` exists.
3. Bind one socket, 7872. The runtime sends the same packet to 7872 and to
   7873, so a bind of both receives everything twice.
4. Drain every queued datagram each frame and keep the newest.
5. Parse 28 floats, the frame id, the bool string and the headset name.
   Tolerate extra trailing tokens and both lengths of the bool string.
6. Normalise every quaternion on ingest, unconditionally.
7. Freeze on the last good pose when the packets stop. Never zero.
8. Send the outbound packet on 7278 every frame, even in a mode that needs
   nothing, because the mode is only ever read from a packet that the mod
   sends.
9. Log the raw packet verbatim every few seconds, behind the verbose flag.

**Send the field of view that the game drew with, every frame.** Not a
setting, and not 104.5. The runtime maps the frame onto the display with those
two angles, so a wrong pair makes the world move at a different rate from the
head, and by a different rate in pitch than in yaw. In a headset that reads as
drift: rotations that do not come back to where they started. A port in this
family sent a square 104.5 pair while the engine drew 99.3 by 67.0, and the
symptom was described as the axes drifting.

The game asks for 1.2217 radians, 70.0 degrees, one time each frame through
`LH3DTech::ChangeFov`, and its vertical angle follows the render aspect:
`tan(v) = tan(h) * height / width`. So the mod takes both angles from the
frame it drew, in degrees. At a square resolution the two are equal, and at
1280x720 they are not, which is exactly why this cannot be a constant.

**The capture is the ground.** A synthetic pose holds what the protocol is
believed to say, and a capture holds what it does.
`Docs/general_wxr/SixDOFinatorCalibration/SamplePoseData.txt` is one packet
from a Quest 3, and the math checks replay it. Its numbers are in
`CURRENT_STATUS.md`. A second capture, with a button held and the hands apart,
would settle the button table as well.

**What is built.** `bwvr/src/vr/xrapi_packet.h` holds the parse, and
`bwvr/test/controller_math/xrapi_packet_checks.h` holds its checks. The parse
is a pure header, so the socket, the handshake files and the outbound packet
are what is left of this feature.

**Settings.** One ini key for the version and one for the verbose log, each
with the shipped value as its default.

**Acceptance.**
- [ ] The log holds a raw packet from the device, and it matches the tester
      field by field
- [ ] The owner presses each button in turn, the log names which index moved,
      and the button table comes from that log

### Feature 2 - The window and the sync pixel

**Goal.** The runtime can match the picture it captured to the pose that the
picture was drawn for.

**The window.** The game runs full screen in the container and does not know
that it is in one, so the picture already fills the display with no chrome and
the top left pixel is the game's own. Check it, and then leave the window
alone. A strip of `WS_CAPTION`, `WS_THICKFRAME` and `WS_BORDER` with a resize
to the display, idempotent and called every frame, is the guard to add only if
a check shows chrome. Do not add it first: a per frame change of the window
fights an engine that has its own consistency tests.

Note that the parent mod measured the game windowed, with
`SetCooperativeLevel` and `DDSCL_NORMAL`. Full screen makes the window, the
client and the render sizes agree, and it can take a different path to the
primary surface. So confirm the present path in the container rather than
assuming the one in `backend.md`.

**The sync pixel.** The frame id of the last packet is the red of a square of
10 by 10 pixels at the top left, drawn last, after the world and after the
interface. Green is 0, and blue is 0: the blue carries an eye flag that only
alternate eye rendering uses. One square.

The last place in the frame is the present itself, so the square goes in
there (`present_watch.cpp`, and the packing of the colour in
`bwvr/src/vr/sync_pixel.h`). A flip chain takes it in the back buffer before
the flip, and a window takes it on the primary surface after the copy, at the
corner where the copy put the picture. The colour is packed with the masks of
that surface, so a surface of 16 bits works too, with 5 bits of red: it holds
32 of the 256 frame ids, and the log says so. `sync_pixel=0` draws no square.

**What must not happen.** No rescale and no gamma correction of the value. The
runtime learns the mapping from the values that it observes, and the
correction belongs on its side. Draw the square with filtering and blending
off, because the runtime samples exact bytes.

**Acceptance.**
- [ ] No title bar, and the game fills the view
- [ ] The log says that the window, the client and the render sizes agree
- [ ] Head tracking feels locked and not swimmy, judged at Feature 3

### Feature 3 - The head on the game camera, mono

**Goal.** The player looks around, and the world stays where it is.

**Approach.** The camera matrix that the scene uses is at `0x00EA9E40`, and
`LH3DTech::UpdateCamera` builds it. The PCVR mod already writes it from a head
pose in a levelled frame, so the work is the conversion and not the camera.
Keep the inverse of the pose at recentre time. Rotate the canonical axes and
flip the results for the handedness of the engine, rather than mirroring the
quaternion. Put the scale from metres to game units in one constant.
Orientation first, position after.

**The space is head centred.** XrApi positions are not floor relative. In a
Quest 3 capture the head is at y of 0.006 and both hands at y of -0.23, a
palm's drop below it. There is no floor in that space. So the height of the
land in the God view, and the 1:1 place of each hand, come from the offset to
the head and not from a height above a floor. Write no check that assumes a
floor: a port in this family gated its recentre behind "the head is at least
0.5 m up", the gate never passed on hardware, and the hands collapsed onto the
head.

Three facts from other ports, each of which cost a session there:

- **The quaternion of a packet passes through unchanged.** Negating its Z
  component, to match a reading of the handedness section, reversed the roll
  of the head: a lean to the left rolled the view to the right. Yaw and pitch
  did not show it, because a pure yaw or pitch quaternion has Z of 0. Flip the
  axes after the rotation, not the quaternion.
- **The recentre takes the yaw only.** Keep the inverse of the yaw of the head
  at recentre time, and not the whole orientation, or a recentre with a tilted
  head tilts the world.
- **Test a compensation against an input that moves.** A correction that
  forgets a frame is exactly right at the origin and wrong everywhere else, so
  a test at one still pose tests the one case that cannot fail. The scripted
  controller has a sweep for this (`sweep` in `FakeControllerSettings`). Use a
  period that no other motion in the test shares.

**Acceptance.**
- [ ] The world stays still when the owner holds his head still
- [ ] A lean moves the view the right way and the right amount
- [ ] The horizon is level

### Feature 4 - The interface on the screen, and the hands in the frame

**Goal.** The hands, the highlights and the arcs appear in the world, and the
interface of the mod appears on the screen.

**The interface is on the screen, not in the room.** No card floats in the
room, no panel hangs beside a wrist, and the cut scene has no window with
walls. XrApi has no layer to put one in, and the mod is not going to build a
head up display of its own. Anything that the mod must show goes on the
screen, in about the middle third of the display across and down, where a
headset shows it clearly.

That removes the whole layer design of the PCVR mod rather than porting it.
`StageCard`, `StagePanelQuad`, `StageWindow`, `SetPanelQuads` and
`EnablePanel` do nothing now, and they go away with their callers.

**Can it be done at all.** Yes, through the game's own path. The game submits
screen space vertices, so the mod transforms its own geometry on the CPU and
submits it the same way. The pictures of the cards are already drawn on the
CPU, so one goes to the screen as a copy of pixels.

**What must be proven first.** That the mod can submit one triangle through
the Direct3D 7 device of the game and see it in the frame.

**Edge cases.** One picture, so one copy. A hint card is 768 by 320 pixels
(`kHintCardWidth` and `kHintCardHeight` in `xr_screen.h`), which is wider
than a render of 700 pixels. So the size of a card comes from the render
width and not from a constant. The interface of the game
has the opposite property: its art is at fixed pixel sizes, so it covers less
of the view as a player raises the resolution of the container. One strength
control, derived from the measured width against a reference width, is better
than a raw value.

### Feature 5 - The input map

**Goal.** Every control of the PCVR mod works from a Quest controller through
XrApi.

**Approach.** The map of the PCVR mod is the start, and the bool string is the
new source. Write the table out, VR action to game action, and have the owner
confirm it before it is built. Swap the hands one time at ingest for a left
handed player, and then never test that flag again for anything spatial.
Haptics are a duration in frames and not an amplitude, and a value of 0 does
not clear them, so decay the number that the mod sends.

**The roll of the hands needs an override.** Meta controllers report the hands
rolled by 180 degrees, and a port in this family confirmed that a Quest 3 does
it too, so key the correction off the **make** on line 1 of `system` and not
off the model on line 2. Some headsets are also reported backwards, so give a
yaw trim beside the roll.

Detection is a guess about hardware that we do not have. Ship the guess with
an override that a player reaches without a console: a `handsfix.txt` beside
the game, read with plain `fopen`, costs twenty lines and saves one round trip
to the device for each wrong guess. Also put it in the menu of the game.
Default to `META` and `QUEST 3` when `system` is missing.

This kind of fault is visible and not measurable. A rolled hand makes no log
line wrong, so no amount of instrumentation finds it. It is one of the cases
that needs a screenshot from the device.

### Feature 6 - The menu inside the game

**Goal.** A tester changes any setting without leaving the headset.

**Approach.** `debug_menu.cpp` is the start. A saved config on the device
overrides a changed default, so say that plainly when a build is handed over.

**Version the settings before they need it.** A saved ini on a tester's device
outlives the default that made it, and a tester is exactly the person who
holds a stale value. A port in this family changed a sentinel from `-1` to
`auto`, every device that had run the build before read `-1` as minus one
degree, and the fault looked as though it had never been corrected. So keep a
`cfg_version` key, raise it when a default changes in a way that an old value
defeats, and migrate at startup. Change only a value that equals the old
default, because dropping a tuning that a player chose is worse than the fault
being corrected. Log each value that changed.

## 7. Verification

**Build.** `.\dev.ps1 build` configures, builds and runs the math tests. The
build is 32 bit. After a change of a header in a build folder under a temp
path, build with `-Clean`.

**Where a build goes.** `bwvr\tools\wxr\deploy.ps1` builds through `dev.ps1`,
runs the guard, and stages the bundle.

| Target | What it is for |
| --- | --- |
| `Release\BnW-VR-WXR` | The files that are ready to test on the device: `ddraw.dll`, `bwvr.ini` and a readme that says what to test and what to send back. Copy the folder beside `runblack.exe` on the device. The script writes nothing else under `Release` |
| The game folder, `E:\tmp\Q3Mods\BnW` | A test on the PC. `deploy.ps1 -Game` also puts the files there, and it keeps the `ddraw.dll` that was there. `.\dev.ps1 uninstall` takes the mod out again. The folder is in `local\game-path.txt` |

**The guard.** The script refuses to stage a `ddraw.dll` that is older than
the newest source file, and it names both times. Do not work around it. A test
of a stale build looks like a change that does nothing, and it costs a
session.

**Which build ran.** The first line of `bwvr.log` carries the version, the
build id and the stamp of the build:

```
Black & White VR Mod 1.2.0 (build abc1234, built 2026-11-03 14:05:59)
```

The id names the commit. The stamp names the build, because two builds of one
commit share the id. The deploy script prints the stamp to look for. Check it
before any report from the device is interpreted.

**The build stamp.** Embed `__DATE__ __TIME__`, log it as line 1, and make the
deploy refuse a binary older than the newest source file. Check that line
before any report from the device is interpreted.

**New math.** A pure header with checks in `bwvr/test/controller_math/`.

**The desktop pass.** The XrAPI UDP Tester for the protocol, and
`bwvr/tools/desk/` for the game. A desktop run proves that the code path runs.
It does not prove that a value is right: the quaternions of a tester are often
not unit length, and its hand separation does not match a headset.

**The device pass**, and it is the only real gate:
- [ ] head tracking still feels locked
- [ ] the frame rate is acceptable at the recommended container resolution
- [ ] correct for a right handed and for a left handed player
- [ ] the features that this change could plausibly have touched still work

Say plainly in the commit message or the pull request what ran, what did not,
and what only a headset can show. Do not merge a change of behavior that no
run has shown.

## 8. Risks

| Risk | Likelihood | What we do about it |
| --- | --- | --- |
| A MinHook hook behaves differently under Box86 and Wine | Medium | Prove one hook on the device at Stage 2, before any feature rests on it |
| The game refuses 1400x1400, or letterboxes it | Medium | 1280x720 runs. Try the square size early, because the whole plan assumes it. The fall back is a 4:3 size of about the same height. Getting a game of this age to take an unusual mode is a recurring fight in these ports |
| The mod cannot submit its geometry through the Direct3D 7 device of the game | Medium | Prove one triangle before Feature 4 is built. The fall back is a copy of pixels into the surfaces of the CPU, which the cards already use |
| The game restores its graphics settings by itself | Medium | Verify after a relaunch, not after a change. Games of this age are known for it |
| A saved config on the device overrides a shipped default | High | Say it when a build is handed over, and name the key |
| The engine fights a change of its window | Low | The game runs full screen in the container, so the mod needs no change of the window. Add one only if a check shows chrome, and make it idempotent |
| The full screen path differs from the windowed path that the parent mod measured | Medium | Confirm the present path, the pointer map and the render size in the container. The parent mod ran the game windowed |
| A value tuned on the desktop is wrong on the device | High | Measure each offset in the frame that it is used in. A correct value transfers. If it works only where it was found, the frame is wrong |
| The simulation of the game runs at 10 Hz while the headset wants 72 | Known | The PCVR mod already deals with this. Read `bwvr/docs/backend.md`, "The simulation runs at 10 Hz" |

## 9. What we learned

Append as the work goes. Not the structure of the code, because git shows
that. The things that are invisible afterwards: which frame an offset is
measured in and why, what a wrong value looked like on the screen, and which
route was tried and dropped and why. This is the section that makes the next
port cheaper, and the one most likely to be skipped.
