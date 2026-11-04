// THE RED SYNC PIXEL of XrApi, as numbers only.
//
// The runtime captures the window of the game on a thread of its own, and it
// reads one pixel at the top left corner of that picture to find which frame
// it has. The frame id of the inbound packet goes back as the red of that
// pixel, with no change at all: the runtime learns the colour space from the
// values that it sees, so a scale or a gamma correction breaks the match of
// a picture to the pose that drew it. Green is 0. Blue carries the eye flag
// of the alternating eye mode, and it is 0 in each other mode.
//
// The colour goes into a surface of DirectDraw, which can have any layout of
// bits. So the value is packed with the masks of that surface.
//
// Docs/general_wxr/XRAPI_V0_4_DESC.md section 5.

#ifndef BWVR_SYNC_PIXEL_H
#define BWVR_SYNC_PIXEL_H

namespace sync_pixel {

// The square is 10 by 10 pixels. The reader takes the one pixel at 0,0, so
// one pixel would do. Ten survive a border, a scale of the capture and a
// filter, and they cost nothing.
const int kSquare = 10;

// The red of this frame, 0 to 255, from float 28 of the packet.
inline int Red(float sync) {
  const int red = (int)(sync + 0.5f);
  if (red < 0) return 0;
  if (red > 255) return 255;
  return red;
}

// How far up a mask starts, and how many bits it has.
inline int MaskShift(unsigned long mask) {
  if (mask == 0) return 0;
  int shift = 0;
  while ((mask & 1ul) == 0ul) {
    mask >>= 1;
    ++shift;
  }
  return shift;
}

inline int MaskBits(unsigned long mask) {
  if (mask == 0) return 0;
  mask >>= MaskShift(mask);
  int bits = 0;
  while ((mask & 1ul) != 0ul) {
    mask >>= 1;
    ++bits;
  }
  return bits;
}

// One channel of 8 bits into its place in the pixel. A channel of fewer bits
// than 8 loses the low bits of the value, and the frame ids that it can tell
// apart are that much fewer: a red of 5 bits holds 32 of the 256.
inline unsigned long Channel(int value, unsigned long mask) {
  const int bits = MaskBits(mask);
  if (bits <= 0) return 0ul;
  unsigned long scaled = (unsigned long)(value & 0xFF);
  if (bits < 8) {
    scaled >>= (8 - bits);
  } else if (bits > 8) {
    scaled <<= (bits - 8);
  }
  return (scaled << MaskShift(mask)) & mask;
}

inline unsigned long Pack(int red, int green, int blue, unsigned long red_mask,
                          unsigned long green_mask, unsigned long blue_mask) {
  return Channel(red, red_mask) | Channel(green, green_mask) |
         Channel(blue, blue_mask);
}

}  // namespace sync_pixel

#endif  // BWVR_SYNC_PIXEL_H
