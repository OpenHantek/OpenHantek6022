// SPDX-License-Identifier: GPL-2.0-or-later

#include "usb/devicereconnectionsupervisor.h"

#include "hantekdso/hantekdsocontrol.h"
#include "usb/finddevices.h"
#include "usb/uploadFirmware.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>

DeviceReconnectionSupervisor::DeviceReconnectionSupervisor( libusb_context *context, HantekDsoControl *dsoControl,
                                                            std::unique_ptr< ScopeDevice > &currentDevice, int verboseLevel,
                                                            QObject *parent )
    : QObject( parent ), context( context ), dsoControl( dsoControl ), currentDevice( currentDevice ), verboseLevel( verboseLevel ) {
    reconnectTimer.setInterval( 1000 );
    reconnectTimer.setSingleShot( false );
    connect( &reconnectTimer, &QTimer::timeout, this, &DeviceReconnectionSupervisor::pollForReconnect );

    if ( currentDevice ) {
        previousUSBid = currentDevice->getUniqueUSBDeviceID();
        previousSerial = currentDevice->getSerialNumber();
        model = currentDevice->getModel();
        connectDeviceSignals( currentDevice.get() );
    }
}


void DeviceReconnectionSupervisor::setClosing( bool closing ) {
    this->closing = closing;
    reconnectTimer.stop();
    if ( currentDevice )
        currentDevice->setExpectedDisconnect( closing );
}


void DeviceReconnectionSupervisor::connectDeviceSignals( ScopeDevice *device ) {
    if ( !device )
        return;

    connect( device, &ScopeDevice::deviceDisconnected, this, &DeviceReconnectionSupervisor::handleDeviceDisconnected,
             Qt::QueuedConnection );
}


bool DeviceReconnectionSupervisor::uploadFirmwareIfNeeded( ScopeDevice *device ) {
    if ( !device || !device->needsFirmware() )
        return false;

    UploadFirmware firmware;
    if ( firmware.startUpload( device ) ) {
        if ( verboseLevel > 0 )
            qWarning() << "Uploaded oscilloscope firmware during reconnect; waiting for USB re-enumeration";
    } else if ( !firmware.getErrorMessage().isEmpty() ) {
        qWarning() << firmware.getErrorMessage();
    }
    return true;
}


void DeviceReconnectionSupervisor::handleDeviceDisconnected( bool expected ) {
    if ( closing || expected )
        return;

    if ( reconnectInProgress || state == DeviceConnectionState::Disconnected || state == DeviceConnectionState::Scanning )
        return;

    if ( verboseLevel > 0 )
        qWarning() << "USB device disconnected; keeping application open and starting reconnect polling";

    reconnectInProgress = true;
    if ( dsoControl )
        QMetaObject::invokeMethod( dsoControl, &HantekDsoControl::parkForReconnect, Qt::BlockingQueuedConnection );

    changeState( DeviceConnectionState::Disconnected, tr( "Oscilloscope disconnected. Waiting for reconnect..." ) );
    reconnectTimer.start();
}


void DeviceReconnectionSupervisor::pollForReconnect() {
    if ( closing || !context || !model )
        return;

    changeState( DeviceConnectionState::Scanning, tr( "Scanning for oscilloscope..." ), 1000 );

    std::unique_ptr< ScopeDevice > candidate = takeReconnectCandidate();
    if ( !candidate )
        return;

    if ( rebind( std::move( candidate ) ) ) {
        reconnectTimer.stop();
        reconnectInProgress = false;
        changeState( DeviceConnectionState::Connected, tr( "Oscilloscope reconnected" ), 3000 );
    } else {
        changeState( DeviceConnectionState::Failed, tr( "Oscilloscope reconnect failed. Retrying..." ), 3000 );
    }
}


std::unique_ptr< ScopeDevice > DeviceReconnectionSupervisor::takeReconnectCandidate() {
    FindDevices finder( context, verboseLevel );
    int changes = finder.updateDeviceList();
    if ( changes < 0 ) {
        qWarning() << "Device reconnect scan failed:" << libUsbErrorString( changes );
        return nullptr;
    }

    const FindDevices::DeviceList *devices = finder.getDevices();
    if ( previousUSBid ) {
        auto previousDevice = devices->find( previousUSBid );
        if ( previousDevice != devices->end() && previousDevice->second && previousDevice->second->getModel() == model ) {
            if ( uploadFirmwareIfNeeded( previousDevice->second.get() ) )
                return nullptr;
            return finder.takeDevice( previousUSBid );
        }
    }

    std::vector< UniqueUSBid > matchingModelIds;
    for ( const auto &entry : *devices ) {
        ScopeDevice *candidate = entry.second.get();
        if ( candidate && candidate->getModel() == model )
            matchingModelIds.push_back( entry.first );
    }

    std::unique_ptr< ScopeDevice > modelFallback;
    const bool allowModelFallback = matchingModelIds.size() == 1;

    for ( UniqueUSBid id : matchingModelIds ) {
        std::unique_ptr< ScopeDevice > candidate = finder.takeDevice( id );
        if ( !candidate )
            continue;
        if ( uploadFirmwareIfNeeded( candidate.get() ) ) {
            candidate->setExpectedDisconnect( true );
            continue;
        }
        if ( previousSerial.isEmpty() )
            return candidate;

        QString errorMessage;
        if ( !candidate->connectDevice( errorMessage ) ) {
            candidate->setExpectedDisconnect( true );
            continue;
        }

        const QString candidateSerial = candidate->getSerialNumber();
        if ( candidateSerial == previousSerial )
            return candidate;

        if ( allowModelFallback && !modelFallback ) {
            qWarning() << "Reconnected oscilloscope serial changed; falling back to the only matching model"
                       << candidateSerial;
            modelFallback = std::move( candidate );
            continue;
        }

        candidate->setExpectedDisconnect( true );
    }

    return modelFallback;
}


bool DeviceReconnectionSupervisor::rebind( std::unique_ptr< ScopeDevice > newDevice ) {
    if ( !newDevice )
        return false;

    changeState( DeviceConnectionState::Rebinding, tr( "Rebinding oscilloscope..." ), 1000 );

    QString errorMessage;
    if ( !newDevice->connectDevice( errorMessage ) ) {
        if ( !errorMessage.isEmpty() )
            qWarning() << errorMessage;
        return false;
    }

    previousUSBid = newDevice->getUniqueUSBDeviceID();
    previousSerial = newDevice->getSerialNumber();
    model = newDevice->getModel();
    connectDeviceSignals( newDevice.get() );

    std::unique_ptr< ScopeDevice > oldDevice = std::move( currentDevice );
    if ( oldDevice )
        oldDevice->setExpectedDisconnect( true );
    currentDevice = std::move( newDevice );

    bool rebound = false;
    if ( dsoControl ) {
        ScopeDevice *device = currentDevice.get();
        rebound = QMetaObject::invokeMethod( dsoControl, [ this, device ]() { dsoControl->rebindDevice( device ); },
                                            Qt::BlockingQueuedConnection );
    }

    if ( oldDevice )
        retiredDevices.emplace_back( std::move( oldDevice ) );
    retiredDevices.clear();

    return rebound;
}


void DeviceReconnectionSupervisor::changeState( DeviceConnectionState nextState, const QString &message, int timeout ) {
    Q_UNUSED( timeout );
    state = nextState;
    emit connectionStateChanged( state, message );
}
