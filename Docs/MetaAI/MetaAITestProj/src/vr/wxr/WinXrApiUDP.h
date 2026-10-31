#pragma once
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <array>

#pragma comment(lib, "ws2_32.lib")

// Wire format: 28 floats + int frameID + button string + optional hmd string
// L quat 0-3, L stick 4-5, L pos 6-8
// R quat 9-12, R stick 13-14, R pos 15-17
// HMD quat 18-21, HMD pos 22-24, IPD 25, FOV_H 26, FOV_V 27
// FrameID int, then 21x T/F chars

struct WinXrPacket {
    std::array<float, 28> f{};
    int frameId = 0;
    std::array<bool, 21> buttons{}; // L_GRIP, L_MENU, L_THUMB_PRESS, L_THUMB_L,R,U,D, L_TRIG, L_X, L_Y, R_A, R_B, R_GRIP, R_THUMB_PRESS, R_THUMB_L,R,U,D, R_TRIG, IS_IMMERSIVE, IS_SBS
    bool hasData = false;
    float ipd = 0.063f;
    float fovH = 90.f;
    float fovV = 90.f;
};

class WinXrApiUDP {
public:
    WinXrApiUDP();
    ~WinXrApiUDP();

    bool Start(); // binds 7872
    void Stop();

    // Blocking get latest, waits up to timeoutMs for new data (like WaitGetPoses)
    bool GetLatest(WinXrPacket& out, int timeoutMs = 20);
    bool TryGetLatest(WinXrPacket& out);

    // Send haptics + FOV to 127.0.0.1:7278
    void SendHapticFov(float leftVib, float rightVib, int modeVR, int mode3D, float targetFovW, float targetFovH);

    // For frame pacing detection
    int GetLastFrameId() const { return m_lastFrameId.load(); }

private:
    void RecvLoop();
    bool ParsePacket(const char* buf, int len, WinXrPacket& pkt);

    SOCKET m_recvSock = INVALID_SOCKET;
    SOCKET m_sendSock = INVALID_SOCKET;
    sockaddr_in m_sendAddr{};

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_lastFrameId{0};

    std::mutex m_mutex;
    std::condition_variable m_cv;
    WinXrPacket m_latest{};
    bool m_newData = false;

    std::atomic<bool> m_wsaInit{false};
};
