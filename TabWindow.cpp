/*
    tLister add tabs support to Lister.
    Copyright (C) 2011 Egor Vlaznev
    Copyright (C) 2019 Aleksei Ilin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "HookManager.h"
#include "TabConfig.h"
#include "TabWindow.h"

#include <CommCtrl.h>

static bool checkmod(int mod) {
    return mod == ((GetKeyState(VK_MENU) < 0) ? 1 : 0) + 
                  ((GetKeyState(VK_CONTROL) < 0) ? 2 : 0) + 
                  ((GetKeyState(VK_SHIFT) < 0) ? 4 : 0);
}

static TCHAR* GetName(TCHAR *title) 
{
    TCHAR *name;
    if (TCHAR* pos = _tcsrchr(title, _T('\\'))) {
        name = pos + 1;
    }
    else {
        name = title;
    }
    if (TCHAR* pos = _tcsrchr(name, _T('['))) {
        name = pos + 1;
    }
    if (TCHAR* pos = _tcsrchr(name, _T(']'))) {
        *pos = 0;
    }
    size_t name_len = _tcslen(name);
    for (size_t i = 0; i < name_len; ++i) {
        if (name[i] == _T('&')) {
            TCHAR* nf = &(name[i]);
            size_t len = _tcslen(nf);
            for (size_t j = len; j > 0; j--) {
                nf[j] = nf[j - 1];
            }
            nf[0] = _T('&');
            nf[len + 1] = 0;
            i++;
        }
    }
    return name;
}

TabWindow::TabWindow(HWND hWndParent, HWND hWndFree, HFONT hTabFont)
{
    CreateMainWindow(hWndParent);
    CreateTabControlWindow(hTabFont);
    
    AddTab(hWndFree);
    AddTab(hWndParent);
}

TabWindow::~TabWindow()
{
    KillTimer(m_hWnd, TimerEventEnslave);

    // deactivate callbacks
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)NULL);
    SetWindowLongPtr(m_hWndTabCtrl, GWLP_USERDATA, (LONG_PTR)NULL);

    DestroyWindow(m_hWndTabCtrl);
    TL_EXPECT(SetMenu(m_hWnd, NULL));
    DestroyWindow(m_hWnd);
}

void TabWindow::AddTab(HWND hWnd)
{
    if (hWnd && IsWindow(hWnd)) {
        m_toBeEnslaved.push_back(hWnd);
        SetTimer(m_hWnd, TimerEventEnslave, 20, NULL);
    }
}

void TabWindow::CreateMainWindow(HWND hWndParent)
{
    RECT r;
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = (HICON)SendMessage(hWndParent, WM_GETICON, ICON_BIG, 0);
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpfnWndProc = &TabWindow::WindowProc;
    wc.lpszClassName = _T("TabListerMain");
    RegisterClass(&wc);
    GetWindowRect(hWndParent, &r);
    m_hWnd = CreateWindow(_T("TabListerMain"),
        NULL,
        WS_POPUP | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | (HookManager::instance().config().ismax ? WS_MAXIMIZE : 0),
        r.left, r.top, r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);

    // keep the pointer to this into GWLP_USERDATA
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
}

void TabWindow::CreateTabControlWindow(HFONT hTabFont)
{
    const TabConfig& config = HookManager::instance().config();

    RECT r;
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);
    GetClientRect(m_hWnd, &r);
    m_hWndTabCtrl = CreateWindow(WC_TABCONTROL, _T(""),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_FOCUSNEVER | 
            (config.fixedwidth ? TCS_FIXEDWIDTH : 0) | TCS_HOTTRACK | TCS_FORCELABELLEFT | (config.multiline ? TCS_MULTILINE : 0), 
        0, 0, r.right, r.bottom, 
        m_hWnd, NULL, hInst, NULL);
    
    // keep the pointer to this into GWLP_USERDATA
    SetWindowLongPtr(m_hWndTabCtrl, GWLP_USERDATA, (LONG_PTR)this);

    m_oldTabControlWndProc = (WNDPROC)SetWindowLongPtr(m_hWndTabCtrl, GWLP_WNDPROC, (LONG_PTR)&TabWindow::TabControlWindowProc);
    SendMessage(m_hWndTabCtrl, WM_SETFONT, (WPARAM)(hTabFont ? hTabFont : GetStockObject(DEFAULT_GUI_FONT)), (LPARAM)true);
    SendMessage(m_hWndTabCtrl, TCM_SETMINTABWIDTH, 0, config.minwidth);
    SendMessage(m_hWndTabCtrl, TCM_SETITEMSIZE, 0, config.minwidth);
}

LRESULT TabWindow::OnWindowMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    const TabConfig& config = HookManager::instance().config();

    switch (nMsg) {
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
            HookManager::instance().SetLastUsedWindow(this);
        }
        break;
    case WM_CLOSE:
        if (m_vecChldrn.size() > 0) {
            int nc = IDOK;
            if (m_vecChldrn.size() > 1 && config.warnonclose) {
                nc = MessageBox(m_hWnd, config.onclosew, _T("tLister"), MB_OKCANCEL | MB_ICONQUESTION);
            }
            if (nc == IDOK) {
                for (const ChildWin& child : m_vecChldrn) {
                    PostMessage(child.hWnd, WM_CLOSE, 0, 0);
                }
            }
            return 0;

        }
        break;
    case WM_COMMAND:
        if (m_vecChldrn.size() > 0) {
            LRESULT curtab = SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0);
            HWND hWndCurrTab = m_vecChldrn[curtab].hWnd;
            if (wParam == 283) {
                SendMessage(m_hWnd, WM_SETREDRAW, false, 0);
                if (IsZoomed(m_hWnd)) {
                    ShowWindow(hWndCurrTab, SW_MAXIMIZE);
                    PostMessage(hWndCurrTab, nMsg, wParam, lParam);
                    ShowWindow(hWndCurrTab, SW_RESTORE);
                } 
                else {
                    RECT r;
                    GetWindowRect(m_hWnd, &r);
                    ShowWindow(hWndCurrTab, SW_RESTORE);
                    MoveWindow(hWndCurrTab, r.left, r.top, r.right - r.left, r.bottom - r.top, true);
                    PostMessage(hWndCurrTab, nMsg, wParam, lParam);
                }
                Resize(hWndCurrTab);
                SendMessage(m_hWnd, WM_SETREDRAW, true, 0);
            }
            else {
                PostMessage(hWndCurrTab, nMsg, wParam, lParam);
            }
            UpdateWindow(m_hWnd);
            DrawMenuBar(m_hWnd);
        }
        break;
    case WM_SETFOCUS:
        if (m_vecChldrn.size() > 0) {
            LRESULT curtab = SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0);
            HWND hWndCurrTab = m_vecChldrn[curtab].hWnd;
            PostMessage(hWndCurrTab, WM_LBUTTONDOWN, 0, 0);
            PostMessage(hWndCurrTab, WM_LBUTTONUP, 0, 0);
            SetFocus(hWndCurrTab);
        }
        break;
    case WM_SIZE:
        if (m_vecChldrn.size() > 0) {
            MoveWindow(m_hWndTabCtrl, 0, 0, LOWORD(lParam), HIWORD(lParam), true);
            Resize(m_vecChldrn[SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0)].hWnd);
            return 0;
        }
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case TCN_KEYDOWN:
            if (m_vecChldrn.size() > 0) {
                PostMessage(m_vecChldrn[SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0)].hWnd,
                    WM_KEYDOWN,
                    (WPARAM)(((NMTCKEYDOWN*)lParam)->wVKey),
                    (LPARAM)(((NMTCKEYDOWN*)lParam)->flags));
            }
            return 0;
        case TCN_SELCHANGE: {
            LRESULT index = SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0);
            if (index >= 0 && static_cast<size_t>(index) < m_vecChldrn.size()) {
                ChangeTab(index);
            }
            }
            break;
        }
        break;
    case WM_PARENTNOTIFY:
        switch (LOWORD(wParam)) {
        case WM_DESTROY:
            if (m_vecChldrn.size() > 0) {
                DelTab((HWND)lParam);
            }
            if (m_vecChldrn.size() == 0) {
                HookManager::instance().DestroyWindow(this);
            }
        }
        break;
    case WM_TIMER:
        switch (wParam) {
        case TimerEventEnslave:
            OnTimerEventEnslave();
            break;
        }
        break;
    }

    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

LRESULT TabWindow::OnTabControlWindowMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg) {
    case WM_MBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        TCHITTESTINFO ht;
        ht.pt.x = LOWORD(lParam);
        ht.pt.y = HIWORD(lParam);
        ht.flags = TCHT_ONITEM;
        LRESULT i = SendMessage(m_hWndTabCtrl, TCM_HITTEST, 0, (LPARAM)&ht);
        if (i != -1) {
            PostMessage(m_vecChldrn[i].hWnd, WM_CLOSE, 0, 0);
        }
        return 0;
        break;
    }
    case WM_RBUTTONDBLCLK:
        TCHITTESTINFO ht;
        ht.pt.x = LOWORD(lParam);
        ht.pt.y = HIWORD(lParam);
        ht.flags = TCHT_ONITEM;
        LRESULT i = SendMessage(m_hWndTabCtrl, TCM_HITTEST, 0, (LPARAM)&ht);
        if (i != -1 && IsWindow(m_vecChldrn[i].hWnd)) {
            HWND win = m_vecChldrn[i].hWnd;
            HMENU menu = m_vecChldrn[i].hMenu;
            Unslave(win, menu);
            return 0;
        }
        break;
    }

    return CallWindowProc(m_oldTabControlWndProc, hWnd, nMsg, wParam, lParam);
}

LRESULT TabWindow::WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    TabWindow* tabWindow = (TabWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (tabWindow) {
        return tabWindow->OnWindowMessage(hWnd, nMsg, wParam, lParam);
    }
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

LRESULT TabWindow::TabControlWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    TabWindow* tabWindow = (TabWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (tabWindow) {
        return tabWindow->OnTabControlWindowMessage(hWnd, nMsg, wParam, lParam);
    }
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

LRESULT TabWindow::HookGetMessageProc(WPARAM wParam, MSG* msg)
{
    if (msg->message == WM_KEYDOWN && GetForegroundWindow() == m_hWnd) {
        const TabConfig& config = HookManager::instance().config();

        if (msg->wParam == config.nexttab && checkmod(config.nexttabmod)) {
            LRESULT curtab = SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0);
            SendMessage(m_hWndTabCtrl, TCM_SETCURFOCUS, (curtab == m_vecChldrn.size() - 1) ? 0 : curtab + 1, 0);
            return 1;
        }

        if (msg->wParam == config.previoustab && checkmod(config.previoustabmod)) {
            LRESULT curtab = SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0);
            SendMessage(m_hWndTabCtrl, TCM_SETCURFOCUS, (curtab == 0) ? m_vecChldrn.size() - 1 : curtab - 1, 0);
            return 1;
        }

        if (msg->wParam == config.detachtab && checkmod(config.detachtabmod)) {
            LRESULT curtab = SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0);
            Unslave(m_vecChldrn[curtab].hWnd, m_vecChldrn[curtab].hMenu);
            return 1;
        }

        if (msg->wParam == VK_F11) {
            ShowWindow(m_hWnd, IsZoomed(m_hWnd) ? SW_RESTORE : SW_MAXIMIZE);
            return 1;
        }

        if (msg->wParam == config.closealltab && checkmod(config.closealltabmod)) {
            PostMessage(m_hWnd, WM_CLOSE, 0, 0);
            return 1;
        }
    }

    return 0;
}

void TabWindow::HookCallWndProcRetProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT lResult)
{
    if (nMsg == WM_SETTEXT /*&&GetForegroundWindow()==TabWindow*/ 
        && (hWnd != m_hWnd)) {
        int wn = FindChildWindow(hWnd);
        if (wn != -1) {
            TCHAR title[1000] = _T("");
            GetWindowText(hWnd, title, 1000);
            if (_tcsrchr(title, _T('['))) {
                SetWindowText(m_hWnd, title);
                TCHAR* name = GetName(title);
                TCITEM tie;
                tie.mask = TCIF_TEXT;
                tie.pszText = name;
                SendMessage(m_hWndTabCtrl, TCM_SETITEM, wn, (LPARAM)&tie);
                DrawMenuBar(m_hWnd);
            }
        }
    }
}

void TabWindow::DelTab(HWND hWndChld) 
{
    if (!m_vecChldrn.size()) {
        return;
    }

    const TabConfig& config = HookManager::instance().config();

    LRESULT curtab = SendMessage(m_hWndTabCtrl, TCM_GETCURFOCUS, 0, 0);
    int tabnum = FindChildWindow(hWndChld);
    if (tabnum != -1) {
        SendMessage(m_hWndTabCtrl, TCM_DELETEITEM, tabnum, 0);
        m_vecChldrn.erase(m_vecChldrn.begin() + tabnum);
    }

    if (m_vecChldrn.size() > 0) {
        if (m_vecChldrn.size() == 1 && (config.AlwaysShowTab == 0)) {
            HookManager::instance().SetFreeWindow(m_vecChldrn[0].hWnd);
            Unslave(m_vecChldrn[0].hWnd, m_vecChldrn[0].hMenu);
        }
        else {
            if (tabnum == curtab) {
                SendMessage(m_hWndTabCtrl, TCM_SETCURFOCUS, tabnum == 0 ? 0 : (tabnum - 1), 0);
            }
            else {
                Resize(m_vecChldrn[curtab].hWnd);
            }
        }
    }
}

HWND TabWindow::hWnd() const
{
    return m_hWnd;
}

bool TabWindow::isChild(HWND hWnd) const
{
    return FindChildWindow(hWnd) != -1;
}

size_t TabWindow::ChildCount() const
{
    return m_vecChldrn.size();
}

void TabWindow::ChangeTab(size_t index)
{
    TL_EXPECT(SetMenu(m_hWnd, m_vecChldrn[index].hMenu));
    DrawMenuBar(m_hWnd);

    for (size_t i = 0; i < m_vecChldrn.size(); i++) {
        ShowWindow(m_vecChldrn[i].hWnd, (i == index) ? SW_SHOW : SW_HIDE);
    }

    HWND hWndCurTab = m_vecChldrn[index].hWnd;
    Resize(hWndCurTab);

    TCHAR title[1000] = _T("");
    GetWindowText(hWndCurTab, title, 1000);
    SetWindowText(m_hWnd, title);

    PostMessage(hWndCurTab, WM_LBUTTONDOWN, 0, 0);
    PostMessage(hWndCurTab, WM_LBUTTONUP, 0, 0);

    if (HWND swnd = FindWindowEx(hWndCurTab, NULL, NULL, NULL)) {
        SetFocus(swnd);
    }
    else {
        SetFocus(hWndCurTab);
    }
}

void TabWindow::Resize(HWND hWndChld)
{
    const TabConfig& config = HookManager::instance().config();
    RECT r, r1;
    GetClientRect(m_hWnd, &r1);
    if (m_vecChldrn.size() > 1 || config.AlwaysShowTab) {
        SetRect(&r, 0, 0, r1.right, r1.bottom);
        SendMessage(m_hWndTabCtrl, TCM_ADJUSTRECT, false, (LPARAM)&r);
        TL_EXPECT(MoveWindow(hWndChld, r.left, r.top, r.right - r.left, r.bottom - r.top, true));
    }
    else {
        TL_EXPECT(MoveWindow(hWndChld, r1.left, r1.top, r1.right - r1.left, r1.bottom - r1.top, true));
    }
}

bool TabWindow::Enslave(HWND window) 
{
    TCHAR title[1000] = _T("");
    if (GetWindowText(window, title, 1000) < 10) {
        return false;
    }

    int wn = FindChildWindow(window);
    if (wn == -1) {
        TCHAR* name = GetName(title);
        TCITEM tie;
        tie.mask = TCIF_TEXT;
        tie.pszText = name;

        m_vecChldrn.push_back({ window, GetMenu(window) });
        SendMessage(m_hWndTabCtrl, TCM_INSERTITEM, m_vecChldrn.size(), (LPARAM)&tie);
    }

    TL_EXPECT(SetParent((HWND)window, m_hWnd));
    LONG_PTR style = GetWindowLongPtr((HWND)window, GWL_STYLE);
    SetWindowLongPtr((HWND)window, GWL_STYLE, (style&(~(WS_OVERLAPPEDWINDOW|WS_POPUP))) | WS_CHILD);
    SetWindowPos((HWND)window, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);

    if (wn == -1) {
        SendMessage(m_hWndTabCtrl, TCM_SETCURFOCUS, m_vecChldrn.size() - 1, 0);
    }

    ChangeTab(m_vecChldrn.size() - 1);
    ShowWindow(m_hWnd, SW_SHOW);
    RedrawWindow(m_hWnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

    SwitchToThisWindow(m_hWnd, true);
    return true;
}

int TabWindow::FindChildWindow(HWND hWndChild) const
{
    int wn = -1;
    for (size_t i = 0; i < m_vecChldrn.size(); i++) {
        if (m_vecChldrn[i].hWnd == hWndChild) {
            wn = static_cast<int>(i);
            break;
        }
    }
    return wn;
}

void TabWindow::OnTimerEventEnslave()
{
    std::vector<HWND> toBeEnslaved;
    std::swap(toBeEnslaved, m_toBeEnslaved);
    for (HWND hWnd : toBeEnslaved) {
        if (IsWindow(hWnd)) {
            if (!Enslave(hWnd)) {
                m_toBeEnslaved.push_back(hWnd);
            }
        }
        else {
            SendMessage(m_hWnd, WM_PARENTNOTIFY, MAKEWPARAM(WM_DESTROY, 0), LPARAM(hWnd));
        }
    }

    if (m_toBeEnslaved.size() == 0) {
        KillTimer(m_hWnd, TimerEventEnslave);
        if (m_vecChldrn.size() == 0) {
            SendMessage(m_hWnd, WM_PARENTNOTIFY, MAKEWPARAM(WM_DESTROY, 0), 0);
        }
    }
}

void TabWindow::Unslave(HWND win, HMENU menu)
{
    RECT r;
    GetWindowRect(m_hWnd, &r);
    ShowWindow(win, SW_HIDE);
    LONG_PTR style = GetWindowLongPtr(win, GWL_STYLE);
    SetWindowLongPtr(win, GWL_STYLE, (style | WS_OVERLAPPEDWINDOW | WS_POPUP)&(~WS_CHILD));
    TL_EXPECT(SetMenu(win, menu));
    TL_EXPECT(SetParent(win, NULL));
    DelTab(win);
    SetWindowPos(win, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED);
    SwitchToThisWindow(win, true);
    ShowWindow(win, SW_SHOW);
    SetFocus(win);
}
