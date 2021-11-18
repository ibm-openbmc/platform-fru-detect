/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/rainier.hpp"

#include <sdbusplus/bus.hpp>

PHOSPHOR_LOG2_USING;

const std::vector<std::string> Rainier0z::modelNames{
    "Rainier 1S4U Pass 1", "Rainier 4U Pass 1", "Rainier 2U Pass 1"};

const std::vector<std::string> Rainier1z::modelNames{
    "Rainier 1S4U", "Rainier 4U", "Rainier 2U"};

static constexpr auto rainierPCIeCardInventoryRoot =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/pcieslot";

bool Rainier0z::isPresent(const std::string& model)
{
    return Platform::isSupportedModel(modelNames, model);
}

void Rainier0z::enrollWith(PlatformManager& pm)
{
    std::for_each(
        modelNames.begin(), modelNames.end(),
        [&pm, this](const auto& name) { pm.enrollPlatform(name, this); });
}

void Rainier0z::detectFrus(Notifier& notifier)
{
    /* Cold-plug devices */
    ingraham.plug(notifier);

    /* Hot-plug devices */
    notifier.run();

    /* Clean up the application state but leave the inventory in-tact. */
    ingraham.unplug(notifier, ingraham.UNPLUG_RETAINS_INVENTORY);
}

void Rainier0z::slotPowerStateChanged(int slot, bool powerOn)
{
    nisqually.slotPowerStateChanged(slot, powerOn);
}

bool Rainier0z::ignoreSlotPowerState(const std::string& slotPath) const
{
    return !slotPath.starts_with(rainierPCIeCardInventoryRoot);
}

bool Rainier1z::isPresent(const std::string& model)
{
    return Platform::isSupportedModel(modelNames, model);
}

void Rainier1z::enrollWith(PlatformManager& pm)
{
    std::for_each(
        modelNames.begin(), modelNames.end(),
        [&pm, this](const auto& name) { pm.enrollPlatform(name, this); });
}

void Rainier1z::detectFrus(Notifier& notifier)
{
    /* Cold-plug devices */
    ingraham.plug(notifier);

    /* Hot-plug devices */
    notifier.run();

    /* Clean up the application state but leave the inventory in-tact. */
    ingraham.unplug(notifier, ingraham.UNPLUG_RETAINS_INVENTORY);
}

void Rainier1z::slotPowerStateChanged(int slot, bool powerOn)
{
    nisqually.slotPowerStateChanged(slot, powerOn);
}

bool Rainier1z::ignoreSlotPowerState(const std::string& slotPath) const
{
    return !slotPath.starts_with(rainierPCIeCardInventoryRoot);
}
