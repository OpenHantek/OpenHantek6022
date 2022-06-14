// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <inttypes.h>
#include <vector>

namespace Hantek {
enum class ControlCode : uint8_t;
}

class ControlCommand : public std::vector< uint8_t > {

  protected:
    ControlCommand( Hantek::ControlCode code, unsigned size );

  public:
    bool pending = false;
    uint8_t code;
    uint8_t value = 0;
    ControlCommand *next = nullptr;
};
