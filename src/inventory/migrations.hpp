/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */
#pragma once

#include "inventory.hpp"

#include <string>

namespace inventory
{
class Migration
{
  public:
    enum Result
    {
        INVALID,
        SUCCESS,
        FAILED
    };

    Migration(const std::string&& name) : migrationName(name)
    {}
    virtual ~Migration() = default;

    virtual enum Result migrate([[maybe_unused]] Inventory* inventory,
                                [[maybe_unused]] const std::string& path,
                                [[maybe_unused]] const ObjectType& object) const
    {
        throw std::logic_error("Unimplemented");
    }

    const std::string& name() const
    {
        return migrationName;
    }

  private:
    std::string migrationName;
};

class MigrateNVMeIPZVPDFromSlotToDrive : public Migration
{
  public:
    MigrateNVMeIPZVPDFromSlotToDrive() : Migration(__func__)
    {}
    ~MigrateNVMeIPZVPDFromSlotToDrive() override = default;

    enum Result migrate(Inventory* inventory, const std::string& path,
                        const inventory::ObjectType& object) const override;
};

class MigrateNVMeI2CEndpointFromSlotToDrive : public Migration
{
  public:
    MigrateNVMeI2CEndpointFromSlotToDrive() : Migration(__func__)
    {}
    ~MigrateNVMeI2CEndpointFromSlotToDrive() override = default;

    enum Result migrate(Inventory* inventory, const std::string& path,
                        const inventory::ObjectType& object) const override;
};
} // namespace inventory
