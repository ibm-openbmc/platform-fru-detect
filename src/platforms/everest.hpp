/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "devices/nvme.hpp"
#include "platform.hpp"

#include <gpiod.hpp>

class Basecamp;
class Bellavista;

class BasecampNVMeDrive : public BasicNVMeDrive, public Device
{
  public:
    static std::string getInventoryPathFor(const Basecamp* basecamp, int index);

    BasecampNVMeDrive(Inventory* inventory, const Basecamp* basecamp,
                      int index);
    ~BasecampNVMeDrive() override = default;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  private:
    Inventory* inventory;
    const Basecamp* basecamp;
    int index;
};

class Basecamp : public Device, public FRU
{
  public:
    explicit Basecamp(Inventory* inventory, const Bellavista* bellavista);
    Basecamp(const Basecamp& other) = delete;
    Basecamp(const Basecamp&& other) = delete;
    ~Basecamp() override = default;

    Basecamp& operator=(const Basecamp& other) = delete;
    Basecamp& operator=(const Basecamp&& other) = delete;

    SysfsI2CBus getDriveBus(int index) const;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    std::string getInventoryPath() const override;
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  private:
    static constexpr const char* driveMetadataBus =
        "/sys/bus/i2c/devices/i2c-14";
    static constexpr int driveMetadataMuxAddress = 0x70;
    static constexpr int driveMetadataMuxChannel = 3;
    static constexpr int drivePresenceDeviceAddress = 0x61;
    static constexpr std::array<int, 10> drivePresenceMap = {0, 1, 2, 3, 4,
                                                             5, 6, 7, 8, 9};

    static constexpr const char* driveManagementBus =
        "/sys/bus/i2c/devices/i2c-15";
    static constexpr std::array<int, 10> driveMuxMap = {
        0x70, 0x70, 0x70, 0x70, 0x71, 0x71, 0x71, 0x71, 0x72, 0x72};
    static constexpr std::array<int, 10> driveChannelMap = {0, 1, 2, 3, 0,
                                                            1, 2, 3, 0, 1};

    Inventory* inventory;
    const Bellavista* bellavista;
    std::array<gpiod::line, 10> lines;
    std::array<PolledConnector<BasecampNVMeDrive>, 10> polledDriveConnectors;
};

class Bellavista : public Device, public FRU
{
  public:
    explicit Bellavista(Inventory* inventory);
    Bellavista(const Bellavista& other) = delete;
    Bellavista(const Bellavista&& other) = delete;
    ~Bellavista() override = default;

    Bellavista& operator=(const Bellavista& other) = delete;
    Bellavista& operator=(const Bellavista&& other) = delete;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    std::string getInventoryPath() const override;
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  private:
    static constexpr const char* basecampPresenceDevicePath =
        "/sys/bus/i2c/devices/0-0062";
    static constexpr int basecampPresenceOffset = 12;

    Inventory* inventory;
    gpiod::chip basecampPresenceChip;
    gpiod::line basecampPresenceLine;
    PolledConnector<Basecamp> polledBasecampConnector;
};

class Tola : public Device
{
  public:
    explicit Tola(Inventory* inventory);
    Tola(const Tola& other) = delete;
    Tola(const Tola&& other) = delete;
    ~Tola() override = default;

    Tola& operator=(const Tola& other) = delete;
    Tola& operator=(const Tola&& other) = delete;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

  private:
    Inventory* inventory;
    Bellavista bellavista;
};

class Everest : public Platform
{
  public:
    Everest() = default;
    ~Everest() override = default;

    /* Platform */
    void enrollWith(PlatformManager& pm) override;
    void detectFrus(Notifier& notifier, Inventory* inventory) override;
};
