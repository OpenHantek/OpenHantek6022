// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Dso {
/// \enum ErrorCode
/// \brief The return codes for device control methods.
enum class ErrorCode {
    NONE = 0,         ///< Successful operation
    CONNECTION = -1,  ///< Device not connected or communication error
    UNSUPPORTED = -2, ///< Not supported by this device
    PARAMETER = -3    ///< Parameter out of range
};

} // namespace Dso
