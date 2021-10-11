/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <string>

class Inventory;

class FRU
{
  public:
    FRU() = default;
    virtual ~FRU() = default;

    virtual void addToInventory([[maybe_unused]] Inventory& inventory) = 0;
};

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
