/*
 * Shell Menu Desk Bar
 *
 * Copyright 2014 David Quintana
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include "shellmenu.h"
#include <atlwin.h>
#include <shlwapi_undoc.h>

#include "CMenuDeskBar.h"

/* As far as I can tell, the submenu hierarchy looks like this:
*
* The DeskBar's Child is the Band it contains.
* The DeskBar's Parent is the SID_SMenuPopup of the Site.
*
* The Band's Child is the IMenuPopup of the child submenu.
* The Band's Parent is the SID_SMenuPopup of the Site (the DeskBar).
*
* When the DeskBar receives a selection event:
* If it requires closing the window, it will notify the Child (Band) using CancelLevel.
* If it has to spread upwards (everything but CancelLevel), it will notify the Parent.
*
* When the Band receives a selection event, this is where it gets fuzzy:
* In which cases does it call the Parent? Probably not CancelLevel.
* In which cases does it call the Child?
* How does it react to calls?
*
*/


WINE_DEFAULT_DEBUG_CHANNEL(CMenuDeskBar);

/* These command ids are duplicated from CStartMenu.cpp and must match the
 * tray/start menu resources exactly. */
#define IDM_LOGOFF   402
#define IDM_SHUTDOWN 506
#define STARTPANEL_HIT_NONE      0
#define STARTPANEL_HIT_LOGOFF    1
#define STARTPANEL_HIT_SHUTDOWN  2

static BOOL GetMenuItemTextByCommand(HMENU hMenu, UINT uId, LPWSTR pszText, UINT cchText)
{
    MENUITEMINFOW mii = { sizeof(mii) };

    if (!hMenu || !pszText || cchText == 0)
        return FALSE;

    mii.fMask = MIIM_STRING;
    mii.dwTypeData = pszText;
    mii.cch = cchText;
    return GetMenuItemInfoW(hMenu, uId, FALSE, &mii);
}

/* The XP start panel header shows the account's display name ("full name")
 * when available; fall back to the plain login name otherwise. */
typedef BOOLEAN (WINAPI *PFN_GETUSERNAMEEXW)(int NameFormat, LPWSTR lpNameBuffer, PULONG nSize);
#define STARTPANEL_NAME_DISPLAY 3 /* EXTENDED_NAME_FORMAT::NameDisplay */

static VOID GetStartPanelUserName(LPWSTR pszName, ULONG cchName)
{
    HMODULE hSecur32;
    ULONG cch;

    pszName[0] = UNICODE_NULL;

    hSecur32 = LoadLibraryW(L"secur32.dll");
    if (hSecur32)
    {
        PFN_GETUSERNAMEEXW pGetUserNameExW =
            (PFN_GETUSERNAMEEXW)GetProcAddress(hSecur32, "GetUserNameExW");

        cch = cchName;
        if (pGetUserNameExW &&
            pGetUserNameExW(STARTPANEL_NAME_DISPLAY, pszName, &cch) &&
            pszName[0])
        {
            FreeLibrary(hSecur32);
            return;
        }

        FreeLibrary(hSecur32);
    }

    cch = cchName;
    if (!GetUserNameW(pszName, &cch))
        pszName[0] = UNICODE_NULL;
}

static VOID SanitizeStartPanelButtonText(LPWSTR pszText)
{
    LPWSTR pRead, pWrite;

    if (!pszText || !*pszText)
        return;

    for (pRead = pszText, pWrite = pszText; *pRead; ++pRead)
    {
        if (*pRead == L'&')
            continue;
        if (*pRead == L'\t' || *pRead == L'%')
            break;

        *pWrite++ = *pRead;
    }

    while (pWrite > pszText && (pWrite[-1] == L'.' || pWrite[-1] == L' '))
        --pWrite;

    *pWrite = UNICODE_NULL;
}

CMenuDeskBar::CMenuDeskBar() :
    m_Client(NULL),
    m_ClientWindow(NULL),
    m_IconSize(0),
    m_Banner(NULL),
    m_Shown(FALSE),
    m_ShowFlags(0),
    m_didAddRef(FALSE),
    m_StartPanelHotButton(STARTPANEL_HIT_NONE),
    m_StartPanelTrackingMouse(FALSE)
{
    ZeroMemory(&m_StartPanelRects, sizeof(m_StartPanelRects));
    m_szLogOff[0] = UNICODE_NULL;
    m_szShutdown[0] = UNICODE_NULL;
    m_szUserName[0] = UNICODE_NULL;
}

CMenuDeskBar::~CMenuDeskBar()
{
}

BOOL CMenuDeskBar::_IsStartPanelLayout() const
{
    CComPtr<ITrayPriv> trayPriv;

    if (!SHELL_GetSetting(SSF_STARTPANELON, fStartPanelOn) || m_Site.p == NULL)
        return FALSE;

    return SUCCEEDED(m_Site->QueryInterface(IID_PPV_ARG(ITrayPriv, &trayPriv)));
}

UINT CMenuDeskBar::_GetStartPanelHeaderHeight() const
{
    return _IsStartPanelLayout() ? GetStartPanelTheme().cyHeader : 0;
}

UINT CMenuDeskBar::_GetStartPanelFooterHeight() const
{
    return _IsStartPanelLayout() ? GetStartPanelTheme().cyFooter : 0;
}

VOID CMenuDeskBar::_GetStartPanelRects(STARTPANELLAYOUTRECTS *pRects) const
{
    const STARTPANELTHEME& Theme = GetStartPanelTheme();
    RECT rc;
    LONG cxContent;

    ZeroMemory(pRects, sizeof(*pRects));
    GetClientRect(&rc);
    pRects->rcClient = rc;

    pRects->rcHeader = rc;
    pRects->rcHeader.bottom = min(rc.bottom, rc.top + Theme.cyHeader);

    pRects->rcFooter = rc;
    pRects->rcFooter.top = max(rc.top, rc.bottom - Theme.cyFooter);

    pRects->rcContent = rc;
    pRects->rcContent.top = pRects->rcHeader.bottom;
    pRects->rcContent.bottom = max(pRects->rcContent.top, pRects->rcFooter.top);

    pRects->rcAvatar = pRects->rcHeader;
    pRects->rcAvatar.right -= Theme.cxAvatarPadding;
    pRects->rcAvatar.left = max(pRects->rcAvatar.left, pRects->rcAvatar.right - Theme.cxAvatar);
    pRects->rcAvatar.top += Theme.cyHeaderPadding;
    pRects->rcAvatar.bottom = min(pRects->rcHeader.bottom - Theme.cyHeaderPadding, pRects->rcAvatar.top + Theme.cyAvatar);

    pRects->rcUserText = pRects->rcHeader;
    pRects->rcUserText.left += Theme.cxHeaderPadding;
    pRects->rcUserText.top += Theme.cyHeaderPadding;
    pRects->rcUserText.right = max(pRects->rcUserText.left, pRects->rcAvatar.left - Theme.cxHeaderPadding);
    pRects->rcUserText.bottom = pRects->rcHeader.bottom - Theme.cyHeaderPadding;

    cxContent = max(0, pRects->rcContent.right - pRects->rcContent.left - Theme.cxColumnGap);

    pRects->rcLeftColumn = pRects->rcContent;
    pRects->rcLeftColumn.right = pRects->rcLeftColumn.left + min(Theme.cxLeftColumn, cxContent);

    pRects->rcRightColumn = pRects->rcContent;
    pRects->rcRightColumn.left = min(pRects->rcContent.right, pRects->rcLeftColumn.right + Theme.cxColumnGap);

    pRects->rcLogOffButton = pRects->rcFooter;
    pRects->rcTurnOffButton = pRects->rcFooter;

    pRects->rcTurnOffButton.right -= Theme.cyFooterPadding;
    pRects->rcTurnOffButton.left = max(pRects->rcTurnOffButton.left,
                                       pRects->rcTurnOffButton.right - Theme.cxTurnOffButton);
    pRects->rcTurnOffButton.top += Theme.cyFooterPadding;
    pRects->rcTurnOffButton.bottom -= Theme.cyFooterPadding;

    pRects->rcLogOffButton.right = max(pRects->rcFooter.left,
                                       pRects->rcTurnOffButton.left - Theme.cxFooterGap);
    pRects->rcLogOffButton.left = max(pRects->rcFooter.left + Theme.cyFooterPadding,
                                      pRects->rcLogOffButton.right - Theme.cxLogOffButton);
    pRects->rcLogOffButton.top += Theme.cyFooterPadding;
    pRects->rcLogOffButton.bottom -= Theme.cyFooterPadding;
}

HRESULT CMenuDeskBar::_LoadStartPanelFooterStrings()
{
    CComPtr<ITrayPriv> trayPriv;
    HMENU hMenu = NULL;

    if (!m_szUserName[0])
        GetStartPanelUserName(m_szUserName, _countof(m_szUserName));

    if (m_szLogOff[0] && m_szShutdown[0])
        return S_OK;

    if (m_Site.p == NULL || FAILED(m_Site->QueryInterface(IID_PPV_ARG(ITrayPriv, &trayPriv))) || trayPriv.p == NULL)
        return E_FAIL;

    if (FAILED(trayPriv->AppendMenu(&hMenu)) || !hMenu)
        return E_FAIL;

    GetMenuItemTextByCommand(hMenu, IDM_LOGOFF, m_szLogOff, _countof(m_szLogOff));
    GetMenuItemTextByCommand(hMenu, IDM_SHUTDOWN, m_szShutdown, _countof(m_szShutdown));
    SanitizeStartPanelButtonText(m_szLogOff);
    SanitizeStartPanelButtonText(m_szShutdown);
    DestroyMenu(hMenu);
    return S_OK;
}

VOID CMenuDeskBar::_InvalidateStartPanelFooter()
{
    if (!_IsStartPanelLayout())
        return;

    InvalidateRect(&m_StartPanelRects.rcFooter, FALSE);
}

INT CMenuDeskBar::_StartPanelHitTestFooterButton(POINT pt) const
{
    if (!_IsStartPanelLayout())
        return STARTPANEL_HIT_NONE;

    if (PtInRect(&m_StartPanelRects.rcLogOffButton, pt))
        return STARTPANEL_HIT_LOGOFF;
    if (PtInRect(&m_StartPanelRects.rcTurnOffButton, pt))
        return STARTPANEL_HIT_SHUTDOWN;
    return STARTPANEL_HIT_NONE;
}

VOID CMenuDeskBar::_SetStartPanelHotButton(INT iButton)
{
    if (m_StartPanelHotButton == iButton)
        return;

    m_StartPanelHotButton = iButton;
    _InvalidateStartPanelFooter();
}

HRESULT CMenuDeskBar::_ExecuteStartPanelFooterCommand(UINT uId)
{
    CComPtr<ITrayPriv> trayPriv;
    HWND hwndTray = NULL;

    if (m_Site.p == NULL || FAILED(m_Site->QueryInterface(IID_PPV_ARG(ITrayPriv, &trayPriv))) || trayPriv.p == NULL)
        return E_FAIL;

    if (FAILED(IUnknown_GetWindow(trayPriv, &hwndTray)) || !hwndTray)
        return E_FAIL;

    ::PostMessageW(hwndTray, WM_COMMAND, uId, 0);
    OnSelect(MPOS_FULLCANCEL);
    return S_OK;
}

LRESULT CMenuDeskBar::_OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!m_didAddRef)
    {
        this->AddRef();
        m_didAddRef = TRUE;
    }

    bHandled = FALSE;
    return 0;
}

void CMenuDeskBar::OnFinalMessage(HWND /* hWnd */)
{
    if (m_didAddRef)
    {
        this->Release();
        m_didAddRef = FALSE;
    }
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Initialize(THIS)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    return IUnknown_OnFocusChangeIS(m_Client, punkObj, fSetFocus);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
    OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_MenuDeskBar))
    {
        switch (nCmdID)
        {
        case 2: // refresh
            return S_OK;
        case 3: // load complete
            return S_OK;
        case 4: // set font metrics
            return _AdjustForTheme(nCmdexecopt);
        }
    }
    if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
    {
    }
    else if (IsEqualIID(*pguidCmdGroup, IID_IDeskBarClient))
    {
        switch (nCmdID)
        {
        case 0:
            // hide current band
            break;
        case 2:
            break;
        case 3:
            break;
        }
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr;

    if (IsEqualGUID(guidService, SID_SMenuPopup) ||
        IsEqualGUID(guidService, SID_SMenuBandParent) ||
        IsEqualGUID(guidService, SID_STopLevelBrowser))
    {
        hr = this->QueryInterface(riid, ppvObject);
        if (SUCCEEDED(hr))
            return hr;
    }

    if (IsEqualGUID(guidService, SID_SMenuBandBottom) ||
        IsEqualGUID(guidService, SID_SMenuBandBottomSelected) ||
        IsEqualGUID(guidService, SID_SMenuBandChild))
    {
        if (m_Client == NULL)
            return E_NOINTERFACE;

        hr = IUnknown_QueryService(m_Client, guidService, riid, ppvObject);
        if (SUCCEEDED(hr))
            return hr;
    }


    if (m_Site == NULL)
        return E_NOINTERFACE;

    return IUnknown_QueryService(m_Site, guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return IUnknown_UIActivateIO(m_Client, fActivate, lpMsg);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::HasFocusIO()
{
    return IUnknown_HasFocusIO(m_Client);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return IUnknown_TranslateAcceleratorIO(m_Client, lpMsg);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetClient(IUnknown *punkClient)
{
    CComPtr<IDeskBarClient> pDeskBandClient;
    HRESULT hr;

    if (m_Client)
    {
        hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pDeskBandClient));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        pDeskBandClient->SetDeskBarSite(NULL);

        pDeskBandClient = NULL;
        m_Client = NULL;
    }

    if (punkClient == NULL)
        return S_OK;

    if (m_hWnd == NULL)
    {
        Create(NULL);
    }

    hr = punkClient->QueryInterface(IID_PPV_ARG(IUnknown, &m_Client));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pDeskBandClient));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDeskBandClient->SetDeskBarSite(static_cast<IDeskBar*>(this));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return IUnknown_GetWindow(m_Client, &m_ClientWindow);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetClient(IUnknown **ppunkClient)
{
    if (ppunkClient == NULL)
        return E_POINTER;

    if (!m_Client)
        return E_FAIL;

    return m_Client->QueryInterface(IID_PPV_ARG(IUnknown, ppunkClient));
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnPosRectChangeDB(LPRECT prc)
{
    if (prc == NULL)
        return E_POINTER;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSite(IUnknown *pUnkSite)
{
    // Windows closes the bar if this is called when the bar is shown

    if (m_Shown)
        _CloseBar();

    m_SubMenuParent = NULL;

    m_Site = pUnkSite;

    if (m_Site)
    {
        IUnknown_QueryService(m_Site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &m_SubMenuParent));
    }
    else
    {
        SetClient(NULL);
        DestroyWindow();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetSite(REFIID riid, void **ppvSite)
{
    if (m_Site == NULL)
        return E_FAIL;

    return m_Site->QueryInterface(riid, ppvSite);
}

static void AdjustForExcludeArea(BOOL alignLeft, BOOL alignTop, BOOL preferVertical, PINT px, PINT py, INT cx, INT cy, RECTL rcExclude) {
    RECT rcWindow = { *px, *py, *px + cx, *py + cy };

    if (rcWindow.right > rcExclude.left && rcWindow.left < rcExclude.right &&
        rcWindow.bottom > rcExclude.top && rcWindow.top < rcExclude.bottom)
    {
        if (preferVertical)
        {
            if (alignTop && rcWindow.bottom > rcExclude.top)
                *py = rcExclude.top - cy;
            else if (!alignTop && rcWindow.top < rcExclude.bottom)
                *py = rcExclude.bottom;
            else if (alignLeft && rcWindow.right > rcExclude.left)
                *px = rcExclude.left - cx;
            else if (!alignLeft && rcWindow.left < rcExclude.right)
                *px = rcExclude.right;
        }
        else
        {
            if (alignLeft && rcWindow.right > rcExclude.left)
                *px = rcExclude.left - cx;
            else if (!alignLeft && rcWindow.left < rcExclude.right)
                *px = rcExclude.right;
            else if (alignTop && rcWindow.bottom > rcExclude.top)
                *py = rcExclude.top - cy;
            else if (!alignTop && rcWindow.top < rcExclude.bottom)
                *py = rcExclude.bottom;
        }
    }
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    HRESULT hr;
    CComPtr<IOleCommandTarget> oct;
    CComPtr<IInputObject> io;
    CComPtr<IDeskBand> band;
    CComPtr<IDeskBarClient> dbc;

    if (m_hWnd == NULL)
        return E_FAIL;

    hr = IUnknown_QueryService(m_Client, SID_SMenuBandChild, IID_PPV_ARG(IOleCommandTarget, &oct));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &dbc));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Windows calls this, but it appears to be unimplemented?
    hr = dbc->SetModeDBC(1);
    // Allow it to fail with E_NOTIMPL.

    // No clue about the arg, using anything != 0
    hr = dbc->UIActivateDBC(TRUE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    RECT rc = { 0 };
    hr = dbc->GetSize(0, &rc);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Unknown meaning
    const int CMD = 19;
    const int CMD_EXEC_OPT = 0;

    hr = IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    ::AdjustWindowRect(&rc, ::GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
    ::OffsetRect(&rc, -rc.left, -rc.top);

    if (m_Banner != NULL && !_IsStartPanelLayout())
    {
        BITMAP bm;
        ::GetObject(m_Banner, sizeof(bm), &bm);
        rc.right += bm.bmWidth;
    }

    if (_IsStartPanelLayout())
    {
        const STARTPANELTHEME& Theme = GetStartPanelTheme();
        rc.bottom += _GetStartPanelHeaderHeight() + _GetStartPanelFooterHeight();
        if ((rc.right - rc.left) < Theme.cxMinPanel)
            rc.right = rc.left + Theme.cxMinPanel;
        _LoadStartPanelFooterStrings();
    }

    RECT rcWorkArea;
    ::GetWindowRect(GetDesktopWindow(), &rcWorkArea);
    int cxWorkArea = rcWorkArea.right - rcWorkArea.left;
    int cyWorkArea = rcWorkArea.bottom - rcWorkArea.top;

    int x = ppt->x;
    int y = ppt->y;
    int cx = rc.right - rc.left;
    int cy = rc.bottom - rc.top;

    // TODO: Make alignLeft default to TRUE in LTR systems or whenever necessary.
    BOOL alignLeft = FALSE;
    BOOL alignTop = FALSE;
    BOOL preferVertical = FALSE;
    switch (dwFlags & MPPF_POS_MASK)
    {
    case MPPF_TOP:
        alignTop = TRUE;
        preferVertical = TRUE;
        break;
    case MPPF_LEFT:
        alignLeft = TRUE;
        break;
    case MPPF_BOTTOM:
        alignTop = FALSE;
        preferVertical = TRUE;
        break;
    case MPPF_RIGHT:
        alignLeft = FALSE;
        break;
    }

    // Try the selected alignment and verify that it doesn't escape the work area.
    if (alignLeft)
    {
        x = ppt->x - cx;
    }
    else
    {
        x = ppt->x;
    }

    if (alignTop)
    {
        y = ppt->y - cy;
    }
    else
    {
        y = ppt->y;
    }

    if (prcExclude)
        AdjustForExcludeArea(alignLeft, alignTop, preferVertical, &x, &y, cx, cy, *prcExclude);

    // Verify that it doesn't escape the work area, and flip.
    if (alignLeft)
    {
        if (x < rcWorkArea.left && (ppt->x+cx) <= rcWorkArea.right)
        {
            alignLeft = FALSE;
            if (prcExclude)
                x = prcExclude->right - ((x + cx) - prcExclude->left);
            else
                x = ppt->x;
        }
    }
    else
    {
        if ((ppt->x + cx) > rcWorkArea.right && x >= rcWorkArea.left)
        {
            alignLeft = TRUE;
            if (prcExclude)
                x = prcExclude->left - cx + (prcExclude->right - x);
            else
                x = ppt->x - cx;
        }
    }

    BOOL flipV = FALSE;
    if (alignTop)
    {
        if (y < rcWorkArea.top && (ppt->y + cy) <= rcWorkArea.bottom)
        {
            alignTop = FALSE;
            if (prcExclude)
                y = prcExclude->bottom - ((y + cy) - prcExclude->top);
            else
                y = ppt->y;

            flipV = true;
        }
    }
    else
    {
        if ((ppt->y + cy) > rcWorkArea.bottom && y >= rcWorkArea.top)
        {
            alignTop = TRUE;
            if (prcExclude)
                y = prcExclude->top - cy + (prcExclude->bottom - y);
            else
                y = ppt->y - cy;

            flipV = true;
        }
    }

    if (prcExclude)
        AdjustForExcludeArea(alignLeft, alignTop, preferVertical, &x, &y, cx, cy, *prcExclude);

    if (x < rcWorkArea.left)
        x = rcWorkArea.left;

    if (cx > cxWorkArea)
        cx = cxWorkArea;

    if (x + cx > rcWorkArea.right)
        x = rcWorkArea.right - cx;

    if (y < rcWorkArea.top)
        y = rcWorkArea.top;

    if (cy > cyWorkArea)
        cy = cyWorkArea;

    if (y + cy > rcWorkArea.bottom)
        y = rcWorkArea.bottom - cy;

    int flags = SWP_SHOWWINDOW | SWP_NOACTIVATE;

    this->SetWindowPos(HWND_TOPMOST, x, y, cx, cy, flags);

    if (flipV)
    {
        if (dwFlags & MPPF_INITIALSELECT)
            dwFlags = (dwFlags ^ MPPF_INITIALSELECT) | MPPF_FINALSELECT;
        else if (dwFlags & MPPF_FINALSELECT)
            dwFlags = (dwFlags ^ MPPF_FINALSELECT) | MPPF_INITIALSELECT;
    }

    m_ShowFlags = dwFlags;
    m_Shown = true;

    // HACK: The bar needs to be notified of the size AFTER it is shown.
    // Quick & dirty way of getting it done.
    BOOL bHandled;
    _OnSize(WM_SIZE, 0, 0, bHandled);

    UIActivateIO(TRUE, NULL);

    if (dwFlags & (MPPF_INITIALSELECT | MPPF_FINALSELECT))
    {
        const int CMD_SELECT = 5;
        int CMD_SELECT_OPTS = dwFlags & MPPF_INITIALSELECT ? 0 : -2;
        IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD_SELECT, CMD_SELECT_OPTS, NULL, NULL);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetIconSize(THIS_ DWORD iIcon)
{
    HRESULT hr;
    m_IconSize = iIcon;

    // Unknown meaning (set flags? set icon size?)
    const int CMD = 16;
    const int CMD_EXEC_OPT = iIcon ? 0 : 2; // seems to work

    hr = IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    BOOL bHandled;
    _OnSize(WM_SIZE, 0, 0, bHandled);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetIconSize(THIS_ DWORD* piIcon)
{
    if (piIcon)
        *piIcon = m_IconSize;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetBitmap(THIS_ HBITMAP hBitmap)
{
    if (m_Banner && m_Banner != hBitmap)
        ::DeleteObject(m_Banner);

    m_Banner = hBitmap;

    BOOL bHandled;
    _OnSize(WM_SIZE, 0, 0, bHandled);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetBitmap(THIS_ HBITMAP* phBitmap)
{
    if (phBitmap)
        *phBitmap = m_Banner;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    // Called by the MenuBand to assign itself as the logical child of the DeskBar

    if (fSet)
    {
        m_SubMenuChild = pmp;
    }
    else
    {
        if (m_SubMenuChild)
        {
            if (pmp == m_SubMenuChild)
            {
                m_SubMenuChild = NULL;
            }
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnSelect(DWORD dwSelectType)
{
    CComPtr<IDeskBar> safeThis = this;
    CComPtr<IMenuPopup> oldParent = m_SubMenuParent;

    TRACE("OnSelect dwSelectType=%d\n", this, dwSelectType);
    switch (dwSelectType)
    {
    case MPOS_EXECUTE:
    case MPOS_FULLCANCEL:
    case MPOS_CANCELLEVEL:

        _CloseBar();

        if (dwSelectType == MPOS_CANCELLEVEL)
            return S_OK;

    case MPOS_SELECTLEFT:
    case MPOS_SELECTRIGHT:
    case MPOS_CHILDTRACKING:
        if (oldParent)
            return oldParent->OnSelect(dwSelectType);
        break;
    }

    return S_OK;
}

HRESULT CMenuDeskBar::_CloseBar()
{
    CComPtr<IDeskBarClient> dbc;
    HRESULT hr;

    // Ensure that our data isn't destroyed while we are working
    CComPtr<IDeskBar> safeThis = this;

    m_Shown = false;

    if (m_SubMenuParent)
    {
        m_SubMenuParent->SetSubMenu(this, FALSE);
    }

    if (m_SubMenuChild)
    {
        hr = m_SubMenuChild->OnSelect(MPOS_CANCELLEVEL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &dbc));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = dbc->UIActivateDBC(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (m_hWnd)
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE);

    return UIActivateIO(FALSE, NULL);
}

BOOL CMenuDeskBar::_IsSubMenuParent(HWND hwnd)
{
    CComPtr<IMenuPopup> popup = m_SubMenuParent;

    while (popup)
    {
        HRESULT hr;
        HWND parent;

        hr = IUnknown_GetWindow(popup, &parent);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;
        if (hwnd == parent)
            return TRUE;

        hr = IUnknown_GetSite(popup, IID_PPV_ARG(IMenuPopup, &popup));
        if (FAILED(hr))
            return FALSE;
    }

    return FALSE;
}

LRESULT CMenuDeskBar::_OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (m_Client)
    {
        RECT rc;

        GetClientRect(&rc);

        if (_IsStartPanelLayout())
        {
            _GetStartPanelRects(&m_StartPanelRects);
            rc = m_StartPanelRects.rcContent;
        }
        else if (m_Banner && m_IconSize != BMICON_SMALL)
        {
            BITMAP bm;
            ::GetObject(m_Banner, sizeof(bm), &bm);
            rc.left += bm.bmWidth;
        }

        ::SetWindowPos(m_ClientWindow, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
    }

    return 0;
}

LRESULT CMenuDeskBar::_OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!m_Client)
        return 0;

    CComPtr<IWinEventHandler> winEventHandler;
    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    if (winEventHandler)
    {
        LRESULT result;
        hr = winEventHandler->OnWinEvent(NULL, uMsg, wParam, lParam, &result);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;
        return result;
    }

    return 0;
}

LRESULT CMenuDeskBar::_OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;

    if (_IsStartPanelLayout())
    {
        const STARTPANELTHEME& Theme = GetStartPanelTheme();
        RECT rcHeader, rcText;
        bHandled = TRUE;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);
        HBRUSH hbrHeader = CreateSolidBrush(Theme.crHeaderDark);
        HBRUSH hbrAccent = CreateSolidBrush(Theme.crHeaderLight);
        HBRUSH hbrFooter = CreateSolidBrush(Theme.crFooterDark);
        HBRUSH hbrFooterLight = CreateSolidBrush(Theme.crFooterLight);
        HBRUSH hbrBorder = CreateSolidBrush(Theme.crBorder);
        HBRUSH hbrButtonEdge = CreateSolidBrush(Theme.crFooterButtonEdge);
        HFONT hFont = NULL, hFontOld;
        LOGFONTW lf;
        HFONT hFooterOld;
        HICON hIcon;

        _GetStartPanelRects(&m_StartPanelRects);

        rcHeader = m_StartPanelRects.rcHeader;
        FillRect(hdc, &rcHeader, hbrHeader);

        rcText = m_StartPanelRects.rcUserText;

        RECT rcAccent = rcHeader;
        rcAccent.left = max(rcAccent.left, rcAccent.right - 72);
        FillRect(hdc, &rcAccent, hbrAccent);

        FillRect(hdc, &m_StartPanelRects.rcFooter, hbrFooter);

        RECT rcLine = rcHeader;
        rcLine.top = rcLine.bottom - 1;
        FillRect(hdc, &rcLine, hbrBorder);
        rcLine = m_StartPanelRects.rcFooter;
        rcLine.bottom = rcLine.top + 1;
        FillRect(hdc, &rcLine, hbrFooterLight);

        if (!m_szUserName[0])
            GetStartPanelUserName(m_szUserName, _countof(m_szUserName));

        if (GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf) == sizeof(lf))
        {
            lf.lfWeight = FW_BOLD;
            lf.lfHeight = -15;
            hFont = CreateFontIndirectW(&lf);
        }

        hFontOld = (HFONT)SelectObject(hdc, hFont ? hFont : GetStockObject(DEFAULT_GUI_FONT));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextW(hdc, m_szUserName, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, hFontOld);

        hIcon = (HICON)LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_USERS2), IMAGE_ICON,
                                  Theme.cxAvatar, Theme.cyAvatar, LR_SHARED);
        if (hIcon)
        {
            DrawIconEx(hdc, m_StartPanelRects.rcAvatar.left, m_StartPanelRects.rcAvatar.top,
                       hIcon, Theme.cxAvatar, Theme.cyAvatar, 0, NULL, DI_NORMAL);
        }

        if (GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf) == sizeof(lf))
        {
            lf.lfWeight = FW_NORMAL;
            lf.lfHeight = -12;
            DeleteObject(hFont);
            hFont = CreateFontIndirectW(&lf);
        }

        hFooterOld = (HFONT)SelectObject(hdc, hFont ? hFont : GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, Theme.crFooterText);
        SetBkMode(hdc, TRANSPARENT);

        {
            RECT rcButtonText = m_StartPanelRects.rcLogOffButton;
            HBRUSH hbrButton = CreateSolidBrush(m_StartPanelHotButton == STARTPANEL_HIT_LOGOFF ?
                                                Theme.crFooterButtonHot : Theme.crFooterButton);
            HICON hButtonIcon = (HICON)LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_LOGOFF1),
                                                  IMAGE_ICON, 16, 16, LR_SHARED);
            FillRect(hdc, &m_StartPanelRects.rcLogOffButton, hbrButton);
            FrameRect(hdc, &m_StartPanelRects.rcLogOffButton, hbrButtonEdge);
            rcButtonText.left += 8;
            if (hButtonIcon)
            {
                INT yIcon = m_StartPanelRects.rcLogOffButton.top +
                            ((m_StartPanelRects.rcLogOffButton.bottom - m_StartPanelRects.rcLogOffButton.top) - 16) / 2;
                DrawIconEx(hdc, m_StartPanelRects.rcLogOffButton.left + 6, yIcon, hButtonIcon, 16, 16, 0, NULL, DI_NORMAL);
                rcButtonText.left += 18;
            }
            rcButtonText.right -= 6;
            DrawTextW(hdc, m_szLogOff[0] ? m_szLogOff : L"Log Off", -1, &rcButtonText,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            DeleteObject(hbrButton);
        }

        {
            RECT rcButtonText = m_StartPanelRects.rcTurnOffButton;
            HBRUSH hbrButton = CreateSolidBrush(m_StartPanelHotButton == STARTPANEL_HIT_SHUTDOWN ?
                                                Theme.crFooterButtonHot : Theme.crFooterButton);
            HICON hButtonIcon = (HICON)LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_TURN_OFF),
                                                  IMAGE_ICON, 16, 16, LR_SHARED);
            FillRect(hdc, &m_StartPanelRects.rcTurnOffButton, hbrButton);
            FrameRect(hdc, &m_StartPanelRects.rcTurnOffButton, hbrButtonEdge);
            rcButtonText.left += 8;
            if (hButtonIcon)
            {
                INT yIcon = m_StartPanelRects.rcTurnOffButton.top +
                            ((m_StartPanelRects.rcTurnOffButton.bottom - m_StartPanelRects.rcTurnOffButton.top) - 16) / 2;
                DrawIconEx(hdc, m_StartPanelRects.rcTurnOffButton.left + 6, yIcon, hButtonIcon, 16, 16, 0, NULL, DI_NORMAL);
                rcButtonText.left += 18;
            }
            rcButtonText.right -= 6;
            DrawTextW(hdc, m_szShutdown[0] ? m_szShutdown : L"Turn Off Computer", -1, &rcButtonText,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            DeleteObject(hbrButton);
        }

        SelectObject(hdc, hFooterOld);

        if (hFont)
            DeleteObject(hFont);
        DeleteObject(hbrButtonEdge);
        DeleteObject(hbrBorder);
        DeleteObject(hbrFooterLight);
        DeleteObject(hbrFooter);
        DeleteObject(hbrAccent);
        DeleteObject(hbrHeader);
        EndPaint(&ps);
        return TRUE;
    }

    if (m_Banner && !m_IconSize)
    {
        BITMAP bm;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);

        HDC hdcMem = ::CreateCompatibleDC(hdc);
        HGDIOBJ hbmOld = ::SelectObject(hdcMem, m_Banner);

        ::GetObject(m_Banner, sizeof(bm), &bm);

        RECT rc;
        if (!GetClientRect(&rc))
            WARN("GetClientRect failed\n");

        const int bx = bm.bmWidth;
        const int by = bm.bmHeight;
        const int cy = rc.bottom;

        TRACE("Painting banner: %d by %d\n", bm.bmWidth, bm.bmHeight);

        if (!::StretchBlt(hdc, 0, 0, bx, cy - by, hdcMem, 0, 0, bx, 1, SRCCOPY))
            WARN("StretchBlt failed\n");

        if (!::BitBlt(hdc, 0, cy - by, bx, by, hdcMem, 0, 0, SRCCOPY))
            WARN("BitBlt failed\n");

        ::SelectObject(hdcMem, hbmOld);
        ::DeleteDC(hdcMem);

        EndPaint(&ps);
    }

    return TRUE;
}

LRESULT CMenuDeskBar::_OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // BUG in ReactOS: WM_ACTIVATE/WA_INACTIVE makes no sense with lParam==hWnd
    if (LOWORD(wParam) != 0 || reinterpret_cast<HWND>(lParam) == m_hWnd)
    {
        return 0;
    }

    // HACK! I just want it to work !!!
    CComPtr<IDeskBar> db;
    HRESULT hr = IUnknown_QueryService(m_Client, SID_SMenuBandChild, IID_PPV_ARG(IDeskBar, &db));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    CComPtr<IUnknown> punk;

    hr = db->GetClient(&punk);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    if (!punk && m_Shown)
    {
        if (!_IsSubMenuParent(reinterpret_cast<HWND>(lParam)))
        {
            OnSelect(MPOS_FULLCANCEL);
        }
    }

    return 0;
}

LRESULT CMenuDeskBar::_OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return MA_NOACTIVATE;
}

LRESULT CMenuDeskBar::_OnAppActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
#if 0
    if (wParam == 0 && m_Shown)
    {
        OnSelect(MPOS_FULLCANCEL);
    }
#endif
    return 0;
}

LRESULT CMenuDeskBar::_OnWinIniChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (wParam == SPI_SETFLATMENU)
        return _OnNotify(uMsg, wParam, lParam, bHandled);

    return 0;
}

LRESULT CMenuDeskBar::_OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    /* If it is a flat style menu we need to handle WM_NCPAINT
     * and paint the border with the right colour */
    if ((GetStyle() & WS_BORDER) == 0)
    {
        /* This isn't a flat style menu. */
        bHandled = FALSE;
        return 0;
    }

    HDC hdc;
    RECT rcWindow;

    hdc = GetWindowDC();
    GetWindowRect(&rcWindow);
    OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
    FrameRect(hdc, &rcWindow, GetSysColorBrush(COLOR_BTNSHADOW));
    ReleaseDC(hdc);
    return 0;
}

LRESULT CMenuDeskBar::_OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    /* Prevent the CMenuDeskBar from destroying on being sent a WM_CLOSE */
    return 0;
}

LRESULT CMenuDeskBar::_OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (_IsStartPanelLayout())
    {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hWnd, 0 };
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

        if (!m_StartPanelTrackingMouse)
        {
            TrackMouseEvent(&tme);
            m_StartPanelTrackingMouse = TRUE;
        }

        _SetStartPanelHotButton(_StartPanelHitTestFooterButton(pt));
    }

    bHandled = FALSE;
    return 0;
}

LRESULT CMenuDeskBar::_OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    m_StartPanelTrackingMouse = FALSE;
    _SetStartPanelHotButton(STARTPANEL_HIT_NONE);
    return 0;
}

LRESULT CMenuDeskBar::_OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    INT iHit = _StartPanelHitTestFooterButton(pt);

    switch (iHit)
    {
    case STARTPANEL_HIT_LOGOFF:
        _ExecuteStartPanelFooterCommand(IDM_LOGOFF);
        return 0;
    case STARTPANEL_HIT_SHUTDOWN:
        _ExecuteStartPanelFooterCommand(IDM_SHUTDOWN);
        return 0;
    default:
        break;
    }

    bHandled = FALSE;
    return 0;
}

HRESULT CMenuDeskBar::_AdjustForTheme(BOOL bFlatStyle)
{
    DWORD style = bFlatStyle ? WS_BORDER : WS_CLIPCHILDREN|WS_DLGFRAME;
    DWORD mask = WS_BORDER|WS_CLIPCHILDREN|WS_DLGFRAME;
    SHSetWindowBits(m_hWnd, GWL_STYLE, mask, style);
    return S_OK;
}

extern "C"
HRESULT WINAPI RSHELL_CMenuDeskBar_CreateInstance(REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreator<CMenuDeskBar>(riid, ppv);
}
