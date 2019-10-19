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

#include <Windows.h>
#include <tchar.h>

#ifndef NDEBUG
    #define TL_EXPECT(cond) if (!(cond)) ShowMessageBoxError(__FILE__, __FUNCTION__, __LINE__, (LONG64)GetLastError())
    void ShowMessageBoxError(char* file, char* location, int line, LONG64 code);
#else
#define TL_EXPECT(cond) cond
#endif // !NDEBUG


extern HINSTANCE hInst;

