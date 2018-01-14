// SPDX-License-Identifier: GPL-2.0+

#include "modelregistry.h"

ModelRegistry *ModelRegistry::instance = new ModelRegistry();

ModelRegistry *ModelRegistry::get() { return instance; }

void ModelRegistry::add(DSOModel *model) { supportedModels.push_back(model); }

const std::list<DSOModel *> ModelRegistry::models() const { return supportedModels; }
