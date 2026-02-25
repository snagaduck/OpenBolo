/*
 * OpenBolo — cross-platform remake of WinBolo (1998)
 * Copyright (C) 2026 OpenBolo Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * preferences_stub.c
 *
 * Windows-only stub for the server build.
 *
 * On Linux, preferences.c emulates the Win32 GetPrivateProfileString /
 * WritePrivateProfileString APIs using POSIX file I/O.  On Windows those
 * functions are already provided by kernel32.dll, so we only need to supply
 * the one custom helper: preferencesGetPreferenceFile().
 *
 * For the server we store the INI file alongside the executable.
 * The client will use its own (gui/win32) preferences path later.
 */

#include <windows.h>
#include <stdio.h>

#define PREFERENCE_FILE "winbolo-server.ini"

/*
 * preferencesGetPreferenceFile
 *
 * Returns the full path to the server preferences file.
 * On Windows we place it next to the executable.
 * 'value' must be at least MAX_PATH bytes.
 */
void preferencesGetPreferenceFile(char *value) {
    char exePath[MAX_PATH];
    char *lastSlash;

    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    /* Strip the executable filename to get the directory */
    lastSlash = strrchr(exePath, '\\');
    if (lastSlash != NULL) {
        *(lastSlash + 1) = '\0';
    }

    snprintf(value, MAX_PATH, "%s%s", exePath, PREFERENCE_FILE);
}