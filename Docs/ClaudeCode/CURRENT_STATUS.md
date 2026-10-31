# Current status

What is true now. The plan is in `PROJECT_PLAN.md` and the goal is in
`PROJECT_OVERVIEW.md`. Keep this file current, and remove an item when it is
done rather than marking it.

## In one paragraph

No line of the port is in the build. The repo is the PCVR mod, which runs on a
Windows PC through OpenXR, plus the general WinlatorXR guidance in
`Docs/general_wxr/` and these four files. Nothing of this fork has run in a
WinlatorXR container, on a Quest 3 or against the XrAPI UDP Tester. The
blocker is Stage 1: the plain game in a container.

`Docs/MetaAI/` holds an earlier pass: notes and a standalone scaffold of an
XrApi client (`MetaAITestProj/src/vr/wxr/`, about 620 lines, with its own
solution and project file). It is outside `bwvr/`, it is in no build, and it
has not run. Whether Feature 1 starts from it is an open question. Its notes
read as a list of corrected faults, and each line is a claim about code that
this fork does not have. They are in `BUG_FIXES.md` as things to verify.

## The milestones

- [ ] **0** Intake complete, the plan agreed
- [ ] **1** The plain game runs in a container
- [ ] **2** The mod loads in the container and logs its build stamp. A MinHook
      hook is proven under Box86 and Wine
- [ ] **3** The window is borderless and fills the display
- [ ] **4** The XrApi client parses a packet captured on the device
- [ ] **5** The sync pixel is drawn last, 10 by 10, in every half
- [ ] **6** Mono 6DOF head tracking feels locked
- [ ] **7** The presentation mode is chosen from a measurement
- [ ] **8** The cards, the panels and the cut scene window are in the frame
- [ ] **9** The hands are at the real hands, and the input map is confirmed
- [ ] **10** The settings menu inside the game

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

The only setup that a person has tested is a Quest 3 through Virtual Desktop,
on a PC.

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

- Does the plain game run in a WinlatorXR container at all, and what did it
  need to run? Each workaround here is a thing that an installer or a readme
  has to carry later.
- Which container resolution does the game accept without letterboxing or
  distortion? 2800x1400 is the usual start. A square resolution suits a
  headset panel.
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
- Which files are release owned, so that no automated step writes them?

## The repo as it stands

Three things in the working tree are not committed: the root `CLAUDE.md` has
local changes, and `CLAUDE_PCVR.md` and `Docs/` are untracked.

One folder in the root is not part of the project:
`System.Management.Automation.Internal.Host.InternalHost`. It is a CMake build
folder that a PowerShell script made when it interpolated `$Host` into a path.
Its cache points at a worktree of the Steam Frame bridge, so it came from
there and not from this checkout. Git does not list it, because `.gitignore`
covers `CMakeCache.txt` and `CMakeFiles`. It can be removed.
