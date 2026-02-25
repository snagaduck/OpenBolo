# ------------------------------------------------------------
# OpenBolo Client — Raylib platform layer
# Replaces original DirectX/Win32 frontend with Raylib 5.5.
# ------------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
# Don't build Raylib's examples or games
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Convenience path vars (same layout as server/CMakeLists.txt)
set(BOLO    "${ORIG_SRC}/bolo")
set(SRV     "${ORIG_SRC}/server")
set(ZLIB    "${ORIG_SRC}/zlib")
set(LZW     "${ORIG_SRC}/lzw")
set(WBNET   "${ORIG_SRC}/winbolonet")
set(GUI_WIN "${ORIG_SRC}/gui/win32")
set(GUI_SRC "${ORIG_SRC}/gui")
set(BSD     "${ORIG_SRC}/bsd")

# ---- zlib (embedded) ----------------------------------------
set(ZLIB_SOURCES
    ${ZLIB}/adler32.c
    ${ZLIB}/compress.c
    ${ZLIB}/crc32.c
    ${ZLIB}/deflate.c
    ${ZLIB}/gzio.c
    ${ZLIB}/inffast.c
    ${ZLIB}/inflate.c
    ${ZLIB}/inftrees.c
    ${ZLIB}/ioapi.c
    ${ZLIB}/trees.c
    ${ZLIB}/uncompr.c
    ${ZLIB}/unzip.c
    ${ZLIB}/zip.c
    ${ZLIB}/zutil.c
)

# ---- LZW compression (embedded) -----------------------------
set(LZW_SOURCES
    ${LZW}/dcodlzw.c
    ${LZW}/ecodlzw.c
)

# ---- Core game engine (full client set) ---------------------
# Same as server BOLO_SOURCES PLUS the client-only files the
# server excluded (those stubs were in server/serverfrontend.c).
set(BOLO_SOURCES
    ${BOLO}/allience.c
    ${BOLO}/backend.c
    ${BOLO}/bases.c
    ${BOLO}/bolo_map.c
    ${BOLO}/building.c
    ${BOLO}/crc.c
    ${BOLO}/explosions.c
    ${BOLO}/floodfill.c
    ${BOLO}/gametype.c
    ${BOLO}/global.c
    ${BOLO}/grass.c
    ${BOLO}/labels.c
    ${BOLO}/lgm.c
    ${BOLO}/log.c
    ${BOLO}/messages.c       # client provides clientMessageAdd
    ${BOLO}/mines.c
    ${BOLO}/minesexp.c
    ${BOLO}/netmt.c
    ${BOLO}/netplayers.c
    ${BOLO}/netpnb.c
    ${BOLO}/network.c        # net layer: netGetStatus, netUdpPacketArrive, etc.
    ${BOLO}/pillbox.c
    ${BOLO}/players.c
    ${BOLO}/playersrejoin.c
    ${BOLO}/rubble.c
    ${BOLO}/screen.c         # client screen layer: screenTankIsDead, clientGet*, etc.
    ${BOLO}/screenbrainmap.c
    ${BOLO}/screenbullet.c
    ${BOLO}/screencalc.c     # client-only
    ${BOLO}/screenlgm.c      # client provides screenLgmAddItem
    ${BOLO}/screentank.c     # client provides screenTanksAddItem
    ${BOLO}/scroll.c         # client-only scroll/view
    ${BOLO}/shells.c
    ${BOLO}/sounddist.c      # client provides clientSoundDist
    ${BOLO}/starts.c
    ${BOLO}/swamp.c
    ${BOLO}/tank.c
    ${BOLO}/tankexp.c
    ${BOLO}/treegrow.c
    ${BOLO}/udppackets.c
    ${BOLO}/util.c
)

# ---- WinBoloNet integration ---------------------------------
set(WBNET_SOURCES
    ${WBNET}/http.c
    ${WBNET}/winbolonet.c
    ${WBNET}/winbolonetevents.c
    ${WBNET}/winbolonetthread.c
)

# ---- New Raylib platform layer ------------------------------
# main.c is compiled as a SEPARATE object library WITHOUT the /FI
# force-include.  Including winbolo_platform.h (→ windows.h → wingdi.h)
# before raylib.h causes a "Rectangle: redefinition" clash because
# wingdi.h declares Rectangle() as a GDI function and raylib.h
# defines Rectangle as a struct.  Compiling main.c without /FI avoids
# the issue; main.c only includes raylib.h so it never sees wingdi.h.
#
# frontend_raylib.c does NOT include raylib.h in Phase A2 (all stubs
# are empty), so it can safely be compiled with /FI alongside the rest
# of the source files.
add_library(winbolo-main-entry OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)
target_include_directories(winbolo-main-entry PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # for game_loop.h
)
target_link_libraries(winbolo-main-entry PRIVATE raylib)
# No /FI here — intentional.

# render_bridge.c includes raylib.h directly — same Rectangle clash risk.
# Compiled as a separate OBJECT library WITHOUT /FI.
add_library(winbolo-render-bridge OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/render_bridge.c
)
target_include_directories(winbolo-render-bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # for render_bridge.h
)
target_link_libraries(winbolo-render-bridge PRIVATE raylib)
# No /FI here — intentional.

set(CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend_raylib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/game_loop.c      # Phase B1: engine bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/enet_transport.c # Phase C1: ENet transport (replaces netclient.c + servertransport.c)
)

# ---- Embedded server (WinBolo client always hosts its own server) -
# The bolo game engine calls serverCore* / serverNet* functions even in
# the client build — the client runs an embedded server for the game
# it is hosting or has joined.  servermain.c (standalone server entry)
# is excluded; everything else is included.
# threads.c provides threadsGetContext / threadsWaitForMutex /
# threadsReleaseMutex which the core bolo engine calls.
set(SERVER_SOURCES
    ${SRV}/servercore.c
    ${SRV}/servermessages.c
    ${SRV}/servernet.c
    ${SRV}/threads.c
    # servertransport.c replaced by enet_transport.c below
)

# ---- Non-DirectX gui/win32 files ----------------------------
# Excluded: draw.c, dsutil.c, gamefront.c, input.c, sound.c, winutil.c (all DirectX)
# Excluded: winbolo.c (WinMain — we replace it with main.c above)
# Excluded: all dialog*.c — only called from WinMain which we're replacing
# Included: support modules the bolo engine or netclient calls directly
set(GUI_WIN32_SOURCES
    ${GUI_WIN}/lang.c             # string resources (called by backend)
    ${GUI_WIN}/brainsHandler.c    # brain DLL loader
    ${GUI_WIN}/clientmutex.c      # screen mutex
    # netclient.c replaced by enet_transport.c below
    ${GUI_WIN}/dnslookups.c       # async DNS
    ${GUI_WIN}/currentgames.c     # tracker game list
    ${GUI_WIN}/mappreview.c       # map preview (needed by netclient)
)

# ---- Stubs for excluded DirectX files -----------------------
# winutil.c has DirectX headers so we exclude it, but brainsHandler.c
# calls winUtilWBSubDirExist().  Stub it out here.
set(WIN32STUBS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/win32stubs.c
)

# ---- Windows preferences stub (same as server) --------------
set(PREFS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/preferences_stub.c
)

# ---- Build the client executable ----------------------------
add_executable(winbolo-client
    $<TARGET_OBJECTS:winbolo-main-entry>      # main.c compiled without /FI
    $<TARGET_OBJECTS:winbolo-render-bridge>   # render_bridge.c compiled without /FI
    ${ZLIB_SOURCES}
    ${LZW_SOURCES}
    ${BOLO_SOURCES}
    ${WBNET_SOURCES}
    ${SERVER_SOURCES}
    ${GUI_WIN32_SOURCES}
    ${WIN32STUBS_SOURCES}
    ${PREFS_SOURCES}
    ${CLIENT_SOURCES}
)

target_include_directories(winbolo-client PRIVATE
    ${CMAKE_SOURCE_DIR}/include   # fixed headers (brain.h, network.h, etc.)
    ${BOLO}
    ${SRV}
    ${SRV}/win32                  # server win32 headers (needed by threads.c)
    ${ZLIB}
    ${LZW}
    ${WBNET}
    ${BSD}
    ${GUI_WIN}                    # gui/win32/ headers (dialogs, draw.h, etc.)
    ${GUI_SRC}                    # gui/ headers (resource.h, lang.h)
    ${enet_SOURCE_DIR}/include    # ENet headers
)

target_link_libraries(winbolo-client PRIVATE raylib enet ws2_32 winmm)

# Force-include platform header (same pattern as server)
target_compile_options(winbolo-client PRIVATE
    "/FI${CMAKE_SOURCE_DIR}/include/winbolo_platform.h"
)

# ---- Copy asset files next to the executable after each build -----
add_custom_command(TARGET winbolo-client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GUI_WIN}/tiles.bmp"
        "$<TARGET_FILE_DIR:winbolo-client>/tiles.bmp"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GUI_WIN}/background.bmp"
        "$<TARGET_FILE_DIR:winbolo-client>/background.bmp"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/sounds"
        "$<TARGET_FILE_DIR:winbolo-client>/sounds"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
        "$<TARGET_FILE_DIR:winbolo-client>/fonts"
    COMMENT "Copying tiles.bmp, background.bmp, sounds/ and fonts/ to output directory"
)
