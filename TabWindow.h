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

#pragma once

#include "tlister.h"
#include <vector>

class TabWindow
{
    enum TimerEvent {
        TimerEventEnslave = 1
    };

    struct ChildWin
    {
        HWND hWnd;
        HMENU hMenu;
    };

public:
    TabWindow(HWND hWndParent, HWND hWndFree, HFONT hTabFont);
    ~TabWindow();

    void AddTab(HWND hWnd);

    HWND hWnd() const;
    bool isChild(HWND hWnd) const;
    size_t ChildCount() const;

    // callbacks
    LRESULT HookGetMessageProc(WPARAM wParam, MSG* msg);
    void HookCallWndProcRetProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT lResult);

private:
    void CreateMainWindow(HWND hWndParent);
    void CreateTabControlWindow(HFONT hTabFont);

    void ChangeTab(size_t index);
    void Resize(HWND hWndChld);
    void DelTab(HWND hWndChld);
    void Unslave(HWND hWnd, HMENU hMenu);
    bool Enslave(HWND hWnd);

    int FindChildWindow(HWND hWndChild) const;

    // event handlers
    void OnTimerEventEnslave();
    LRESULT OnWindowMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnTabControlWindowMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

    // callbacks
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK TabControlWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hWnd = NULL;
    HWND m_hWndTabCtrl = NULL;
    WNDPROC m_oldTabControlWndProc = NULL;
    std::vector<ChildWin> m_vecChldrn;
    std::vector<HWND> m_toBeEnslaved;
};
