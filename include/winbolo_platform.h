/*
 * winbolo_platform.h
 *
 * Force-included at the top of every server compilation unit via CMake /FI.
 * Ensures Windows platform headers arrive in the correct order and that
 * headers stripped by WIN32_LEAN_AND_MEAN are explicitly re-added where the
 * game code needs them (e.g. mmsystem.h for TIME_PERIODIC / timeSetEvent).
 */
#ifdef _WIN32

/* Must be defined before ANY windows.h / winsock include */
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif

/* winsock2.h must come before windows.h; including it here at default   */
/* packing ensures every TU gets it first, before any #pragma pack push. */
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>

/* Multimedia timer API (TIME_PERIODIC, timeSetEvent, etc.)              */
/* Stripped by WIN32_LEAN_AND_MEAN, so we re-include explicitly.         */
#  include <mmsystem.h>

#endif /* _WIN32 */
