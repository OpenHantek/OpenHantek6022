#include "controlspecification.h"

Dso::ControlSpecification::ControlSpecification(unsigned channels) : channels(channels) {
    voltageLimit.resize(channels);
}
