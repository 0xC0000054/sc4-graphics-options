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
#include <string>

// Provides information about a SC4 graphics driver.
class SC4GDriverDescription
{
public:

	static SC4GDriverDescription& DirectX();

	static SC4GDriverDescription& OpenGL();

	static SC4GDriverDescription& Software();

	uint32_t GetGZCLSID() const;

	const std::string& GetName() const;

	bool IsHardwareDriver() const;

private:

	SC4GDriverDescription(uint32_t clsid, const std::string& name, bool hardwareDriver);

	uint32_t clsid;
	std::string name;
	bool isHardwareDriver;
};

