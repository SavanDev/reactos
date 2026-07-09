/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of GetUserDefaultLocaleName
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "k32_vista.h"

INT
WINAPI
GetUserDefaultLocaleName(
    LPWSTR lpLocaleName,
    INT cchLocaleName)
{
    return LCIDToLocaleName(LOCALE_USER_DEFAULT, lpLocaleName, cchLocaleName, 0);
}
