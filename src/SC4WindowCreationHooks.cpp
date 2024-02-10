/*
 *  SC4GraphicsOptions - a DLL plugin for SimCity 4 that sets
 *  the game's rendering mode and resolution options.
 *
 *  Copyright (C) 2024 Nicholas Hayes
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation, under
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "SC4WindowCreationHooks.h"
#include <Windows.h>
#include <string>
#include "boost/algorithm/string.hpp"
#include "detours/detours.h"

namespace
{
	bool StringViewEquals(const std::string_view& lhs, const std::string_view& rhs)
	{
		return lhs.size() == rhs.size()
			&& boost::equals(lhs, rhs);
	}

	bool IsSC4AppWindowClassName(const std::string_view& className)
	{
		return StringViewEquals(className, "GDriverClass--DirectX")
			|| StringViewEquals(className, "GDriverClass--OpenGL")
			|| StringViewEquals(className, "GDriverClass--Software");
	}

	bool IsSC4AppWindowName(const std::string_view& windowName)
	{
		return StringViewEquals(windowName, "GDriverWindow--DirectX")
			|| StringViewEquals(windowName, "GDriverWindow--OpenGL")
			|| StringViewEquals(windowName, "GDriverWindow--Software");
	}

	bool IsSC4AppWindow(
		_In_opt_ LPCSTR lpClassName,
		_In_opt_ LPCSTR lpWindowName)
	{
		bool result = false;

		if (lpClassName && lpWindowName)
		{
			// We check the window name first, as some windows may
			// not use a valid string for the class name.
			result = IsSC4AppWindowName(lpWindowName)
				  && IsSC4AppWindowClassName(lpClassName);
		}

		return result;
	}
}

typedef HWND(WINAPI *PFN_CREATE_WINDOW_EX_A)(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCSTR lpClassName,
	_In_opt_ LPCSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam);

typedef BOOL(WINAPI* PFN_SET_WINDOW_POS)(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags);

typedef BOOL(WINAPI* PFN_SHOW_WINDOW)(_In_ HWND hWnd, _In_ int nCmdShow);

static PFN_CREATE_WINDOW_EX_A RealCreateWindowExA = &CreateWindowExA;
static PFN_SET_WINDOW_POS RealSetWindowPos = &SetWindowPos;
static PFN_SHOW_WINDOW RealShowWindow = &ShowWindow;

static SC4WindowMode s_WindowMode = SC4WindowMode::Windowed;
static HWND s_SC4MainWindowHWND = nullptr;

static HWND WINAPI HookedCreateWindowExA(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCSTR lpClassName,
	_In_opt_ LPCSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam)
{
	if (IsSC4AppWindow(lpClassName, lpWindowName))
	{
		if (s_WindowMode == SC4WindowMode::BorderlessFullScreen)
		{
			// Change the parameters to the values required for a borderless full screen window.
			// SimCity 4 does not set any of the extended window styles.

			// A borderless full screen window uses the WS_POPUP style instead of WS_OVERLAPPED.
			// The WS_MAXIMIZE style is also required to make the OS hide the task bar when the
			// window is displayed, this may be due to the fact that SC4 only calls ShowWindow
			// if some condition is met, and that condition is not met when starting the game.
			dwStyle = WS_VISIBLE | WS_POPUP | WS_MAXIMIZE;
			X = 0;
			Y = 0;
			nWidth = GetSystemMetrics(SM_CXSCREEN);
			nHeight = GetSystemMetrics(SM_CYSCREEN);
		}

		// Save the window handle, this is used by the other hook functions
		// to ensure that they are only modifying the game's main window.
		s_SC4MainWindowHWND = RealCreateWindowExA(
			dwExStyle,
			lpClassName,
			lpWindowName,
			dwStyle,
			X,
			Y,
			nWidth,
			nHeight,
			hWndParent,
			hMenu,
			hInstance,
			lpParam);

		return s_SC4MainWindowHWND;
	}
	else
	{
		return RealCreateWindowExA(
			dwExStyle,
			lpClassName,
			lpWindowName,
			dwStyle,
			X,
			Y,
			nWidth,
			nHeight,
			hWndParent,
			hMenu,
			hInstance,
			lpParam);
	}
}

static BOOL WINAPI HookedSetWindowPos(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags)
{
	if (hWnd == s_SC4MainWindowHWND)
	{
		if (s_WindowMode == SC4WindowMode::BorderlessFullScreen)
		{
			// We already set the correct window position and size
			// in our CreateWindowExA hook.
			uFlags |= SWP_NOMOVE | SWP_NOSIZE;
		}
	}

	return RealSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

static BOOL WINAPI HookedShowWindow(_In_ HWND hWnd, _In_ int nCmdShow)
{
	if (hWnd == s_SC4MainWindowHWND)
	{
		if (s_WindowMode == SC4WindowMode::BorderlessFullScreen)
		{
			nCmdShow = SW_SHOWMAXIMIZED;
		}
	}

	return RealShowWindow(hWnd, nCmdShow);
}


void SC4WindowCreationHooks::Install(SC4WindowMode windowMode)
{
	s_WindowMode = windowMode;

	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)RealCreateWindowExA, HookedCreateWindowExA);
	DetourAttach(&(PVOID&)RealSetWindowPos, HookedSetWindowPos);
	DetourAttach(&(PVOID&)RealShowWindow, HookedShowWindow);
	DetourTransactionCommit();
}

void SC4WindowCreationHooks::Remove()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)RealCreateWindowExA, HookedCreateWindowExA);
	DetourDetach(&(PVOID&)RealSetWindowPos, HookedSetWindowPos);
	DetourDetach(&(PVOID&)RealShowWindow, HookedShowWindow);
	DetourTransactionCommit();
}
