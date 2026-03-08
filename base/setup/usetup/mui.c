/*
 *  ReactOS kernel
 *  Copyright (C) 2008 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/mui.c
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:
 */

#include "usetup.h"
#include "muilanguages.h"

#define NDEBUG
#include <debug.h>

/* Special characters */
CHAR CharBullet                     = 0x07; /* bullet */
CHAR CharBlock                      = 0xDB; /* block */
CHAR CharHalfBlock                  = 0xDD; /* half-left block */
CHAR CharUpArrow                    = 0x18; /* up arrow */
CHAR CharDownArrow                  = 0x19; /* down arrow */
CHAR CharHorizontalLine             = 0xC4; /* horizontal line */
CHAR CharVerticalLine               = 0xB3; /* vertical line */
CHAR CharUpperLeftCorner            = 0xDA; /* upper left corner */
CHAR CharUpperRightCorner           = 0xBF; /* upper right corner */
CHAR CharLowerLeftCorner            = 0xC0; /* lower left corner */
CHAR CharLowerRightCorner           = 0xD9; /* lower right corner */
CHAR CharVertLineAndRightHorizLine  = 0xC3; /* |- (vertical line and right horizontal line) */
CHAR CharLeftHorizLineAndVertLine   = 0xB4; /* -| (left horizontal line and vertical line) */
CHAR CharDoubleHorizontalLine       = 0xCD; /* double horizontal line (and underline) */
CHAR CharDoubleVerticalLine         = 0xBA; /* double vertical line */
CHAR CharDoubleUpperLeftCorner      = 0xC9; /* double upper left corner */
CHAR CharDoubleUpperRightCorner     = 0xBB; /* double upper right corner */
CHAR CharDoubleLowerLeftCorner      = 0xC8; /* double lower left corner */
CHAR CharDoubleLowerRightCorner     = 0xBC; /* double lower right corner */

typedef struct _MUI_SCREEN_CELL
{
    CHAR Char;
    UCHAR Attribute;
} MUI_SCREEN_CELL, *PMUI_SCREEN_CELL;

static
VOID
MUI_InitializeScreenBuffer(
    _Out_writes_(CellCount) PMUI_SCREEN_CELL Buffer,
    _In_ SIZE_T CellCount)
{
    SIZE_T Index;

    for (Index = 0; Index < CellCount; ++Index)
    {
        Buffer[Index].Char = ' ';
        Buffer[Index].Attribute = FOREGROUND_WHITE | BACKGROUND_BLUE;
    }
}

static
BOOLEAN
MUI_ComputeTextOrigin(
    _In_ SHORT X,
    _In_ SHORT Y,
    _In_ INT Flags,
    _In_ SHORT Length,
    _Out_ PSHORT TextX,
    _Out_ PSHORT TextY)
{
    SHORT DrawX = X;
    SHORT DrawY = (Flags & TEXT_TYPE_STATUS) ? (yScreen - 1) : Y;

    if (Flags & TEXT_ALIGN_CENTER)
    {
        DrawX = (xScreen - Length) / 2;
    }
    else if (Flags & TEXT_ALIGN_RIGHT)
    {
        DrawX -= Length;

        if (Flags & TEXT_PADDING_SMALL)
            DrawX -= 1;
        else if (Flags & TEXT_PADDING_MEDIUM)
            DrawX -= 2;
        else if (Flags & TEXT_PADDING_BIG)
            DrawX -= 3;
    }
    else
    {
        if (Flags & TEXT_PADDING_SMALL)
            DrawX += 1;
        else if (Flags & TEXT_PADDING_MEDIUM)
            DrawX += 2;
        else if (Flags & TEXT_PADDING_BIG)
            DrawX += 3;
    }

    *TextX = DrawX;
    *TextY = DrawY;
    return (DrawY >= 0 && DrawY < yScreen);
}

static
VOID
MUI_FillScreenLine(
    _Inout_updates_(xScreen * yScreen) PMUI_SCREEN_CELL Buffer,
    _In_ SHORT Y,
    _In_ UCHAR Attribute)
{
    SHORT X;

    if (Y < 0 || Y >= yScreen)
        return;

    for (X = 0; X < xScreen; ++X)
    {
        SIZE_T Index = (SIZE_T)Y * xScreen + X;
        Buffer[Index].Char = ' ';
        Buffer[Index].Attribute = Attribute;
    }
}

static
VOID
MUI_WriteTextRun(
    _Inout_updates_(xScreen * yScreen) PMUI_SCREEN_CELL Buffer,
    _In_ SHORT X,
    _In_ SHORT Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attribute)
{
    SHORT CurrentX;

    if (Y < 0 || Y >= yScreen)
        return;

    for (CurrentX = X; *Text; ++Text, ++CurrentX)
    {
        SIZE_T Index;

        if (CurrentX < 0)
            continue;
        if (CurrentX >= xScreen)
            break;

        Index = (SIZE_T)Y * xScreen + CurrentX;
        Buffer[Index].Char = *Text;
        Buffer[Index].Attribute = Attribute;
    }
}

static
VOID
MUI_RenderPageEntry(
    _Inout_updates_(xScreen * yScreen) PMUI_SCREEN_CELL Buffer,
    _In_ const MUI_ENTRY* Entry)
{
    SHORT TextX, TextY, Length;
    UCHAR Attribute;

    Length = (SHORT)strlen(Entry->Buffer);
    if (!MUI_ComputeTextOrigin(Entry->X, Entry->Y, Entry->Flags, Length, &TextX, &TextY))
        return;

    if (Entry->Flags & TEXT_TYPE_STATUS)
    {
        MUI_FillScreenLine(Buffer, TextY, BACKGROUND_WHITE);
        Attribute = BACKGROUND_WHITE;
    }
    else if (Entry->Flags & TEXT_STYLE_HIGHLIGHT)
    {
        Attribute = FOREGROUND_WHITE | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
    }
    else
    {
        Attribute = FOREGROUND_WHITE | BACKGROUND_BLUE;
    }

    MUI_WriteTextRun(Buffer, TextX, TextY, Entry->Buffer, Attribute);

    if ((Entry->Flags & TEXT_TYPE_STATUS) == 0 &&
        (Entry->Flags & TEXT_STYLE_UNDERLINE) &&
        TextY + 1 < yScreen)
    {
        SHORT i;

        for (i = 0; i < Length; ++i)
        {
            SIZE_T Index;
            SHORT UnderlineX = TextX + i;

            if (UnderlineX < 0)
                continue;
            if (UnderlineX >= xScreen)
                break;

            Index = (SIZE_T)(TextY + 1) * xScreen + UnderlineX;
            Buffer[Index].Char = CharDoubleHorizontalLine;
            Buffer[Index].Attribute = FOREGROUND_WHITE | BACKGROUND_BLUE;
        }
    }
}

static
VOID
MUI_RenderPageToConsole(
    _In_ const MUI_ENTRY* Entry)
{
    SIZE_T CellCount;
    PMUI_SCREEN_CELL Buffer;
    ULONG Index;

    CellCount = (SIZE_T)xScreen * yScreen;
    Buffer = RtlAllocateHeap(ProcessHeap, 0, CellCount * sizeof(*Buffer));
    if (!Buffer)
        return;

    MUI_InitializeScreenBuffer(Buffer, CellCount);

    for (Index = 0; Entry[Index].Buffer != NULL; ++Index)
        MUI_RenderPageEntry(Buffer, &Entry[Index]);

    DrawConsoleScreenBuffer(StdOutput, 0, 0, xScreen, yScreen, Buffer, CellCount * sizeof(*Buffer));
    RtlFreeHeap(ProcessHeap, 0, Buffer);
}

static
ULONG
FindLanguageIndex(VOID)
{
    ULONG lngIndex = 0;

    if (SelectedLanguageId == NULL)
    {
        /* Default to en-US */
        return 0;   // FIXME!!
        // SelectedLanguageId = L"00000409";
    }

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (_wcsicmp(ResourceList[lngIndex].LanguageID, SelectedLanguageId) == 0)
        {
            return lngIndex;
        }

        lngIndex++;
    }

    return 0;
}


#if 0
BOOLEAN
IsLanguageAvailable(
    PWCHAR LanguageId)
{
    ULONG lngIndex = 0;

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (_wcsicmp(ResourceList[lngIndex].LanguageID, LanguageId) == 0)
            return TRUE;

        lngIndex++;
    }

    return FALSE;
}
#endif


static
const MUI_ENTRY *
FindMUIEntriesOfPage(
    IN ULONG PageNumber)
{
    ULONG muiIndex = 0;
    ULONG lngIndex;
    const MUI_PAGE * Pages = NULL;

    lngIndex = max(FindLanguageIndex(), 0);
    Pages = ResourceList[lngIndex].MuiPages;

    while (Pages[muiIndex].MuiEntry != NULL)
    {
        if (Pages[muiIndex].Number == PageNumber)
            return Pages[muiIndex].MuiEntry;

        muiIndex++;
    }

    return NULL;
}

static
const MUI_ERROR *
FindMUIErrorEntries(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return ResourceList[lngIndex].MuiErrors;
}

static
const MUI_STRING *
FindMUIStringEntries(VOID)
{
    ULONG lngIndex = max(FindLanguageIndex(), 0);
    return ResourceList[lngIndex].MuiStrings;
}


VOID
MUIClearPage(
    IN ULONG page)
{
    const MUI_ENTRY * entry;
    ULONG index;

    entry = FindMUIEntriesOfPage(page);
    if (!entry)
    {
        PopupError("Error: Failed to find translated page",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    index = 0;
    while (entry[index].Buffer != NULL)
    {
        CONSOLE_ClearStyledText(entry[index].X,
                                entry[index].Y,
                                entry[index].Flags,
                                (USHORT)strlen(entry[index].Buffer));
        index++;
    }
}

VOID
MUIDisplayPage(
    IN ULONG page)
{
    const MUI_ENTRY * entry;

    entry = FindMUIEntriesOfPage(page);
    if (!entry)
    {
        PopupError("Error: Failed to find translated page",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    MUI_RenderPageToConsole(entry);
}

VOID
MUIDisplayErrorV(
    IN ULONG ErrorNum,
    OUT PINPUT_RECORD Ir,
    IN ULONG WaitEvent,
    IN va_list args)
{
    const MUI_ERROR* entry;
    CHAR Buffer[2048];

    if (ErrorNum >= ERROR_LAST_ERROR_CODE)
    {
        PopupError("Invalid error number provided",
                   "Press ENTER to continue",
                   Ir,
                   POPUP_WAIT_ENTER);
        return;
    }

    entry = FindMUIErrorEntries();
    if (!entry)
    {
        PopupError("Error: Failed to find translated error message",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    vsprintf(Buffer, entry[ErrorNum].ErrorText, args);

    PopupError(Buffer,
               entry[ErrorNum].ErrorStatus,
               Ir,
               WaitEvent);
}

VOID
__cdecl
MUIDisplayError(
    IN ULONG ErrorNum,
    OUT PINPUT_RECORD Ir,
    IN ULONG WaitEvent,
    ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, WaitEvent);
    MUIDisplayErrorV(ErrorNum, Ir, WaitEvent, arg_ptr);
    va_end(arg_ptr);
}

PCSTR
MUIGetString(
    ULONG Number)
{
    ULONG i;
    const MUI_STRING * entry;
    CHAR szErr[128];

    entry = FindMUIStringEntries();
    if (entry)
    {
        for (i = 0; entry[i].Number != 0; i++)
        {
            if (entry[i].Number == Number)
            {
                return entry[i].String;
            }
        }
    }

    sprintf(szErr, "Error: failed find string id %lu for language index %lu\n", Number, FindLanguageIndex());

    PopupError(szErr,
               NULL,
               NULL,
               POPUP_WAIT_NONE);

    return "<nostring>";
}

/**
 * @MUIGetEntry
 *
 * Retrieves a MUI entry of a page, given the page number and the text ID.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as a unsigned integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @return
 *     Returns a constant MUI entry.
 *
 */
const MUI_ENTRY *
MUIGetEntry(
    IN ULONG Page,
    IN INT TextID)
{
    const MUI_ENTRY * entry;
    ULONG index;

    /* Retrieve the entries of a MUI page */
    entry = FindMUIEntriesOfPage(Page);
    if (!entry)
    {
        DPRINT("MUIGetEntryData(): Failed to get the translated entry page!\n");
        return NULL;
    }

    /* Loop over the ID entries and check if it matches with one of them */
    for (index = 0; entry[index].Buffer != NULL; index++)
    {
        if (entry[index].TextID == TextID)
        {
            /* They match so return the MUI entry */
            return &entry[index];
        }
    }

    /* Page number or ID are incorrect so in this case bail out */
    DPRINT("Couldn't get the MUI entry field from the page!\n");
    return NULL;
}

/**
 * @MUIClearText
 *
 * Clears a portion of text from the console output.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUIClearText(
    IN ULONG Page,
    IN INT TextID)
{
    const MUI_ENTRY * entry;
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Remove the text by using CONSOLE_ClearTextXY() */
        CONSOLE_ClearTextXY(
            entry[Index].X,
            entry[Index].Y,
            (USHORT)strlen(entry[Index].Buffer));

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
}

/**
 * @MUIClearStyledText
 *
 * Clears a portion of text from the console output, given the actual state style flag of the text.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @param[in]   Flags
 *      The text style flag, as an integer. The flag determines the style of the text, such
 *      as being highlighted, underlined, high padding and so on.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUIClearStyledText(
    IN ULONG Page,
    IN INT TextID,
    IN INT Flags)
{
    const MUI_ENTRY * entry;
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Now, begin removing the text by calling CONSOLE_ClearStyledText() */
        CONSOLE_ClearStyledText(
            entry[Index].X,
            entry[Index].Y,
            Flags,
            (USHORT)strlen(entry[Index].Buffer));

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
}

/**
 * @MUISetText
 *
 * Prints a text to the console output.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUISetText(
    IN ULONG Page,
    IN INT TextID)
{
    const MUI_ENTRY * entry;
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Print the text to the console output by calling CONSOLE_SetTextXY() */
        CONSOLE_SetTextXY(entry[Index].X, entry[Index].Y, entry[Index].Buffer);

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
}

/**
 * @MUISetStyledText
 *
 * Prints a text to the console output, with a style for it.
 *
 * @param[in]   Page
 *     The MUI (Multilingual User Interface) entry page number, as a unsigned long integer.
 *
 * @param[in]   TextID
 *      The text identification number (ID), as an integer. The parameter is used to identify
 *      its MUI properties like the coordinates, text style flag and its buffer content.
 *
 *  @param[in]   Flags
 *      The text style flag, as an integer. The flag determines the style of the text, such
 *      as being highlighted, underlined, high padding and so on.
 *
 * @return
 *     Nothing.
 *
 */
VOID
MUISetStyledText(
    IN ULONG Page,
    IN INT TextID,
    IN INT Flags)
{
    const MUI_ENTRY * entry;
    ULONG Index = 0;

    /* Get the MUI entry */
    entry = MUIGetEntry(Page, TextID);

    if (!entry)
        return;

    /* Ensure that the text string given by the text ID and page is not NULL */
    while (entry[Index].Buffer != NULL)
    {
        /* If text ID is not correct, skip the entry */
        if (entry[Index].TextID != TextID)
        {
            Index++;
            continue;
        }

        /* Print the text to the console output by calling CONSOLE_SetStyledText() */
        CONSOLE_SetStyledText(entry[Index].X, entry[Index].Y, Flags, entry[Index].Buffer);

        /* Increment the index and loop over next entires with the same ID */
        Index++;
    }
}

VOID
SetConsoleCodePage(VOID)
{
    UINT wCodePage;

#if 0
    ULONG lngIndex = 0;

    while (ResourceList[lngIndex].MuiPages != NULL)
    {
        if (_wcsicmp(ResourceList[lngIndex].LanguageID, SelectedLanguageId) == 0)
        {
            wCodePage = ResourceList[lngIndex].OEMCPage;
            SetConsoleOutputCP(wCodePage);
            return;
        }

        lngIndex++;
    }
#else
    wCodePage = MUIGetOEMCodePage(SelectedLanguageId);
    SetConsoleOutputCP(wCodePage);
#endif

    switch (wCodePage)
    {
        case 28606: /* Romanian */
        case 932: /* Japanese */
            /* Set special characters */
            CharBullet = 0x07;
            CharBlock = 0x01;
            CharHalfBlock = 0x02;
            CharUpArrow = 0x03;
            CharDownArrow = 0x04;
            CharHorizontalLine = 0x05;
            CharVerticalLine = 0x06;
            CharUpperLeftCorner = 0x08;
            CharUpperRightCorner = 0x09;
            CharLowerLeftCorner = 0x0B;
            CharLowerRightCorner = 0x0C;
            CharVertLineAndRightHorizLine = 0x0E;
            CharLeftHorizLineAndVertLine = 0x0F;
            CharDoubleHorizontalLine = 0x10;
            CharDoubleVerticalLine = 0x11;
            CharDoubleUpperLeftCorner = 0x12;
            CharDoubleUpperRightCorner = 0x13;
            CharDoubleLowerLeftCorner = 0x14;
            CharDoubleLowerRightCorner = 0x15;

            /* FIXME: Enter 640x400 video mode */
            break;

        default: /* Other codepages */
            /* Set special characters */
            CharBullet = 0x07;
            CharBlock = 0xDB;
            CharHalfBlock = 0xDD;
            CharUpArrow = 0x18;
            CharDownArrow = 0x19;
            CharHorizontalLine = 0xC4;
            CharVerticalLine = 0xB3;
            CharUpperLeftCorner = 0xDA;
            CharUpperRightCorner = 0xBF;
            CharLowerLeftCorner = 0xC0;
            CharLowerRightCorner = 0xD9;
            CharVertLineAndRightHorizLine = 0xC3;
            CharLeftHorizLineAndVertLine = 0xB4;
            CharDoubleHorizontalLine = 0xCD;
            CharDoubleVerticalLine = 0xBA;
            CharDoubleUpperLeftCorner = 0xC9;
            CharDoubleUpperRightCorner = 0xBB;
            CharDoubleLowerLeftCorner = 0xC8;
            CharDoubleLowerRightCorner = 0xBC;

            /* FIXME: Enter 720x400 video mode */
            break;
    }
}
