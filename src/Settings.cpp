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

#include "Settings.h"
#include "Logger.h"
#include "boost/algorithm/string.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"
#include <string>
#include <Windows.h>

namespace
{
	bool EqualsIgnoreCase(const std::string_view& lhs, const std::string_view& rhs)
	{
		return lhs.size() == rhs.size()
			&& boost::iequals(lhs, rhs);
	}

	bool StartsWithIgnoreCase(const std::string_view& lhs, const std::string_view& rhs)
	{
		return lhs.size() >= rhs.size()
			&& boost::istarts_with(lhs, rhs);
	}

	SC4GDriverDescription DriverDescriptionFromProperty(
		const boost::property_tree::ptree& tree,
		const char* const propertyPath)
	{
		const std::string value = tree.get<std::string>(propertyPath);

		if (EqualsIgnoreCase(value, "DirectX"))
		{
			return SC4GDriverDescription::DirectX();
		}
		else if (EqualsIgnoreCase(value, "OpenGL") || EqualsIgnoreCase(value, "SCGL"))
		{
			return SC4GDriverDescription::OpenGL();
		}
		else if (StartsWithIgnoreCase(value, "Soft")) // SC4 only checks the first 4 letters of Software.
		{
			return SC4GDriverDescription::Software();
		}
		else
		{
			Logger& logger = Logger::GetInstance();

			logger.WriteLineFormatted(
				LogLevel::Error,
				"Unknown Driver value '%s', falling back to DirectX.",
				value.c_str());

			return SC4GDriverDescription::DirectX();
		}
	}

	SC4WindowMode WindowModeFromProperty(
		const boost::property_tree::ptree& tree,
		const char* const propertyPath)
	{
		const std::string value = tree.get<std::string>(propertyPath);

		if (EqualsIgnoreCase(value, "Windowed"))
		{
			return SC4WindowMode::Windowed;
		}
		else if (EqualsIgnoreCase(value, "FullScreen"))
		{
			return SC4WindowMode::FullScreen;
		}
		else if (StartsWithIgnoreCase(value, "Borderless"))
		{
			return SC4WindowMode::BorderlessFullScreen;
		}
		else
		{
			Logger& logger = Logger::GetInstance();

			logger.WriteLineFormatted(
				LogLevel::Error,
				"Unknown WindowMode value '%s', falling back to Windowed.",
				value.c_str());

			return SC4WindowMode::Windowed;
		}
	}
}

Settings::Settings()
	: enableIntroVideo(true),
	  pauseGameOnFocusLoss(false),
	  driverDescription(SC4GDriverDescription::DirectX()),
	  windowWidth(1024),
	  windowHeight(768),
	  colorDepth(32),
	  windowMode(SC4WindowMode::Windowed)
{
}

void Settings::Load(const std::filesystem::path& path)
{
	Logger& logger = Logger::GetInstance();

	std::ifstream stream(path, std::ifstream::in);

	if (!stream)
	{
		throw std::runtime_error("Failed to open the settings file.");
	}

	boost::property_tree::ptree tree;

	boost::property_tree::ini_parser::read_ini(stream, tree);

	enableIntroVideo = tree.get<bool>("GraphicsOptions.EnableIntroVideo");
	pauseGameOnFocusLoss = tree.get<bool>("GraphicsOptions.PauseGameOnFocusLoss");
	driverDescription = DriverDescriptionFromProperty(tree, "GraphicsOptions.Driver");
	windowMode = WindowModeFromProperty(tree, "GraphicsOptions.WindowMode");
	colorDepth = tree.get<uint32_t>("GraphicsOptions.ColorDepth");

	if (colorDepth != 16 && colorDepth != 32)
	{
		logger.WriteLineFormatted(
			LogLevel::Error,
			"Unsupported color depth value %d, must be one of 16 or 32. Defaulting to 32.",
			colorDepth);
		colorDepth = 32;
	}

	const uint32_t primaryMonitorWidth = static_cast<uint32_t>(GetSystemMetrics(SM_CXSCREEN));
	const uint32_t primaryMonitorHeight = static_cast<uint32_t>(GetSystemMetrics(SM_CYSCREEN));

	if (windowMode == SC4WindowMode::Windowed)
	{
		windowWidth = tree.get<uint32_t>("GraphicsOptions.WindowWidth");
		windowHeight = tree.get<uint32_t>("GraphicsOptions.WindowHeight");

		if (windowWidth < 800 || windowHeight < 600)
		{
			logger.WriteLineFormatted(
				LogLevel::Error,
				"The window dimensions must be at least 800x600, defaulting to 800x600.",
				primaryMonitorWidth,
				primaryMonitorHeight);

			windowWidth = 800;
			windowHeight = 600;
		}
		else if (windowWidth > primaryMonitorWidth || windowHeight > primaryMonitorHeight)
		{
			logger.WriteLineFormatted(
				LogLevel::Error,
				"The window dimensions are larger than the monitor size, switching"
				"to borderless full screen mode with a resolution of %u\u0078%u.",
				primaryMonitorWidth,
				primaryMonitorHeight);

			windowWidth = primaryMonitorWidth;
			windowHeight = primaryMonitorHeight;
			windowMode = SC4WindowMode::BorderlessFullScreen;
		}
	}
	else
	{
		// For the full screen and borderless full screen modes we use the dimensions of
		// the primary monitor. SC4 does not support any monitor other than the primary.
		windowWidth = primaryMonitorWidth;
		windowHeight = primaryMonitorHeight;
	}
}

bool Settings::EnableIntroVideo() const
{
	return enableIntroVideo;
}

const SC4GDriverDescription& Settings::GetGDriverDescription() const
{
	return driverDescription;
}

uint32_t Settings::GetWindowWidth() const
{
	return windowWidth;
}

uint32_t Settings::GetWindowHeight() const
{
	return windowHeight;
}

uint32_t Settings::GetColorDepth() const
{
	return colorDepth;
}

SC4WindowMode Settings::GetWindowMode() const
{
	return windowMode;
}

bool Settings::IsUsingGDriver(uint32_t clsid) const
{
	return driverDescription.GetGZCLSID() == clsid;
}

bool Settings::PauseGameOnFocusLoss() const
{
	return pauseGameOnFocusLoss;
}
