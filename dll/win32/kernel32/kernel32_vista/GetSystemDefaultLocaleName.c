/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of GetSystemDefaultLocaleName
 * COPYRIGHT:   Copyright 2026
 */

#include "k32_vista.h"

INT
WINAPI
GetSystemDefaultLocaleName(
    LPWSTR lpLocaleName,
    INT cchLocaleName)
{
    return LCIDToLocaleName(LOCALE_SYSTEM_DEFAULT, lpLocaleName, cchLocaleName, 0);
}
