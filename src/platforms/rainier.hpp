/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "devices/nvme.hpp"
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

class FlettNVMeDrive : public NVMeDrive
{
  public:
    static bool isPresent(SysfsI2CBus bus);

    FlettNVMeDrive(Inventory& inventory, const Nisqually& nisqually,
                   const Flett& flett, int index);
    ~FlettNVMeDrive() = default;

    /* Device */
    virtual void plug() override;
    virtual void unplug(int mode = UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    virtual std::string getInventoryPath() const override;
    virtual void addToInventory(Inventory& inventory) override;
    virtual void removeFromInventory(Inventory& inventory) override;

  private:
    std::filesystem::path getEEPROMDevicePath() const;
    SysfsI2CDevice getEEPROMDevice() const;
    std::array<uint8_t, 2> getSerial() const;

    void decorateWithI2CDevice(const std::string& path,
                               Inventory& inventory) const;
    void decorateWithVINI(const std::string& path, Inventory& inventory) const;

    const Nisqually& nisqually;
    const Flett& flett;
};

class Flett : public Device
{
  public:
    Flett(Inventory& inventory, const Nisqually& nisqually, int slot);
    ~Flett() = default;

    int getIndex() const;
    SysfsI2CBus getDriveBus(int index) const;

    /* Device */
    virtual void plug() override;
    virtual void unplug(int mode = UNPLUG_REMOVES_INVENTORY) override;

  private:
    void detectDrives(std::vector<FlettNVMeDrive>& drives);

    Inventory& inventory;
    const Nisqually& nisqually;
    int slot;
    std::vector<FlettNVMeDrive> drives;
};

class Nisqually : public Device, FRU
{
  public:
    static int getFlettIndex(int slot);
    static SysfsI2CBus getFlettSlotI2CBus(int slot);

    Nisqually(Inventory& inventory);
    ~Nisqually() = default;

    /* Device */
    virtual void plug() override;

    /* FRU */
    virtual std::string getInventoryPath() const override;
    virtual void addToInventory(Inventory& inventory) override;
    virtual void removeFromInventory(Inventory& inventory) override;

  private:
    static constexpr int slotMuxAddress = 0x70;

    static constexpr const char* flett_presence_device_path =
        "/sys/bus/i2c/devices/8-0061";
    gpiod::chip flett_presence;

    static constexpr const char* williwakas_presence_device_path =
        "/sys/bus/i2c/devices/0-0020";
    static constexpr std::array<int, 3> williwakas_presence_map = {7, 6, 5};
    gpiod::chip williwakas_presence;

    bool isFlettPresentAt(int slot) const;
    int getFlettSlot(int index) const;
    bool isFlettSlot(int slot) const;

    bool isWilliwakasPresent(int index) const;

    void detectExpanderCards(std::vector<Flett>& expanders);
    void detectDriveBackplanes(std::vector<Williwakas>& driveBackplanes);

    Inventory& inventory;
    std::vector<Williwakas> driveBackplanes;
    std::vector<Flett> ioExpanders;
};

class WilliwakasNVMeDrive : public NVMeDrive
{
  public:
    WilliwakasNVMeDrive(Inventory& inventory, const Williwakas& backplane,
                        int index);
    ~WilliwakasNVMeDrive() = default;

    /* Device */
    virtual void plug() override;
    virtual void unplug(int mode = UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    virtual std::string getInventoryPath() const override;
    virtual void addToInventory(Inventory& inventory) override;
    virtual void removeFromInventory(Inventory& inventory) override;

  private:
    void markPresent(const std::string& path, Inventory& inventory) const;

    const Williwakas& williwakas;
};

class Williwakas : public Device, FRU
{
  public:
    static std::string getInventoryPathFor(const Nisqually& nisqually,
                                           int index);

    Williwakas(Inventory& inventory, Nisqually& backplane, int index);
    ~Williwakas() = default;

    int getIndex() const;

    /* Device */
    virtual void plug() override;
    virtual void unplug(int mode = UNPLUG_REMOVES_INVENTORY) override;

    /* FRU */
    virtual std::string getInventoryPath() const override;
    virtual void addToInventory(Inventory& inventory) override;
    virtual void removeFromInventory(Inventory& inventory) override;

  private:
    static constexpr int drivePresenceDeviceAddress = 0x60;

    static constexpr std::array<const char*, 3> drive_backplane_bus = {
        "/sys/bus/i2c/devices/i2c-13",
        "/sys/bus/i2c/devices/i2c-14",
        "/sys/bus/i2c/devices/i2c-15",
    };

    static constexpr std::array<int, 8> drive_presence_map = {
        8, 9, 10, 11, 12, 13, 14, 15,
    };

    Inventory& inventory;
    Nisqually& nisqually;
    int index;
    std::vector<WilliwakasNVMeDrive> drives;
    gpiod::chip chip;
    gpiod::line_bulk lines;

    bool isDrivePresent(int index);
    void detectDrives(std::vector<WilliwakasNVMeDrive>& drives);
};

class Ingraham : public Device
{
  public:
    static SysfsI2CBus getPCIeSlotI2CBus(int slot);

    Ingraham(Inventory& inventory);
    ~Ingraham() = default;

    /* Device */
    virtual void plug() override;

  private:
    static constexpr std::array<const char*, 4> pcie_slot_busses = {
        "i2c-4",
        "i2c-5",
        "i2c-6",
        "i2c-11",
    };

    static constexpr std::array<const char*, 12> pcie_slot_bus_map = {
        "/sys/bus/i2c/devices/i2c-4",  "/sys/bus/i2c/devices/i2c-4",
        "/sys/bus/i2c/devices/i2c-4",  "/sys/bus/i2c/devices/i2c-5",
        "/sys/bus/i2c/devices/i2c-5",  nullptr,
        "/sys/bus/i2c/devices/i2c-6",  "/sys/bus/i2c/devices/i2c-6",
        "/sys/bus/i2c/devices/i2c-6",  "/sys/bus/i2c/devices/i2c-6",
        "/sys/bus/i2c/devices/i2c-11", "/sys/bus/i2c/devices/i2c-11",
    };

    static constexpr std::array<const char*, 3> williwakas_bus_map = {
        "/sys/bus/i2c/devices/i2c-13",
        "/sys/bus/i2c/devices/i2c-14",
        "/sys/bus/i2c/devices/i2c-15",
    };

    Inventory& inventory;
    Nisqually nisqually;
};

class Rainier
{
  public:
    static std::set<std::string> getSupportedModels();
};
