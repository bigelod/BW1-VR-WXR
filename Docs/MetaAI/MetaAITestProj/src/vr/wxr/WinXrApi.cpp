#include "WinXrApi.h"
#include <filesystem>
#include <fstream>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

namespace fs = std::filesystem;

WinXrApi::WinXrApi() {}
WinXrApi::~WinXrApi() { Shutdown(); }

bool WinXrApi::EnsureXrPath(std::string& outBase) {
    // Check Z: first (WinlatorXR container), then D: for desktop test
    if (GetFileAttributesA("Z:/") != INVALID_FILE_ATTRIBUTES) {
        outBase = "Z:/tmp/xr";
        return true;
    }
    if (GetFileAttributesA("D:/") != INVALID_FILE_ATTRIBUTES) {
        // Check if D:/xrtemp exists or create
        outBase = "D:/xrtemp";
        return true;
    }
    // Fallback to temp
    outBase = "C:/tmp/xr";
    return false;
}

bool WinXrApi::Init() {
    std::string base;
    EnsureXrPath(base);
    m_xrBasePath = base;

    try {
        fs::create_directories(base);
    } catch (...) {}

    // Write marker files: vr contains "VR", version contains "0.5"
    {
        std::ofstream f(base + "/vr");
        f << "VR";
    }
    {
        std::ofstream f(base + "/version");
        f << "0.5";
    }

    // Load FOV calibration
    LoadFovFile((base + "/../VR/fov.txt").c_str());
    LoadFovFile("VR/fov.txt");

    if (!m_udp.Start()) return false;

    // Wait for first packet up to 15s (like COD4 WXR)
    WinXrPacket pkt;
    if (!m_udp.GetLatest(pkt, 15000)) {
        // No packet, but still consider init ok for desktop test - will stall on first Update
        return true;
    }
    m_lastPacket = pkt;
    return true;
}

void WinXrApi::Shutdown() {
    m_udp.Stop();
    if (!m_xrBasePath.empty()) {
        try {
            fs::remove(m_xrBasePath + "/vr");
            // Don't delete version, leave for debugging
        } catch (...) {}
    }
}

void WinXrApi::LoadFovFile(const char* path) {
    std::ifstream f(path);
    if (!f) return;
    // File contains 6 lines: A B C D E F
    float vals[6];
    for (int i = 0; i < 6; ++i) {
        if (!(f >> vals[i])) return;
    }
    m_fovVarA = vals[0]; m_fovVarB = vals[1]; m_fovVarC = vals[2]; m_fovVarD = vals[3];
    m_fovVarE = vals[4]; m_fovVarF = vals[5];
    m_targetFovW = m_fovVarE;
    m_targetFovH = m_fovVarF;
}

bool WinXrApi::Update(int timeoutMs) {
    WinXrPacket pkt;
    // Blocking wait for new pose, up to timeoutMs, like WaitGetPoses
    if (!m_udp.GetLatest(pkt, timeoutMs)) {
        // If no new data, keep last pose but return false to indicate stall
        // For frame pacing, caller should skip input update if frameId repeats
        return false;
    }

    m_lastPacket = pkt;
    m_frameId = pkt.frameId;

    // Parse poses - XrAPI is OpenXR convention (+Y up, -Z forward, meters)
    // HMD
    m_hmdPos = XMFLOAT3(pkt.f[22], pkt.f[23], pkt.f[24]);
    m_hmdQuat = XMFLOAT4(pkt.f[18], pkt.f[19], pkt.f[20], pkt.f[21]);

    // Left hand
    m_lPos = XMFLOAT3(pkt.f[6], pkt.f[7], pkt.f[8]);
    m_lQuat = XMFLOAT4(pkt.f[0], pkt.f[1], pkt.f[2], pkt.f[3]);
    m_lStick = XMFLOAT2(pkt.f[4], pkt.f[5]);

    // Right hand
    m_rPos = XMFLOAT3(pkt.f[15], pkt.f[16], pkt.f[17]);
    m_rQuat = XMFLOAT4(pkt.f[9], pkt.f[10], pkt.f[11], pkt.f[12]);
    m_rStick = XMFLOAT2(pkt.f[13], pkt.f[14]);

    m_buttons = pkt.buttons;
    m_isImmersive = pkt.buttons[WXR_IS_IMMERSIVE];

    // Apply 180deg roll fixup for controllers (XrAPI vs OpenXR aim)
    ApplyControllerFixup();

    // FOV negotiation: FOVH = A*inH + B, etc. (from HaloCEWXR)
    float inFovH = pkt.fovH;
    float inFovV = pkt.fovV;
    float outFovH = m_fovVarA * inFovH + m_fovVarB;
    float outFovV = m_fovVarC * inFovV + m_fovVarD;
    // Clamp
    if (outFovH < 60) outFovH = inFovH;
    if (outFovV < 60) outFovV = inFovV;
    m_targetFovW = outFovH;
    m_targetFovH = outFovV;

    return true;
}

void WinXrApi::ApplyControllerFixup() {
    // XrAPI controller rotation is rolled 180 deg vs OpenXR aim pose
    // Create quat that rotates 180 around forward axis? For simplicity, flip Y/Z per HWXR reference
    // HWXR does: extra 180 deg flip on Quest 2/Pro and Pico auto-detect
    // Here we apply 180 deg around Z for grip

    auto FixQuat = [](XMFLOAT4& q) {
        // Rotate 180 around X to convert from XrAPI grip to aim? 
        // Minimal: invert x,y as per HaloCEWXR controller quirks
        // This is placeholder - tune with actual headset
        XMVECTOR quat = XMLoadFloat4(&q);
        XMVECTOR flip = XMQuaternionRotationRollPitchYaw(0, 0, XM_PI); // 180 roll
        quat = XMQuaternionMultiply(quat, flip);
        XMStoreFloat4(&q, quat);
    };

    // Only apply if headset is Quest-like, for now always apply like COD4 does
    FixQuat(m_lQuat);
    FixQuat(m_rQuat);
}

void WinXrApi::SendHaptic(float left, float right) {
    // MODE_VR: 1 = VR, 2 = Non-VR. MODE_3D: 0 mono, 1 SBS, 2 AER
    int modeVR = m_isImmersive ? 1 : 2;
    int mode3D = 1; // SBS for BW1 - we render side-by-side
    m_udp.SendHapticFov(left, right, modeVR, mode3D, m_targetFovW, m_targetFovH);
}

void WinXrApi::MakeFullscreenBorderless(HWND hwnd) {
    if (!hwnd) return;
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    style &= ~(WS_THICKFRAME | WS_BORDER | WS_CAPTION);
    SetWindowLongA(hwnd, GWL_STYLE, style);
    // Cover entire screen
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, sw, sh, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void WinXrApi::DrawRedPixelMarker(void* d3dDevice) {
    // This is D3D9 example. For B&W's D3D7/DDraw, you need equivalent: draw 10x10 quad at 0,0
    // Color = ARGB(255, frameId % 256, 0, eye*255)
    // WinlatorXR scrapes this pixel to sync

    // Alternate eye each Present call? Or set eye based on which half you just rendered.
    // For SBS, we need to tell WinlatorXR which eye pass we are presenting.
    // Simple: toggle each frame, or use m_eyeForPixel set by caller before Present.

    // Example D3D9 path:
    struct IDirect3DDevice9* dev = (IDirect3DDevice9*)d3dDevice;
    if (!dev) return;

    // Save state
    DWORD oldZ, oldLighting, oldCull;
    dev->GetRenderState(D3DRS_ZENABLE, &oldZ);
    dev->GetRenderState(D3DRS_LIGHTING, &oldLighting);
    dev->GetRenderState(D3DRS_CULLMODE, &oldCull);

    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // 10x10 quad at top-left
    struct Vertex { float x,y,z,rhw; DWORD color; };
    BYTE frameByte = (BYTE)(m_frameId & 0xFF);
    BYTE eyeByte = (BYTE)(m_eyeForPixel ? 255 : 0);
    DWORD col = D3DCOLOR_ARGB(255, frameByte, 0, eyeByte);

    Vertex verts[6] = {
        {0,0,0,1, col}, {10,0,0,1, col}, {0,10,0,1, col},
        {0,10,0,1, col}, {10,0,0,1, col}, {10,10,0,1, col}
    };

    // Use fixed function
    dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, verts, sizeof(Vertex));

    dev->SetRenderState(D3DRS_ZENABLE, oldZ);
    dev->SetRenderState(D3DRS_LIGHTING, oldLighting);
    dev->SetRenderState(D3DRS_CULLMODE, oldCull);

    // Toggle eye for next frame (for AER signal)
    m_eyeForPixel = 1 - m_eyeForPixel;
}
