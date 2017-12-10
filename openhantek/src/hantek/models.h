
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>
#include <list>

/// \brief All supported Hantek DSO models.
enum Model {
    MODEL_UNKNOWN = -1, ///< Unknown model
    MODEL_DSO2090,      ///< %Hantek DSO-2090 USB
    MODEL_DSO2150,      ///< %Hantek DSO-2150 USB
    MODEL_DSO2250,      ///< %Hantek DSO-2250 USB
    MODEL_DSO5200,      ///< %Hantek DSO-5200 USB
    MODEL_DSO5200A,     ///< %Hantek DSO-5200A USB
    MODEL_DSO6022BE,    ///< %Hantek6022BE USB
    MODEL_COUNT
};

class DSOModel {
public:
    Model uniqueModelID;
    long vendorID;
    long productID;
    long vendorIDnoFirmware;
    long productIDnoFirmware;
    std::string firmwareToken;
    std::string name;
    DSOModel(Model model, long vendorID, long productID,
             long vendorIDnoFirmware,long productIDnoFirmware,
             std::string firmwareToken,const std::string name)
        : uniqueModelID(model), vendorID(vendorID),productID(productID),
          vendorIDnoFirmware(vendorIDnoFirmware),productIDnoFirmware(productIDnoFirmware),
          firmwareToken(firmwareToken),name(name) { }
};

static std::list<DSOModel> supportedModels = \
        std::list<DSOModel>({DSOModel(MODEL_DSO2090,  0x04b5, 0x2090, 0x04b4, 0x2090, "dso2090x86", "DSO-2090"),
                             DSOModel(MODEL_DSO2090,  0x04b5, 0x2090, 0x04b4, 0x8613, "dso2090x86", "DSO-2090"),
                             DSOModel(MODEL_DSO2150,  0x04b5, 0x2150, 0x04b4, 0x2150, "dso2150x86", "DSO-2150"),
                             DSOModel(MODEL_DSO2250,  0x04b5, 0x2250, 0x04b4, 0x2250, "dso2250x86", "DSO-2250"),
                             DSOModel(MODEL_DSO5200,  0x04b5, 0x5200, 0x04b4, 0x5200, "dso5200x86", "DSO-5200"),
                             DSOModel(MODEL_DSO5200A, 0x04b5, 0x520a, 0x04b4, 0x520a, "dso5200ax86","DSO-5200A"),
                             DSOModel(MODEL_DSO6022BE,0x04b5, 0x6022, 0x04b4, 0x6022, "dso6022be",  "DSO-6022BE"),
                             DSOModel(MODEL_DSO6022BE,0x04b5, 0x602a, 0x04b4, 0x602a, "dso6022be",  "DSO-6022LE")});

