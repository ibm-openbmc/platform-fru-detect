/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "devices/nvme.hpp"
#include "platform.hpp"

class Driskill;
class Pennybacker;

class DriskillNVMeDrive : public NVMeDrive, public Device, public FRU
{
  public:
    DriskillNVMeDrive(Inventory* inventory, const Driskill* driskill,
                      int index);
    DriskillNVMeDrive(const DriskillNVMeDrive& other) = delete;
    DriskillNVMeDrive(DriskillNVMeDrive&& other) = delete;
    virtual ~DriskillNVMeDrive() = default;

    DriskillNVMeDrive& operator=(const DriskillNVMeDrive& other) = delete;
    DriskillNVMeDrive& operator=(DriskillNVMeDrive&& other) = delete;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    std::string getInventoryPath() const override;
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  private:
    Inventory* inventory;
    const Driskill* driskill;
    int index;
    std::optional<BasicNVMeDrive> drive;
};

class Driskill : public Device, public FRU
{
  public:
    explicit Driskill(Inventory* inventory, const Pennybacker* pennybacker);
    Driskill(const Driskill& other) = delete;
    Driskill(const Driskill&& other) = delete;
    virtual ~Driskill() = default;

    Driskill& operator=(const Driskill& other) = delete;
    Driskill& operator=(const Driskill&& other) = delete;

    static SysfsI2CBus getDriveBus(int driveIndex);

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    std::string getInventoryPath() const override;
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  private:
    static constexpr int drivePresenceDeviceAddress = 0x60;
    static constexpr std::array<int, 4> drivePresenceMap = {4, 5, 6, 7};

    const Pennybacker* pennybacker;
    std::array<PolledConnector<DriskillNVMeDrive>, 4> polledDriveConnectors;
};

class Pennybacker : public Device, public FRU
{
  public:
    explicit Pennybacker(Inventory* inventory);
    Pennybacker(const Pennybacker& other) = delete;
    Pennybacker(const Pennybacker&& other) = delete;
    virtual ~Pennybacker() = default;

    Pennybacker& operator=(const Pennybacker& other) = delete;
    Pennybacker& operator=(const Pennybacker&& other) = delete;

    static SysfsI2CBus getDrivePresenceBus();
    static SysfsI2CBus getDriveBus(int driveIndex);

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    std::string getInventoryPath() const override;
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  private:
    static constexpr const char* driskillPresenceDevicePath =
        "/sys/bus/i2c/devices/0-0020";
    static constexpr int driskillPresenceOffset = 7;
    static constexpr const char* drivePresenceBus =
        "/sys/bus/i2c/devices/i2c-13";
    static constexpr const char* driveManagementMux =
        "/sys/bus/i2c/devices/11-0075";
    static constexpr std::array<int, 4> driveChannelMap = {0, 2, 1, 3};

    Inventory* inventory;
    PolledConnector<Driskill> polledDriskillConnector;
};

class Bonnell : public Platform
{
  public:
    Bonnell() = default;

    /* Platform */
    void enrollWith(PlatformManager& pm) override;
    void detectFrus(Notifier& notifier, Inventory* inventory) override;
};
