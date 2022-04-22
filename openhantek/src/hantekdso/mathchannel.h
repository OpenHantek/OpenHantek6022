// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "dsosamples.h"
#include "scopesettings.h"

class MathChannel {
  public:
    explicit MathChannel( const DsoSettingsScope *scope );
    void calculate( DSOsamples &result );

  private:
    const DsoSettingsScope *scope;
};
