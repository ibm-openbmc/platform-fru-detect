/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "platform.hpp"

#include <phosphor-logging/lg2.hpp>

#include <array>
#include <stdexcept>

class Inventory;

class NVMeDrive : public FRU
{
  public:
    NVMeDrive(Inventory& inventory, int index) :
        inventory(inventory), index(index)
    {}
    virtual ~NVMeDrive() = default;

    /* FRU */
    virtual std::string getInventoryPath() const override
    {
        throw std::logic_error("Not implemented");
    }

    virtual void addToInventory([[maybe_unused]] Inventory& inventory) override
    {
        throw std::logic_error("Not implemented");
    }

  protected:
    /* FRU Information Device, NVMe Storage Device (non-Carrier) */
    static constexpr int eepromAddress = 0x53;

    Inventory& inventory;
    int index;
};
