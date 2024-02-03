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

#pragma once
#include "SC4GDriverDescription.h"
#include <filesystem>

enum class SC4WindowMode
{
	Windowed = 0,
	FullScreen,
	BorderlessFullScreen
};

class Settings
{
public:

	Settings();

	void Load(const std::filesystem::path& path);

	bool EnableIntroVideo() const;

	const SC4GDriverDescription& GetGDriverDescription() const;

	uint32_t GetWindowWidth() const;

	uint32_t GetWindowHeight() const;

	uint32_t GetColorDepth() const;

	SC4WindowMode GetWindowMode() const;

	bool IsUsingGDriver(uint32_t clsid) const;

	bool PauseGameOnFocusLoss() const;

private:

	bool enableIntroVideo;
	bool pauseGameOnFocusLoss;
	SC4GDriverDescription driverDescription;
	uint32_t windowWidth;
	uint32_t windowHeight;
	uint32_t colorDepth;
	SC4WindowMode windowMode;
};

