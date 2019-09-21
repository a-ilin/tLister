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

void CreateTabControl(HWND ParentWin);
LRESULT CALLBACK TabWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void AddTab(HWND window);
void DelTab(HWND window);
void CloseTabWindow();

