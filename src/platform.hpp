/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <string>

class Platform
{
  public:
    static std::string getModel();
    static bool isSupported();
};
