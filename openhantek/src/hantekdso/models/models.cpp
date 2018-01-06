// SPDX-License-Identifier: GPL-2.0+

#include "models.h"
#include "modelDSO2090.h"
#include "modelDSO2150.h"
#include "modelDSO2250.h"
#include "modelDSO5200.h"
#include "modelDSO6022.h"

std::list<DSOModel*> supportedModels =
    std::list<DSOModel*>({new ModelDSO2090(), new ModelDSO2090A(), new ModelDSO2150(),
                         new  ModelDSO2250(), new ModelDSO5200(), new ModelDSO5200A(),
                         new ModelDSO6022BE(), new ModelDSO6022BL()});
