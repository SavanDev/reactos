/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/term.h
 * PURPOSE:         Internal Frontend Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* Macros used to call functions in the TERMINAL_VTBL virtual table */

#define TermReadStream(Console, /**/ Unicode, /**/ Buffer, ReadControl, Parameter, NumCharsToRead, NumCharsRead) \
    (Console)->TermIFace.Vtbl->ReadStream(&(Console)->TermIFace, /**/ (Unicode), /**/ \
                                           (Buffer), (ReadControl), (Parameter), (NumCharsToRead), (NumCharsRead))

#define TermWriteStream(Console, ScreenBuffer, Buffer, Length, Attrib) \
    (Console)->TermIFace.Vtbl->WriteStream(&(Console)->TermIFace, (ScreenBuffer), (Buffer), \
                                           (Length), (Attrib))


#define TermDrawRegion(Console, Region) \
    (Console)->TermIFace.Vtbl->DrawRegion(&(Console)->TermIFace, (Region))
#define TermSetCursorInfo(Console, ScreenBuffer) \
    (Console)->TermIFace.Vtbl->SetCursorInfo(&(Console)->TermIFace, (ScreenBuffer))
#define TermSetScreenInfo(Console, ScreenBuffer, OldCursorX, OldCursorY) \
    (Console)->TermIFace.Vtbl->SetScreenInfo(&(Console)->TermIFace, (ScreenBuffer), (OldCursorX), (OldCursorY))
#define TermResizeTerminal(Console) \
    (Console)->TermIFace.Vtbl->ResizeTerminal(&(Console)->TermIFace)
#define TermSetActiveScreenBuffer(Console) \
    (Console)->TermIFace.Vtbl->SetActiveScreenBuffer(&(Console)->TermIFace)
#define TermReleaseScreenBuffer(Console, ScreenBuffer) \
    (Console)->TermIFace.Vtbl->ReleaseScreenBuffer(&(Console)->TermIFace, (ScreenBuffer))
#define TermGetLargestConsoleWindowSize(Console, pSize) \
    (Console)->TermIFace.Vtbl->GetLargestConsoleWindowSize(&(Console)->TermIFace, (pSize))
#define TermSetPalette(Console, PaletteHandle, PaletteUsage) \
    (Console)->TermIFace.Vtbl->SetPalette(&(Console)->TermIFace, (PaletteHandle), (PaletteUsage))
#define TermSetCodePage(Console, CodePage) \
    (Console)->TermIFace.Vtbl->SetCodePage(&(Console)->TermIFace, (CodePage))
#define TermShowMouseCursor(Console, Show) \
    (Console)->TermIFace.Vtbl->ShowMouseCursor(&(Console)->TermIFace, (Show))


/* Macros used to call functions in the FRONTEND_VTBL virtual table */

#define TermRefreshInternalInfo(Console) \
    (Console)->FrontendHost.FrontEnd.Vtbl->RefreshInternalInfo(&(Console)->FrontendHost.FrontEnd)
#define TermChangeTitle(Console) \
    (Console)->FrontendHost.FrontEnd.Vtbl->ChangeTitle(&(Console)->FrontendHost.FrontEnd)
#define TermChangeIcon(Console, IconHandle) \
    (Console)->FrontendHost.FrontEnd.Vtbl->ChangeIcon(&(Console)->FrontendHost.FrontEnd, (IconHandle))
#define TermGetThreadConsoleDesktop(Console) \
    (Console)->FrontendHost.FrontEnd.Vtbl->GetThreadConsoleDesktop(&(Console)->FrontendHost.FrontEnd)
#define TermGetConsoleWindowHandle(Console) \
    (Console)->FrontendHost.FrontEnd.Vtbl->GetConsoleWindowHandle(&(Console)->FrontendHost.FrontEnd)
#define TermGetSelectionInfo(Console, pSelectionInfo) \
    (Console)->FrontendHost.FrontEnd.Vtbl->GetSelectionInfo(&(Console)->FrontendHost.FrontEnd, (pSelectionInfo))
#define TermGetDisplayMode(Console) \
    (Console)->FrontendHost.FrontEnd.Vtbl->GetDisplayMode(&(Console)->FrontendHost.FrontEnd)
#define TermSetDisplayMode(Console, NewMode) \
    (Console)->FrontendHost.FrontEnd.Vtbl->SetDisplayMode(&(Console)->FrontendHost.FrontEnd, (NewMode))
#define TermSetMouseCursor(Console, CursorHandle) \
    (Console)->FrontendHost.FrontEnd.Vtbl->SetMouseCursor(&(Console)->FrontendHost.FrontEnd, (CursorHandle))
#define TermMenuControl(Console, CmdIdLow, CmdIdHigh) \
    (Console)->FrontendHost.FrontEnd.Vtbl->MenuControl(&(Console)->FrontendHost.FrontEnd, (CmdIdLow), (CmdIdHigh))
#define TermSetMenuClose(Console, Enable) \
    (Console)->FrontendHost.FrontEnd.Vtbl->SetMenuClose(&(Console)->FrontendHost.FrontEnd, (Enable))

/* EOF */
