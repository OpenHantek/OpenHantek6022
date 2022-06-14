// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QTemporaryFile>
#ifdef Q_OS_FREEBSD
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#include <memory>

#include "ezusb.h"
#include "scopedevice.h"
#include "uploadFirmware.h"

#include "dsomodel.h"

#define TR( str ) ( QString( "UploadFirmware: " ) + QCoreApplication::translate( "UploadFirmware", str ) )

bool UploadFirmware::startUpload( ScopeDevice *scopeDevice ) {
    if ( scopeDevice->isConnected() || !scopeDevice->needsFirmware() )
        return false;

    // Open device
    libusb_device_handle *handle;
    int status = libusb_open( scopeDevice->getUSBDevice(), &handle );
    if ( status != LIBUSB_SUCCESS ) {
        handle = nullptr;
        errorMessage = TR( "Couldn't open device: %1" ).arg( libUsbErrorString( status ) );
        return false;
    }

    // Write firmware from resources to temp files
    QFile firmwareRes( QString( ":/firmware/%1-firmware.hex" ).arg( scopeDevice->getModel()->firmwareToken ) );
    auto temp_firmware_path = std::unique_ptr< QTemporaryFile >( QTemporaryFile::createNativeFile( firmwareRes ) );
    if ( !temp_firmware_path )
        return false;
    temp_firmware_path->open();

#ifdef Q_OS_LINUX
    // Detach kernel driver, reported to lead to an error on FreeBSD, MacOSX and Windows
    status = libusb_set_auto_detach_kernel_driver( handle, 1 );
    if ( status != LIBUSB_SUCCESS && status != LIBUSB_ERROR_NOT_SUPPORTED ) {
        errorMessage = TR( "libusb_set_auto_detach_kernel_driver() failed: %1" ).arg( libusb_error_name( status ) );
        libusb_close( handle );
        return false;
    }
#endif

    // We need to claim the first interface (num=0)
    status = libusb_claim_interface( handle, 0 );
    if ( status != LIBUSB_SUCCESS ) {
        errorMessage = TR( "libusb_claim_interface() failed: %1" ).arg( libusb_error_name( status ) );
        libusb_close( handle );
        return false;
    }

    // Write firmware into internal RAM using first stage loader built into EZ-USB hardware
    // use local 8bit encoding (utf8 for Linux, iso-8859-x for Windows)
    status = ezusb_load_ram( handle, temp_firmware_path->fileName().toLocal8Bit().constData(), FX_TYPE_FX2LP, 0 );
    if ( status != LIBUSB_SUCCESS ) {
        errorMessage = TR( "Writing the main firmware failed: %1" ).arg( libusb_error_name( status ) );
        libusb_release_interface( handle, 0 );
        libusb_close( handle );
        return false;
    }

    status = libusb_release_interface( handle, 0 );
    libusb_close( handle );

    return status == LIBUSB_SUCCESS;
}

const QString &UploadFirmware::getErrorMessage() const { return errorMessage; }
