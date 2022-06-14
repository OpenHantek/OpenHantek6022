// SPDX-License-Identifier: GPL-2.0-or-later

#include "finddevices.h"

#include <QCoreApplication>
#include <QDebug>
#include <QList>
#include <QTemporaryFile>

#include "ezusb.h"
#include "utils/printutils.h"
#include <algorithm>
#ifdef Q_OS_FREEBSD
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "modelregistry.h"


FindDevices::FindDevices( libusb_context *context, int verboseLevel ) : context( context ), verboseLevel( verboseLevel ) {
    if ( verboseLevel > 1 )
        qDebug() << " FindDevices::FindDevices()";
}


// Iterate all devices on USB and keep track of all supported scopes
int FindDevices::updateDeviceList() {
    if ( verboseLevel > 2 )
        qDebug() << "  FindDevices::updateDeviceList()";
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

        if ( 0x0000 == descriptor.idVendor ) // windows sometimes reports bogus value vid=0x0000
            continue;

        if ( 0x1d6b == descriptor.idVendor ) // skip linux foundation devices, e.g. usb root hubs
            continue;

        const UniqueUSBid USBid = ScopeDevice::computeUSBdeviceID( device );

        DeviceList::const_iterator inList = devices.find( USBid );
        if ( inList != devices.end() ) { // already in list, update heartbeat only
            inList->second->setFindIteration( findIteration );
            continue;
        }
        // else check against all supported models for match
        for ( DSOModel *model : ModelRegistry::get()->models() ) {
            if ( DemoDeviceID == model->ID ) // skip the DEMO device
                continue;
            // Check VID and PID for firmware flashed devices
            bool supported = descriptor.idVendor == model->vendorID && descriptor.idProduct == model->productID;
            // Devices without firmware have different VID/PIDs
            supported |= descriptor.idVendor == model->vendorIDnoFirmware && descriptor.idProduct == model->productIDnoFirmware;
            if ( supported ) { // put matching device into list if not already in use
                ++changes;
                if ( verboseLevel > 2 )
                    qDebug() << "  +++" << QString( "0x%1" ).arg( USBid, 8, 16, QChar( '0' ) ) << model->name;
                devices[ USBid ] = std::unique_ptr< ScopeDevice >( new ScopeDevice( model, device, findIteration ) );
                break; // stop after 1st supported model (there can be more models with identical VID/PID)
            }
        }
    }

    // Remove non existing devices
    for ( DeviceList::iterator it = devices.begin(); it != devices.end(); ) {
        if ( it->second->getFindIteration() != findIteration ) { // heartbeat not up to date, no more on the bus
            ++changes;
            if ( verboseLevel > 2 )
                qDebug() << "  ---" << QString( "0x%1" ).arg( it->first, 8, 16, QChar( '0' ) ) << it->second->getModel()->name;
            // printf( "- %016lX\n", it->first );
            it = devices.erase( it ); // it points to next entry
        } else {
            ++it;
        }
    }
    libusb_free_device_list( deviceList, false );
    return changes; // report number of all detected bus changes (added + removed devices)
}


const FindDevices::DeviceList *FindDevices::getDevices() { return &devices; }


std::unique_ptr< ScopeDevice > FindDevices::takeDevice( UniqueUSBid id ) {
    DeviceList::iterator it = devices.find( id );
    if ( it == devices.end() )
        return nullptr;
    return std::move( it->second );
}
