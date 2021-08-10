/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "sysfs/i2c.hpp"

#include <filesystem>

#include <iostream>

class SysfsGPIOChip : public SysfsEntry {
    public:
	SysfsGPIOChip(SysfsEntry entry) :
	    SysfsEntry(SysfsGPIOChip::getGPIOChipPath(entry)) { }
	virtual ~SysfsGPIOChip() = default;

	static bool hasGPIOChip(SysfsEntry entry)
	{
	    /* FIXME: This is the deprecated attribute */
	    return std::filesystem::exists(entry.getPath() / "gpio");
	}

	std::filesystem::path getName()
	{
	    return path.filename();
	}

    private:
	static std::filesystem::path getGPIOChipPath(SysfsEntry entry)
	{
	    namespace fs = std::filesystem;

	    for(auto const& dirent: fs::directory_iterator{entry.getPath()}) {
		if (dirent.path().filename().string().starts_with("gpiochip"))
		{
		    return dirent.path();
		}
	    }

	    /* FIXME: Throw something better? */
	    throw -1;
	}
};
