/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#ifndef _SV_WINCOMP_HXX
#define _SV_WINCOMP_HXX

#ifndef _STRING_H
#include <string.h>
#endif

// ----------
// - Strict -
// ----------

// Anpassungen fuer TypeChecking

inline HPEN SelectPen( HDC hDC, HPEN hPen )
{
	return (HPEN)SelectObject( hDC, (HGDIOBJ)hPen );
}

inline void DeletePen( HPEN hPen )
{
	DeleteObject( (HGDIOBJ)hPen );
}

inline HPEN GetStockPen( int nObject )
{
	return (HPEN)GetStockObject( nObject );
}

inline HBRUSH SelectBrush( HDC hDC, HBRUSH hBrush )
{
	return (HBRUSH)SelectObject( hDC, (HGDIOBJ)hBrush );
}

inline void DeleteBrush( HBRUSH hBrush )
{
	DeleteObject( (HGDIOBJ)hBrush );
}

inline HBRUSH GetStockBrush( int nObject )
{
	return (HBRUSH)GetStockObject( nObject );
}

inline HFONT SelectFont( HDC hDC, HFONT hFont )
{
	return (HFONT)SelectObject( hDC, (HGDIOBJ)hFont );
}

inline void DeleteFont( HFONT hFont )
{
	DeleteObject( (HGDIOBJ)hFont );
}

inline HFONT GetStockFont( int nObject )
{
	return (HFONT)GetStockObject( nObject );
}

inline HBITMAP SelectBitmap( HDC hDC, HBITMAP hBitmap )
{
	return (HBITMAP)SelectObject( hDC, (HGDIOBJ)hBitmap );
}

inline void DeleteBitmap( HBITMAP hBitmap )
{
	DeleteObject( (HGDIOBJ)hBitmap );
}

inline void DeleteRegion( HRGN hRegion )
{
	DeleteObject( (HGDIOBJ)hRegion );
}

inline HPALETTE GetStockPalette( int nObject )
{
	return (HPALETTE)GetStockObject( nObject );
}

inline void DeletePalette( HPALETTE hPalette )
{
	DeleteObject( (HGDIOBJ)hPalette );
}

inline void SetWindowStyle( HWND hWnd, DWORD nStyle )
{
	SetWindowLong( hWnd, GWL_STYLE, nStyle );
}

inline DWORD GetWindowStyle( HWND hWnd )
{
	return GetWindowLong( hWnd, GWL_STYLE );
}

inline void SetWindowExStyle( HWND hWnd, DWORD nStyle )
{
	SetWindowLong( hWnd, GWL_EXSTYLE, nStyle );
}

inline DWORD GetWindowExStyle( HWND hWnd )
{
	return GetWindowLong( hWnd, GWL_EXSTYLE );
}

inline BOOL IsMinimized( HWND hWnd )
{
	return IsIconic( hWnd );
}

inline BOOL IsMaximized( HWND hWnd )
{
	return IsZoomed( hWnd );
}

inline void SetWindowFont( HWND hWnd, HFONT hFont, BOOL bRedraw )
{
	SendMessage( hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM((UINT)bRedraw,0) );
}

inline HFONT GetWindowFont( HWND hWnd )
{
	return (HFONT)(UINT)SendMessage( hWnd, WM_GETFONT, 0, 0 );
}

inline void SetClassCursor( HWND hWnd, HCURSOR hCursor )
{
	SetClassLong( hWnd, GCL_HCURSOR, (DWORD)hCursor );
}

inline HCURSOR GetClassCursor( HWND hWnd )
{
	return (HCURSOR)GetClassLong( hWnd, GCL_HCURSOR );
}

inline void SetClassIcon( HWND hWnd, HICON hIcon )
{
	SetClassLong( hWnd, GCL_HICON, (DWORD)hIcon );
}

inline HICON GetClassIcon( HWND hWnd )
{
	return (HICON)GetClassLong( hWnd, GCL_HICON );
}

inline HBRUSH SetClassBrush( HWND hWnd, HBRUSH hBrush )
{
	return (HBRUSH)SetClassLong( hWnd, GCL_HBRBACKGROUND, (DWORD)hBrush );
}

inline HBRUSH GetClassBrush( HWND hWnd )
{
	return (HBRUSH)GetClassLong( hWnd, GCL_HBRBACKGROUND );
}

inline HINSTANCE GetWindowInstance( HWND hWnd )
{
	return (HINSTANCE)GetWindowLong( hWnd, GWL_HINSTANCE );
}

// ------------------------
// - ZMouse Erweiterungen -
// ------------------------

#define MSH_MOUSEWHEEL "MSWHEEL_ROLLMSG"

#define MOUSEZ_CLASSNAME  "MouseZ"            // wheel window class
#define MOUSEZ_TITLE	  "Magellan MSWHEEL"  // wheel window title

#define MSH_WHEELMODULE_CLASS (MOUSEZ_CLASSNAME)
#define MSH_WHEELMODULE_TITLE (MOUSEZ_TITLE)

#define MSH_SCROLL_LINES "MSH_SCROLL_LINES_MSG"

#ifndef WHEEL_DELTA
#define WHEEL_DELTA 				120
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL				0x020A
#endif
#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES		104
#endif
#ifndef SPI_SETWHEELSCROLLLINES
#define SPI_SETWHEELSCROLLLINES		105
#endif
#ifndef WHEEL_PAGESCROLL
#define WHEEL_PAGESCROLL			(UINT_MAX)
#endif


// -----------------------------
// - SystemAgent Erweiterungen -
// -----------------------------

#define ENABLE_AGENT			1
#define DISABLE_AGENT			2
#define GET_AGENT_STATUS		3
typedef int (APIENTRY* SysAgt_Enable_PROC)( int );

// ---------------------
// - 5.0-Erweiterungen -
// ---------------------

#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION 	27
#endif
#ifndef COLOR_GRADIENTINACTIVECAPTION
#define COLOR_GRADIENTINACTIVECAPTION	28
#endif

#ifndef SPI_GETFLATMENU
#define SPI_GETFLATMENU     0x1022
#endif
#ifndef COLOR_MENUBAR
#define COLOR_MENUBAR       30
#endif
#ifndef COLOR_MENUHILIGHT
#define COLOR_MENUHILIGHT   29   
#endif

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW       0x00020000
#endif

// -------------------------------------------------------
// MT 12/03: From winuser.h, only needed in salframe.cxx
// Better change salframe.cxx to include winuser.h
// -------------------------------------------------------

#define WS_EX_LAYERED           0x00080000

#ifndef WM_UNICHAR
#define WM_UNICHAR              0x0109
#define UNICODE_NOCHAR          0xFFFF
#endif

#endif // _SV_WINCOMP_HXX
