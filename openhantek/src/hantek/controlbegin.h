
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "definitions.h"
#include "utils/dataarray.h"

//////////////////////////////////////////////////////////////////////////////
/// \class ControlBeginCommand                                  hantek/types.h
/// \brief The CONTROL_BEGINCOMMAND builder.
class ControlBeginCommand : public DataArray<uint8_t> {
  public:
    ControlBeginCommand(Hantek::BulkIndex index = Hantek::COMMANDINDEX_0);

    Hantek::BulkIndex getIndex();
    void setIndex(Hantek::BulkIndex index);

  private:
    void init();
};
