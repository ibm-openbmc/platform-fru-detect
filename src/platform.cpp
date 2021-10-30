/* SPDX-License-Identifier: Apache-2.0 */

#include "platform.hpp"

#include "platforms/rainier.hpp"
#include "sysfs/devicetree.hpp"

#include <set>

std::string PlatformManager::getPlatformModel()
{
    return SysfsDevicetree::getModel();
}

bool PlatformManager::isSupportedPlatform()
{
    std::set<std::string> rainier = Rainier::getSupportedModels();

    return rainier.contains(PlatformManager::getPlatformModel());
}
