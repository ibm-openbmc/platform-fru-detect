/* SPDX-License-Identifier: Apache-2.0 */
#include "devices/nvme.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/i2c.hpp"
#include "sysfs/gpio.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <map>
#include <string>

PHOSPHOR_LOG2_USING;

static constexpr const char *name = "foo";

static const std::map<std::string, std::map<int, int>> flett_mux_slot_map = {
    { "i2c-6",  { { 0x74, 9 },
		  { 0x75, 8 } } },
    { "i2c-11", { { 0x74, 10 },
		  { 0x75, 11 } } },
};

static const std::map<int, int> flett_slot_mux_map = {
    {  8, 0x75 },
    {  9, 0x74 },
    { 10, 0x74 },
    { 11, 0x75 },
};

static const std::map<int, int> flett_slot_eeprom_map = {
    {  8, 0x51 },
    {  9, 0x50 },
    { 10, 0x50 },
    { 11, 0x51 },
};

Flett::Flett(int slot) : slot(slot)
{
    debug("Instantiated Flett in slot {PCIE_SLOT}", "PCIE_SLOT", slot);
}

int Flett::getSlot() const
{
    return slot;
}

int Flett::getIndex() const
{
    return Nisqually::getFlettIndex(slot);
}

int Flett::probe()
{
    SysfsI2CBus bus = Nisqually::getFlettSlotI2CBus(slot);

#if 0 /* FIXME: Well, fix qemu */
    bus.probeDevice("24c02", flett_slot_eeprom_map.at(slot));
#endif
    bus.probeDevice("pca9546", flett_slot_mux_map.at(slot));

    return 0;
}

SysfsI2CBus Flett::getDriveBus(int index) const
{
    SysfsI2CMux flettMux(Nisqually::getFlettSlotI2CBus(slot),
			 flett_slot_mux_map.at(slot));

    return SysfsI2CBus(flettMux, index);
}

bool Flett::isDriveEEPROMPresent(int index) const
{
    return NVMeDrive::isPresent(getDriveBus(index));
}

NVMeDrive Flett::getDrive(Williwakas backplane, int index) const
{
    return NVMeDrive(backplane, index);
}
