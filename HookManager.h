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

#include "TabConfig.h"

#include <vector>
#include "tlister.h"

class TabWindow;

class HookManager
{
public:
    typedef std::vector<TabWindow*> WindowCont;

public:
    static HookManager& instance();
    ~HookManager();

    void AddTab(HWND window);
    void DestroyWindow(TabWindow* tabWindow);
    void SetLastUsedWindow(TabWindow* tabWindow);
    
    WindowCont& windows();
    const TabConfig& config() const;

    HWND GetFreeWindow() const;
    void SetFreeWindow(HWND hWnd);

private:
    HookManager();

    TabWindow* CreateNewWindow(HWND hWndParent);
    void CreateTabFont();
    void AddNewChild(HWND hWndChild);

    TabWindow* FindParentTabWindow(HWND hWndChild) const;
    TabWindow* FindTabWindow(HWND hWnd) const;

    // event handlers
    void OnWindowDestroyed(HWND hWnd);

    // calbacks
    static LRESULT CALLBACK HookCallWndProcRetProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK HookGetMessageProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    TabConfig m_config;
    HFONT m_hTabFont = NULL;
    HHOOK m_hkGetMessage = NULL;
    HHOOK m_hkCallWndProcRet = NULL;
    HWND m_hWndFree = NULL;
    WindowCont m_windows;
    TabWindow* m_wndLastUsed = nullptr;
};
