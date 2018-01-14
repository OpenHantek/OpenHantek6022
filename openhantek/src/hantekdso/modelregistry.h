
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "dsomodel.h"

class ModelRegistry {
public:
    static ModelRegistry *get();
    void add(DSOModel* model);
    const std::list<DSOModel*> models() const;
private:
    static ModelRegistry* instance;
    std::list<DSOModel*> supportedModels;
};
