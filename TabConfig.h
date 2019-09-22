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

struct TabConfig
{
    void parse();

    TCHAR onclosew[100] = _T("");
    int warnonclose = 0;
    int nexttabmod = 0;
    int nexttab = 0;
    int previoustabmod = 0;
    int previoustab = 0;
    int detachtabmod = 0;
    int detachtab = 0;
    int closealltabmod = 0;
    int closealltab = 0;
    int ismax = 0;
    int fixedwidth = 0;
    int minwidth = 0;
    int multiline = 0;
    int AlwaysShowTab = 0;
    int MaxWinCount = 0;
};

