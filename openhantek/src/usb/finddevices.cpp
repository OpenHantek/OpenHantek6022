// SPDX-License-Identifier: GPL-2.0+

#include "finddevices.h"

#include <QCoreApplication>
#include <QDebug>
#include <QList>
#include <QTemporaryFile>

#include "ezusb.h"
#include "utils/printutils.h"
#include <algorithm>
#ifdef __FreeBSD__
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "modelregistry.h"


FindDevices::FindDevices( libusb_context *context ) : context( context ) {}


// Iterate all devices on USB and keep track of all supported scopes
int FindDevices::updateDeviceList() {
    libusb_device **deviceList;
    ssize_t deviceCount = libusb_get_device_list( context, &deviceList );
    if ( deviceCount < 0 ) {
        return int( deviceCount );
    }

    ++findIteration;
    int changes = 0;

    for ( ssize_t deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex ) {
        libusb_device *device = deviceList[ deviceIndex ];
        // Get device descriptor
        struct libusb_device_descriptor descriptor;
        libusb_get_device_descriptor( device, &descriptor );

        if ( 0x1d6b == descriptor.idVendor ) // skip linux foundation devices, e.g. usb root hubs
            continue;

        const UniqueUSBid USBid = USBDevice::computeUSBdeviceID( device );

        DeviceList::const_iterator inList = devices.find( USBid );
        if ( inList != devices.end() ) { // already in list, update heartbeat only
            inList->second->setFindIteration( findIteration );
            continue;
        }
        // else check against all supported models for match
        for ( DSOModel *model : ModelRegistry::get()->models() ) {
            // Check VID and PID for firmware flashed devices
            bool supported = descriptor.idVendor == model->vendorID && descriptor.idProduct == model->productID;
            // Devices without firmware have different VID/PIDs
            supported |= descriptor.idVendor == model->vendorIDnoFirmware && descriptor.idProduct == model->productIDnoFirmware;
            if ( supported ) { // put matching device into list
                ++changes;
                // printf( "+ %016lX %s\n", USBid, model->name.c_str() );
                devices[ USBid ] = std::unique_ptr< USBDevice >( new USBDevice( model, device, findIteration ) );
                break; // stop after 1st supported model (there can be more models with identical VID/PID)
            }
        }
    }

    // Remove non existing devices
    for ( DeviceList::iterator it = devices.begin(); it != devices.end(); ) {
        if ( it->second->getFindIteration() != findIteration ) { // heartbeat not up to date, no more on the bus
            ++changes;
            // printf( "- %016lX\n", it->first );
            it = devices.erase( it ); // it points to next entry
        } else {
            ++it;
        }
    }
#if defined __FreeBSD__
    libusb_free_device_list( deviceList, false ); // free the list but don't unref the devices
#else
    // TODO check if this crashes on MacOSX, Windows
    // TODO check if change true -> false solves it
    // move it up by appending " || defined __YOUR_OS__" to the line "#if defined ..."
    libusb_free_device_list( deviceList, true ); // linux and some other systems unref also the USB devices
#endif
    return changes; // report number of all detected bus changes (added + removed devices)
}


const FindDevices::DeviceList *FindDevices::getDevices() { return &devices; }


std::unique_ptr< USBDevice > FindDevices::takeDevice( UniqueUSBid id ) {
    DeviceList::iterator it = devices.find( id );
    if ( it == devices.end() )
        return nullptr;
    return std::move( it->second );
}
