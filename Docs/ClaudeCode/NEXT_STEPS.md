# Next steps

The short board. `CURRENT_STATUS.md` holds the detail of what is true,
`PROJECT_PLAN.md` the plan and the decisions, `BUG_FIXES.md` the faults.

## Done

- The build and the deploy: `bwvr\tools\wxr\deploy.ps1` builds through
  `dev.ps1`, runs the math tests, refuses a build older than the newest
  source, stages `Release\BnW-VR-WXR` with a hash of each file, and installs
  to the game folder. Line 1 of `bwvr.log` names the build.
- No OpenXR in the build. The mod needs no `openxr_loader.dll`.
- The XrApi client: the handshake files, the socket on 7872, the parse of a
  packet, and the outbound packet on 7278 with the angles that the game drew
  with.
- The head carries the camera of the game. The hands are at the controllers.
- Mono, one walk of the scene. No readback of the render and no submit: the
  window is the output.
- The red sync pixel, drawn at the present.
- The mod loads in the container, and about 120 MinHook hooks all say ok.

## Next, in this order

1. **The interface of the mod on the screen** (stage 7). The cards, the
   prompts and the cut scene window go into the picture, in about the middle
   third across and down. The OpenXR layers go with it: `StageCard`,
   `StagePanelQuad`, `StageWindow`, `SetPanelQuads` and `EnablePanel` are
   no-ops now, and they and their callers come out.
2. **A census of the buttons** (stage 4). The mod has no line that names which
   index of the T and F string moved. One is needed, so the owner can press
   each button once and the log answers. The table in `xrapi_packet.h` is
   believed and not proven.
3. **The field of view that goes out.** It is the tuning value of the port and
   it has no ini key yet. Give it one, so it is tuned through the headset with
   no build.
4. **The subtraction list** in `PROJECT_PLAN.md`: dxwrapper, the bridge to
   Direct3D 11, the Vulkan renderer of the mod, the render thread, the merge
   of draw calls, the launcher and the host of the Steam Frame. Each one is
   dead weight in a container.

## A quick test on the PC, with the XrAPI UDP Tester

Start the game, then the tester, or the tester first. The tries last 120
seconds. The handshake files go to a folder `xrtemp` beside `runblack.exe`.

- [ ] Line 1 of `bwvr.log` holds the stamp that the deploy printed.
- [ ] `stereo : stereo=0, mono`, and `xrapi : the session runs, ... 1 eye`.
- [ ] `xrapi : the first packet: client0 ...`, and no line that says a packet
      did not parse.
- [ ] A dark red square at the top left corner of the window, and one
      `sync pixel :` line that names the surface and its bits of red.
- [ ] No line that says the session ended, and the window keeps drawing.
- [ ] The report of 450 frames says `scene x1` and has no `grab`.
- [ ] `head camera : ... carried the head` on each frame, and the view follows
      the poses of the tester.

## A quick test on the device

- [ ] The game fills the display, and no title bar or border is in the corner
      where the square goes (stage 3). The log says that the window, the
      client and the render sizes agree.
- [ ] The handshake folder is `Z:\tmp\xr`, and the name of the headset comes
      from the `system` file and not from the default.
- [ ] **Head tracking feels locked and not swimmy.** This is the test of the
      sync pixel. Nothing of the square can be judged by eye.
- [ ] The frame time at 1280x720 and at 1400x1400, from the report of 450
      frames. The walk of the scene does not grow with the resolution, so the
      two should be close.
- [ ] The game takes 1400x1400 with no letterbox and no stretch. If it
      refuses, which 4:3 size of about the same height it takes, and what
      forces it.
- [ ] A hand feels a pulse, and the pulse stops.
