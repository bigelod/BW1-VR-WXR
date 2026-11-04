# Current status

What is true now. The plan is in `PROJECT_PLAN.md` and the goal is in
`PROJECT_OVERVIEW.md`. Keep this file current, and remove an item when it is
done rather than marking it.

## In one paragraph

The XrApi client of the port runs. On a PC with the XrAPI UDP Tester the mod
does the handshake, binds the socket, reads the poses of a packet and drives
the camera of the game with the head. The sync pixel goes into each present.
The plain game runs on the Quest 3 in a WinlatorXR container, and the mod has
not yet driven a frame there. What is still the PCVR mod underneath is the
transport that the port does not need: dxwrapper, the Direct3D 11 bridge, the
Vulkan renderer and the OpenXR layers of the interface. The subtraction list
is in `PROJECT_PLAN.md`.

**What is built and proven on the PC:**

- `bwvr\tools\wxr\deploy.ps1` builds the mod through `dev.ps1`, runs the math
  tests, refuses a build that is older than the newest source file, and stages
  the bundle to `Release\BnW-VR-WXR` with a SHA-256 check of each file. The
  guard was tested by making a source newer than the build, and it stops.
- The first line of `bwvr.log` now carries the stamp of the build beside the
  build id, so a report from a device names the build that ran. CMake writes
  the stamp again for each build, and the string is in the built `ddraw.dll`.
- `dev.ps1` finds a CMake of 3.20 or later by itself. An older CMake first on
  the PATH gave an error that named the build folder as the source folder.
- The bundle holds `ddraw.dll`, `bwvr.ini` with `backend=system` and
  `gpu_bridge=0`, and a readme that says what this build tests and what to
  send back.
- `bwvr/src/vr/xrapi_packet.h` parses one packet of the wire: 29 numbers, the
  characters of T and F from both words, and the name of the headset. It
  normalises each quaternion, it reads a number with no locale, and it works
  out the vertical angle of the field of view from the shape of the render. It
  is a pure header: no socket, no Windows.
- `bwvr/test/controller_math/xrapi_packet_checks.h` replays the packet that a
  Quest 3 sent and holds its numbers.
- `bwvr/src/vr/xr_screen_wxr.cpp` is the output of the mod on the XrApi. It
  writes the handshake files, reads the name of the headset, binds 7872,
  drains each frame, gives the head and the hands to the mod in the shape
  that the rest of the mod already reads, holds the frame id for the sync
  pixel, and sends the outbound packet on 7278 with the two angles that the
  game drew with.
- **No OpenXR is in the build.** The mod needs no `openxr_loader.dll`. Its
  only hard imports are `KERNEL32`, `USER32`, `GDI32` and `WS2_32`, and
  `d3d11.dll` is delay loaded, so a container with no Direct3D 11 still
  starts the game.
- The full build passes: 5102 math checks, 0 failed.

## What the run on the device showed

A build ran in the container at 1920x1080. The log answers most of stage 2:

- The mod loads, and the first line names the build that ran.
- `runblack.exe` is the v1.42 file, checked in 34 ms.
- **Every MinHook hook says ok**, about 120 of them, from the renderer to the
  audio to the physics. A hook holds under Box86 and Wine. That was the open
  question of stage 2, and it is answered.
- `backend : system DirectDraw`, so the game keeps its own DirectDraw 7 and
  Direct3D 7 and the container handles the upgrade.
- The game plays: the logos, the start of the island, the advisors, the
  camera of a script, the sounds and the zoom all run.

Two faults came out of that run, and both are corrected in the build that
follows it.

- **The XrApi client never started.** The mod starts a headset in three steps,
  `StartInstance`, `StartSystem` and `StartSession`, and the client did the
  whole handshake in a fourth call that the mod never makes. So no socket
  opened, no line of the log began with `xrapi`, and after 120 seconds the mod
  said "no headset arrived, the game stays on the monitor". The client now
  does its work in those three steps.
- **The mod reshaped the window.** `pointer : window 1920 x 1080 set to
  1848 x 1040` and, later, `window client was 1920 x 1080, not 1848 x 1040, so
  it was corrected`. That fit matches the shape of the render for the mouse,
  and on WinlatorXR it takes the game off the full display, which is where the
  sync pixel must be. `window_fit=0` turns it off, and the ini of the
  container has it. `window_fit=1` is the behavior of the PCVR mod.

## What the run on the PC with the tester showed

A run with the XrAPI UDP Tester answers stage 2 and most of stage 6:

- `xrapi : 0.5, in on 7872, out on 7278, files in ...\xrtemp`. The
  handshake files and the socket are there.
- The tries found the runtime on try 2, and the session started.
- **The poses reach the camera of the game.** `head camera : 450 camera
  updates, 450 carried the head, 450 recorded`, and the head pose of the log
  follows the tester. The hands are at the controllers, the hand of the mesh
  is drawn at the index tip, and the grab of the world with a grip works. So
  the ingest of the packet, the recentre and the conversion to the room are
  right for the poses that the tester sends.
- The game holds 60 frames a second at 1920x1080, and that run still walked
  the scene two times.

Three faults came out of that run, and each is corrected in the build that
follows it.

- **The mod read the frame back and tried to submit it.** The flat path of the
  frame grabbed the render target and called `SubmitBgra`, which has nothing
  to submit on the XrApi: the window of the game is the output. The submit
  said no, so the log held one line "the session ended at frame N" for every
  frame of the run, and the readback cost 2 to 3 ms of each frame for nothing.
  The path now stops before the readback.
- **The mod skipped the copy to the window.** `skip_desktop_present` saves the
  copy to the monitor while a headset has the frame, and it put the log line
  "the desktop copy is now skipped, the monitor will not update". On the XrApi
  that copy is the frame of the headset. The skip is now off whenever the
  window is the output, whatever the ini says.
- **The first packet did not reach the log.** The tries for a headset are
  quiet after the first one, so the line of the first packet went into a log
  that throws lines away. The line is kept now and goes out when the log is
  loud again.

## Mono, and what it drops

The port is mono and stays mono: `stereo=0`. A session of one eye walks the
scene one time, the two eye cameras are one camera at the head, the interface
has no shift, and the window goes to both eyes of the headset (MODE_3D=0).
The second walk was 2 to 11 ms of each frame of the run on the PC, and it is
the largest cost that the port can drop.

What goes with it, each one because it is work for a second eye that is not
there: one walk of two eyes, the depth of the interface, the pair of eye
pictures, and the readback of the render target for each pass. The readback
is also off with two eyes, because the window is the output and the picture
is already where the runtime reads it.

The hooks of the mod go in for `vr=1` now, and not only for `stereo=1`. With
`stereo=0` the PCVR mod installed nothing at all and sent a flat picture.

## The test on the PC, with the XrAPI UDP Tester

The mod takes the tester with no change of settings. Where it looks for the
handshake folder:

- `Z:\tmp\xr` when a drive Z: is there, which means a container.
- A folder `xrtemp` beside `runblack.exe` when it is not. It is not a folder
  in the root of C:, because a standard account cannot always make one there.

On the PC there is no `system` file, because only the runtime of a headset
writes one. So the mod waits for a packet instead, and the first packet says
that a runtime is there. The tries run for 120 seconds, so the tester can
start after the game. With no `system` the log names the default, `META
QUEST 3`, and says where the name came from.

The first packet that arrives goes in the log whole, and a packet that does
not parse goes in the log one time. Those two lines separate a socket that
hears nothing from a format that the mod reads wrongly.

The XrApi client of the port starts from the Shogo port,
`game/XrApiUDP.cpp` in `Shogo-VR-WXR`: C++, 32 bit, in the process of the
game, and proven on a Quest 3.

`Docs/MetaAI/` holds an earlier pass: notes and a standalone scaffold of an
XrApi client (`MetaAITestProj/src/vr/wxr/`, about 620 lines, with its own
solution and project file). It is outside `bwvr/`, it is in no build, and it
has not run. It is a second reading of the same protocol, useful as a cross
check and not as the start. Its notes read as a list of corrected faults, and
each line is a claim about code that this fork does not have. They are in
`BUG_FIXES.md` as things to verify.

## What the capture of the device says

`Docs/general_wxr/SixDOFinatorCalibration/SamplePoseData.txt` holds one packet
that a Quest 3 sent. The math checks replay it, so these are measurements and
not readings of a document.

| What | The value |
| --- | --- |
| The head | x 0.037, y 0.006, z -0.017 |
| The left hand | x -0.036, y -0.229, z -0.298 |
| The right hand | x 0.335, y -0.240, z -0.279 |
| The hands apart | 0.371 m |
| The interpupillary distance | 0.0678 m |
| The field of view that the runtime asks for | 99.00 across, 103.40 up and down, in degrees |
| The frame id | 176 |
| The characters of T and F | 21: 19 buttons and 2 flags |
| The name | META |

Three facts come out of it:

- **The space is head centred.** The head is at y of 0.006 and the hands are a
  palm below it. There is no floor. A check that asks whether the head is
  above a floor never passes on the device.
- **The quaternions are not unit length.** They are 0.27 to 0.44 percent off,
  so each one is normalised as it arrives.
- **The two fields of the field of view are degrees**, not tangents of half
  angles. The float table of `XRAPI_V0_4_DESC.md` said tangents, and this
  capture says otherwise, so that file is corrected.

Note that the runtime asks for more angle up and down (103.40) than across
(99.00). A headset panel is nearer square than 16:9.

## The container

The plain game runs on the Quest 3 in a WinlatorXR container with no change to
any container setting. The resolution is 1280x720, the default.

The game runs full screen in the container, and it does not know that it is in
one. So the picture already fills the display with no chrome, and the pixel at
the top left corner is the game's own. The mod needs no change of the window,
which removes the usual first fault of a port of this kind: chrome in the
corner means the sync pixel is never read.

Full screen also means that the window, the client and the render sizes agree.
The parent mod measured the game windowed, with `SetCooperativeLevel` and
`DDSCL_NORMAL`, so the present path and the pointer map need a check in the
container rather than a reading of `bwvr/docs/backend.md`.

**The size to plan for is 1400x1400**, and a 4:3 size of about the same height
where the game refuses a square one. A square size suits a headset panel, which
is nearer square than 16:9. Whether the game takes it is not known yet, and it
is a thing to try early, because the rest of the plan assumes it. Getting a
game of this age to accept an unusual mode is a recurring fight in these ports.

The square size is cheap. The drawing of the game does not grow with the
resolution: the parent mod measured 4.33 to 6.52 ms at every size from 1280x720
to 2560x1440. The cost that did grow was the readback of each eye across the
bus, and this fork has none, because the picture stays in the window.

The mod must still read the render size each frame and work at any size. The
resolution belongs to the player, and it is the supersampling control.

Two things follow for the port:

- Mono gives both eyes the full 1400 by 1400, so the square size fits as it
  is. The game takes its vertical angle from the aspect of the render, so the
  angle follows the shape that the player chose.
- A hint card of the mod is 768 pixels wide, which is wider than a render of
  700. The size of a card comes from the render width.

## The game that the mod works against

The install is at `E:\tmp\Q3Mods\BnW` on the PC, and it is a copy of the
install that runs on the Quest 3. It has no mod of any kind in it.

- `runblack.exe` has the SHA-1 `9c9aea96f046911ab1b7e496fae511ed5ebbfefe`,
  which is `kExpectedSha1` in `bwvr/src/common/bw_addresses.h`. So it is the
  v1.42 file that the mod hooks, and the check of the mod will pass.
  `readme-patch-v142.txt` is in the folder, as the README asks for.
- The folder has no `ddraw.dll`, so the proxy of the mod has a clear place.
  It has no dxwrapper, no dgVoodoo and no other graphics wrapper.
- It has `d3dim.dll` and `d3dim700.dll`, each 6 KB and each newer than the
  game. That is the shape of the community fix for Windows 10, which puts
  small files in place of the Direct3D immediate mode libraries. Whether the
  container needs them, or works better without them, is not known.
- `Profiles` holds the player's profiles. No test and no script of the mod
  reads, changes or deletes one.

`.\dev.ps1` puts the mod in that folder for a test on the PC, and
`.\dev.ps1 uninstall` takes it out and puts the `ddraw.dll` that was there
back. The owner holds a backup of the folder. `local\game-path.txt` names it.

The files that are ready to test on the device go to `Release\` in the root of
the repo, for the copy to the headset. Git does not track that folder.

## The stages that are open

The stages and their checkpoints are in `PROJECT_PLAN.md`. These are the ones
that no run has shown yet.

- **3** A check that the window fills the display with no chrome, and that the
  window, the client and the render sizes agree
- **4** The button table from a device log and not from a document: which
  index of the string each button of the Quest 3 moves
- **5** The sync pixel is drawn, and its square is in the log. What no run has
  shown is the square in the picture that the runtime captures
- **6** Head tracking carries the camera on the PC, with one walk of the
  scene. What is open is the feel through the headset
- **7** The interface of the mod on the screen, in about the middle third of
  the display across and down. Nothing of it floats in the room
- **8** The hands at the real hands, and the input map confirmed by the owner
- **9** The settings menu inside the game, and the comfort settings

The port is mono. What that gives and what it gives up is in
`PROJECT_PLAN.md`, "What mono gives".

## What is inherited and works on a PC

This is the state of the parent mod, not of the port. It is here so that a
later fault in the headset is not mistaken for a fault that the game always
had.

- The game draws the scene one time for each eye, the head moves the camera,
  and the picture goes to the headset through OpenXR.
- With motion controllers the player takes a thing with the grip, brings a far
  thing to the hand, throws, strokes and slaps the Creature, commands and
  leashes it, casts a miracle, and uses the village and the Temple.
- The mouse and the keyboard play as in the plain game.
- A story of the game shows in a window in the room, and the sound comes from
  where its source is.
- `.\dev.ps1` builds the mod and puts it in the game folder of a contributor.
- The open faults of the parent mod are in `bwvr/docs/roadmap.md`. Most of them
  wait on a check in a PCVR headset, and that check does not transfer to this
  fork.

The only PCVR setup that a person has tested is a Quest 3 through Virtual
Desktop.

## What is not written yet

In the order of the plan:

- The composite of the cards, the panels and the cut scene window into the
  frame, through the Direct3D 7 device of the game.
- The input map from the bool string.
- The settings menu inside the game.
- The removal of the OpenXR transport, dxwrapper, the Direct3D 11 bridge, the
  Vulkan renderer of the mod, the render thread, the merge of draw calls, the
  Steam Frame host and the launcher. The subtraction list is in
  `PROJECT_PLAN.md`.

## Open questions that only the owner or the device can answer

- Does the game need the `d3dim` files that are in the folder, or does the
  container work better without them?
- Does the game take 1400x1400 without letterboxing or distortion? The game
  runs full screen and picks a display mode, so the answer needs both the
  virtual display of the container and the mode that the game chooses. Which
  of `Setup.exe`, a value of the profile or the container settles it? Which 4:3
  size is the fall back?
- Does the container hand the game a working DirectDraw 7 and Direct3D 7, and
  through which path?
- Which index of the bool string moves for each button of the Quest 3
  controller?
- Which pair of field of view values looks right through the headset? The
  outbound pair is in degrees and is applied only above 1. Start at 104.5 and
  104.5, and keep the values in a file so they can be tuned without a build.
- Does the game keep its graphics settings off after a relaunch?
- Which files under `Release\` are release owned, so that no automated step
  writes them?

## The repo as it stands

Three things in the working tree are not committed: the root `CLAUDE.md` has
local changes, and `CLAUDE_PCVR.md` and `Docs/` are untracked.

One folder in the root is not part of the project:
`System.Management.Automation.Internal.Host.InternalHost`. It is a CMake build
folder that a PowerShell script made when it interpolated `$Host` into a path.
Its cache points at a worktree of the Steam Frame bridge, so it came from
there and not from this checkout. Git does not list it, because `.gitignore`
covers `CMakeCache.txt` and `CMakeFiles`. It can be removed.
