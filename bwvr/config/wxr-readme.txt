Black & White VR Mod, the WinlatorXR build
==========================================

WHAT THIS BUILD IS FOR

This build does not give you VR yet. It is the test of stage 2 of the port:
does the mod load inside a WinlatorXR container, do its hooks hold under
Box86 and Wine, and does a packet of the XrApi reach it.

The mod needs no openxr_loader.dll and no other file beside it. There is no
OpenXR in this build at all. It speaks the XrApi: it writes Z:\tmp\xr\version
and Z:\tmp\xr\vr, it reads Z:\tmp\xr\system for the name of the headset, it
listens on port 7872 and it sends on port 7278.

What is not built yet: the red sync pixel, the camera of the game on the
head, the hands, and the cards of the mod in the picture. So expect the game
to run flat.


HOW TO INSTALL

1. Copy ddraw.dll and bwvr.ini into the folder of the game, beside
   runblack.exe. Keep a copy of a ddraw.dll that is already there.
2. Start the game in the container as you did without the mod.
3. Quit the game.

The mod fits one build of the game: the unofficial patch v1.42. On any other
build it installs no hook at all and the game runs as normal.


WHAT TO SEND BACK

The file bwvr.log, beside runblack.exe.

Its first line says which build ran, for example:

  Black & White VR Mod 1.2.0 (build abc1234, built 2026-11-03 14:44:18)

Check that stamp against the one the deploy script printed. A different stamp
means that another build ran, and then nothing else in the log is about the
change you meant to test.

Four things in that log answer stage 2:

- the first line, so the build is known,
- the lines that start with "xrapi", which say which folder the handshake
  files went in, whether the socket opened, and what the headset calls
  itself,
- the lines of the hooks, so a hook under Box86 and Wine is known to hold,
- any line of a fault, which says what the container did not give the mod.

Also say what the resolution of the container was.


WHAT THIS BUILD DOES NOT TOUCH

It adds no file to the game and changes none. It does not read, change or
delete a saved game or a profile. It writes bwvr.log, and the logs of the
sessions before it, beside runblack.exe. It writes two small files in
Z:\tmp\xr, which is the folder that the runtime itself uses, and it removes
its own "vr" file when the game ends.
