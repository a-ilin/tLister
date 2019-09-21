/* 
	tLister add tabs support to Lister.
	Copyright (C) 2011 Egor Vlaznev 
	
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
#include <windows.h>
#include "tlister.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
HINSTANCE  hInst;
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain(HINSTANCE hinst, unsigned long reason, void* lpReserved) {
    switch(reason) {
    case DLL_PROCESS_ATTACH:
        hInst=hinst;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;

}
extern "C" __declspec(dllexport)void __stdcall ListGetDetectString(char* DetectString,int maxlen){
    strcpy_s(DetectString,maxlen,"MULTIMEDIA");
}
///////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) HWND __stdcall ListLoadW(HWND ParentWin,char* FileToLoad,int ShowFlags) {
	AddTab(ParentWin);
	return NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags) {
	return ListLoadW(ParentWin,NULL,ShowFlags);
}

