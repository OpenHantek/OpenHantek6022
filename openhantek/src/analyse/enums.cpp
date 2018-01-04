#include "enums.h"

namespace Dso {

Enum<Dso::MathMode,Dso::MathMode::ADD_CH1_CH2,Dso::MathMode::SUB_CH1_FROM_CH2> MathModeEnum;
Enum<Dso::WindowFunction,Dso::WindowFunction::RECTANGULAR,Dso::WindowFunction::FLATTOP> WindowFunctionEnum;

}
