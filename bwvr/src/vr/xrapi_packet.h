// The packet of the XrApi of WinlatorXR, parsed. No socket and no Windows, so
// the math checks can replay a packet that a device sent.
//
// The runtime sends one line of ASCII to port 7872 each frame:
//
//   client0 <28 floats> <frame id> <19 or 21 characters of T and F> <name>
//
// A packet from a Quest 3, which the checks replay
// (Docs/general_wxr/SixDOFinatorCalibration/SamplePoseData.txt):
//
//   client0 0.210 0.290 -0.930 0.040 0.0 0.0 -0.035777777 -0.229 ...
//           ... 0.0678 99.00 103.40 176 FFFFFFFFFFFFFFFFFFFFF META
//
// Three things about that packet decide the code here.
//
// THE POSITIONS ARE HEAD CENTRED. They are not above a floor. In that packet
// the head is at y of 0.006 and the two hands at y of -0.229 and -0.240, a
// palm below it. So no test may ask whether the head is above a floor: there
// is no floor in this space.
//
// THE QUATERNIONS ARE NOT UNIT LENGTH. In that packet they are 0.27 to 0.44
// percent off. The shortcut that turns a vector by a quaternion also scales
// it by the square of the length, and the scale lands on the basis of a hand
// by a different amount for each hand. So each quaternion is normalised as it
// arrives, with no test.
//
// THE TWO FIELDS OF THE FIELD OF VIEW ARE DEGREES. That packet asks for 99.00
// and 103.40. They are not tangents of half angles.
//
// See Docs/general_wxr/XRAPI_V0_4_DESC.md for every field.
#pragma once

#include <math.h>

namespace xrapi_packet {

// The floats of the packet, in the order of the wire.
const int kFloats = 29;

// The characters of T and F: 19 for the buttons, and 2 more for the immersive
// flag and the side by side flag.
//
// BOTH FORMS COME OVER THE WIRE. The 21 arrive as one word sometimes, as in
// the capture of the Quest 3 above, and at other times with a gap before the
// last two:
//
//   ...FFFFFFFFFFFFFFFFFFFFF META      one word of 21
//   ...FTFTFTF FT META                 a gap before the two flags
//
// So the parse joins each run of T and F that follows the numbers, and the
// two flags keep the indices 19 and 20 either way. A parse that takes one
// word only loses the flags when the gap is there, and a parse that counts
// from the end of the line puts every button at the wrong index when it is
// not.
const int kBoolsMost = 24;

// The index of each control in the characters of T and F. The order is the
// order of the string, with no gap: the four directions of each stick are
// there as well, between the click of that stick and the trigger of that
// hand.
const int kBoolLeftGrip = 0;
const int kBoolLeftMenu = 1;
const int kBoolLeftStickClick = 2;
const int kBoolLeftStickLeft = 3;
const int kBoolLeftStickRight = 4;
const int kBoolLeftStickUp = 5;
const int kBoolLeftStickDown = 6;
const int kBoolLeftTrigger = 7;
const int kBoolX = 8;
const int kBoolY = 9;
const int kBoolA = 10;
const int kBoolB = 11;
const int kBoolRightGrip = 12;
const int kBoolRightStickClick = 13;
const int kBoolRightStickLeft = 14;
const int kBoolRightStickRight = 15;
const int kBoolRightStickUp = 16;
const int kBoolRightStickDown = 17;
const int kBoolRightTrigger = 18;

// The two flags after the buttons. The runtime sends them as their own word,
// so a parse that steps over the space keeps them at 19 and 20.
const int kBoolImmersive = 19;
const int kBoolSideBySide = 20;

// The count that the runtime writes for the buttons, before the flags.
const int kButtons = 19;

struct Packet {
  // Quaternions x y z w, normalised. Positions in metres, head centred.
  float left_orientation[4];
  float left_stick[2];
  float left_position[3];
  float right_orientation[4];
  float right_stick[2];
  float right_position[3];
  float head_orientation[4];
  float head_position[3];
  float ipd;              // metres
  float fov_x;            // degrees
  float fov_y;            // degrees
  float sync;             // the frame id, and the red of the sync pixel
  char bools[kBoolsMost]; // the characters of T and F, and a zero
  int bool_count;
  char name[32];          // the name of the headset, "META" in that packet
};

// True when the character of the button at this index is T. Every read goes
// through here, because an index can be past the end of a shorter string.
inline bool Held(const Packet& packet, int index) {
  if (index < 0 || index >= packet.bool_count) return false;
  return packet.bools[index] == 'T';
}

// --------------------------------------------------------------- the numbers

// A decimal number of the wire, with no locale.
//
// The runtime writes a point for the decimal mark. The functions of the C
// library that read a float take the mark of the locale of the process, so on
// a machine whose locale uses a comma they stop at the point and every value
// after it is lost. The game can set a locale, so the parse cannot depend on
// one.
//
// Reads an optional sign, digits, an optional point with digits, and an
// optional exponent. Returns the count of characters that it read, and 0 when
// the text is not a number.
inline int ReadNumber(const char* text, float* out) {
  if (text == nullptr) return 0;
  const char* p = text;
  double sign = 1.0;
  if (*p == '+' || *p == '-') {
    if (*p == '-') sign = -1.0;
    ++p;
  }
  int digits = 0;
  double value = 0.0;
  while (*p >= '0' && *p <= '9') {
    value = value * 10.0 + (double)(*p - '0');
    ++p;
    ++digits;
  }
  if (*p == '.') {
    ++p;
    double place = 0.1;
    while (*p >= '0' && *p <= '9') {
      value += (double)(*p - '0') * place;
      place *= 0.1;
      ++p;
      ++digits;
    }
  }
  if (digits == 0) return 0;
  if (*p == 'e' || *p == 'E') {
    const char* mark = p;
    ++p;
    double exp_sign = 1.0;
    if (*p == '+' || *p == '-') {
      if (*p == '-') exp_sign = -1.0;
      ++p;
    }
    int exp_digits = 0;
    int exponent = 0;
    while (*p >= '0' && *p <= '9') {
      exponent = exponent * 10 + (*p - '0');
      ++p;
      ++exp_digits;
    }
    if (exp_digits == 0) {
      p = mark;  // An e with no digits is not part of the number.
    } else {
      double factor = 1.0;
      for (int i = 0; i < exponent; ++i) factor *= 10.0;
      if (exp_sign < 0.0) {
        value /= factor;
      } else {
        value *= factor;
      }
    }
  }
  *out = (float)(sign * value);
  return (int)(p - text);
}

// A quaternion of the wire is a direction and not a length.
inline void Normalise(float q[4]) {
  const double n2 = (double)q[0] * q[0] + (double)q[1] * q[1] +
                    (double)q[2] * q[2] + (double)q[3] * q[3];
  if (n2 < 1e-12) {
    q[0] = 0.0f;
    q[1] = 0.0f;
    q[2] = 0.0f;
    q[3] = 1.0f;
    return;
  }
  const double inv = 1.0 / sqrt(n2);
  q[0] = (float)(q[0] * inv);
  q[1] = (float)(q[1] * inv);
  q[2] = (float)(q[2] * inv);
  q[3] = (float)(q[3] * inv);
}

// --------------------------------------------------------------- the parse

inline bool IsSpace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// One line of the wire into a Packet. False leaves the Packet as it was, so a
// packet that the mod cannot read never replaces a pose that it can.
//
// The line starts with a word that is not a number ("client0", and the digit
// is part of it), and the parse steps over any such word. A line can hold
// more tokens after the name, because the format takes new fields at its end
// between versions, and those are left alone.
inline bool Parse(const char* line, Packet* out) {
  if (line == nullptr || out == nullptr) return false;

  const char* p = line;
  float numbers[kFloats];
  int have = 0;

  // The words that are not numbers, before the numbers.
  while (*p != '\0') {
    while (IsSpace(*p)) ++p;
    if (*p == '\0') break;
    float value = 0.0f;
    const int read = ReadNumber(p, &value);
    // A number must end the word. "client0" starts with a letter, and a word
    // like "0.4x" is not a number either.
    if (read > 0 && (IsSpace(p[read]) || p[read] == '\0')) break;
    while (*p != '\0' && !IsSpace(*p)) ++p;
  }

  // The numbers.
  while (have < kFloats) {
    while (IsSpace(*p)) ++p;
    if (*p == '\0') break;
    float value = 0.0f;
    const int read = ReadNumber(p, &value);
    if (read == 0 || !(IsSpace(p[read]) || p[read] == '\0')) break;
    numbers[have] = value;
    ++have;
    p += read;
  }
  if (have < kFloats) return false;

  Packet packet;
  packet.left_orientation[0] = numbers[0];
  packet.left_orientation[1] = numbers[1];
  packet.left_orientation[2] = numbers[2];
  packet.left_orientation[3] = numbers[3];
  packet.left_stick[0] = numbers[4];
  packet.left_stick[1] = numbers[5];
  packet.left_position[0] = numbers[6];
  packet.left_position[1] = numbers[7];
  packet.left_position[2] = numbers[8];
  packet.right_orientation[0] = numbers[9];
  packet.right_orientation[1] = numbers[10];
  packet.right_orientation[2] = numbers[11];
  packet.right_orientation[3] = numbers[12];
  packet.right_stick[0] = numbers[13];
  packet.right_stick[1] = numbers[14];
  packet.right_position[0] = numbers[15];
  packet.right_position[1] = numbers[16];
  packet.right_position[2] = numbers[17];
  packet.head_orientation[0] = numbers[18];
  packet.head_orientation[1] = numbers[19];
  packet.head_orientation[2] = numbers[20];
  packet.head_orientation[3] = numbers[21];
  packet.head_position[0] = numbers[22];
  packet.head_position[1] = numbers[23];
  packet.head_position[2] = numbers[24];
  packet.ipd = numbers[25];
  packet.fov_x = numbers[26];
  packet.fov_y = numbers[27];
  packet.sync = numbers[28];

  Normalise(packet.left_orientation);
  Normalise(packet.right_orientation);
  Normalise(packet.head_orientation);

  // The characters of T and F, from every word that holds only those. The
  // runtime sends the buttons and the two flags as two words, so they join
  // into one run and the flags keep the indices 19 and 20.
  packet.bool_count = 0;
  packet.bools[0] = '\0';
  packet.name[0] = '\0';
  while (*p != '\0') {
    while (IsSpace(*p)) ++p;
    if (*p == '\0') break;
    const char* word = p;
    while (*p != '\0' && !IsSpace(*p)) ++p;
    const int length = (int)(p - word);
    bool only_bools = (length > 0);
    for (int i = 0; i < length; ++i) {
      if (word[i] != 'T' && word[i] != 'F') {
        only_bools = false;
        break;
      }
    }
    if (!only_bools) {
      // The first word that is not a run of T and F is the name.
      int n = 0;
      while (n < length && n < (int)sizeof(packet.name) - 1) {
        packet.name[n] = word[n];
        ++n;
      }
      packet.name[n] = '\0';
      break;
    }
    for (int i = 0; i < length && packet.bool_count < kBoolsMost - 1; ++i) {
      packet.bools[packet.bool_count] = word[i];
      ++packet.bool_count;
    }
    packet.bools[packet.bool_count] = '\0';
  }

  *out = packet;
  return true;
}

// --------------------------------------------------------------- the frame

// The vertical angle that goes with a horizontal angle at this shape of
// render, in degrees. The game asks for 70.0 degrees across, and its vertical
// angle follows the aspect of the render:
//
//   tan(v / 2) = tan(h / 2) * height / width
//
// The mod sends the two angles that it drew with, and not a setting: the
// runtime maps the picture onto the display with them, so a wrong pair makes
// the world move at a rate that the head does not, and by a different rate up
// and down than left and right. In a headset that is drift.
//
// Side by side halves the width of an eye, so the height and the width here
// are the height and the width of one eye and not of the window.
inline float VerticalDegrees(float horizontal_degrees, int width, int height) {
  if (width <= 0 || height <= 0) return horizontal_degrees;
  const double pi = 3.14159265358979323846;
  const double half = (double)horizontal_degrees * 0.5 * pi / 180.0;
  const double v = atan(tan(half) * (double)height / (double)width);
  return (float)(2.0 * v * 180.0 / pi);
}

}  // namespace xrapi_packet
