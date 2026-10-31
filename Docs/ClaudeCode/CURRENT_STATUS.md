# Current status

What is true now. The plan is in `PROJECT_PLAN.md` and the goal is in
`PROJECT_OVERVIEW.md`. Keep this file current, and remove an item when it is
done rather than marking it.

## In one paragraph

No line of the port is in the build. The repo is the PCVR mod, which runs on a
Windows PC through OpenXR, plus the general WinlatorXR guidance in
`Docs/general_wxr/` and these four files. The plain game runs on the Quest 3 in
a WinlatorXR container. Nothing of the mod has run there, and nothing has run
against the XrAPI UDP Tester. The next stage is 2: the mod loads in the
container and writes a log.

`Docs/MetaAI/` holds an earlier pass: notes and a standalone scaffold of an
XrApi client (`MetaAITestProj/src/vr/wxr/`, about 620 lines, with its own
solution and project file). It is outside `bwvr/`, it is in no build, and it
has not run. Whether Feature 1 starts from it is an open question. Its notes
read as a list of corrected faults, and each line is a claim about code that
this fork does not have. They are in `BUG_FIXES.md` as things to verify.

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

- Side by side cuts the width in two, so each eye gets 700 by 1400 at the
  square size. The game takes its vertical angle from the aspect of the
  render, so the angle of an eye comes from the size of its own half. Mono and
  alternate eye rendering give each eye the full 1400 by 1400, so the square
  size fits them and side by side would want a width of 2800.
- A hint card of the mod is 768 pixels wide, which is wider than the 700 of an
  eye under side by side. The size of a card comes from the render width.

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

- **2** The mod loads in the container and writes a log whose first line is
  the build stamp. A MinHook hook holds under Box86 and Wine
- **3** A check that the window fills the display with no chrome, and that the
  window, the client and the render sizes agree
- **4** The XrApi client parses a packet captured on the device, and the button
  table comes from a device log and not from a document
- **5** The sync pixel, drawn last, 10 by 10
- **6** Mono 6DOF head tracking feels locked. This is the big one
- **7** The cards, the panels and the cut scene window in the frame
- **8** The hands at the real hands, and the input map confirmed by the owner
- **9** The settings menu inside the game, and the comfort settings
- **10** Stereo, chosen from a measurement of alternate eye rendering against
  side by side. Mono stays a shipping mode

The port is built on mono and ships mono first. What that defers is listed in
`PROJECT_PLAN.md`, "What mono defers".

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

Everything of the port. In the order of the plan:

- The XrApi client: the handshake files, the socket on 7872, the parse, the
  outbound packet on 7278.
- The window that fills the display and keeps no chrome.
- The sync pixel.
- The conversion from an XrApi pose to the camera of the game.
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
- Does a MinHook hook on `runblack.exe` hold under Box86 and Wine?
- Does the container hand the game a working DirectDraw 7 and Direct3D 7, and
  through which path?
- What does one raw inbound packet look like on the device, field by field,
  next to what the tester sends?
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
