// The time that this build was built.
//
// It is on the first line of bwvr.log, with the build id. The id says which
// commit the build is from, and the stamp says when the files on the device
// were built. Two builds of the same commit have the same id and a different
// stamp, so a report from a test says which of them ran.
//
// A deploy script reads the stamp from the built DLL and refuses a binary
// that is older than the newest source file. See bwvr/tools/wxr/deploy.ps1.
#ifndef BWVR_BUILD_STAMP_H
#define BWVR_BUILD_STAMP_H

namespace bw {

// "YYYY-MM-DD HH:MM:SS", local time of the machine that built the mod.
extern const char kBuildStamp[];

}  // namespace bw

#endif  // BWVR_BUILD_STAMP_H
