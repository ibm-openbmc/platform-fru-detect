/* SPDX-License-Identifier: Apache-2.0 */

#include "platform.hpp"

#include "platforms/rainier.hpp"
#include "sysfs/devicetree.hpp"

#include <set>

std::string Platform::getModel()
{
    return SysfsDevicetree::getModel();
}

bool Platform::isSupported()
{
    std::set<std::string> rainier = Rainier::getSupportedModels();

    return rainier.contains(Platform::getModel());
}
