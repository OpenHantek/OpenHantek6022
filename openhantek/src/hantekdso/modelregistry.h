// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsomodel.h"

class ModelRegistry {
  public:
    static ModelRegistry *get();
    void add( DSOModel *model );
    const std::list< DSOModel * > models() const;

  private:
    std::list< DSOModel * > supportedModels;
};
