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

#include "tlister.h"
#include "HookManager.h"
#include <string>

///////////////////////////////////////////////////////////////////////////////////////////////////
HINSTANCE  hInst = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain(HINSTANCE hinst, unsigned long reason, void* lpReserved) {
    switch(reason) {
    case DLL_PROCESS_ATTACH:
        hInst = hinst;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;

}
extern "C" __declspec(dllexport)void __stdcall ListGetDetectString(char* DetectString, int maxlen)
{
    strcpy_s(DetectString, maxlen, "MULTIMEDIA");
}
///////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) HWND __stdcall ListLoadW(HWND ParentWin, char* FileToLoad, int ShowFlags) 
{
    HookManager::instance().AddTab(ParentWin);
	return NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) HWND __stdcall ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags) 
{
	return ListLoadW(ParentWin, NULL, ShowFlags);
}

#ifndef NDEBUG

void ShowMessageBoxError(char* file, char* location, int line, LONG64 code)
{
    std::string buf = "File: ";
    buf += file;
    buf += "\nLocation: ";
    buf += location;
    buf += " : ";
    buf += std::to_string(line);
    buf += "\nError code: ";
    buf += std::to_string(code);

    MessageBoxA(NULL, buf.c_str(), "Error occured!", MB_ICONERROR);
}

#endif