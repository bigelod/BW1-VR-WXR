# Writes build_stamp.cpp with the time of this build.
#
# The build runs this script again for each build, so the stamp is the time
# that the mod was built and not the time that CMake configured. The file is a
# source file and not a header, because MSBuild does not track a header in a
# build folder under a temp path.
#
#   cmake -DBWVR_STAMP_OUT=<path> -P build_stamp.cmake

if(NOT BWVR_STAMP_OUT)
  message(FATAL_ERROR "Give the path: -DBWVR_STAMP_OUT=<file>")
endif()

string(TIMESTAMP BWVR_STAMP_NOW "%Y-%m-%d %H:%M:%S")

file(WRITE "${BWVR_STAMP_OUT}"
"// CMake writes this file for each build. See bwvr/cmake/build_stamp.cmake.
namespace bw {
extern const char kBuildStamp[];
const char kBuildStamp[] = \"${BWVR_STAMP_NOW}\";
}  // namespace bw
")
