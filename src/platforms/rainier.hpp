/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "notify.hpp"
#include "platform.hpp"
#include "sysfs/i2c.hpp"

#include <gpiod.hpp>

#include <array>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <vector>

class Flett;
class Inventory;
class Nisqually;
class Williwakas;

class FlettNVMeDrive : public NVMeDrive, public Device, public FRU
{
  public:
    static bool isPresent(SysfsI2CBus bus);

    explicit FlettNVMeDrive(Inventory* inventory, const Nisqually* nisqually,
                            const Flett* flett, int index);
    FlettNVMeDrive(const FlettNVMeDrive& other) = delete;
    FlettNVMeDrive(const FlettNVMeDrive&& other) = delete;
    ~FlettNVMeDrive() override = default;

    FlettNVMeDrive& operator=(const FlettNVMeDrive& other) = delete;
    FlettNVMeDrive& operator=(const FlettNVMeDrive&& other) = delete;

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
    const Nisqually* nisqually;
    const Flett* flett;
    int index;
    std::optional<BasicNVMeDrive> drive;
};

class Flett : public Device
{
  public:
    static std::string getInventoryPathFor(const Nisqually* nisqually,
                                           int slot);

    explicit Flett(Inventory* inventory, const Nisqually* nisqually, int slot);
    Flett(const Flett& other) = delete;
    Flett(const Flett&& other) = delete;
    ~Flett() override = default;

    Flett& operator=(const Flett& other) = delete;
    Flett& operator=(const Flett&& other) = delete;

    int getIndex() const;
    SysfsI2CBus getDriveBus(int index) const;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

  private:
    Inventory* inventory;
    const Nisqually* nisqually;
    int slot;
    std::array<PolledConnector<FlettNVMeDrive>, 8> polledDriveConnectors;
};

class WilliwakasNVMeDrive : public NVMeDrive, public Device, public FRU
{
  public:
    explicit WilliwakasNVMeDrive(Inventory* inventory,
                                 const Williwakas* backplane, int index);
    WilliwakasNVMeDrive(const WilliwakasNVMeDrive& other) = delete;
    WilliwakasNVMeDrive(const WilliwakasNVMeDrive&& other) = delete;
    ~WilliwakasNVMeDrive() override = default;

    WilliwakasNVMeDrive& operator=(const WilliwakasNVMeDrive& other) = delete;
    WilliwakasNVMeDrive& operator=(const WilliwakasNVMeDrive&& other) = delete;

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
    const Williwakas* williwakas;
    int index;
};

class Williwakas : public Device, public FRU
{
  public:
    static std::string getInventoryPathFor(const Nisqually* nisqually,
                                           int index);

    explicit Williwakas(Inventory* inventory, const Nisqually* backplane,
                        int index);
    Williwakas(const Williwakas& other) = delete;
    Williwakas(const Williwakas&& other) = delete;
    ~Williwakas() override = default;

    Williwakas& operator=(const Williwakas& other) = delete;
    Williwakas& operator=(const Williwakas&& other) = delete;

    int getIndex() const;

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

    static constexpr std::array<const char*, 3> driveBackplaneBus = {
        "/sys/bus/i2c/devices/i2c-13",
        "/sys/bus/i2c/devices/i2c-14",
        "/sys/bus/i2c/devices/i2c-15",
    };

    static constexpr std::array<int, 8> drivePresenceMap = {
        8, 9, 10, 11, 12, 13, 14, 15,
    };

    Inventory* inventory;
    const Nisqually* nisqually;
    int index;
    std::array<PolledConnector<WilliwakasNVMeDrive>, 8> polledDriveConnectors;

    bool isDrivePresent(int index);
    void detectDrives(Notifier& notifier);
};

class Nisqually : public Device, FRU
{
  public:
    static int getFlettIndex(int slot);

    explicit Nisqually(Inventory* inventory);
    Nisqually(const Nisqually& other) = delete;
    Nisqually(const Nisqually&& other) = delete;
    ~Nisqually() override = default;

    Nisqually& operator=(const Nisqually& other) = delete;
    Nisqually& operator=(const Nisqually&& other) = delete;

    virtual SysfsI2CBus getFlettSlotI2CBus(int slot) const = 0;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    std::string getInventoryPath() const override;
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  protected:
    bool isFlettPresentAt(int slot);

    Inventory* inventory;

  private:
    static constexpr const char* williwakasPresenceDevicePath =
        "/sys/bus/i2c/devices/0-0020";
    static constexpr std::array<int, 3> williwakasPresenceMap = {7, 6, 5};

    void detectFlettCards(Notifier& notifier);
    void detectWilliwakasCards(Notifier& notifier);

    std::array<Connector<Flett>, 4> flettConnectors;
    std::array<Connector<Williwakas>, 3> williwakasConnectors;
};

class Nisqually0z : public Nisqually
{
  public:
    explicit Nisqually0z(Inventory* inventory);
    Nisqually0z(const Nisqually0z& other) = delete;
    Nisqually0z(const Nisqually0z&& other) = delete;
    ~Nisqually0z() override = default;

    Nisqually0z& operator=(const Nisqually0z& other) = delete;
    Nisqually0z& operator=(const Nisqually0z&& other) = delete;

    /* Nisqually */
    SysfsI2CBus getFlettSlotI2CBus(int slot) const override;
};

class Nisqually1z : public Nisqually
{
  public:
    explicit Nisqually1z(Inventory* inventory);
    Nisqually1z(const Nisqually1z& other) = delete;
    Nisqually1z(const Nisqually1z&& other) = delete;
    ~Nisqually1z() override = default;

    Nisqually1z& operator=(const Nisqually1z& other) = delete;
    Nisqually1z& operator=(const Nisqually1z&& other) = delete;

    /* Nisqually */
    SysfsI2CBus getFlettSlotI2CBus(int slot) const override;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = UNPLUG_REMOVES_INVENTORY) override;

  private:
    static constexpr int slotMuxAddress = 0x70;
};

class Ingraham : public Device
{
  public:
    static SysfsI2CBus getPCIeSlotI2CBus(int slot);

    explicit Ingraham(Inventory* inventory, Nisqually* nisqually);
    Ingraham(const Ingraham& other) = delete;
    Ingraham(const Ingraham&& other) = delete;
    ~Ingraham() override = default;

    Ingraham& operator=(const Ingraham& other) = delete;
    Ingraham& operator=(const Ingraham&& other) = delete;

    /* Device */
    void plug(Notifier& notifier) override;
    void unplug(Notifier& notifier,
                int mode = Device::UNPLUG_REMOVES_INVENTORY) override;

  private:
    static constexpr std::array<const char*, 12> pcieSlotBusMap = {
        "/sys/bus/i2c/devices/i2c-4",  "/sys/bus/i2c/devices/i2c-4",
        "/sys/bus/i2c/devices/i2c-4",  "/sys/bus/i2c/devices/i2c-5",
        "/sys/bus/i2c/devices/i2c-5",  nullptr,
        "/sys/bus/i2c/devices/i2c-6",  "/sys/bus/i2c/devices/i2c-6",
        "/sys/bus/i2c/devices/i2c-6",  "/sys/bus/i2c/devices/i2c-6",
        "/sys/bus/i2c/devices/i2c-11", "/sys/bus/i2c/devices/i2c-11",
    };

    Inventory* inventory;
    Nisqually* nisqually;
};

class Rainier0z : public Platform
{
  public:
    Rainier0z() = default;
    ~Rainier0z() override = default;

    void enrollWith(PlatformManager& pm) override;
    void detectFrus(Notifier& notifier, Inventory* inventory) override;
};

class Rainier1z : public Platform
{
  public:
    Rainier1z() = default;
    ~Rainier1z() override = default;

    void enrollWith(PlatformManager& pm) override;
    void detectFrus(Notifier& notifier, Inventory* inventory) override;
};
