/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "sysfs/i2c.hpp"

#include <cassert>

int SysfsI2CMux::extractChannel(std::string& name)
{
    /* Extract '0' from 'i2c-29-mux (chan_id 0)' */

    std::string::size_type pos = name.find(' ');
    assert(pos != std::string::npos);
    pos = name.find('(', pos + 1);
    assert(pos != std::string::npos);
    pos = name.find(' ', pos + 1);
    assert(pos != std::string::npos);

    return std::stoi(name.substr(pos + 1));
}
