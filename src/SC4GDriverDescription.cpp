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

#include "SC4GDriverDescription.h"
#include "SC4GDriverCLSIDDefs.h"

SC4GDriverDescription& SC4GDriverDescription::DirectX()
{
	static SC4GDriverDescription instance(kSCGDriverDirectX, "DirectX", true);

	return instance;
}

SC4GDriverDescription& SC4GDriverDescription::OpenGL()
{
	static SC4GDriverDescription instance(kSCGDriverOpenGL, "OpenGL", true);

	return instance;
}

SC4GDriverDescription& SC4GDriverDescription::Software()
{
	static SC4GDriverDescription instance(kSCGDriverSoftware, "Software", false);

	return instance;
}

SC4GDriverDescription::SC4GDriverDescription(
	uint32_t clsid,
	const std::string& name,
	bool hardwareDriver)
	: clsid(clsid),
	  name(name),
	  isHardwareDriver(hardwareDriver)
{
}

uint32_t SC4GDriverDescription::GetGZCLSID() const
{
	return clsid;
}

const std::string& SC4GDriverDescription::GetName() const
{
	return name;
}

bool SC4GDriverDescription::IsHardwareDriver() const
{
	return isHardwareDriver;
}
