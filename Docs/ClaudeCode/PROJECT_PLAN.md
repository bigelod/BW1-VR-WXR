# Plan: Black & White to standalone VR through WinlatorXR

The goal, in plain words, is in `PROJECT_OVERVIEW.md`. Where the work stands is
in `CURRENT_STATUS.md`. This file is the plan and the decisions.

## 1. Intake

### The game

| Question | Answer |
| --- | --- |
| Game, version, patch level | Black & White (2001), official patch 1.10, then 1.20, then the unofficial patch v1.42. No other patch and no other mod |
| Distribution | The owner's own copy. No store sells the game |
| Install path | Kept in `local\game-path.txt` by `.\dev.ps1 setup` |
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
| Presentation mode | Mono VR first (`MODE_VR=1`, `MODE_3D=0`). Then measure side by side against alternate eye rendering on the device and choose |
| Ship a non-VR mode | To decide after the first measurement. It needs no camera work, so it stays available as a comfort and performance choice |
| Container resolution | TBD. Start at 2800x1400 and report what the game accepts. A square resolution suits a headset panel, and the resolution of the container is the supersampling control |
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

### Decisions taken deliberately

Record these so no session relitigates them.

| Decision | Choice | Why |
| --- | --- | --- |
| Bitness | 32 bit | There is no OpenXR runtime to load, so there is no bitness boundary to cross. The mod stays in the process of the game |
| Renderer | The game's own DirectDraw 7 and Direct3D 7 | Known good, and the container handles the upgrade above the game |
| Wrappers | None. No dxwrapper, no dgVoodoo, no ReShade | Each is a layer inside Box86 and Wine. WinlatorXR can inject ReShade itself if a player wants it |
| Process model | In the process of the game | No IPC, no second program to ship, start and reap |
| Transport | XrApi 0.5, direct | OXRWXR and OpenComposite would carry the PCVR mod with no new code, and they cost heavily in performance for content that already needs a desktop GPU. A direct mod pays nothing for generality |
| Presentation | Mono first, then measure side by side against alternate eye rendering | Mono isolates a camera fault from a stereo fault. The game is draw call bound, so one walk a frame can beat two |
| Supersampling | The resolution of the container | The runtime filters when it samples the window down. It needs no code |
| Settings | A menu inside the game | Editing an ini on a headset is painful. `debug_menu.cpp` is the start |
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
| The OpenXR session, the layers and the input | `src/vr/xr_screen.cpp`, `src/mod/vr_start.cpp` |
| The bridge to Direct3D 11 and the shared textures | `src/mod/gpu_bridge.cpp`, `src/mod/frame_send.cpp` |
| The Direct3D 9 backend and dxwrapper | `src/mod/d3d9_backend.cpp`, `bwvr/config/dxwrapper.reference.ini` |
| The Vulkan renderer of the mod | `src/mod/vk_ddraw.cpp`, `vk_window.cpp`, `vk_fallback.cpp` |
| The render thread and the thread of Direct3D 7 | `src/mod/render_thread.cpp`, `d3d7_thread.cpp` |
| The merge of draw calls | `src/mod/draw_merge.cpp` |
| The x64 host for Steam Frame | `src/vr/bridge/` |
| The launcher | `src/launcher/` |

`XrScreen` is the seam to keep. Its header already states what the mod needs
from a headset: an `EyeView` for each eye, a `HandInput` for each hand, the
cards, the panels and the cut scene window. The new transport gives that same
shape from a UDP packet, and the rest of the mod does not learn that OpenXR is
gone. Where the shape cannot hold, because XrApi has no layers, the difference
belongs in one place and not spread through the mod.

## 5. Milestones

Each one leaves something testable, and each has a checkpoint that only the
owner, the device or the hardware can answer. `CURRENT_STATUS.md` holds the
live state of these.

- [ ] **0** Intake complete, this plan agreed
- [ ] **1** The plain game runs in a container, and every workaround it needed
      is written down
- [ ] **2** The mod loads inside the container and writes a log whose first
      line is the build stamp. A MinHook hook is proven under Box86 and Wine
- [ ] **3** The window is borderless and fills the display, re-applied every
      frame
- [ ] **4** The XrApi client parses a packet captured on the device, and the
      button table comes from a device log and not from a document
- [ ] **5** The sync pixel is drawn last, 10 by 10, in every half
- [ ] **6** Mono 6DOF head tracking feels locked. This is the big one
- [ ] **7** The presentation mode is chosen from a measurement on the device
- [ ] **8** The cards, the panels and the cut scene window are in the frame
- [ ] **9** The hands are at the real hands, and the owner confirms the full
      input map
- [ ] **10** The settings menu inside the game, and the comfort settings

## 6. Features

One subsection for each feature, written before it is built. Keep the
acceptance of each one honest: at least one line of it is an observation by
the owner on the device.

### Feature 1 - The XrApi client

**Goal.** The mod knows where the head and the hands are, which buttons are
down, the interpupillary distance, the field of view that the runtime wants
and the frame id.

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

**Approach.** Strip `WS_CAPTION`, `WS_THICKFRAME` and `WS_BORDER`, size to the
display, and re-apply every frame because the engine rebuilds its window. Then
echo the frame id of the last packet as red in a 10 by 10 square at the top
left, drawn last, after the world and after the interface. Green stays 0. Blue
stays 0 until alternate eye rendering needs it. Draw it in both halves under
side by side.

**What must not happen.** No rescale and no gamma correction of the value. The
runtime learns the mapping from the values that it observes, and the
correction belongs on its side. Draw the square with filtering and blending
off, because the runtime samples exact bytes.

**Acceptance.**
- [ ] No title bar, and the game fills the view
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

**Acceptance.**
- [ ] The world stays still when the owner holds his head still
- [ ] A lean moves the view the right way and the right amount
- [ ] The horizon is level

### Feature 4 - The layers into the frame

**Goal.** The hands, the highlights, the arcs, the hint cards, the subtitles,
the ring, the panels and the cut scene window all appear, in one picture.

**Can it be done at all.** Yes, through the game's own path. The game submits
screen space vertices, so the mod transforms its own geometry on the CPU and
submits it the same way. The cards are already CPU pixels, and the surfaces of
the game are in CPU memory, so a card is a copy of pixels.

**What must be proven first.** That the mod can submit one triangle through
the Direct3D 7 device of the game and see it in the frame.

**Edge cases.** A card has to appear in both halves under side by side, at the
place that each eye needs. A quad in the room shows one time to each eye, so
what is on a card must not become double.

### Feature 5 - The input map

**Goal.** Every control of the PCVR mod works from a Quest controller through
XrApi.

**Approach.** The map of the PCVR mod is the start, and the bool string is the
new source. Write the table out, VR action to game action, and have the owner
confirm it before it is built. Swap the hands one time at ingest for a left
handed player, and then never test that flag again for anything spatial.
Haptics are a duration in frames and not an amplitude, and a value of 0 does
not clear them, so decay the number that the mod sends.

### Feature 6 - The menu inside the game

**Goal.** A tester changes any setting without leaving the headset.

**Approach.** `debug_menu.cpp` is the start. A saved config on the device
overrides a changed default, so say that plainly when a build is handed over.

## 7. Verification

**Build.** `.\dev.ps1 build` configures, builds and runs the math tests. The
build is 32 bit. After a change of a header in a build folder under a temp
path, build with `-Clean`.

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
| The game does not accept a square or a large resolution without letterboxing | Medium | Report what it accepts at Stage 1. The resolution of the container is also the supersampling control, so this is not only a question of looks |
| The mod cannot submit its geometry through the Direct3D 7 device of the game | Medium | Prove one triangle before Feature 4 is built. The fall back is a copy of pixels into the surfaces of the CPU, which the cards already use |
| Stereo costs too much on the mobile GPU | Medium | Mono ships. Alternate eye rendering gives depth at the cost of one walk a frame, which suits a draw call bound game |
| The game restores its graphics settings by itself | Medium | Verify after a relaunch, not after a change. Games of this age are known for it |
| A saved config on the device overrides a shipped default | High | Say it when a build is handed over, and name the key |
| The engine fights a change of its window | Medium | Make the fix idempotent and call it every frame |
| A value tuned on the desktop is wrong on the device | High | Measure each offset in the frame that it is used in. A correct value transfers. If it works only where it was found, the frame is wrong |
| The simulation of the game runs at 10 Hz while the headset wants 72 | Known | The PCVR mod already deals with this. Read `bwvr/docs/backend.md`, "The simulation runs at 10 Hz" |

## 9. What we learned

Append as the work goes. Not the structure of the code, because git shows
that. The things that are invisible afterwards: which frame an offset is
measured in and why, what a wrong value looked like on the screen, and which
route was tried and dropped and why. This is the section that makes the next
port cheaper, and the one most likely to be skipped.
