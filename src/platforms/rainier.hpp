/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "platform.hpp"
#include "sysfs/i2c.hpp"

#include <gpiod.hpp>

#include <array>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <vector>

class Inventory;
class NVMeDrive;
class Williwakas;

class Flett
{
  public:
    Flett(Inventory& inventory, int index);
    ~Flett() = default;

    int getSlot() const;
    int getIndex() const;

    SysfsI2CBus getDriveBus(int index) const;
    bool isDriveEEPROMPresent(int index) const;
    NVMeDrive getDrive(Williwakas backplane, int index) const;

    int probe();

  private:
    Inventory& inventory;
    int slot;
    int expander;
};

class Williwakas;

class Nisqually
{
  public:
    static int getFlettIndex(int slot);
    static SysfsI2CBus getFlettSlotI2CBus(int slot);

    Nisqually(Inventory& inventory);
    ~Nisqually() = default;

    void probe();

    std::vector<Williwakas> getDriveBackplanes() const;
    std::string getInventoryPath() const;

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

    std::vector<Flett> getExpanderCards() const;
    Inventory& inventory;
};

class Williwakas
{
  public:
    Williwakas(Inventory& inventory, Nisqually backplane, Flett flett);
    ~Williwakas() = default;

    const Flett& getFlett() const;
    std::vector<NVMeDrive> getDrives() const;
    std::string getInventoryPath() const;
    int getIndex() const;

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
    Nisqually nisqually;
    Flett flett;
    gpiod::chip chip;
    gpiod::line_bulk lines;

    bool isDrivePresent(int index);
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

    Nisqually getBackplane() const;

    Inventory& inventory;
};

class Rainier
{
  public:
    static std::set<std::string> getSupportedModels();
};
