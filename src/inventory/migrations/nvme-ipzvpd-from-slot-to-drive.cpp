/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */

#include "inventory.hpp"
#include "inventory/migrations.hpp"

#include <filesystem>

using namespace inventory;

Migration::Result
    MigrateNVMeIPZVPDFromSlotToDrive::migrate(Inventory* inventory,
                                              const std::string& path,
                                              const ObjectType& object) const
{
    std::filesystem::path fp(path);

    if (!object.contains(INVENTORY_ITEM_PCIESLOT_IFACE))
    {
        return Result::INVALID;
    }

    // A bit hacky as D-Bus paths aren't filesystem paths, but whatever
    if (!fp.filename().string().starts_with("nvme"))
    {
        return Result::INVALID;
    }

    if (!object.contains(INVENTORY_IPZVPD_VINI_IFACE))
    {
        return Result::INVALID;
    }

    const InterfaceType& viniData = object.at(INVENTORY_IPZVPD_VINI_IFACE);
    const auto& rtData = std::get<std::vector<uint8_t>>(viniData.at("RT"));
    const bool isVINI = rtData == std::vector<uint8_t>({'V', 'I', 'N', 'I'});
    const auto& ccData = std::get<std::vector<uint8_t>>(viniData.at("CC"));
    const bool haveNVMeCC = ccData ==
                            std::vector<uint8_t>({'N', 'V', 'M', 'e'});

    if (!(isVINI && haveNVMeCC))
    {
        return Result::INVALID;
    }

    inventory->remove(path, interfaces::VINI());

    return Result::SUCCESS;
}
