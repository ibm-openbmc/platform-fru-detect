/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "log.hpp"

#include <filesystem>
#include <iostream>

class SysfsEntry {
    public:
	SysfsEntry(const SysfsEntry& entry) : path(entry.path) { }
	SysfsEntry(std::filesystem::path path) : path(path)
	{
	    log_debug("%s\n", path.string().c_str());

	    if (!std::filesystem::exists(path))
		throw -1;
	}

	virtual ~SysfsEntry() = default;

	std::filesystem::path getPath()
	{
	    return std::filesystem::path(path);
	}

    protected:
	std::filesystem::path path;
};
