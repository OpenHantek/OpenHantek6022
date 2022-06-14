// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <list>
#include <map>
#include <memory>

#include "scopedevice.h"

struct libusb_context;

/**
 * @brief Search for Hantek devices.
 * Use usually want to call `updateDeviceList` and then retrieve the list via `getDevices`.
 * You can call `updateDeviceList` as often as you want.
 * If you have found your favorite device, you want to call `takeDevice`. The device will
 * not be available in `getDevices` anymore and this will not change with calls to `updateDeviceList`.
 *
 * Do not close the given usb context before this class object is destroyed.
 */
class FindDevices {
  public:
    typedef std::map< UniqueUSBid, std::unique_ptr< ScopeDevice > > DeviceList;
    explicit FindDevices( libusb_context *context, int verboseLevel = 0 );
    /// Updates the device list. To clear the list, just dispose this object
    /// \return If negative it represents a libusb error code otherwise the amount of updates
    int updateDeviceList();
    const DeviceList *getDevices();
    /**
     * @brief takeDevice
     * @param id The unique usb id for the current bus layout
     * @return A shared reference to the
     */
    std::unique_ptr< ScopeDevice > takeDevice( UniqueUSBid id );

  private:
    libusb_context *context; ///< The usb context used for this device
    DeviceList devices;
    unsigned findIteration = 0;
    int verboseLevel = 0;
};
