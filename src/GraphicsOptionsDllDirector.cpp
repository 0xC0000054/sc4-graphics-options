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

#include "version.h"
#include "Logger.h"
#include "SC4VersionDetection.h"
#include "Settings.h"
#include "cGZDisplayMetrics.h"
#include "cGZDisplayTiming.h"
#include "cGZGPixelFormatDesc.h"
#include "cIGZApp.h"
#include "cIGZCmdLine.h"
#include "cIGZCOM.h"
#include "cIGZFrameWork.h"
#include "cIGZFrameWorkW32.h"
#include "cIGZGraphicSystem.h"
#include "cIGZGraphicSystem2.h"
#include "cIGZMessageServer2.h"
#include "cIGZString.h"
#include "cISC4App.h"
#include "cRZMessage2COMDirector.h"
#include "cRZMessage2Standard.h"
#include "cRZAutoRefCount.h"
#include "cRZBaseString.h"
#include "GZCLSIDDefs.h"
#include "GZServPtrs.h"
#include "SC4Preferences.h"

#include "sGDMode.h"
#include "cIGZGDriver.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <map>
#include <string>
#include <Windows.h>
#include "wil/resource.h"
#include "wil/win32_helpers.h"

#include "EASTLConfigSC4.h"
#include "EASTL\vector.h"

static constexpr uint32_t kGZGraphicSystem_SystemServiceID = 0xC416025C;

static constexpr uint32_t kSCGDriverDirectX = 0xBADB6906;
static constexpr uint32_t kSCGDriverOpenGL = 0xC4554841;
static constexpr uint32_t kSCGDriverSoftware = 0x7ACA35C6;

static constexpr uint32_t kGraphicsOptionsDirectorID = 0x50A4C948;

static constexpr uint32_t GZIID_cISC4App = 0x26CE01C0;

static constexpr std::string_view PluginConfigFileName = "SC4GraphicsOptions.ini";
static constexpr std::string_view PluginLogFileName = "SC4GraphicsOptions.log";

namespace
{
	std::filesystem::path GetModuleFolderPath(HMODULE module)
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(module);

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}

	std::filesystem::path GetDllFolderPath()
	{
		return GetModuleFolderPath(wil::GetModuleInstanceHandle());
	}

	std::filesystem::path GetSC4AppFolderPath()
	{
		return GetModuleFolderPath(nullptr);
	}

	bool DriverTypesMatch(uint8_t existingDriverType, SC4GraphicsDriverType newDriverType)
	{
		switch (newDriverType)
		{
		case SC4GraphicsDriverType::DirectX:
		case SC4GraphicsDriverType::OpenGL:
			return existingDriverType != 0;
		case SC4GraphicsDriverType::Software:
			return existingDriverType == 0;
		default:
			return false;
		}
	}

	bool WindowModesMatch(bool isFullScreen, SC4WindowMode windowMode)
	{
		switch (windowMode)
		{
		case SC4WindowMode::Windowed:
		case SC4WindowMode::BorderlessFullScreen:
			return !isFullScreen;
		case SC4WindowMode::FullScreen:
			return isFullScreen;
		default:
			return false;
		}
	}

	void OverwriteMemory(uintptr_t address, uint8_t newValue)
	{
		DWORD oldProtect;
		// Allow the executable memory to be written to.
		THROW_IF_WIN32_BOOL_FALSE(VirtualProtect(
			reinterpret_cast<LPVOID>(address),
			sizeof(newValue),
			PAGE_EXECUTE_READWRITE,
			&oldProtect));

		// Patch the memory at the specified address.
		*((uint8_t*)address) = newValue;
	}
}

class GraphicsOptionsDllDirector : public cRZCOMDllDirector
{
public:

	GraphicsOptionsDllDirector()
	{
		std::filesystem::path dllFolderPath = GetDllFolderPath();

		std::filesystem::path configFilePath = dllFolderPath;
		configFilePath /= PluginConfigFileName;

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
		logger.Init(logFilePath, LogLevel::Error);
		logger.WriteLogFileHeader("SC4GraphicsOptions v" PLUGIN_VERSION_STR);

		try
		{
			settings.Load(configFilePath);
		}
		catch (const std::exception& e)
		{
			logger.WriteLine(LogLevel::Error, e.what());
		}
	}

	uint32_t GetDirectorID() const
	{
		return kGraphicsOptionsDirectorID;
	}

	bool PreFrameWorkInit()
	{
		cIGZFrameWork* const pFramework = RZGetFrameWork();

		cIGZApp* const pApp = pFramework->Application();

		if (pApp)
		{
			cRZAutoRefCount<cISC4App> pSC4App;

			if (pApp->QueryInterface(GZIID_cISC4App, pSC4App.AsPPVoid()))
			{
				SC4Preferences* prefs = pSC4App->GetPreferences();
				SC4VideoPreferences& videoPrefs = prefs->videoPreferences;

				uint32_t windowWidth = settings.GetWindowWidth();
				uint32_t windowHeight = settings.GetWindowHeight();
				uint32_t colorDepth = settings.GetColorDepth();
				SC4WindowMode windowMode = settings.GetWindowMode();
				SC4GraphicsDriverType driverType = settings.GetDriverType();

				if (videoPrefs.width != windowWidth
					|| videoPrefs.height != windowHeight
					|| videoPrefs.bitDepth != colorDepth
					|| !WindowModesMatch(videoPrefs.bFullScreen != 0, windowMode)
					|| !DriverTypesMatch(videoPrefs.driverType, driverType))
				{
					videoPrefs.width = windowWidth;
					videoPrefs.height = windowHeight;
					videoPrefs.bitDepth = colorDepth;
					videoPrefs.bFullScreen = windowMode == SC4WindowMode::FullScreen;

					// The game preferences UI treats the driver type as a Boolean, where a value
					// of 1 indicates hardware rendering and a value of 0 indicates software rendering.
					// Unlike the base game, we consider OpenGL to be hardware rendering.
					switch (driverType)
					{
					case SC4GraphicsDriverType::DirectX:
					case SC4GraphicsDriverType::OpenGL:
						videoPrefs.driverType = 1;
						break;
					case SC4GraphicsDriverType::Software:
						videoPrefs.driverType = 0;
						break;
					}

					pSC4App->SavePreferences();
				}

				CheckDirectX7ResolutionLimit(videoPrefs.width, videoPrefs.height);
				FixFullScreen32BitColorDepth();
				SetGraphicsOptions();
			}
		}

		if (!settings.EnableIntroVideo())
		{
			// Add the command line argument to disable the intro videos
			// that the game plays on startup.
			cIGZCmdLine* pCmdLine = pFramework->CommandLine();
			int argCount = pCmdLine->argc();

			if (!pCmdLine->IsSwitchPresent(cRZBaseString("Intro")))
			{
				pCmdLine->InsertArgument(cRZBaseString("-Intro:off"), argCount);
			}
		}

		return true;
	}

	bool PreAppInit()
	{
		Logger& logger = Logger::GetInstance();

		cIGZFrameWork* const pFramework = RZGetFrameWork();

		if (settings.GetWindowMode() == SC4WindowMode::BorderlessFullScreen)
		{
			// Convert the dialog to a borderless full screen window.

			cRZAutoRefCount<cIGZFrameWorkW32> pFrameworkW32;

			if (pFramework->QueryInterface(GZIID_cIGZFrameWorkW32, pFrameworkW32.AsPPVoid()))
			{
				HWND mainWindowHWND = pFrameworkW32->GetMainHWND();

			    LONG windowStyle = GetWindowLongA(mainWindowHWND, GWL_STYLE);

				windowStyle &= ~WS_OVERLAPPEDWINDOW;
				// A borderless full screen window uses the WS_POPUP style instead of WS_OVERLAPPED.
				windowStyle |= WS_POPUP;

				SetWindowLongA(mainWindowHWND, GWL_STYLE, windowStyle);

				// SimCity 4 does not set any of the extended window styles.

				constexpr int x = 0;
				constexpr int y = 0;
				int cx = GetSystemMetrics(SM_CXSCREEN);
				int cy = GetSystemMetrics(SM_CYSCREEN);

				SetWindowPos(mainWindowHWND, HWND_TOP, x, y, cx, cy, SWP_FRAMECHANGED);
				ShowWindow(mainWindowHWND, SW_MAXIMIZE);
			}
		}

		// This checks to ensure that the game is using the options that
		// we requested in PreFrameWorkInit.
		VerifyGraphicsOptions();

		return true;
	}

	bool OnStart(cIGZCOM * pCOM)
	{
		cIGZFrameWork* const pFramework = RZGetFrameWork();

		const cIGZFrameWork::FrameworkState state = pFramework->GetState();

		if (state < cIGZFrameWork::kStatePreAppInit)
		{
			pFramework->AddHook(this);
		}
		else
		{
			PreAppInit();
		}
		return true;
	}

private:

	void CheckDirectX7ResolutionLimit(uint32_t width, uint32_t height)
	{
		if (settings.GetDriverType() == SC4GraphicsDriverType::DirectX)
		{
			// SC4 was built with DirectX 7, which has a resolution limit of 2048x2048 or less.
			// This limit can be exceeded with the use of DirectX wrappers that translate the
			// game's DirectX 7 API calls over to the newer DirectX APIs.
			constexpr uint32_t DX7TextureLimit = 2048;

			if (width > DX7TextureLimit || height > DX7TextureLimit)
			{
				// The DirectX wrappers used with SC4 work by having SC4 load their ddraw.dll
				// wrapper which is placed in the application folder next to SimCity 4.exe.
				// This works because the default Windows DLL search behavior is to search the
				// folder that the executable is located in before it searches the OS folders.

				const std::filesystem::path appFolder = GetSC4AppFolderPath();

				std::filesystem::path ddrawDllPath = appFolder;
				ddrawDllPath /= L"ddraw.dll";

				if (!std::filesystem::exists(ddrawDllPath))
				{
					Logger& logger = Logger::GetInstance();
					logger.WriteLine(
						LogLevel::Info,
						"Warning: A DirectX wrapper is required for the resolution you are using.");
				}
			}
		}
	}

	void FixFullScreen32BitColorDepth()
	{
		// Maxis hard-coded the DirectX driver to use 16-bit color depth when in full screen mode, so we patch the
		// game's memory to fix that.
		// This fix is based on the patched SimCity 4 executable at https://github.com/dege-diosg/dgVoodoo2/issues/3

		if (settings.GetDriverType() == SC4GraphicsDriverType::DirectX
			&& settings.GetWindowMode() == SC4WindowMode::FullScreen
			&& settings.GetColorDepth() == 32)
		{
			Logger& logger = Logger::GetInstance();
			const uint16_t gameVersion = SC4VersionDetection::GetInstance().GetGameVersion();

			if (gameVersion == 641)
			{
				try
				{
					// Replace the hard-coded value of 16 with 32.
					OverwriteMemory(0x887738, 32);
					logger.WriteLine(LogLevel::Info, "Forced the DirectX full screen color depth to 32-bit.");
				}
				catch (const std::exception& e)
				{
					logger.WriteLineFormatted(
						LogLevel::Error,
						"Failed to force the DirectX full screen color depth to 32-bit: %s",
						e.what());
				}
			}
			else
			{
				logger.WriteLineFormatted(
					LogLevel::Error,
					"Unable to force the DirectX full screen color depth to 32-bit. Requires "
					"game version 641, found game version %d.",
					gameVersion);
			}
		}
	}

	void SetGraphicsOptions()
	{
		// These settings will override the values that SC4 already set
		// when reading its preferences and/or command line arguments.

		cIGZGraphicSystemPtr pGS;

		if (pGS)
		{
			cGZDisplayMetrics metrics{};
			metrics.width = settings.GetWindowWidth();
			metrics.height = settings.GetWindowHeight();
			metrics.bitDepth = settings.GetColorDepth();
			bool windowedMode = settings.GetWindowMode() != SC4WindowMode::FullScreen;

			pGS->PreInitSetDesiredGameResolution(metrics);
			pGS->PreInitSetWindowedMode(windowedMode);
		}

		cIGZGraphicSystem2Ptr pGS2;

		if (pGS2)
		{
			uint32_t requestedDriverID = 0;

			switch (settings.GetDriverType())
			{
			case SC4GraphicsDriverType::OpenGL:
				requestedDriverID = kSCGDriverOpenGL;
				break;
			case SC4GraphicsDriverType::Software:
				requestedDriverID = kSCGDriverSoftware;
				break;
			case SC4GraphicsDriverType::DirectX:
			default:
				requestedDriverID = kSCGDriverDirectX;
				break;
			}

			// SC4 will use driver with the requested ID
			// when it initializes the graphics system.
			pGS2->SetDefaultDriverClassID(requestedDriverID);
		}
	}

	void VerifyGraphicsOptions()
	{
		Logger& logger = Logger::GetInstance();

		cIGZGraphicSystemPtr pGS;

		if (pGS)
		{
			cGZDisplayMetrics gameMetrics{};
			pGS->GetGameMetrics(gameMetrics);
			bool isFullScreen = pGS->IsFullScreenMode();

			uint32_t requestedWidth = settings.GetWindowWidth();
			uint32_t requestedHeight = settings.GetWindowHeight();
			uint32_t requestedBitDepth = settings.GetColorDepth();
			SC4WindowMode windowMode = settings.GetWindowMode();

			if (gameMetrics.width != requestedWidth
				|| gameMetrics.height != requestedHeight
				|| gameMetrics.bitDepth != requestedBitDepth
				|| !WindowModesMatch(isFullScreen, windowMode))
			{
				logger.WriteLineFormatted(
					LogLevel::Error,
					"SC4's graphics options (%u\u0078%u\u0078%u, %s) doesn't match the requested options (%u\u0078%u\u0078%u, %s).",
					gameMetrics.width,
					gameMetrics.height,
					gameMetrics.bitDepth,
					isFullScreen ? "full screen" : "windowed",
					requestedWidth,
					requestedHeight,
					requestedBitDepth,
				    windowMode == SC4WindowMode::FullScreen ? "full screen" : "windowed");
			}
		}

		cIGZGraphicSystem2Ptr pGS2;

		if (pGS2)
		{
			cIGZGDriver* pDriver = pGS2->GetGDriver();

			if (pDriver)
			{
				uint32_t requestedDriverID = 0;
				const char* driverName = nullptr;

				switch (settings.GetDriverType())
				{
				case SC4GraphicsDriverType::OpenGL:
					requestedDriverID = kSCGDriverOpenGL;
					driverName = "OpenGL";
					break;
				case SC4GraphicsDriverType::Software:
					requestedDriverID = kSCGDriverSoftware;
					driverName = "Software";
					break;
				case SC4GraphicsDriverType::DirectX:
				default:
					requestedDriverID = kSCGDriverDirectX;
					driverName = "DirectX";
					break;
				}

				uint32_t currentDriverID = pDriver->GetGZCLSID();

				if (currentDriverID != requestedDriverID)
				{
					logger.WriteLineFormatted(
						LogLevel::Error,
						"Failed to set the game's driver to %s.",
						driverName);
				}
			}
		}
	}

	Settings settings;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static GraphicsOptionsDllDirector sDirector;
	return &sDirector;
}