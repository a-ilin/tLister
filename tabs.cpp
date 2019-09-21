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
#include <CommCtrl.h>
#include <stdio.h>
#include "tlister.h"
#pragma comment( lib, "comctl32.lib" )
extern HINSTANCE  hInst;
HWND TabWindow=NULL;
HWND TabWindowControl=NULL;
#define MAXWIN 300
HWND WinList[MAXWIN];
HMENU MenuList[MAXWIN];
WNDPROC OldTabControlWndProc=NULL;
int NumWindows=0;
bool needwait=false;
bool isclose=true;
int needact;
int AlwaysShowTab=0;
HHOOK hkb,spy;
int nexttabmod,nexttab,previoustabmod,previoustab,detachtabmod,detachtab,closealltabmod,closealltab;

wchar_t onclosew[100]=L"";
HWND fwindow=NULL;
int warnonclose=0;
int ismax,fixedwidth,minwidth,multiline;
void CloseTabWindow();

///////////////////////////////////////////////////////////////////////////////////////////////////
void resize(HWND win) {
	RECT r,r1;
	GetClientRect(TabWindow,&r1);
	if(NumWindows>1||AlwaysShowTab){
		SetRect(&r, 0, 0,r1.right,r1.bottom ); 
		SendMessage(TabWindowControl,TCM_ADJUSTRECT,false,(LPARAM)&r);
		MoveWindow(win,r.left,r.top,r.right-r.left,r.bottom-r.top,true);
	} else
		MoveWindow(win,r1.left,r1.top,r1.right-r1.left,r1.bottom-r1.top,true);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void ChangeTab() {

	LRESULT curtab = SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0);

	SetMenu(TabWindow,MenuList[curtab]);
	DrawMenuBar(TabWindow);
	
	for(int i=0;i<NumWindows;i++)ShowWindow(WinList[i],(i==curtab)?SW_SHOW:SW_HIDE);

	resize(WinList[curtab]);

	wchar_t title[1000]=L"";
	GetWindowTextW(WinList[curtab],title,1000);
	SetWindowTextW(TabWindow,title);

	PostMessage(WinList[curtab], WM_LBUTTONDOWN, 0, 0);
	PostMessage(WinList[curtab], WM_LBUTTONUP, 0, 0);
	HWND swnd=FindWindowEx(WinList[curtab], NULL, NULL, NULL);
	if (swnd)SetFocus(swnd);else
	SetFocus(WinList[curtab]);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK TabWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if(!needwait)
		switch(message) {
		case WM_CLOSE:
			if(NumWindows>0){
				int nc=IDOK;
				if(NumWindows>1&&warnonclose) {
					nc=MessageBoxW(TabWindow,onclosew,L"tLister",MB_OKCANCEL|MB_ICONQUESTION);
				}
				if(nc==IDOK) {
					int nw=NumWindows;
					for(int i=0;i<nw;i++)PostMessage(WinList[i],WM_CLOSE,0,0);
				}
				return 0;

			}break;
		case WM_COMMAND:
			if(NumWindows>0){
				LRESULT curtab = SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0);
				if(wParam==283) {
					SendMessage(TabWindow,WM_SETREDRAW,false,0);
					if(IsZoomed(TabWindow)) {
						ShowWindow(WinList[curtab],SW_MAXIMIZE);
						PostMessage(WinList[curtab],message,wParam,lParam);
						ShowWindow(WinList[curtab],SW_RESTORE);
					} else {
						RECT r;
						GetWindowRect(TabWindow,&r);
						ShowWindow(WinList[curtab],SW_RESTORE);
						MoveWindow(WinList[curtab],r.left,r.top,r.right-r.left,r.bottom-r.top,true);
						PostMessage(WinList[curtab],message,wParam,lParam);
					}
					resize(WinList[curtab]);
					SendMessage(TabWindow,WM_SETREDRAW,true,0);

				} else{
					PostMessage(WinList[curtab],message,wParam,lParam);
				}
				UpdateWindow(TabWindow);
				DrawMenuBar(TabWindow);
			}break;
		case WM_SETFOCUS:
			if(NumWindows>0){
				LRESULT curtab = SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0);
				PostMessage(WinList[curtab], WM_LBUTTONDOWN, 0, 0);
				PostMessage(WinList[curtab], WM_LBUTTONUP, 0, 0);
				SetFocus(WinList[curtab]);
			}break;
		case WM_SIZE:
			if(NumWindows>0){
				MoveWindow(TabWindowControl,0,0,LOWORD(lParam),HIWORD(lParam),true);
				resize(WinList[SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0)]);
				return 0;
			}break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code) {
			case TCN_KEYDOWN:
				if(NumWindows>0)PostMessage(WinList[SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0)],WM_KEYDOWN,(WPARAM)(((NMTCKEYDOWN*)lParam)->wVKey),(LPARAM)(((NMTCKEYDOWN*)lParam)->flags));
				return 0;
			case TCN_SELCHANGE:
				if(NumWindows>0)ChangeTab();
				break;
			}
			break;
		case WM_PARENTNOTIFY:
			switch(LOWORD(wParam)) {
			case WM_DESTROY:
				if(NumWindows>0)DelTab((HWND)lParam);else if((!isclose))CloseTabWindow();
			}
			break;
	}
	return DefWindowProcW(hWnd, message, wParam, lParam); 
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void unslave(HWND &win, HMENU &menu) {
	RECT r;
	GetWindowRect(TabWindow,&r);
	ShowWindow(win,SW_HIDE);
	LONG_PTR style=GetWindowLongPtr(win, GWL_STYLE);
	SetWindowLongPtr(win, GWL_STYLE, (style|WS_OVERLAPPEDWINDOW)&(~WS_CHILD));
	SetMenu(win,menu);
	SetParent(win,NULL);
	DelTab(win);
	SetWindowPos(win,HWND_TOP,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_FRAMECHANGED);
	SwitchToThisWindow(win,true);
	ShowWindow(win,SW_SHOW);
	SetFocus(win);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT WINAPI TabControlSubClassProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam) {

	if(!needwait)
	switch(nMsg) {
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDBLCLK:{
			TCHITTESTINFO ht;
			ht.pt.x=LOWORD(lParam);
			ht.pt.y=HIWORD(lParam);
			ht.flags=TCHT_ONITEM;
			LRESULT i=SendMessage(TabWindowControl,TCM_HITTEST,0,(LPARAM)&ht);
			if(i!=-1)PostMessage(WinList[i],WM_CLOSE,0,0);
			return 0;
			break;
		}
		case WM_RBUTTONDBLCLK: 
			TCHITTESTINFO ht;
			ht.pt.x=LOWORD(lParam);
			ht.pt.y=HIWORD(lParam);
			ht.flags=TCHT_ONITEM;
			LRESULT i=SendMessage(TabWindowControl,TCM_HITTEST,0,(LPARAM)&ht);
			if(i!=-1&& IsWindow(WinList[i])) {
				HWND win=WinList[i];
				HMENU menu=MenuList[i];
				unslave(win, menu);
				return 0;
			}
			break;
	}
	return CallWindowProc((WNDPROC)OldTabControlWndProc, hWnd, nMsg, wParam, lParam);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
wchar_t *GetName(wchar_t *title) {
	wchar_t *name;
	if(wcsrchr(title, L'\\'))name=wcsrchr(title, L'\\')+1;
	else name=title;
	if(wcsrchr(name, L'['))name=wcsrchr(name, L'[')+1;
	if(wcsrchr(name, L']'))*wcsrchr(name, L']')=0;
	for(int i=0;i<wcslen(name);i++) {
		if(name[i]=='&') {
			wchar_t *nf=&(name[i]);
			size_t len=wcslen(nf);
			for(size_t j=len;j>0;j--)nf[j]=nf[j-1];
			nf[0]='&';
			nf[len+1]=0;
			i++;
		}
	}
	return name;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT __declspec(dllexport)__stdcall  CALLBACK SpyProc(int nCode,WPARAM wParam, LPARAM lParam) {
	if(!needwait&&nCode==HC_ACTION){
		if(((PCWPRETSTRUCT)lParam)->message==WM_SETTEXT/*&&GetForegroundWindow()==TabWindow*/&&((PCWPRETSTRUCT)lParam)->hwnd!=TabWindow) {
			for(int i=0;i<NumWindows;i++)
			if(WinList[i]==((PCWPRETSTRUCT)lParam)->hwnd) {
					wchar_t title[1000]=L"";
					GetWindowTextW(((PCWPRETSTRUCT)lParam)->hwnd,title,1000);
					if(!wcsrchr(title, L'['))break;
					SetWindowTextW(TabWindow,title);
					wchar_t* name = GetName(title);
					TCITEMW tie;
					tie.mask = TCIF_TEXT; 
					tie.pszText = name; 
					SendMessageW(TabWindowControl,TCM_SETITEMW,i,(LPARAM) &tie);
					DrawMenuBar(TabWindow);
					break;
			}
		}
	}
		return  CallNextHookEx( spy, nCode, wParam, lParam );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkmod(int mod) {
	return mod==((GetKeyState(VK_MENU)<0)?1:0)+((GetKeyState(VK_CONTROL)<0)?2:0)+((GetKeyState(VK_SHIFT)<0)?4:0);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT __declspec(dllexport)__stdcall  CALLBACK KeySpy(int nCode,WPARAM wParam, LPARAM lParam) {
	if(nCode==HC_ACTION) {
		if(((PMSG)lParam)->message==WM_KEYDOWN&&GetForegroundWindow()==TabWindow){

			if(((PMSG)lParam)->wParam==nexttab&&checkmod(nexttabmod)) {
				LRESULT curtab = SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0);
				SendMessage(TabWindowControl,TCM_SETCURFOCUS,(curtab==NumWindows-1)?0:curtab+1,0);
				return 1;
			}

			if(((PMSG)lParam)->wParam==previoustab&&checkmod(previoustabmod)) {
				LRESULT curtab = SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0);
				SendMessage(TabWindowControl,TCM_SETCURFOCUS,(curtab==0)?NumWindows-1:curtab-1,0);
				return 1;
			}

			if(((PMSG)lParam)->wParam==detachtab&&checkmod(detachtabmod)) {
				LRESULT curtab = SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0);
				HWND win=WinList[curtab];
				HMENU menu=MenuList[curtab];
				unslave(win, menu);
				return 1;
			}

			if(((PMSG)lParam)->wParam==VK_F11) {
				ShowWindow(TabWindow,IsZoomed(TabWindow)?SW_RESTORE:SW_MAXIMIZE);
				return 1;
			}

			if(((PMSG)lParam)->wParam==closealltab&&checkmod(closealltabmod)) {
				PostMessage(TabWindow,WM_CLOSE,0,0);
				return 1;
			}
		}
	}
	return  CallNextHookEx( 0, nCode, wParam, lParam );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void parseini(){
	wchar_t iniismax[10]=L"";
	wchar_t path[MAX_PATH]=L"";
	GetEnvironmentVariableW(L"COMMANDER_INI",path,MAX_PATH);
	GetPrivateProfileStringW(L"Lister",L"Maximized",L"",iniismax,10,path);
	GetModuleFileNameW(hInst, path, MAX_PATH);
	if(wcsrchr(path, L'\\'))*wcsrchr(path, L'\\') = L'\0';
	wcscat_s(path,MAX_PATH,L"\\tlister.ini");
	if(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES) {
		GetEnvironmentVariableW(L"COMMANDER_INI",path,MAX_PATH);
		if(wcsrchr(path, L'\\'))*wcsrchr(path, L'\\') = L'\0';
		wcscat_s(path,MAX_PATH,L"\\tlister.ini");
	}

	wchar_t inimultiline[10],inifixedwidth[10],iniminwidth[10],ininexttabmod[10],ininexttab[10],iniprevioustabmod[10],iniprevioustab[10],
		iniast[10],iniwarnonclose[10],inidetachtab[10],inidetachtabmod[10],iniclosealltab[10],iniclosealltabmod[10];
	if(GetFileAttributesW(path)!=INVALID_FILE_ATTRIBUTES) {
		GetPrivateProfileStringW(L"tlister",L"multiline",L"0",inimultiline,10,path);
		GetPrivateProfileStringW(L"tlister",L"fixedwidth",L"1",inifixedwidth,10,path);
		GetPrivateProfileStringW(L"tlister",L"minwidth",L"100",iniminwidth,10,path);
		GetPrivateProfileStringW(L"tlister",L"alwaysshowtab",L"0",iniast,10,path);
		GetPrivateProfileStringW(L"tlister",L"nexttabmod",L"",ininexttabmod,10,path);
		GetPrivateProfileStringW(L"tlister",L"nexttab",L"",ininexttab,10,path);
		GetPrivateProfileStringW(L"tlister",L"previoustabmod",L"",iniprevioustabmod,10,path);
		GetPrivateProfileStringW(L"tlister",L"previoustab",L"",iniprevioustab,10,path);

		GetPrivateProfileStringW(L"tlister",L"detachtab",L"",inidetachtab,10,path);
		GetPrivateProfileStringW(L"tlister",L"detachtabmod",L"",inidetachtabmod,10,path);

		GetPrivateProfileStringW(L"tlister",L"closealltab",L"",iniclosealltab,10,path);
		GetPrivateProfileStringW(L"tlister",L"closealltabmod",L"",iniclosealltabmod,10,path);


		GetPrivateProfileStringW(L"tlister",L"warnonclose",L"0",iniwarnonclose,10,path);
		GetPrivateProfileStringW(L"tlister",L"onclosew",L"Close all tabs?",onclosew,100,path);        
	}
	if(wcscmp(iniwarnonclose,L"1")==0)warnonclose=1;
	int a,b;
	a=swscanf_s(ininexttabmod,L"%X",&nexttabmod);
	b=swscanf_s(ininexttab,L"%X",&nexttab);
	if(a!=1||b!=1) {
		nexttabmod=0x2;
		nexttab=0xC0;
	}
	a=swscanf_s(iniprevioustabmod,L"%X",&previoustabmod);
	b=swscanf_s(iniprevioustab,L"%X",&previoustab);
	if(a!=1||b!=1) {
		previoustabmod=0x1;
		previoustab=0xC0;
	}

	a=swscanf_s(iniclosealltabmod,L"%X",&closealltabmod);
	b=swscanf_s(iniclosealltab,L"%X",&closealltab);
	if(a!=1||b!=1) {
		closealltabmod=0x2;
		closealltab=VK_CANCEL;
	}

	a=swscanf_s(inidetachtabmod,L"%X",&detachtabmod);
	b=swscanf_s(inidetachtab,L"%X",&detachtab);
	if(a!=1||b!=1) {
		detachtabmod=0x4;
		detachtab=VK_ESCAPE;
	}
	ismax=(wcscmp(iniismax,L"1")==0)?1:0;
	fixedwidth=(wcscmp(inifixedwidth,L"1")==0)?1:0;
	minwidth=_wtoi(iniminwidth);
	AlwaysShowTab=_wtoi(iniast);
	multiline=(wcscmp(inimultiline,L"1")==0)?1:0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void CreateTabControl(HWND ParentWin) {
	RECT r;
	WNDCLASSW wc;
	wc.style =CS_HREDRAW | CS_VREDRAW; 
	wc.cbClsExtra = 0; 
	wc.cbWndExtra = 0; 
	wc.hInstance = hInst; 
	wc.hIcon = (HICON)SendMessage(ParentWin,WM_GETICON,ICON_BIG,0); 
	wc.hCursor = NULL; 
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); 
	wc.lpszMenuName =  NULL; 
	wc.lpfnWndProc = (WNDPROC)TabWindowProc; 
	wc.lpszClassName = L"TLister"; 
	RegisterClassW(&wc);
	GetWindowRect(ParentWin,&r);
	TabWindow = CreateWindowW(L"TLister",NULL,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|(ismax?WS_MAXIMIZE:0),r.left,r.top,r.right-r.left,r.bottom-r.top,NULL,NULL,hInst,NULL);
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icex);
	GetClientRect(TabWindow,&r);
	TabWindowControl = CreateWindowW(WC_TABCONTROLW,L"",WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE |TCS_FOCUSNEVER|(fixedwidth?TCS_FIXEDWIDTH:0)|TCS_HOTTRACK|TCS_FORCELABELLEFT|(multiline?TCS_MULTILINE:0), 0, 0, r.right,r.bottom, TabWindow, NULL, hInst, NULL);
	OldTabControlWndProc = (WNDPROC)SetWindowLongPtr(TabWindowControl,GWLP_WNDPROC, (LONG_PTR)TabControlSubClassProc);
	SendMessage(TabWindowControl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)true);
	SendMessage(TabWindowControl, TCM_SETMINTABWIDTH, 0, minwidth);
	SendMessage(TabWindowControl, TCM_SETITEMSIZE, 0, minwidth);


	hkb=SetWindowsHookEx(WH_GETMESSAGE,(HOOKPROC)KeySpy,hInst,GetCurrentThreadId());
	spy=SetWindowsHookEx(WH_CALLWNDPROCRET,(HOOKPROC)SpyProc,hInst,GetCurrentThreadId());
	
	isclose=false;

}
///////////////////////////////////////////////////////////////////////////////////////////////////

void enslave(HWND window) {
	wchar_t title[1000];
	GetWindowTextW((HWND)window,title,1000);

	while (needwait)Sleep(10);
	needwait=true;
	int wn=-1;
	for (int i=0;i<NumWindows;i++)if (WinList[i]==(HWND)window) {
		wn=i;
		break;
	}
	wchar_t* name = GetName(title);
	TCITEMW tie;
	tie.mask = TCIF_TEXT; 
	tie.pszText = name; 

	if (wn==-1) {
		SendMessageW(TabWindowControl,TCM_INSERTITEMW,NumWindows,(LPARAM) &tie);
		WinList[NumWindows]=(HWND)window;
		MenuList[NumWindows]=GetMenu((HWND)window);
		NumWindows++;
	}
	SetParent((HWND)window,TabWindow);
	LONG_PTR style;
	style=GetWindowLongPtr((HWND)window, GWL_STYLE);
	SetWindowLongPtr((HWND)window, GWL_STYLE, (style&(~WS_OVERLAPPEDWINDOW))|WS_CHILD);
	SetWindowPos((HWND)window,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_FRAMECHANGED);

	if (wn==-1) SendMessage(TabWindowControl,TCM_SETCURFOCUS,NumWindows-1,0);

	ChangeTab();
	ShowWindow(TabWindow,SW_SHOW);
	RedrawWindow(TabWindow, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

	SwitchToThisWindow(TabWindow,true);

	needwait=false;

}
///////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WaitLister(LPVOID window) {

	wchar_t title[1000];
	while(GetWindowTextW((HWND)window,title,1000)<10&&IsWindow((HWND)window))Sleep(10);
	if(!IsWindow((HWND)window)){
		if(NumWindows==0)SendMessage(TabWindow,WM_PARENTNOTIFY,MAKEWPARAM(WM_DESTROY,0),0);
		ExitThread(0);
		return 0;
	}

	enslave((HWND)window);

	if(!IsWindow((HWND)window)){
		SendMessage(TabWindow,WM_PARENTNOTIFY,MAKEWPARAM(WM_DESTROY,0),LPARAM(window));
		ExitThread(0);
		return 0;
	}
	ExitThread(0);
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void AddTab(HWND window) {
	if(isclose) parseini();
	if((!(WS_CHILD&GetWindowLongPtr((HWND)window, GWL_STYLE)))&&(AlwaysShowTab==0)&&((!(fwindow&&IsWindow(fwindow)))||fwindow==window)&&(!TabWindow)){
		fwindow=window;
		return;
	}

	if(NumWindows>MAXWIN-2)return;
	if(WS_CHILD&GetWindowLongPtr((HWND)window, GWL_STYLE)) {
		int wn=-1;
		for(int i=0;i<NumWindows;i++)if(WinList[i]==(HWND)window) {
			wn=i;
			break;
		}
		if(wn==-1)return;
		LONG_PTR style=GetWindowLongPtr((HWND)window, GWL_STYLE);
		SetWindowLongPtr((HWND)window, GWL_STYLE, style&(~WS_CHILD));
		SetWindowTextW((HWND)window,L"Lister");
	}

	if(!TabWindow){
		CreateTabControl((HWND)window);

		if(fwindow&&IsWindow(fwindow)) enslave(fwindow);
		fwindow=NULL;
	}
	CreateThread(NULL,0,WaitLister,window,0,NULL);
}
///////////////////////////////////////////////////////////////////////////////////////////////////

void CloseTabWindow() {
	if((!isclose)&&TabWindow) {
		isclose=true;
		UnhookWindowsHookEx(hkb);
		UnhookWindowsHookEx(spy);
		DestroyWindow(TabWindowControl);
		SetMenu(TabWindow,NULL);
		DestroyWindow(TabWindow);
		TabWindow=NULL;
	}
	isclose=true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void DelTab(HWND window) {
	if(NumWindows<1)return;
	while(needwait)Sleep(10);
	needwait=true;
	LRESULT curtab = SendMessage(TabWindowControl,TCM_GETCURFOCUS,0,0);
	int tabnum=-1;
	for(int i=0;i<NumWindows;i++)
	if(WinList[i]==window) {
		tabnum=i;
		break;
	}
	
	if(tabnum!=-1) {
		SendMessage(TabWindowControl,TCM_DELETEITEM,tabnum,0);
		NumWindows--;
		for(int i=tabnum;i<NumWindows;i++) {
				WinList[i]=WinList[i+1];
				MenuList[i]=MenuList[i+1];
		}
	}

	needwait=false;
	if(NumWindows>0) {
		if(NumWindows==1&&(AlwaysShowTab==0)) {
				fwindow=WinList[0];
				unslave(WinList[0],MenuList[0]);
		}else{
				if(tabnum==curtab)SendMessage(TabWindowControl,TCM_SETCURFOCUS,tabnum==0?0:tabnum-1,0);
				else resize(WinList[curtab]);
		}
	} else CloseTabWindow();
}


