#pragma once
#include "WinXrApiUDP.h"
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

// Button indices (matches HaloCEWXR / COD4 WXR spec)
enum WXR_Button {
    WXR_L_GRIP = 0,
    WXR_L_MENU = 1,
    WXR_L_THUMB_PRESS = 2,
    WXR_L_THUMB_LEFT = 3,
    WXR_L_THUMB_RIGHT = 4,
    WXR_L_THUMB_UP = 5,
    WXR_L_THUMB_DOWN = 6,
    WXR_L_TRIGGER = 7,
    WXR_L_X = 8,
    WXR_L_Y = 9,
    WXR_R_A = 10,
    WXR_R_B = 11,
    WXR_R_GRIP = 12,
    WXR_R_THUMB_PRESS = 13,
    WXR_R_THUMB_LEFT = 14,
    WXR_R_THUMB_RIGHT = 15,
    WXR_R_THUMB_UP = 16,
    WXR_R_THUMB_DOWN = 17,
    WXR_R_TRIGGER = 18,
    WXR_IS_IMMERSIVE = 19,
    WXR_IS_SBS = 20,
};

// Minimal interface that mirrors BW1-VR's VR backend.
// If BW1-VR already has an IVR interface, make this inherit from it.
// For now, this is standalone and you can adapt it.

class WinXrApi {
public:
    WinXrApi();
    ~WinXrApi();

    // Lifecycle
    bool Init(); // writes marker files, starts UDP
    void Shutdown(); // deletes marker, stops UDP

    // Called each game frame, blocks up to 20ms for new pose (frame pacing like SteamVR WaitGetPoses)
    bool Update(int timeoutMs = 20);

    // Accessors for game
    XMFLOAT3 GetHMDPos() const { return m_hmdPos; }
    XMFLOAT4 GetHMDQuat() const { return m_hmdQuat; }
    XMFLOAT3 GetLeftPos() const { return m_lPos; }
    XMFLOAT4 GetLeftQuat() const { return m_lQuat; }
    XMFLOAT3 GetRightPos() const { return m_rPos; }
    XMFLOAT4 GetRightQuat() const { return m_rQuat; }
    XMFLOAT2 GetLeftStick() const { return m_lStick; }
    XMFLOAT2 GetRightStick() const { return m_rStick; }
    bool GetButton(int idx) const { return m_buttons[idx]; }

    // Convenience for BW1-VR hand logic
    bool IsLeftGrip() const { return GetButton(WXR_L_GRIP); }
    bool IsRightGrip() const { return GetButton(WXR_R_GRIP); }
    bool IsLeftTrigger() const { return GetButton(WXR_L_TRIGGER); }
    bool IsRightTrigger() const { return GetButton(WXR_R_TRIGGER); }
    bool IsA() const { return GetButton(WXR_R_A); }
    bool IsB() const { return GetButton(WXR_R_B); }
    bool IsX() const { return GetButton(WXR_L_X); }
    bool IsY() const { return GetButton(WXR_L_Y); }
    bool IsMenu() const { return GetButton(WXR_L_MENU); }

    // Haptics + FOV out
    void SendHaptic(float left, float right);
    void SetTargetFov(float fovW, float fovH) { m_targetFovW = fovW; m_targetFovH = fovH; }

    int GetFrameId() const { return m_frameId; }
    int GetEyeForRedPixel() const { return m_eyeForPixel; } // 0 = left, 1 = right for AER detection

    // FOV calibration from VR/fov.txt (like HaloCEWXR)
    void LoadFovFile(const char* path = "VR/fov.txt");

    // Window handling - call after D3D device created
    void MakeFullscreenBorderless(HWND hwnd);

    // Red pixel drawing - call after you finished rendering both eyes but before Present
    // Requires IDirect3DDevice7/8/9 or your current device. This version uses D3D9 for example.
    // For BW1's DDraw/D3D7, adapt to draw 10x10 quad at (0,0)
    void DrawRedPixelMarker(void* d3dDevice); // pass IDirect3DDevice9*

private:
    bool EnsureXrPath(std::string& outBase); // returns Z:/tmp/xr or D:/xrtemp
    void ApplyControllerFixup();

    WinXrApiUDP m_udp;
    WinXrPacket m_lastPacket{};

    XMFLOAT3 m_hmdPos{0,0,0};
    XMFLOAT4 m_hmdQuat{0,0,0,1};
    XMFLOAT3 m_lPos{0,0,0};
    XMFLOAT4 m_lQuat{0,0,0,1};
    XMFLOAT3 m_rPos{0,0,0};
    XMFLOAT4 m_rQuat{0,0,0,1};
    XMFLOAT2 m_lStick{0,0};
    XMFLOAT2 m_rStick{0,0};
    std::array<bool,21> m_buttons{};

    int m_frameId = 0;
    int m_eyeForPixel = 0;

    // FOV mapping vars like Halo: FOVH = A*inH + B etc.
    float m_fovVarA = 1.0f, m_fovVarB = 0.0f, m_fovVarC = 1.0f, m_fovVarD = 0.0f;
    float m_fovVarE = 104.5f, m_fovVarF = 104.5f;
    float m_targetFovW = 104.5f, m_targetFovH = 104.5f;

    std::string m_xrBasePath;
    bool m_isImmersive = true;
};
