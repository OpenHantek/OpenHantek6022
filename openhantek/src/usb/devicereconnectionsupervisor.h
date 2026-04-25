// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "usb/scopedevice.h"

#include <QObject>
#include <QTimer>

#include <memory>
#include <vector>

class HantekDsoControl;
class ScopeDevice;
struct libusb_context;

enum class DeviceConnectionState { Connected, Disconnected, Scanning, Rebinding, Failed };

/// Supervises runtime USB loss without terminating the Qt application.
class DeviceReconnectionSupervisor : public QObject {
    Q_OBJECT

  public:
    DeviceReconnectionSupervisor( libusb_context *context, HantekDsoControl *dsoControl,
                                  std::unique_ptr< ScopeDevice > &currentDevice, int verboseLevel,
                                  QObject *parent = nullptr );

    void setClosing( bool closing = true );

  public slots:
    void handleDeviceDisconnected( bool expected );

  private slots:
    void pollForReconnect();

  private:
    void connectDeviceSignals( ScopeDevice *device );
    bool uploadFirmwareIfNeeded( ScopeDevice *device );
    std::unique_ptr< ScopeDevice > takeReconnectCandidate();
    bool rebind( std::unique_ptr< ScopeDevice > newDevice );
    void changeState( DeviceConnectionState nextState, const QString &message, int timeout = 0 );

    libusb_context *context;
    HantekDsoControl *dsoControl;
    std::unique_ptr< ScopeDevice > &currentDevice;
    std::vector< std::unique_ptr< ScopeDevice > > retiredDevices;
    QTimer reconnectTimer;
    UniqueUSBid previousUSBid = 0;
    QString previousSerial;
    const DSOModel *model = nullptr;
    int verboseLevel = 0;
    bool closing = false;
    bool reconnectInProgress = false;
    DeviceConnectionState state = DeviceConnectionState::Connected;

  signals:
    void connectionStateChanged( DeviceConnectionState state, const QString &message );
};
