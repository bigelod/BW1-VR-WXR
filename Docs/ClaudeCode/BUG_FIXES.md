# Faults

The register of faults of this fork. A fault goes in when it is seen, and it
comes out when it is corrected: the code and the doc of its area then say what
the mod does now. The history of the correction is in git, not here.

No fault of the port is open, because no part of the port runs yet.

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
  is true it belongs in the ingest, in one place.
- **The length of the bool string.** The claim is 21 characters, and that some
  containers send 19. The runtime writes 19 button characters and then a
  separate token of two flags, so a parse that skips whitespace sees 21 and a
  parse that does not sees 19. Bounds check every index either way.
- **The button table.** The scaffold names 21 buttons and includes four thumb
  directions for each hand. The general notes mark those indices as unused or
  unverified. Build the table from a device log, not from either document.
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
