# ------------------------------------------------------------
# OpenBolo Server — standalone dedicated server
# No graphics dependency. Links only against Winsock2 on Windows.
# ------------------------------------------------------------

set(BOLO    "${ORIG_SRC}/bolo")
set(SRV     "${ORIG_SRC}/server")
set(ZLIB    "${ORIG_SRC}/zlib")
set(LZW     "${ORIG_SRC}/lzw")
set(WBNET   "${ORIG_SRC}/winbolonet")
set(GUI_WIN "${ORIG_SRC}/gui/win32")
set(GUI_LIN "${ORIG_SRC}/gui/linux")
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

# ---- Core game engine (shared between client and server) ----
# Source list mirrors the 'b' target in the original Linux Makefile.
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
    # messages.c — stubs provided by server/serverfrontend.c (clientMessageAdd)
    ${BOLO}/mines.c
    ${BOLO}/minesexp.c
    ${BOLO}/netmt.c
    ${BOLO}/netplayers.c
    ${BOLO}/netpnb.c
    ${BOLO}/pillbox.c
    ${BOLO}/players.c
    ${BOLO}/playersrejoin.c
    ${BOLO}/rubble.c
    ${BOLO}/screenbrainmap.c
    ${BOLO}/screenbullet.c
    # screencalc.c  — client-only; compiled in orig Makefile 'b' target but NOT linked into server
    # screenlgm.c   — stubs provided by server/serverfrontend.c (screenLgmAddItem)
    # screentank.c  — stubs provided by server/serverfrontend.c (screenTanksAddItem)
    # scroll.c      — client-only; references screenTankIsDead which is client-only
    ${BOLO}/shells.c
    # sounddist.c   — stubs provided by server/serverfrontend.c (clientSoundDist)
    ${BOLO}/starts.c
    ${BOLO}/swamp.c
    ${BOLO}/tank.c
    ${BOLO}/tankexp.c
    ${BOLO}/treegrow.c
    ${BOLO}/udppackets.c
    ${BOLO}/util.c
)

# ---- WinBoloNet integration (ranking/tracker HTTP client) ---
set(WBNET_SOURCES
    ${WBNET}/http.c
    ${WBNET}/winbolonet.c
    ${WBNET}/winbolonetevents.c
    ${WBNET}/winbolonetthread.c
)

# ---- Server-specific files ----------------------------------
set(SERVER_SOURCES
    ${SRV}/servercore.c
    ${SRV}/serverfrontend.c   # stub frontend (no GUI) — NOT bolo/serverfrontend.c
    ${SRV}/servermain.c
    ${SRV}/servermessages.c
    ${SRV}/servernet.c
    ${SRV}/threads.c
)

# Platform-specific transport and support files
if(WIN32)
    list(APPEND SERVER_SOURCES
        ${SRV}/win32/servertransport.c   # Winsock2 implementation
        ${GUI_WIN}/lang.c                 # Windows string resources
        ${CMAKE_CURRENT_SOURCE_DIR}/win32/preferences_stub.c  # no Win32 preferences.c exists
    )
else()
    list(APPEND SERVER_SOURCES
        ${SRV}/servertransport.c          # POSIX sockets implementation
        ${GUI_LIN}/lang.c
        ${GUI_LIN}/preferences.c
    )
endif()

# ---- Build the server executable ----------------------------
add_executable(winbolo-server
    ${ZLIB_SOURCES}
    ${LZW_SOURCES}
    ${BOLO_SOURCES}
    ${WBNET_SOURCES}
    ${SERVER_SOURCES}
)

target_include_directories(winbolo-server PRIVATE
    ${CMAKE_SOURCE_DIR}/include   # fixed headers (e.g. brain.h), searched before originals
    ${BOLO}
    ${SRV}
    ${ZLIB}
    ${LZW}
    ${WBNET}
    ${BSD}
)

if(WIN32)
    target_include_directories(winbolo-server PRIVATE
        ${GUI_WIN}
        ${SRV}/win32
    )
    target_link_libraries(winbolo-server PRIVATE ws2_32 winmm)
    # Force-include winbolo_platform.h at the top of every .c file.
    # This guarantees winsock2.h arrives before windows.h and that
    # mmsystem.h (TIME_PERIODIC etc.) is available project-wide.
    target_compile_options(winbolo-server PRIVATE
        "/FI${CMAKE_SOURCE_DIR}/include/winbolo_platform.h"
    )
else()
    target_include_directories(winbolo-server PRIVATE ${GUI_LIN})
    # SDL is only needed on Linux (mutex in threads.c)
    find_package(SDL REQUIRED)
    target_include_directories(winbolo-server PRIVATE ${SDL_INCLUDE_DIRS})
    target_link_libraries(winbolo-server PRIVATE ${SDL_LIBRARIES} pthread)
endif()
