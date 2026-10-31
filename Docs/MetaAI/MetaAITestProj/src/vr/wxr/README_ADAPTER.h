#pragma once
// Shim that adapts BW1-VR's existing VR backend interface to WinXrApi
// Include this in your main VR factory:
//
// #ifdef BWVR_WINLATORXR
// #include "wxr/WinXrApi.h"
// #endif
//
// IVR* CreateVRBackend() {
// #ifdef BWVR_WINLATORXR
//   if (GetFileAttributesA("Z:/tmp/xr/system") != INVALID_FILE_ATTRIBUTES ||
//       GetFileAttributesA("D:/xrtemp/system") != INVALID_FILE_ATTRIBUTES ||
//       GetFileAttributesA("Z:/tmp/xr") != INVALID_FILE_ATTRIBUTES) {
//       auto* wxr = new WinXrApi();
//       if (wxr->Init()) return wxr; // if Init fails, fallback
//   }
// #endif
//   return new OpenXRBackend(); // original
// }
//
// And in your render loop:
//
// void BWVR_RenderFrame(IDirect3DDevice* dev) {
//   // 1. Update tracking (blocks max 20ms)
//   g_wxr->Update(20);
//   // 2. Get poses and render left eye
//   SetViewport(0,0,width/2,height); RenderEye(Left, g_wxr->GetLeftPos() etc);
//   // 3. Render right eye
//   SetViewport(width/2,0,width/2,height); RenderEye(Right, ...);
//   // 4. Red pixel for WinlatorXR compositor sync (critical!)
//   g_wxr->DrawRedPixelMarker(dev);
//   // 5. Present - WinlatorXR scrapes top-left 10x10 pixel
//   dev->Present();
//   // 6. Send haptics/FOV
//   g_wxr->SendHaptic(leftVib, rightVib);
// }
//
// For Black & White specifically:
// - leftGrip = move land, rightGrip = grab
// - leftStick = walk, rightStick = zoom/turn
// - A = teleport, B = miracle gesture, Y = ring menu
// Map in your InputHandler:
//
// if (wxr->IsRightGrip()) HandClose();
// if (wxr->IsLeftGrip()) LandGrab();
// if (wxr->IsA()) StartTeleport();
// etc.
