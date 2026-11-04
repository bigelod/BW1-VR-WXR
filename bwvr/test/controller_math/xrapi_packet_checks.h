// The packet of the XrApi (xrapi_packet.h). main.cpp includes this file after
// Check and Near, just before main.
//
// The first checks replay a packet that a Quest 3 sent, held in
// Docs/general_wxr/SixDOFinatorCalibration/SamplePoseData.txt. A packet that
// a tester makes holds what the protocol is believed to say. A packet from a
// device holds what it does, so this one is the ground of the checks.
#pragma once

#include "../../src/vr/xrapi_packet.h"

namespace xrapi_packet_checks {

// The capture, one line, as the runtime sent it.
const char* const kDevicePacket =
    "client0 0.210 0.290 -0.930 0.040 0.0 0.0 -0.035777777 -0.229 "
    "-0.29800004 0.100 -0.300 0.950 -0.080 0.0 0.0 0.33455563 -0.240 "
    "-0.27888894 -0.020 -0.120 0.010 0.990 0.037 0.006 -0.017 0.0678 99.00 "
    "103.40 176 FFFFFFFFFFFFFFFFFFFFF META";

inline void XrApiPacketChecks() {
  using namespace xrapi_packet;
  printf("The packet of the XrApi\n");

  // --------------------------------------------------- the packet of a Quest 3
  Packet p;
  Check(Parse(kDevicePacket, &p), "the packet of the device parses", 1, 1);

  Near("the left hand is left of the head", p.left_position[0], -0.035778,
       1e-5);
  Near("and a palm below it", p.left_position[1], -0.229, 1e-5);
  Near("and ahead of it", p.left_position[2], -0.298, 1e-5);
  Near("the right hand is right of the head", p.right_position[0], 0.334556,
       1e-5);
  Near("and a palm below it", p.right_position[1], -0.240, 1e-5);

  // The positions are head centred. There is no floor in this space, so no
  // test of the mod may ask whether the head is above one.
  Near("the head is near the origin of its own space", p.head_position[1],
       0.006, 1e-5);
  Check(p.head_position[1] < 0.5f,
        "the head is below the half metre that a test of a floor would want",
        p.head_position[1], 0.0);
  Check(p.left_position[1] < 0.0f && p.right_position[1] < 0.0f,
        "and both hands are below the head, not above a floor",
        p.left_position[1], 0.0);

  Near("the interpupillary distance", p.ipd, 0.0678, 1e-6);
  Near("the field of view across, in degrees", p.fov_x, 99.0, 1e-4);
  Near("the field of view up and down, in degrees", p.fov_y, 103.4, 1e-4);
  Near("the frame id, which is also the red of the sync pixel", p.sync, 176.0,
       1e-4);
  Check(p.name[0] == 'M' && p.name[1] == 'E', "the name of the headset",
        p.name[0], 'M');

  // The quaternions of that packet are 0.27 to 0.44 percent off unit length.
  // Each one is a direction, so each is normalised as it arrives.
  for (int hand = 0; hand < 3; ++hand) {
    const float* q = (hand == 0)   ? p.left_orientation
                     : (hand == 1) ? p.right_orientation
                                   : p.head_orientation;
    const double n2 = (double)q[0] * q[0] + (double)q[1] * q[1] +
                      (double)q[2] * q[2] + (double)q[3] * q[3];
    Near("the quaternion is unit length after the parse", n2, 1.0, 1e-6);
  }

  // --------------------------------------------------- the characters of T and F
  // The runtime writes 19 buttons and then a word of 2 flags. A parse that
  // steps over the space between them sees one run of 21, and then the two
  // flags keep the indices 19 and 20.
  Check(p.bool_count == 21, "the buttons and the two flags make 21",
        p.bool_count, 21);
  Check(!Held(p, kBoolRightTrigger), "no button is held in that packet", 0, 0);
  Check(!Held(p, kBoolImmersive), "the immersive flag is not set", 0, 0);
  Check(!Held(p, kBoolSideBySide), "the side by side flag is not set", 0, 0);
  Check(!Held(p, 99), "an index past the end is not held", 0, 0);
  Check(!Held(p, -1), "and neither is an index below zero", 0, 0);

  // The order of the string, with no gap. The four directions of a stick sit
  // between the click of that stick and the trigger of that hand.
  Check(kBoolLeftGrip == 0, "the left grip is first", kBoolLeftGrip, 0);
  Check(kBoolLeftStickDown == 6, "the directions of the left stick are 3 to 6",
        kBoolLeftStickDown, 6);
  Check(kBoolLeftTrigger == 7, "then the left trigger", kBoolLeftTrigger, 7);
  Check(kBoolRightStickDown == 17,
        "the directions of the right stick are 14 to 17", kBoolRightStickDown,
        17);
  Check(kBoolRightTrigger == kButtons - 1, "the right trigger is the last one",
        kBoolRightTrigger, kButtons - 1);
  Check(kBoolImmersive == kButtons, "and the flags come after the buttons",
        kBoolImmersive, kButtons);

  // Each index in turn, so a change of the table cannot pass unseen.
  const char* const kEachInTurn =
      "client0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 0.064 99 103 "
      "7 FFFFFFFFFFFFFFFFFFT TF META";
  Packet each;
  Check(Parse(kEachInTurn, &each), "a packet with only the right trigger held",
        1, 1);
  Check(Held(each, kBoolRightTrigger), "the right trigger is held", 1, 1);
  Check(!Held(each, kBoolRightStickDown), "and the one before it is not", 0, 0);

  // THE GAP. The runtime sends the buttons and the two flags as one word
  // sometimes and as two words at other times. The packet of the device above
  // has one word of 21. This one has the gap, with buttons that alternate, so
  // a character that moved by one place would show.
  Packet gap;
  Check(Parse("client0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 "
              "0.064 99 103 7 FTFTFTFTFTFTFTFTFTF FT META",
              &gap),
        "a packet with a gap before the two flags parses", 1, 1);
  Check(gap.bool_count == 21, "the two words join into 21", gap.bool_count, 21);
  Check(!Held(gap, kBoolLeftGrip), "index 0 is F", 0, 0);
  Check(Held(gap, kBoolLeftMenu), "index 1 is T", 1, 1);
  Check(Held(gap, kBoolRightStickDown), "index 17 is T", 1, 1);
  Check(!Held(gap, kBoolRightTrigger), "index 18 is F", 0, 0);
  // The word of the flags is "FT", so the immersive flag is F and the side by
  // side flag is T.
  Check(!Held(gap, kBoolImmersive), "the first flag is past the gap, at 19", 0,
        0);
  Check(Held(gap, kBoolSideBySide), "and the second at 20", 1, 1);
  Check(gap.name[0] == 'M', "the name comes after the flags", gap.name[0], 'M');

  // The same characters with no gap must give the same answer.
  Packet joined;
  Check(Parse("client0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 "
              "0.064 99 103 7 FTFTFTFTFTFTFTFTFTFFT META",
              &joined),
        "the same characters as one word parse", 1, 1);
  Check(joined.bool_count == gap.bool_count, "to the same count",
        joined.bool_count, gap.bool_count);
  bool same = true;
  for (int i = 0; i < joined.bool_count; ++i) {
    if (Held(joined, i) != Held(gap, i)) same = false;
  }
  Check(same, "and to the same value at every index", same ? 1 : 0, 1);

  // A press, and the two words of the wire.
  Packet q;
  Check(Parse("client0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 "
              "0.064 99 103 7 FFFFFFFFFFFFTFFFFFF TF META",
              &q),
        "a packet with the buttons and the flags as two words parses", 1, 1);
  Check(q.bool_count == 21, "the two words join into 21 characters",
        q.bool_count, 21);
  Check(Held(q, kBoolRightGrip), "the right grip is held", 1, 1);
  Check(!Held(q, kBoolLeftGrip), "and the left grip is not", 0, 0);
  // The word of the flags is "TF": the immersive flag and then the side by
  // side flag.
  Check(Held(q, kBoolImmersive), "the immersive flag is set", 1, 1);
  Check(!Held(q, kBoolSideBySide), "and the side by side flag is not", 0, 0);

  // A runtime that sends only the 19 buttons.
  Packet r;
  Check(Parse("client 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 "
              "0.064 99 103 7 TFFFFFFFFFFFFFFFFFF",
              &r),
        "a packet of 19 characters and no name parses", 1, 1);
  Check(r.bool_count == 19, "and it holds 19", r.bool_count, 19);
  Check(Held(r, kBoolLeftGrip), "the left grip is held", 1, 1);
  Check(!Held(r, kBoolImmersive),
        "the flags are past the end, so they are not held", 0, 0);
  Check(r.name[0] == '\0', "and the name is empty", r.name[0], 0);

  // --------------------------------------------------- what must not parse
  Packet before = p;
  Packet keep = before;
  Check(!Parse("client0 0.1 0.2 0.3", &keep),
        "a packet with too few numbers does not parse", 0, 0);
  Check(keep.bool_count == before.bool_count,
        "and it leaves the packet that was there", keep.bool_count,
        before.bool_count);
  Check(!Parse("", &keep), "an empty line does not parse", 0, 0);
  Check(!Parse(nullptr, &keep), "and neither does no line at all", 0, 0);

  // A token that the mod does not know, after the name. The format takes new
  // fields at its end between versions, so they are left alone.
  Packet later;
  Check(Parse("client0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 1 0 0 0 "
              "0.064 99 103 7 TFFFFFFFFFFFFFFFFFF TF META 0.5 something",
              &later),
        "a packet with tokens that the mod does not know still parses", 1, 1);
  Near("and its frame id is right", later.sync, 7.0, 1e-4);

  // --------------------------------------------------- the numbers themselves
  float value = 0.0f;
  Check(ReadNumber("0.0678", &value) == 6, "a plain decimal", value, 0.0678);
  Near("its value", value, 0.0678, 1e-7);
  Check(ReadNumber("-0.035777777", &value) == 12, "a long negative decimal",
        value, -0.0358);
  Near("its value", value, -0.035777777, 1e-7);
  Check(ReadNumber("176", &value) == 3, "a whole number", value, 176.0);
  Near("its value", value, 176.0, 1e-4);
  Check(ReadNumber("1.5e2", &value) == 5, "a number with an exponent", value,
        150.0);
  Near("its value", value, 150.0, 1e-4);
  Check(ReadNumber("2e-3", &value) == 4, "and a negative exponent", value,
        0.002);
  Near("its value", value, 0.002, 1e-9);
  Check(ReadNumber("1e", &value) == 1,
        "an e with no digits is not part of the number", 1, 1);
  Check(ReadNumber("client0", &value) == 0, "a word is not a number", 0, 0);
  Check(ReadNumber("", &value) == 0, "and neither is nothing", 0, 0);
  Check(ReadNumber(".5", &value) == 2, "a point with no whole part", value,
        0.5);
  Near("its value", value, 0.5, 1e-7);

  // The decimal mark of the wire is a point, whatever locale the game set.
  // A reader that takes the mark of the locale stops at the point on a
  // machine whose locale uses a comma, and loses every value after it.
  Check(ReadNumber("103,40", &value) == 3,
        "a comma does not continue a number", value, 103.0);
  Near("so the value stops at the comma", value, 103.0, 1e-4);

  // --------------------------------------------------- the field of view out
  // The game asks for 70.0 degrees across, and its vertical angle follows the
  // aspect of the render. The mod sends the pair that it drew with.
  Near("a square render has the same angle up and down",
       VerticalDegrees(70.0f, 1400, 1400), 70.0, 1e-3);
  Near("16 by 9 is much less up and down",
       VerticalDegrees(70.0f, 1280, 720), 42.9957, 1e-3);
  Near("one eye of a square render under side by side is more up and down",
       VerticalDegrees(70.0f, 700, 1400), 108.9407, 1e-3);
  Near("4 by 3", VerticalDegrees(70.0f, 1400, 1050), 55.4129, 1e-3);
  Near("a render with no size keeps the angle across",
       VerticalDegrees(70.0f, 0, 0), 70.0, 1e-4);
}

}  // namespace xrapi_packet_checks
