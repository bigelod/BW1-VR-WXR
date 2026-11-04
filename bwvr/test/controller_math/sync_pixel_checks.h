// The red sync pixel (sync_pixel.h). main.cpp includes this file after Check
// and Near, just before main.
#pragma once

#include "../../src/vr/sync_pixel.h"

namespace sync_pixel_checks {

inline void SyncPixelChecks() {
  using namespace sync_pixel;
  printf("The red sync pixel\n");

  // The frame id of the capture of the device, 176, and of the tester, 144.
  Check(Red(176.0f) == 176, "the frame id of a packet is the red", 1, 1);
  Check(Red(144.0f) == 144, "and the id of the tester too", 1, 1);
  // The value arrives as a float of a text line, so a value just under the
  // whole number must not fall to the number below.
  Check(Red(143.999f) == 144, "a float just below a whole number rounds up",
        1, 1);
  Check(Red(-1.0f) == 0, "a value below the range is 0", 1, 1);
  Check(Red(300.0f) == 255, "and above it 255", 1, 1);

  // X8R8G8B8, the format of a render target of 32 bits.
  const unsigned long r8 = 0x00FF0000ul;
  const unsigned long g8 = 0x0000FF00ul;
  const unsigned long b8 = 0x000000FFul;
  Check(Pack(176, 0, 0, r8, g8, b8) == 0x00B00000ul,
        "32 bits: the red is the id and green and blue are 0", 1, 1);
  Check(Pack(255, 0, 255, r8, g8, b8) == 0x00FF00FFul,
        "and the eye flag of the alternating mode is the blue", 1, 1);

  // R5G6B5, the format the game asks for at 16 bits. The red holds 5 bits,
  // so 176 of 256 becomes 22 of 32.
  const unsigned long r5 = 0xF800ul;
  const unsigned long g6 = 0x07E0ul;
  const unsigned long b5 = 0x001Ful;
  Check(MaskShift(r5) == 11, "the red of 5-6-5 starts at bit 11", 1, 1);
  Check(MaskBits(r5) == 5, "and it is 5 bits wide", 1, 1);
  Check(MaskBits(g6) == 6, "the green is 6", 1, 1);
  Check(Pack(176, 0, 0, r5, g6, b5) == (22ul << 11),
        "16 bits: the id loses its low bits, and the high bits keep their "
        "place", 1, 1);
  Check(Pack(0, 255, 0, r5, g6, b5) == g6, "a full green fills its mask",
        1, 1);

  // A1R5G5B5, the other 16 bit format of the game.
  Check(Pack(176, 0, 0, 0x7C00ul, 0x03E0ul, 0x001Ful) == (22ul << 10),
        "1-5-5-5 puts the red at bit 10", 1, 1);

  // A mask that is not there takes nothing, and no other channel moves.
  Check(Pack(176, 255, 255, r8, 0ul, 0ul) == 0x00B00000ul,
        "a channel with no mask writes no bits", 1, 1);

  Check(kSquare == 10, "the square is 10 by 10 pixels", 1, 1);
}

}  // namespace sync_pixel_checks
