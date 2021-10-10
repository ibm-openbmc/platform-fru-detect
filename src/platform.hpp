/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <stdexcept>
#include <string>

class Device
{
  public:
    virtual ~Device() = default;

    virtual void plug() = 0;
};

class Platform
{
  public:
    static std::string getModel();
    static bool isSupported();
};
