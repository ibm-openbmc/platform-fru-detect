/* SPDX-License-Identifier: Apache-2.0 */
#include "log.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/i2c.hpp"
#include "sysfs/gpio.hpp"

#include <cassert>
#include <filesystem>
#include <gpiod.hpp>

static constexpr const char *app_name = "NVMe Drive Management";

static const std::map<int, int> flett_mux_channel_map = {
    {  8, 3 },
    {  9, 2 },
    { 10, 0 },
    { 11, 1 },
};

static const std::map<int, int> flett_slot_presence_map = {
    {  8,  8 },
    {  9,  9 },
    { 10, 10 },
    { 11, 11 },
};

static const std::map<int, int> flett_index_map = {
    {  8, 0 },
    {  9, 1 },
    { 10, 0 },
    { 11, 2 },
};

int Nisqually::getFlettIndex(int slot)
{
    return flett_index_map.at(slot);
}

SysfsI2CBus Nisqually::getFlettSlotI2CBus(int slot)
{
    SysfsI2CBus rootBus = Ingraham::getPCIeSlotI2CBus(slot);
    SysfsI2CMux mux(rootBus, Nisqually::slotMuxAddress);

    log_debug("Looking up mux channel for Flett in slot %d\n", slot);

    int channel = flett_mux_channel_map.at(slot);

    return SysfsI2CBus(mux, channel);
}

Nisqually::Nisqually() :
    flett_presence(SysfsGPIOChip(std::filesystem::path(Nisqually::flett_presence_device_path)).getName().string(),
		   gpiod::chip::OPEN_BY_NAME),
    williwakas_presence(SysfsGPIOChip(std::filesystem::path(Nisqually::williwakas_presence_device_path)).getName().string(),
			gpiod::chip::OPEN_BY_NAME)
{
}

void Nisqually::probe()
{
    /* Slot 9 is on the same mux as slot 8 */
    Ingraham::getPCIeSlotI2CBus(8).probeDevice("pca9546", Nisqually::slotMuxAddress);
    /* Slot 11 is on the same mux as slot 10 */
    Ingraham::getPCIeSlotI2CBus(10).probeDevice("pca9546", Nisqually::slotMuxAddress);
}

std::vector<Williwakas> Nisqually::getDriveBackplanes() const
{
    std::vector<Williwakas> dbps{};

    log_debug("Matching expander cards to drive backplanes\n");

    std::vector<Flett> expanders = getExpanderCards();

    log_debug("Have %zu expanders\n", expanders.size());

    for (auto& flett : expanders)
    {
	int index = flett.getIndex();

	log_debug("Testing for Williwakas presence at index %d\n", index);

	if (!isWilliwakasPresent(index))
	{
	    log_info("Williwakas %d is not present\n", index);
	    continue;
	}

	log_debug("Found Williwakas %d\n", index);
	dbps.emplace_back(Williwakas(*this, flett));
    }

    log_debug("Found %zu drive backplanes\n", dbps.size());

    return dbps;
}

std::string Nisqually::getInventoryPath() const
{
    return "/system/chassis/motherboard";
}

bool Nisqually::isFlettPresentAt(int slot) const
{
    log_debug("Looking up Flet presence GPIO index for slot %d\n", slot);

    int flett_presence_index = flett_slot_presence_map.at(slot);

    log_debug("Testing for Flett presence via index %d\n", flett_presence_index);

    gpiod::line line = flett_presence.get_line(flett_presence_index);
    line.request({
	    app_name,
	    gpiod::line::DIRECTION_INPUT,
	    gpiod::line::ACTIVE_LOW
    });

    bool present = line.get_value();

    log_debug("Flett presence at index %d for slot %d: %d\n",
	  flett_presence_index, slot, present);

    line.release();

    return present;
}

int Nisqually::getFlettSlot(int index) const
{
    log_debug("Inspecting whether Flett %d is present\n", index);
    if (index == 0)
    {
	log_debug("Probing PCIe slot 8 presence\n");
	if (Nisqually::isFlettPresentAt(8))
	{
	    return 8;
	}

	log_debug("Probing PCIe slot 10 presence\n");
	if (Nisqually::isFlettPresentAt(10))
	{
	    return 10;
	}

	return -1;
    }
    else if (index == 1)
    {
	log_debug("Probing PCIe slot 9 presence\n");
	return Nisqually::isFlettPresentAt(9) ? 9 : -1;
    }
    else if (index == 2)
    {
	log_debug("Probing PCIe slot 11 presence\n");
	return Nisqually::isFlettPresentAt(11) ? 11 : -1;
    }

    return -1;
}

bool Nisqually::isFlettSlot(int slot) const
{
    bool contains = flett_index_map.contains(slot);

    log_debug("Is %d a Flett slot? %d\n", slot, contains);

    return contains;
}

bool Nisqually::isWilliwakasPresent(int index) const
{
    log_debug("Looking up Williwakas presence GPIO index for backplane %d\n", index);

    int williwakas_presence_index = williwakas_presence_map.at(index);

    log_debug("Testing for Williwakas presence via index %d\n", williwakas_presence_index);

    gpiod::line line = williwakas_presence.get_line(williwakas_presence_index);

    line.request({
	    app_name,
	    gpiod::line::DIRECTION_INPUT,
	    gpiod::line::ACTIVE_LOW
    });

    bool present = line.get_value();

    log_debug("Williwakas presence at index %d: %d\n", index, present);

    line.release();

    return present;
}

std::vector<Flett> Nisqually::getExpanderCards() const
{
    static constexpr std::array<int, 3> expander_indexes = {
	0, 1, 2
    };

    std::vector<Flett> expanders{};

    log_debug("Locating expander cards\n");

    for (auto& index : expander_indexes) {
	int slot = getFlettSlot(index);
	if (isFlettSlot(slot))
	{
	    log_debug("Found Flett %d in slot %d\n", index, slot);
	    expanders.emplace_back(Flett(slot));
	    expanders.back().probe();
	}
	else
	{
	    log_debug("No Flett for index %d\n", index);
	}
    }

    log_debug("Found %zu expander cards\n", expanders.size());

    return expanders;
}
