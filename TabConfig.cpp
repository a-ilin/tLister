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

#include "TabConfig.h"

#include <stdio.h>

#define MAXWIN 300

void TabConfig::parse()
{
    TCHAR iniismax[10] = _T("");
    TCHAR path[MAX_PATH] = _T("");
    GetEnvironmentVariable(_T("COMMANDER_INI"), path, MAX_PATH);
    GetPrivateProfileString(_T("Lister"), _T("Maximized"), _T(""), iniismax, 10, path);
    GetModuleFileName(hInst, path, MAX_PATH);
    if (TCHAR* pos = _tcsrchr(path, _T('\\'))) {
        *pos = _T('\0');
    }
    _tcscat_s(path, MAX_PATH, _T("\\tlister.ini"));
    if (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) {
        GetEnvironmentVariable(_T("COMMANDER_INI"), path, MAX_PATH);
        if (TCHAR* pos = _tcsrchr(path, _T('\\'))) {
            *pos = _T('\0');
        }
        _tcscat_s(path, MAX_PATH, _T("\\tlister.ini"));
    }

    TCHAR inimultiline[10], inifixedwidth[10], iniminwidth[10], ininexttabmod[10], ininexttab[10], iniprevioustabmod[10], iniprevioustab[10],
        iniast[10], iniwarnonclose[10], inidetachtab[10], inidetachtabmod[10], iniclosealltab[10], iniclosealltabmod[10], inimaxwincount[10];
    
    if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
        GetPrivateProfileString(_T("tlister"), _T("multiline"), _T("0"), inimultiline, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("fixedwidth"), _T("1"), inifixedwidth, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("minwidth"), _T("100"), iniminwidth, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("alwaysshowtab"), _T("0"), iniast, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("nexttabmod"), _T(""), ininexttabmod, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("nexttab"), _T(""), ininexttab, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("previoustabmod"), _T(""), iniprevioustabmod, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("previoustab"), _T(""), iniprevioustab, 10, path);

        GetPrivateProfileString(_T("tlister"), _T("detachtab"), _T(""), inidetachtab, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("detachtabmod"), _T(""), inidetachtabmod, 10, path);

        GetPrivateProfileString(_T("tlister"), _T("closealltab"), _T(""), iniclosealltab, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("closealltabmod"), _T(""), iniclosealltabmod, 10, path);

        GetPrivateProfileString(_T("tlister"), _T("warnonclose"), _T("0"), iniwarnonclose, 10, path);
        GetPrivateProfileString(_T("tlister"), _T("onclosew"), _T("Close all tabs?"), onclosew, 100, path);

        GetPrivateProfileString(_T("tlister"), _T("maxwincount"), _T(""), inimaxwincount, 10, path);
    }

    if (_tcscmp(iniwarnonclose, _T("1")) == 0) {
        warnonclose = 1;
    }
    int a, b;
    a = _stscanf_s(ininexttabmod, _T("%X"), &nexttabmod);
    b = _stscanf_s(ininexttab, _T("%X"), &nexttab);
    if (a != 1 || b != 1) {
        nexttabmod = 0x2;
        nexttab = 0xC0;
    }
    a = _stscanf_s(iniprevioustabmod, _T("%X"), &previoustabmod);
    b = _stscanf_s(iniprevioustab, _T("%X"), &previoustab);
    if (a != 1 || b != 1) {
        previoustabmod = 0x1;
        previoustab = 0xC0;
    }

    a = _stscanf_s(iniclosealltabmod, _T("%X"), &closealltabmod);
    b = _stscanf_s(iniclosealltab, _T("%X"), &closealltab);
    if (a != 1 || b != 1) {
        closealltabmod = 0x2;
        closealltab = VK_CANCEL;
    }

    a = _stscanf_s(inidetachtabmod, _T("%X"), &detachtabmod);
    b = _stscanf_s(inidetachtab, _T("%X"), &detachtab);
    if (a != 1 || b != 1) {
        detachtabmod = 0x4;
        detachtab = VK_ESCAPE;
    }
    ismax = (_tcscmp(iniismax, _T("1")) == 0) ? 1 : 0;
    fixedwidth = (_tcscmp(inifixedwidth, _T("1")) == 0) ? 1 : 0;
    minwidth = _ttoi(iniminwidth);
    AlwaysShowTab = _ttoi(iniast);
    multiline = (_tcscmp(inimultiline, _T("1")) == 0) ? 1 : 0;
    
    MaxWinCount = max(0, min(_ttoi(inimaxwincount), MAXWIN));
    if (MaxWinCount == 0) {
        MaxWinCount = MAXWIN;
    }
}
