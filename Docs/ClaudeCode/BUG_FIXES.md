# Faults

The register of faults of this fork. A fault goes in when it is seen, and it
comes out when it is corrected: the code and the doc of its area then say what
the mod does now. The history of the correction is in git, not here.

No fault of the port is open.

## How to write an entry

```
### A short name for the symptom

**The symptom.** What the owner saw or heard, in his own words where possible.
**Where.** The build stamp, the headset, the runtime, the container resolution
and the ini keys that were not the default.
**The cause.** What was actually wrong. Not the first guess.
**The proof.** The log line, the number or the screenshot that settled it.
**Open or corrected.** If it is open, the next thing to measure.
```

Read the words of the owner about a symptom literally. "It swims when I turn"
and "it lags" are different reports, and the second one is not a frame rate
report in this stack.

## First suspects in this stack

Each of these presents as a deep fault and is not one. Check them before
reading any pose math.

| What is seen | The first thing to check |
| --- | --- |
| A change appears to do nothing | The build stamp on line 1 of the log. A stale binary is invisible without it |
| The log repeats one line for every frame | A call of the frame path that has nothing to do on this transport and answers no. The window is the output here, so each submit of a picture and each readback of the render is work for nobody |
| A line of the log that a try should hold is missing | The tries for a headset are quiet after the first one. A line that only a quiet try can write never arrives |
| No line of the log begins with `xrapi`, and the mod waits 120 s for a headset | The client did not start. The mod starts a headset in three steps, `StartInstance`, `StartSystem` and `StartSession`, and work that sits in any other call never runs |
| The game does not fill the display in the container | `window_fit`. The mod reshapes the window to the shape of the render for the mouse, and that takes the game off the full display. The log says "window A x B set to C x D" |
| No packet arrives at all, and no error | The `version` file. It is read one time at startup, and an unrecognised value starts no listener at all |
| Packets arrived and then stopped | The mod sent `MODE_VR=0`. That disables the whole inbound stream |
| Every packet arrives twice | Both 7872 and 7873 are bound. The runtime sends the same packet to both |
| Head tracking swims or sickens, and the math is right | The sync pixel. A wrong value, the wrong corner, drawn before something that covers it, filtered, blended, or green not 0 |
| The sync pixel is right and nothing changed | The window has chrome, so the runtime samples the title bar |
| The hands swing when the head turns | A hand pose went through the head transform. Convert each pose on its own and share only the recentre origin |
| Left handed mode picks the wrong hand | The handedness swap is applied twice |
| A change of the field of view does nothing | The outbound pair is in degrees and is applied only above 1. A tangent of a half angle is below 1 and is ignored |
| Haptics never stop | A value of 0 does not clear them. They are a duration in frames, so the number that the mod sends has to decay |
| A hand basis is stretched or not symmetric | A quaternion that is not unit length. Normalise on ingest, unconditionally |
| The hands collapse onto the head, or a hand sticks to the view | A check that assumes a floor. XrApi positions are head centred: a Quest 3 capture has the head at y of 0.006 and the hands at y of -0.23. A gate on "the head is above the floor" never passes, and with no offset both hands land in one place |
| The setting for the dominant hand appears to do nothing | The same fault. Two hands at one place cannot differ |
| The view slides, and a rotation does not come back to where it started | The field of view in the outbound packet is not the one the game drew with. The runtime maps the frame with those angles, so the world moves at a different rate from the head, and by a different rate in pitch than in yaw |
| A correction works at one pose and nowhere else | The frame it is measured in. Many such faults are exactly zero at the origin, so a test at one still pose tests the only case that cannot fail |
| A value tuned on the desktop is wrong on the device | The frame that the offset is measured in, not the precision |
| A shipped default does nothing on the device | A saved config on the device overrides it |
| Tracking is about a quarter too fast in a flat window mode | The window is attached to the head. Read bool 19 and damp by about 0.8 |

`Docs/general_wxr/README.md` holds the same table with the reason for each
line, and `Docs/general_wxr/XRAPI_V0_4_DESC.md` holds the field detail.

## Claims to verify, from the earlier notes

`Docs/MetaAI/` holds notes and a standalone scaffold
(`MetaAITestProj/src/vr/wxr/`, about 620 lines) from an earlier pass. None of
it is in the build, and none of it has run. Its fault list reads as a list of
corrections, but each line is a claim about code that this fork does not have
yet. Treat each one as a thing to check at the matching stage, not as a fault
that is gone.

The ones worth checking, because each would cost a session:

- **The decimal separator.** A packet uses a point. A Windows locale that uses
  a comma can break a parse of a float. The claim is that a parse has to be
  given the classic locale.
- **The frame id and a byte.** The frame id arrives as a number and goes out
  as the red channel of a pixel. The claim is that it has to be masked to 8
  bits.
- **A block on the first packet.** The claim is that a wait for the first
  packet needs a timeout, or the game never starts in a container with no XR.
- **A zombie receive thread.** The claim is that a shutdown has to unblock
  `recvfrom` before it joins the thread.
- **The roll of a grip pose.** The claim is that an XrApi grip is rolled by
  180 degrees against the OpenXR aim pose that the PCVR mod expects. If that
  is true it belongs in the ingest, in one place. The general notes now say
  that the roll is per headset model, that a Quest 2 reports the hands upside
  down and a Quest 3 does not, and that a port needs a "hands roll" setting
  and a file to override the detection. So expect this one to be real, and
  expect it to need a switch rather than a constant.
- **The length of the bool string.** Settled. Both forms come over the wire:
  the 21 characters arrive as one word, and at other times with a gap before
  the last two (`...FTFTFTF FT`). The parse joins each run of T and F, so the
  two flags keep the indices 19 and 20 either way, and the math checks hold
  both forms. A parse that takes one word only loses the flags when the gap is
  there. A parse that counts back from the end of the line puts every button
  at the wrong index when it is not. A runtime of 0.2 sends no flags at all,
  so every index is still bounds checked.
- **The button table.** Settled by the owner: the indices are exactly as the
  string gives them, with no gap in the middle. The four directions of a stick
  sit between the click of that stick and the trigger of that hand, so the
  left hand holds 0 to 7, the face buttons hold 8 to 11, and the right hand
  holds 12 to 18.
- **The field of view.** The claim is that a container reported 108.9 by 103.4
  while the game rendered 90, and that an affine transform in a file settled
  it. The outbound pair is a tuning value, so keep it in a file and tune it by
  eye through the headset.
- **The red pixel behind the logos.** The claim is that the movies of the game
  at the start cover the corner. Skipping the logos is already a hint that the
  mod gives the player.
- **The right stick click of the runtime.** The claim is that WinlatorXR has
  its own menu on that click, which can put the headset back to a flat window.
  If that is true, the input map must not need that click.
