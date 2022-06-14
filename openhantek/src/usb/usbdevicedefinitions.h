// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define HANTEK_TIMEOUT 500       ///< Timeout for USB transfers in ms
#define HANTEK_TIMEOUT_MULTI 500 ///< Timeout for multi packet USB transfers in ms
#define HANTEK_ATTEMPTS 3        ///< The number of transfer attempts
#define HANTEK_ATTEMPTS_MULTI 1  ///< The number of multi packet transfer attempts

#define HANTEK_EP_OUT 0x02 ///< OUT Endpoint for bulk transfers
#define HANTEK_EP_IN 0x86  ///< IN Endpoint for bulk transfers

/// \brief The speed level of the USB connection.
enum ConnectionSpeed {
    CONNECTION_FULLSPEED = 0, ///< FullSpeed USB, 64 byte bulk transfers
    CONNECTION_HIGHSPEED = 1  ///< HighSpeed USB, 512 byte bulk transfers
};
