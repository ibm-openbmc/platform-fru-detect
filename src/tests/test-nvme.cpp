/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */
#include "devices/nvme.hpp"
#include "sysfs/i2c.hpp"

#include "gtest/gtest.h"

class TestNVMeDrive : public BasicNVMeDrive, public Device
{
  public:
    TestNVMeDrive(const SysfsI2CBus& bus, Inventory* inventory, int index,
                  const std::vector<uint8_t>&& metadata) :
        // NOLINTNEXTLINE(performance-move-const-arg)
        BasicNVMeDrive(bus, inventory, index, std::nullopt, std::move(metadata))
    {}

    /* Device */
    void plug([[maybe_unused]] Notifier& notifier) override
    {}
    void unplug([[maybe_unused]] Notifier& notifier,
                [[maybe_unused]] int mode = UNPLUG_REMOVES_INVENTORY) override
    {}

    /* FRU */
    std::string getInventoryPath() const override
    {
        return {""};
    }
};

TEST(DriveMetadata, smallMetadata)
{
    SysfsI2CBus bus("/sys/bus/i2c/devices/i2c-0", false);
    auto drive =
        TestNVMeDrive(bus, nullptr, 0, std::vector<uint8_t>{0, 1, 0x44});
}
