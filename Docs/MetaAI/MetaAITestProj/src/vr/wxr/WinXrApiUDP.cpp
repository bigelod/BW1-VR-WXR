#include "WinXrApiUDP.h"
#include <sstream>
#include <locale>
#include <algorithm>

WinXrApiUDP::WinXrApiUDP() {}
WinXrApiUDP::~WinXrApiUDP() { Stop(); }

bool WinXrApiUDP::Start() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return false;
    m_wsaInit = true;

    m_recvSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_recvSock == INVALID_SOCKET) return false;

    // Allow reuse
    BOOL bReuse = TRUE;
    setsockopt(m_recvSock, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuse, sizeof(bReuse));
    // Non-blocking timeout via select, we use blocking recv with 100ms timeout
    DWORD timeout = 100;
    setsockopt(m_recvSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(7872);
    if (bind(m_recvSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        return false;
    }

    m_sendSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sendSock == INVALID_SOCKET) return false;
    m_sendAddr.sin_family = AF_INET;
    m_sendAddr.sin_port = htons(7278);
    inet_pton(AF_INET, "127.0.0.1", &m_sendAddr.sin_addr);

    m_running = true;
    m_thread = std::thread(&WinXrApiUDP::RecvLoop, this);
    return true;
}

void WinXrApiUDP::Stop() {
    m_running = false;
    if (m_thread.joinable()) {
        // Wake up recv by sending dummy packet to self
        SOCKET tmp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (tmp != INVALID_SOCKET) {
            sockaddr_in loop{};
            loop.sin_family = AF_INET;
            loop.sin_port = htons(7872);
            inet_pton(AF_INET, "127.0.0.1", &loop.sin_addr);
            const char* dummy = "STOP";
            sendto(tmp, dummy, 4, 0, (sockaddr*)&loop, sizeof(loop));
            closesocket(tmp);
        }
        m_thread.join();
    }
    if (m_recvSock != INVALID_SOCKET) { closesocket(m_recvSock); m_recvSock = INVALID_SOCKET; }
    if (m_sendSock != INVALID_SOCKET) { closesocket(m_sendSock); m_sendSock = INVALID_SOCKET; }
    if (m_wsaInit) { WSACleanup(); m_wsaInit = false; }
}

void WinXrApiUDP::RecvLoop() {
    char buf[2048];
    while (m_running) {
        sockaddr_in from{};
        int fromLen = sizeof(from);
        int len = recvfrom(m_recvSock, buf, sizeof(buf)-1, 0, (sockaddr*)&from, &fromLen);
        if (len <= 0) continue;
        buf[len] = '\0';

        WinXrPacket pkt;
        if (ParsePacket(buf, len, pkt)) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_latest = pkt;
                m_newData = true;
                m_lastFrameId = pkt.frameId;
            }
            m_cv.notify_all();
        }
    }
}

bool WinXrApiUDP::ParsePacket(const char* buf, int len, WinXrPacket& pkt) {
    // Use C locale for '.' decimal parsing (mandatory)
    std::istringstream iss(std::string(buf, len));
    iss.imbue(std::locale::classic());

    // Packet may start with "client0" or similar, skip non-numeric tokens at start
    // HaloCEWXR example: floats start directly, but some impls prefix with client ID.
    // We'll attempt to parse 28 floats wherever they appear.

    // First try simple: read 28 floats sequentially, ignoring leading token if needed
    // Approach: extract all floats from stream by trying to read floats
    // But spec says exact order: L quat 4, L stick 2, L pos 3, R quat 4, R stick 2, R pos 3, HMD quat 4, HMD pos 3, IPD, FOV_H, FOV_V

    // Reset to start, try to find first float
    std::string s(buf, len);
    // Replace any non float chars except ' ' '-' '.' 'e' 'E' digits?
    // Simpler: use iss >> float extraction

    // Skip initial word if it contains no digit
    std::string firstToken;
    std::istringstream tmp(s);
    tmp.imbue(std::locale::classic());
    tmp >> firstToken;
    if (firstToken.find_first_of("0123456789-.") == std::string::npos) {
        // Skip it, create new stream without first token
        // Find position after first token
        size_t pos = s.find(firstToken);
        if (pos != std::string::npos) {
            s = s.substr(pos + firstToken.length());
            iss.str(s);
            iss.clear();
        }
    }

    for (int i = 0; i < 28; ++i) {
        if (!(iss >> pkt.f[i])) return false;
    }
    int frameId = 0;
    if (!(iss >> frameId)) return false;
    pkt.frameId = frameId;

    std::string btnStr;
    if (!(iss >> btnStr)) {
        // No buttons? Still valid
        btnStr = "";
    }
    // Button string is 21 chars of T/F
    for (int i = 0; i < 21 && i < (int)btnStr.size(); ++i) {
        pkt.buttons[i] = (btnStr[i] == 'T' || btnStr[i] == 't' || btnStr[i] == '1');
    }

    pkt.ipd = pkt.f[25];
    pkt.fovH = pkt.f[26];
    pkt.fovV = pkt.f[27];
    pkt.hasData = true;
    return true;
}

bool WinXrApiUDP::GetLatest(WinXrPacket& out, int timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_newData) {
        m_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]{ return m_newData || !m_running; });
    }
    if (!m_newData) return false;
    out = m_latest;
    m_newData = false; // consume
    return true;
}

bool WinXrApiUDP::TryGetLatest(WinXrPacket& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_latest.hasData) return false;
    out = m_latest;
    return true;
}

void WinXrApiUDP::SendHapticFov(float leftVib, float rightVib, int modeVR, int mode3D, float targetFovW, float targetFovH) {
    if (m_sendSock == INVALID_SOCKET) return;
    // Format: "<left> <right> <MODE_VR> <MODE_3D> <fovW> <fovH>"
    char buf[128];
    // Use C locale
    int n = snprintf(buf, sizeof(buf), "%.4f %.4f %d %d %.3f %.3f", leftVib, rightVib, modeVR, mode3D, targetFovW, targetFovH);
    if (n > 0) {
        sendto(m_sendSock, buf, n, 0, (sockaddr*)&m_sendAddr, sizeof(m_sendAddr));
    }
}
