// The output of the mod on WinlatorXR, through the XrApi.
//
// XrApi is not OpenXR. There is no loader, no instance, no session, no
// swapchain and no layer. There are two sockets of UDP on the loopback, a few
// files under Z:\tmp\xr, and one red square that the mod draws in the corner
// of the picture that the game already draws.
//
//   in   port 7872   the head, the hands, the buttons, the sticks, the
//                    interpupillary distance, the field of view, the frame id
//   out  port 7278   the haptics, the two modes, and the field of view that
//                    the mod drew with
//
// So this file does four things each frame, and no more:
//
//   1. drain the socket and keep the newest packet (BeginFrame),
//   2. give the head and the hands to the mod, in the shape that the rest of
//      the mod already reads (EyeView and HandInput),
//   3. hold the frame id for the sync pixel,
//   4. send the packet out (EndFrame).
//
// The picture does not pass through here at all. The runtime captures the
// window of the game, so every call about a swapchain, a texture, a layer or
// a Direct3D 11 device does nothing. Each one is marked. They go away with
// their callers as the mod learns to draw its cards into the frame of the
// game.
//
// See Docs/general_wxr/XRAPI_V0_4_DESC.md and
// Docs/ClaudeCode/PROJECT_PLAN.md.

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "xr_screen.h"
#include "xrapi_packet.h"

#pragma comment(lib, "ws2_32.lib")

namespace bwvr {
namespace {

const int kPortIn = 7872;
const int kPortOut = 7278;

// The mode of the outbound packet. 1 is VR. The mod draws one picture for
// both eyes, so the 3D mode is 0. The port is mono.
const int kModeVrOn = 1;
const int kMode3dMono = 0;

// The angle that the game draws with, across. The mod sends the pair that it
// drew with, because the runtime maps the picture onto the display with it.
const float kGameFovDegrees = 70.0f;

void QuatYaw(float angle, float q[4]) {
  q[0] = 0.0f;
  q[1] = (float)sin((double)angle * 0.5);
  q[2] = 0.0f;
  q[3] = (float)cos((double)angle * 0.5);
}

void QuatMul(const float a[4], const float b[4], float out[4]) {
  const float x = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
  const float y = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
  const float z = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
  const float w = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
  out[0] = x;
  out[1] = y;
  out[2] = z;
  out[3] = w;
}

void QuatRotate(const float q[4], const float v[3], float out[3]) {
  const float tx = 2.0f * (q[1] * v[2] - q[2] * v[1]);
  const float ty = 2.0f * (q[2] * v[0] - q[0] * v[2]);
  const float tz = 2.0f * (q[0] * v[1] - q[1] * v[0]);
  out[0] = v[0] + q[3] * tx + (q[1] * tz - q[2] * ty);
  out[1] = v[1] + q[3] * ty + (q[2] * tx - q[0] * tz);
  out[2] = v[2] + q[3] * tz + (q[0] * ty - q[1] * tx);
}

// The yaw of an orientation, about the up of the room.
float YawOf(const float q[4]) {
  const float ahead[3] = {0.0f, 0.0f, -1.0f};
  float out[3];
  QuatRotate(q, ahead, out);
  return (float)atan2(-(double)out[0], -(double)out[2]);
}

bool MakeDir(const char* path) {
  if (CreateDirectoryA(path, nullptr)) return true;
  return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

// Lines 1 and 2 of a small file of the runtime. Each value stays as it was
// when the file or the line is missing, so a headset that writes no `system`
// keeps the default of a Quest 3.
void ReadLineOneAndTwo(const char* path, char* first, int first_size,
                       char* second, int second_size) {
  HANDLE file =
      CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return;
  char text[512];
  DWORD got = 0;
  const BOOL read = ReadFile(file, text, (DWORD)sizeof(text) - 1, &got, nullptr);
  CloseHandle(file);
  if (!read || got == 0) return;
  text[got] = 0;

  char* line = text;
  for (int which = 0; which < 2 && *line != 0; ++which) {
    char* end = line;
    while (*end != 0 && *end != 13 && *end != 10) ++end;
    const char stop = *end;
    *end = 0;
    if (*line != 0) {
      if (which == 0) {
        lstrcpynA(first, line, first_size);
      } else {
        lstrcpynA(second, line, second_size);
      }
    }
    if (stop == 0) break;
    ++end;
    if (stop == 13 && *end == 10) ++end;
    line = end;
  }
}

void WriteOneLine(const char* path, const char* text) {
  HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return;
  DWORD written = 0;
  WriteFile(file, text, (DWORD)strlen(text), &written, nullptr);
  CloseHandle(file);
}

}  // namespace

// The state of the client. The members of the class itself stay as the rest
// of the mod reads them.
struct XrScreen::Impl {
  LogFn log = nullptr;
  SOCKET in_socket = INVALID_SOCKET;
  sockaddr_in out_address = {};
  bool winsock = false;

  char dir[MAX_PATH] = {0};
  char make[64] = "META";
  char model[64] = "QUEST 3";

  xrapi_packet::Packet packet = {};
  bool have_packet = false;   // a packet has arrived at some time
  bool from_system = false;   // the name came from the runtime's own file
  bool said_bad = false;      // a packet that did not parse is logged one time
  unsigned long packets = 0;

  // The first packet, as it arrived. The tries for a headset are quiet after
  // the first one, so the line is kept and goes out when the log is loud
  // again. One capture of a runtime is worth more than any reading of the
  // format.
  char first_line[1024] = {0};
  bool said_first = false;

  // The recentre: the inverse of the yaw of the head, and its place. Only the
  // yaw, because a recentre with a tilted head must not tilt the room.
  bool centred = false;
  float centre_inverse[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float centre_position[3] = {0.0f, 0.0f, 0.0f};

  // What the mod drew with, for the outbound packet.
  int eye_width = 0;
  int eye_height = 0;
  float haptic[2] = {0.0f, 0.0f};

  unsigned long long frames = 0;
  double last_log = 0.0;
};

// --------------------------------------------------------------- the frame

namespace {

// A pose of the packet into the room, by the recentre.
//
// The quaternion of the packet is used as it arrives. A port of this family
// negated its z to match a reading of the handedness, and that reversed the
// roll of the head: a lean to the left rolled the view to the right. Yaw and
// pitch did not show it, because a pure yaw or pitch quaternion has z of 0.
// So an axis is flipped after the rotation, never the quaternion.
//
// Each pose goes through this on its own, and the recentre is the only thing
// that a hand shares with the head. A hand that goes through the transform of
// the head swings when the head turns.
void ToRoom(const float centre_inverse[4], const float centre_position[3],
            const float orientation[4], const float position[3],
            float orientation_out[4], float position_out[3]) {
  QuatMul(centre_inverse, orientation, orientation_out);

  const float moved[3] = {position[0] - centre_position[0],
                          position[1] - centre_position[1],
                          position[2] - centre_position[2]};
  QuatRotate(centre_inverse, moved, position_out);
}

}  // namespace

XrScreen::XrScreen()
    : impl_(new Impl()),
      running_(false),
      should_render_(false),
      eyes_(1),
      presented_(0),
      skipped_(0),
      projection_(true),
      recommended_w_(0),
      recommended_h_(0),
      render_half_angle_(0.0f),
      have_render_pose_(false) {
  memset(views_, 0, sizeof(views_));
  memset(render_views_, 0, sizeof(render_views_));
  memset(hands_, 0, sizeof(hands_));
}

XrScreen::~XrScreen() {
  Stop();
  delete impl_;
  impl_ = nullptr;
}

void XrScreen::SetLog(LogFn log) {
  if (impl_ != nullptr) impl_->log = log;
}

// THE START, IN THREE STEPS.
//
// The mod starts the headset from a thread of its own, and it tries again
// until a headset answers: StartInstance, then StartSystem until it says
// ready, and then StartSession on the frame thread. The three names come
// from the OpenXR mod, and they fit the XrApi well enough:
//
//   StartInstance   the handshake files and the socket, one time
//   StartSystem     is a runtime there, and what does it call itself
//   StartSession    the render size, and from now on the frames go out
//
// The runtime empties its directory when it starts, so the files go in after
// it is up. An unknown value in `version` leaves the runtime with no
// implementation at all: it starts no listener and sends nothing, and there
// is no error. So that file is the first thing to doubt when no packet comes.
bool XrScreen::StartInstance(LogFn log) {
  if (impl_ == nullptr) return false;
  if (impl_->in_socket != INVALID_SOCKET) return true;
  impl_->log = log;

  WSADATA data;
  if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
    if (log != nullptr) log("xrapi        : winsock did not start");
    return false;
  }
  impl_->winsock = true;

  // Z: is the container. A machine with no Z: is a desktop, and then the
  // folder goes beside the game, where the XrAPI UDP Tester can find it.
  // Not the root of C:, because a standard account cannot always make a
  // folder there, and the folder of the game is one the mod can always write.
  lstrcpynA(impl_->dir, "Z:\\tmp\\xr", MAX_PATH);
  if (GetFileAttributesA("Z:\\") == INVALID_FILE_ATTRIBUTES) {
    char exe[MAX_PATH];
    const DWORD got = GetModuleFileNameA(nullptr, exe, MAX_PATH);
    if (got > 0 && got < MAX_PATH) {
      char* slash = nullptr;
      for (char* walk = exe; *walk != 0; ++walk) {
        if (*walk == '\\' || *walk == '/') slash = walk;
      }
      if (slash != nullptr) {
        *slash = 0;
        wsprintfA(impl_->dir, "%s\\xrtemp", exe);
      }
    }
  }
  MakeDir(impl_->dir);

  char path[MAX_PATH];
  wsprintfA(path, "%s\\version", impl_->dir);
  WriteOneLine(path, "0.5");
  wsprintfA(path, "%s\\vr", impl_->dir);
  WriteOneLine(path, "VR");

  // `system` is written by the runtime. Line 1 is the make and line 2 is a
  // codename of the model. It is also the test for "is this a headset".
  wsprintfA(path, "%s\\system", impl_->dir);
  SOCKET in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (in == INVALID_SOCKET) {
    if (log != nullptr) log("xrapi        : no socket (error %d)", WSAGetLastError());
    Stop();
    return false;
  }
  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(kPortIn);
  // The runtime sends the same packet to 7872 and to 7873, so one bind takes
  // everything. A bind of both would read each packet twice.
  if (bind(in, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
    if (log != nullptr) {
      log("xrapi        : port %d is busy (error %d). Another process has it",
          kPortIn, WSAGetLastError());
    }
    closesocket(in);
    Stop();
    return false;
  }
  u_long no_block = 1;
  ioctlsocket(in, FIONBIO, &no_block);
  impl_->in_socket = in;

  impl_->out_address.sin_family = AF_INET;
  impl_->out_address.sin_port = htons(kPortOut);
  impl_->out_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (log != nullptr) {
    log("xrapi        : 0.5, in on %d, out on %d, files in %s", kPortIn,
        kPortOut, impl_->dir);
  }
  return true;
}

// Is a runtime there. Two things say yes, and either is enough:
//
//   the file `system`, which only the runtime writes, so it is the test for
//   "this is a headset and not a desktop",
//   a packet, which is what the XrAPI UDP Tester gives on a desktop.
//
// Neither yet is "not now", so the tries keep the socket and ask again. A
// player can start the game before the runtime is up.
int XrScreen::StartSystem() {
  if (impl_ == nullptr || impl_->in_socket == INVALID_SOCKET) {
    return -1;  // kSystemFault
  }

  char path[MAX_PATH];
  wsprintfA(path, "%s\\system", impl_->dir);
  const bool have_system =
      GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
  if (have_system) {
    ReadLineOneAndTwo(path, impl_->make, sizeof(impl_->make), impl_->model,
                      sizeof(impl_->model));
    impl_->from_system = true;
  }

  // A packet answers the question too, and it drops nothing: BeginFrame reads
  // the newest of what is there.
  if (!have_system && !impl_->have_packet) {
    char line[1024];
    sockaddr_in from;
    int from_size = (int)sizeof(from);
    const int got = recvfrom(impl_->in_socket, line, (int)sizeof(line) - 1,
                             0, (sockaddr*)&from, &from_size);
    if (got > 0) {
      line[got] = 0;
      xrapi_packet::Packet packet;
      if (xrapi_packet::Parse(line, &packet)) {
        lstrcpynA(impl_->first_line, line, sizeof(impl_->first_line));
        SayFirstPacket();
        impl_->packet = packet;
        impl_->have_packet = true;
        ++impl_->packets;
      }
    }
  }

  if (!have_system && !impl_->have_packet) return 0;  // kSystemNotNow
  return 1;                                           // kSystemReady
}

// The first packet goes into the log one time, as soon as there is a log
// that keeps it.
void XrScreen::SayFirstPacket() {
  if (impl_ == nullptr || impl_->log == nullptr) return;
  if (impl_->said_first || impl_->first_line[0] == '\0') return;
  impl_->said_first = true;
  impl_->log("xrapi        : the first packet: %s", impl_->first_line);
}

void XrScreen::LogSystem() {
  if (impl_ == nullptr || impl_->log == nullptr) return;
  SayFirstPacket();
  impl_->log("xrapi        : the headset says %s %s%s", impl_->make,
             impl_->model,
             impl_->from_system ? "" : " (no system file, so the default)");
}

// From here the frames go out. The size is the size of the render, which the
// outbound field of view is worked out from.
bool XrScreen::StartSession(uint32_t width, uint32_t height, int eyes) {
  if (impl_ == nullptr || impl_->in_socket == INVALID_SOCKET) return false;
  eyes_ = (eyes >= 2) ? 2 : 1;
  recommended_w_ = width;
  recommended_h_ = height;
  impl_->eye_width = (int)width;
  impl_->eye_height = (int)height;
  running_ = true;
  if (impl_->log != nullptr) {
    impl_->log("xrapi        : the session runs, render %u x %u, %d eye%s",
               width, height, eyes_, eyes_ == 1 ? "" : "s");
  }
  return true;
}

// The three steps in one call, for a caller that wants no tries.
bool XrScreen::Start(uint32_t width, uint32_t height, int eyes, LogFn log) {
  if (running_) return true;
  if (!StartInstance(log)) return false;
  if (StartSystem() != 1) return false;
  LogSystem();
  return StartSession(width, height, eyes);
}

void XrScreen::Stop() {
  if (impl_ == nullptr) return;
  if (impl_->dir[0] != '\0') {
    char path[MAX_PATH];
    wsprintfA(path, "%s\\vr", impl_->dir);
    // A `vr` file that stays can convince the runtime that a process which
    // has ended still wants VR.
    DeleteFileA(path);
  }
  if (impl_->in_socket != INVALID_SOCKET) {
    closesocket(impl_->in_socket);
    impl_->in_socket = INVALID_SOCKET;
  }
  if (impl_->winsock) {
    WSACleanup();
    impl_->winsock = false;
  }
  running_ = false;
  should_render_ = false;
}

// Every packet that waits, and keep the newest. One packet for each frame
// would build a queue and the mod would act on a pose that gets older.
bool XrScreen::BeginFrame() {
  should_render_ = false;
  if (!running_ || impl_ == nullptr) return false;

  SayFirstPacket();

  char line[1024];
  for (int i = 0; i < 64; ++i) {
    sockaddr_in from;
    int from_size = (int)sizeof(from);
    const int got = recvfrom(impl_->in_socket, line, (int)sizeof(line) - 1, 0,
                             (sockaddr*)&from, &from_size);
    if (got <= 0) break;
    line[got] = 0;
    xrapi_packet::Packet packet;
    if (!xrapi_packet::Parse(line, &packet)) {
      // A packet that the mod cannot read is worth one line, one time. It
      // says that the socket works and that the format is not what the mod
      // expects, which are two very different faults.
      if (!impl_->said_bad && impl_->log != nullptr) {
        impl_->said_bad = true;
        impl_->log("xrapi        : a packet did not parse: %s", line);
      }
      continue;
    }
    if (impl_->first_line[0] == '\0') {
      lstrcpynA(impl_->first_line, line, sizeof(impl_->first_line));
    }
    impl_->packet = packet;
    impl_->have_packet = true;
    ++impl_->packets;
  }

  // A packet that does not come leaves the pose that was there. A zero would
  // be a violent jump of the view.
  if (!impl_->have_packet) return false;

  const xrapi_packet::Packet& packet = impl_->packet;

  if (!impl_->centred) {
    QuatYaw(-YawOf(packet.head_orientation), impl_->centre_inverse);
    impl_->centre_position[0] = packet.head_position[0];
    impl_->centre_position[1] = packet.head_position[1];
    impl_->centre_position[2] = packet.head_position[2];
    impl_->centred = true;
  }

  float orientation[4];
  float position[3];
  ToRoom(impl_->centre_inverse, impl_->centre_position,
         packet.head_orientation, packet.head_position, orientation,
         position);

  // One picture for both eyes, so both eyes get the pose of the head. The
  // angles are the ones the game draws with.
  const double to_radians = 3.14159265358979323846 / 180.0;
  const float half_x = (float)(kGameFovDegrees * 0.5 * to_radians);
  const float vertical = xrapi_packet::VerticalDegrees(
      kGameFovDegrees, impl_->eye_width, impl_->eye_height);
  const float half_y = (float)(vertical * 0.5 * to_radians);
  for (int eye = 0; eye < 2; ++eye) {
    EyeView& view = views_[eye];
    view.position[0] = position[0];
    view.position[1] = position[1];
    view.position[2] = position[2];
    view.orientation[0] = orientation[0];
    view.orientation[1] = orientation[1];
    view.orientation[2] = orientation[2];
    view.orientation[3] = orientation[3];
    view.fov_left = -half_x;
    view.fov_right = half_x;
    view.fov_up = half_y;
    view.fov_down = -half_y;
    view.valid = true;
  }

  // The hands. Each pose is turned into the room on its own, and only the
  // recentre is shared with the head. A hand that goes through the transform
  // of the head swings when the head turns.
  for (int hand = 0; hand < 2; ++hand) {
    const float* hand_orientation = (hand == 0) ? packet.left_orientation
                                                : packet.right_orientation;
    const float* hand_position =
        (hand == 0) ? packet.left_position : packet.right_position;
    float room_orientation[4];
    float room_position[3];
    ToRoom(impl_->centre_inverse, impl_->centre_position, hand_orientation,
           hand_position, room_orientation, room_position);

    HandInput& input = hands_[hand];
    input.active = true;
    input.aim_valid = true;
    input.grip_valid = true;
    input.aim_tracked = true;
    input.grip_tracked = true;
    for (int i = 0; i < 3; ++i) {
      input.aim_position[i] = room_position[i];
      input.grip_position[i] = room_position[i];
    }
    for (int i = 0; i < 4; ++i) {
      input.aim_orientation[i] = room_orientation[i];
      input.grip_orientation[i] = room_orientation[i];
    }
    // XrApi 0.5 has no analog trigger or squeeze: each is a character of T or
    // F. 0.6 adds the analog values.
    const int trigger = (hand == 0) ? xrapi_packet::kBoolLeftTrigger
                                    : xrapi_packet::kBoolRightTrigger;
    const int grip = (hand == 0) ? xrapi_packet::kBoolLeftGrip
                                 : xrapi_packet::kBoolRightGrip;
    const int stick_click = (hand == 0) ? xrapi_packet::kBoolLeftStickClick
                                        : xrapi_packet::kBoolRightStickClick;
    const int primary = (hand == 0) ? xrapi_packet::kBoolX
                                    : xrapi_packet::kBoolA;
    const int secondary = (hand == 0) ? xrapi_packet::kBoolY
                                      : xrapi_packet::kBoolB;
    input.trigger = Held(packet, trigger) ? 1.0f : 0.0f;
    input.squeeze = Held(packet, grip) ? 1.0f : 0.0f;
    input.stick[0] = (hand == 0) ? packet.left_stick[0] : packet.right_stick[0];
    input.stick[1] = (hand == 0) ? packet.left_stick[1] : packet.right_stick[1];
    input.stick_click = Held(packet, stick_click);
    input.primary = Held(packet, primary);
    input.secondary = Held(packet, secondary);
    // Only the left controller has a menu button on the wire.
    input.menu = (hand == 0) && Held(packet, xrapi_packet::kBoolLeftMenu);
    ++input.serial;
  }

  should_render_ = true;
  return true;
}

// The outbound packet: the haptics, the modes, and the two angles that the
// mod drew with.
//
// It goes out each frame, also when nothing changed, because the runtime
// reads the mode only from a packet that the mod sends. A mod that stops
// sending cannot turn the stream back on.
void XrScreen::EndFrame() {
  if (!running_ || impl_ == nullptr) return;

  const float vertical = xrapi_packet::VerticalDegrees(
      kGameFovDegrees, impl_->eye_width, impl_->eye_height);

  char line[160];
  const int length =
      _snprintf_s(line, sizeof(line), _TRUNCATE, "%.2f %.2f %d %d %.2f %.2f",
                  (double)impl_->haptic[0], (double)impl_->haptic[1],
                  kModeVrOn, kMode3dMono, (double)kGameFovDegrees,
                  (double)vertical);
  if (length > 0) {
    line[length] = '\0';
    sendto(impl_->in_socket, line, length, 0, (sockaddr*)&impl_->out_address,
           (int)sizeof(impl_->out_address));
  }

  // The haptics are a count of frames and not a level, and a zero does not
  // clear them: the runtime keeps a value above zero only. So the number that
  // the mod sends comes down by itself.
  for (int hand = 0; hand < 2; ++hand) {
    if (impl_->haptic[hand] > 0.0f) impl_->haptic[hand] -= 1.0f;
    if (impl_->haptic[hand] < 0.0f) impl_->haptic[hand] = 0.0f;
  }

  ++presented_;
  ++impl_->frames;
}

// The red of the sync pixel for this frame: the frame id of the last packet,
// echoed with no change. The runtime learns the colour space from the values
// that it sees, so the mod must not scale or correct this number.
float XrScreen::SyncValue() const {
  if (impl_ == nullptr || !impl_->have_packet) return 0.0f;
  return impl_->packet.sync;
}

void XrScreen::SetRenderSize(int width, int height) {
  if (impl_ == nullptr) return;
  impl_->eye_width = width;
  impl_->eye_height = height;
}

void XrScreen::Haptic(int index, float amplitude, float seconds) {
  if (impl_ == nullptr) return;
  const int hand = (index != 0) ? 1 : 0;
  // A duration in frames. 72 frames is about a second on a Quest 3.
  float frames = seconds * 72.0f * (amplitude > 0.0f ? 1.0f : 0.0f);
  if (frames < 1.0f && amplitude > 0.0f) frames = 1.0f;
  if (frames > impl_->haptic[hand]) impl_->haptic[hand] = frames;
}

const char* XrScreen::HandProfile(int index) const {
  (void)index;
  if (impl_ == nullptr) return "";
  return impl_->model;
}

bool XrScreen::InputFocused() const {
  return running_ && impl_ != nullptr && impl_->have_packet;
}

uint32_t XrScreen::width() const { return recommended_w_; }
uint32_t XrScreen::height() const { return recommended_h_; }

bool XrScreen::RecommendedSize(uint32_t* width, uint32_t* height) const {
  if (width != nullptr) *width = recommended_w_;
  if (height != nullptr) *height = recommended_h_;
  return recommended_w_ != 0 && recommended_h_ != 0;
}

void XrScreen::SetRenderPose(int index, const float position[3],
                             const float orientation[4]) {
  const int eye = (index != 0) ? 1 : 0;
  for (int i = 0; i < 3; ++i) render_views_[eye].position[i] = position[i];
  for (int i = 0; i < 4; ++i) render_views_[eye].orientation[i] = orientation[i];
  render_views_[eye].valid = true;
  have_render_pose_ = true;
}

void XrScreen::SetPlacement(float distance_meters, float width_meters) {
  (void)distance_meters;
  (void)width_meters;
}

// ------------------------------------------------- the calls with no work
//
// The picture reaches the headset because the runtime captures the window of
// the game. Nothing here carries a picture, so each of these does nothing.
// They stay only until their callers are changed, and then they go.

void XrScreen::ConfigureBridge(bool enabled) { (void)enabled; }
bool XrScreen::has_instance() const {
  return impl_ != nullptr && impl_->in_socket != INVALID_SOCKET;
}
void XrScreen::UseVulkan(bool on) { (void)on; }
bool XrScreen::vulkan() const { return false; }
void XrScreen::UseD3D11(bool on) { (void)on; }
bool XrScreen::VulkanFailed() const { return false; }
bool XrScreen::VulkanInfo(XrVulkan* out) const { (void)out; return false; }
void XrScreen::WantEyeGaze(bool on) { (void)on; }
bool XrScreen::EyeGazeSupported() const { return false; }
bool XrScreen::EyeGaze(float direction[3]) const { (void)direction; return false; }
void XrScreen::LockQueue() {}
void XrScreen::UnlockQueue() {}
void XrScreen::SetEyeImage(int index, const void* pixels, int pitch) {
  (void)index;
  (void)pixels;
  (void)pitch;
}
void XrScreen::SetEyeTexture(int index, void* texture) {
  (void)index;
  (void)texture;
}
void XrScreen::SetEyeVulkan(int index, uint64_t image, uint32_t layer) {
  (void)index;
  (void)image;
  (void)layer;
}
void XrScreen::SetVulkanUpload(bool on) { (void)on; }
bool XrScreen::vulkan_upload() const { return false; }
void XrScreen::SetHoldMs(int ms) { (void)ms; }
int XrScreen::HoldMs() const { return 0; }
void XrScreen::SetRefreshRate(int hz) { (void)hz; }
double XrScreen::DisplaySeconds() const { return 0.0; }
void* XrScreen::D3D11Device() const { return nullptr; }
uint32_t XrScreen::SwapchainFormat() const { return 0; }
void XrScreen::EnablePanel(bool on) { (void)on; }
bool XrScreen::panel_ready() const { return false; }
void XrScreen::SetPanelTexture(void* texture) { (void)texture; }
void XrScreen::SetPanelVulkan(uint64_t image) { (void)image; }
void XrScreen::SetPanelQuads(const PanelQuad quads[2]) { (void)quads; }
void XrScreen::StagePanelQuad(const PanelQuad& quad) { (void)quad; }
void XrScreen::StageCard(int card, const CardLayer& layer) {
  (void)card;
  (void)layer;
}
bool XrScreen::CardFailed(int card) { (void)card; return false; }
void XrScreen::StageWindow(const WindowLayer& layer) { (void)layer; }
void XrScreen::EndFrameLate(const LateFrame& frame, bool on_thread) {
  (void)frame;
  (void)on_thread;
  EndFrame();
}
void XrScreen::Join() {}
bool XrScreen::sending() const { return false; }
uint64_t XrScreen::LateWaitTicks() const { return 0; }
bool XrScreen::LastDisplayTime(int64_t* time, int64_t* period) const {
  (void)time;
  (void)period;
  return false;
}
bool XrScreen::SubmitBgra(const void* pixels, int pitch) {
  (void)pixels;
  (void)pitch;
  return false;
}
bool XrScreen::SubmitVulkan(uint64_t image, uint32_t layer) {
  (void)image;
  (void)layer;
  return false;
}
void XrScreen::CaptureSwapchainNext(uint64_t name) { (void)name; }
bool XrScreen::SubmitTexture(void* texture) { (void)texture; return false; }
void XrScreen::ReportUpload(uint64_t frames) { (void)frames; }

}  // namespace bwvr
