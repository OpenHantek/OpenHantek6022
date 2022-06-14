// SPDX-License-Identifier: GPL-2.0-or-later

#include "modelregistry.h"

ModelRegistry *ModelRegistry::get() {
    static ModelRegistry inst;
    return &inst;
}

void ModelRegistry::add( DSOModel *model ) { supportedModels.push_back( model ); }

const std::list< DSOModel * > ModelRegistry::models() const { return supportedModels; }
