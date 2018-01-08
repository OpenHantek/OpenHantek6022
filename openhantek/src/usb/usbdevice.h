// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QObject>
#include <QStringList>
#include <libusb-1.0/libusb.h>
#include <memory>

#include "usbdevicedefinitions.h"
#include "controlbegin.h"

class DSOModel;
class ControlCommand;

typedef unsigned long UniqueUSBid;

/// \brief This class handles the USB communication with an usb device that has
/// one in and one out endpoint.
class USBDevice : public QObject {
    Q_OBJECT

  public:
    USBDevice(DSOModel* model, libusb_device *device, unsigned findIteration = 0);
    ~USBDevice();
    bool connectDevice(QString &errorMessage);
    void disconnectFromDevice();

    /// \brief Check if the oscilloscope is connected.
    /// \return true, if a connection is up.
    bool isConnected();

    /**
     * @return Return true if this device needs a firmware first
     */
    bool needsFirmware();

    /**
     * Keep track of the find iteration on which this device was found
     * @param iteration The new iteration value
     */
    void setFindIteration(unsigned iteration);
    unsigned getFindIteration() const;

    /// \brief Bulk transfer to/from the oscilloscope.
    /// \param endpoint Endpoint number, also sets the direction of the transfer.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \param timeout The timeout in ms.
    /// \return Number of transferred bytes on success, libusb error code on
    /// error.
    int bulkTransfer(unsigned char endpoint, const unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS,
                     unsigned int timeout = HANTEK_TIMEOUT);

    /// \brief Bulk write to the oscilloscope.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    int bulkWrite(const unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS);

    /// \brief Bulk read from the oscilloscope.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    int bulkRead(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS);

    /// \brief Send a bulk command to the oscilloscope.
    /// \param command The command, that should be sent.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    int bulkCommand(const DataArray<unsigned char> *command, int attempts = HANTEK_ATTEMPTS);

    /// \brief Multi packet bulk read from the oscilloscope.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length of data contained in the packets.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    int bulkReadMulti(unsigned char *data, unsigned length, int attempts = HANTEK_ATTEMPTS_MULTI);

    /// \brief Control transfer to the oscilloscope.
    /// \param type The request type, also sets the direction of the transfer.
    /// \param request The request field of the packet.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length field of the packet.
    /// \param value The value field of the packet.
    /// \param index The index field of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of transferred bytes on success, libusb error code on error.
    int controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value,
                        int index, int attempts = HANTEK_ATTEMPTS);

    /// \brief Control write to the oscilloscope.
    /// \param request The request field of the packet.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length field of the packet.
    /// \param value The value field of the packet.
    /// \param index The index field of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    int controlWrite(const ControlCommand *command);

    /// \brief Control read to the oscilloscope.
    /// \param request The request field of the packet.
    /// \param data Buffer for the sent/recieved data.
    /// \param length The length field of the packet.
    /// \param value The value field of the packet.
    /// \param index The index field of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    int controlRead(const ControlCommand *command);

    /// \brief Gets the speed of the connection.
    /// \return The ::ConnectionSpeed of the USB connection.
    int getConnectionSpeed();

    /// \brief Gets the maximum size of one packet transmitted via bulk transfer.
    /// \return The maximum packet size in bytes, negative libusb error code on error.
    int getPacketSize();

    /**
     * @return Returns the raw libusb device
     */
    libusb_device *getRawDevice() const;

    /**
     * @return Return the unique usb device id {@link USBDevice::computeUSBdeviceID()}.
     */
    unsigned long getUniqueUSBDeviceID() const;
    /**
     * The USB bus is organised in a tree hierarchy. A device is connected to a port on a bus device,
     * which is connected to a port on another bus device etc up to the root usb device.
     *
     * The USB 3.0 standard allows up to 7 levels with 256 devices on each level (1 Byte). We generate
     * a unique number for the connected device.
     */
    static UniqueUSBid computeUSBdeviceID(libusb_device *device);

    /// \brief Get the oscilloscope model.
    /// \return The ::Model of the connected Hantek DSO.
    const DSOModel *getModel() const;
    void setEnableBulkTransfer(bool enable);
    void overwriteInPacketLength(int len);
  protected:
    int claimInterface(const libusb_interface_descriptor *interfaceDescriptor, int endpointOut, int endPointIn);

    // Command buffers
    ControlBeginCommand beginCommandControl;
    DSOModel* model;

    // Libusb specific variables
    struct libusb_device_descriptor descriptor;
    libusb_device *device; ///< The USB handle for the oscilloscope
    libusb_device_handle *handle = nullptr;
    unsigned findIteration;
    const unsigned long uniqueUSBdeviceID;
    int interface;
    int outPacketLength; ///< Packet length for the OUT endpoint
    int inPacketLength;  ///< Packet length for the IN endpoint
    bool allowBulkTransfer = true;
  signals:
    void deviceDisconnected(); ///< The device has been disconnected
};
