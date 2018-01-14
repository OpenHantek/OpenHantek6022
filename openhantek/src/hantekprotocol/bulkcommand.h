// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <inttypes.h>
#include <vector>

namespace Hantek {
enum class BulkCode : uint8_t;
}

class BulkCommand : public std::vector<uint8_t> {
protected:
    BulkCommand(Hantek::BulkCode code, unsigned size);
public:
    Hantek::BulkCode code;
    bool pending = false;
    BulkCommand* next = nullptr;
};
