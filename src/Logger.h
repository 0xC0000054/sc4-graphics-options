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
#include <filesystem>
#include <fstream>

enum class LogLevel : int32_t
{
	Info = 0,
	Error = 1,
	Debug = 2,
	Trace = 3
};

class Logger
{
public:

	static Logger& GetInstance();

	void Init(std::filesystem::path logFilePath, LogLevel logLevel, bool includeTimeStamp = true);

	bool IsEnabled(LogLevel option) const;

	void WriteLogFileHeader(const char* const message);

	void WriteLine(LogLevel level, const char* const message);

	void WriteLineFormatted(LogLevel level, const char* const format, ...);

private:

	Logger();
	~Logger();

	void WriteLineCore(const char* const message);

	bool initialized;
	bool writeTimeStamp;
	LogLevel logLevel;
	std::ofstream logFile;
};

