/* SPDX-License-Identifier: Apache-2.0 */

#include "platform.hpp"

#include "sysfs/devicetree.hpp"

PlatformManager::PlatformManager() : model(SysfsDevicetree::getModel())
{}

const std::string& PlatformManager::getPlatformModel() noexcept
{
    return model;
}

void PlatformManager::enrollPlatform(const std::string& model,
                                     Platform* platform)
{
    platforms[model] = platform;
}

bool PlatformManager::isSupportedPlatform() noexcept
{
    return platforms.contains(model);
}

void PlatformManager::detectPlatformFrus()
{
    Notifier notifier;

    platforms[model]->detectFrus(notifier);
}
