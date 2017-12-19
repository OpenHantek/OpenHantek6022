// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QObject>
#include <QStringList>
#include <libusb-1.0/libusb.h>
#include <memory>

#include "controlbegin.h"
#include "definitions.h"
#include "models.h"
#include "utils/dataarray.h"

#define HANTEK_TIMEOUT 500       ///< Timeout for USB transfers in ms
#define HANTEK_TIMEOUT_MULTI 100 ///< Timeout for multi packet USB transfers in ms
#define HANTEK_ATTEMPTS 3        ///< The number of transfer attempts
#define HANTEK_ATTEMPTS_MULTI 1  ///< The number of multi packet transfer attempts

/// \brief This class handles the USB communication with an usb device that has
/// one in and one out endpoint.
class USBDevice : public QObject {
    Q_OBJECT

  public:
    USBDevice(DSOModel model, libusb_device *device);
    ~USBDevice();
    bool connectDevice(QString &errorMessage);

    /// \brief Check if the oscilloscope is connected.
    /// \return true, if a connection is up.
    bool isConnected();
    bool needsFirmware();

    // Various methods to handle USB transfers

    /// \brief Bulk transfer to/from the oscilloscope.
    /// \param endpoint Endpoint number, also sets the direction of the transfer.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \param timeout The timeout in ms.
    /// \return Number of transferred bytes on success, libusb error code on
    /// error.
    int bulkTransfer(unsigned char endpoint, unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS,
                     unsigned int timeout = HANTEK_TIMEOUT);
    int bulkWrite(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS);
    int bulkRead(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS);

    int bulkCommand(DataArray<unsigned char> *command, int attempts = HANTEK_ATTEMPTS);
    int bulkReadMulti(unsigned char *data, unsigned length, int attempts = HANTEK_ATTEMPTS_MULTI);

    int controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value,
                        int index, int attempts = HANTEK_ATTEMPTS);
    int controlWrite(unsigned char request, unsigned char *data, unsigned int length, int value = 0, int index = 0,
                     int attempts = HANTEK_ATTEMPTS);
    int controlRead(unsigned char request, unsigned char *data, unsigned int length, int value = 0, int index = 0,
                    int attempts = HANTEK_ATTEMPTS);

    int getConnectionSpeed();
    int getPacketSize();
    int getUniqueModelID();

    libusb_device *getRawDevice() const;
    const DSOModel &getModel() const;
    void setEnableBulkTransfer(bool enable);
    void overwriteInPacketLength(int len);

  protected:
    int claimInterface(const libusb_interface_descriptor *interfaceDescriptor, int endpointOut, int endPointIn);
    void connectionLost();

    // Command buffers
    ControlBeginCommand *beginCommandControl = new ControlBeginCommand();
    DSOModel model;

    // Libusb specific variables
    struct libusb_device_descriptor descriptor;
    libusb_device *device; ///< The USB handle for the oscilloscope
    libusb_device_handle *handle = nullptr;
    int interface;
    int outPacketLength; ///< Packet length for the OUT endpoint
    int inPacketLength;  ///< Packet length for the IN endpoint
    bool allowBulkTransfer = true;
  signals:
    void deviceDisconnected(); ///< The device has been disconnected
};
