# The VR output layer, on the XrApi of WinlatorXR.
#
# XrApi is not OpenXR. There is no loader to link and no DLL to ship beside
# the game: the mod speaks two sockets of UDP and writes a few files. So this
# library takes no OpenXR SDK, and the game needs no openxr_loader.dll.
#
# The OpenXR session of the PCVR mod is xr_screen.cpp, and the x64 host for
# Steam Frame is bridge/. Neither is built here. See
# Docs/ClaudeCode/PROJECT_PLAN.md, "The subtraction list".
add_library(bwvr_vr STATIC
    xr_screen_wxr.cpp
)

target_include_directories(bwvr_vr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
  target_compile_options(bwvr_vr PRIVATE /W4 /permissive-)
endif()
