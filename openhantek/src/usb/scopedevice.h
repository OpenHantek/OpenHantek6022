// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QStringList>

#ifdef Q_OS_FREEBSD
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#include <memory>

#include "models/modelDEMO.h"
#include "usbdevicedefinitions.h"

class DSOModel;

typedef uint64_t UniqueUSBid;


/// \brief Returns string representation for libusb errors.
/// \param error The error code.
/// \return String explaining the error.
const QString libUsbErrorString( int error );


/// \brief This class handles the USB communication with an usb device that has
/// one in and one out endpoint.
class ScopeDevice : public QObject {
    Q_OBJECT

  public:
    explicit ScopeDevice( DSOModel *model, libusb_device *device, unsigned findIteration = 0 );
    explicit ScopeDevice();
    ScopeDevice( const ScopeDevice & ) = delete;
    ~ScopeDevice() override;
    bool connectDevice( QString &errorMessage );
    void disconnectFromDevice();

    /// \brief Check if the oscilloscope is connected.
    /// \return true, if a connection is up.
    bool isConnected();

    /// \brief Distinguish between real hw or demo device
    bool isRealHW() const { return realHW; }
    bool isDemoDevice() const { return !realHW; }

    /// \brief Stop a long running (interruptible) bulk transfer
    void stopSampling() { stopTransfer = true; }

    bool hasStopped() {
        bool stopped = stopTransfer;
        stopTransfer = false;
        return stopped;
    }

    /**
     * @return Return true if this device needs a firmware first
     */
    bool needsFirmware();

    /**
     * @return Return device version as unsigned int
     */
    inline unsigned int getFwVersion() const { return descriptor.bcdDevice; }

    /**
     * @brief getSerialNumber
     * @return Return serial number string of the device
     */
    inline const QString getSerialNumber() const { return serialNumber; }


    /**
     * @brief readUSBdescriptor
     * @param handle The device handle
     * @param index of requested string
     * @return serial number string
     */
    QString readUSBdescriptor( libusb_device_handle *handle, uint8_t index );

    /**
     * Keep track of the find iteration on which this device was found
     * @param iteration The new iteration value
     */
    inline void setFindIteration( unsigned iteration ) { findIteration = iteration; }
    inline unsigned getFindIteration() const { return findIteration; }

    /// \brief Multi packet bulk read from the oscilloscope.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of data contained in the packets.
    /// \param captureSmallBlocks Capture many small blocks instead of one big block (faster gui update)
    /// \param received The amount of already captured samples
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    int bulkReadMulti( unsigned char *data, unsigned length, bool captureSmallBlocks, unsigned &received,
                       int attempts = HANTEK_ATTEMPTS_MULTI );

    /// \brief Control transfer to the oscilloscope.
    /// \param type The request type, also sets the direction of the transfer.
    /// \param request The request field of the packet.
    /// \param data Buffer for the sent/received data.
    /// \param length The length field of the packet.
    /// \param value The value field of the packet.
    /// \param index The index field of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of transferred bytes on success, libusb error code on error.
    int controlTransfer( unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value, int index,
                         int attempts = HANTEK_ATTEMPTS );

    /// \brief Control write to the oscilloscope.
    /// \param command Buffer for the sent/received data.
    /// \return Number of sent bytes on success, libusb error code on error.
    template < class T > inline int controlWrite( const T *command ) {
        return controlTransfer( uint8_t( LIBUSB_REQUEST_TYPE_VENDOR ) | uint8_t( LIBUSB_ENDPOINT_OUT ), uint8_t( command->code ),
                                const_cast< unsigned char * >( command->data() ), unsigned( command->size() ), command->value, 0,
                                HANTEK_ATTEMPTS );
    }

    /// \brief Control read to the oscilloscope.
    /// \param command Buffer for the sent/received data.
    /// \return Number of received bytes on success, libusb error code on error.
    template < class T > inline int controlRead( const T *command ) {
        return controlTransfer( uint8_t( LIBUSB_REQUEST_TYPE_VENDOR ) | uint8_t( LIBUSB_ENDPOINT_IN ), uint8_t( command->code ),
                                const_cast< unsigned char * >( command->data() ), unsigned( command->size() ), command->value, 0,
                                HANTEK_ATTEMPTS );
    }

    /**
     * @return Returns the raw libusb device
     */
    inline libusb_device *getUSBDevice() const { return device; }

    /**
     * @return Return the unique usb device id {@link USBDevice::computeUSBdeviceID()}.
     */
    inline UniqueUSBid getUniqueUSBDeviceID() const { return uniqueUSBdeviceID; }
    /**
     * ID built from bus, port, VID, PID and FW version
     */
    static UniqueUSBid computeUSBdeviceID( libusb_device *device );

    /// \brief Get the oscilloscope model.
    /// \return The ::Model of the connected Hantek DSO.
    inline const DSOModel *getModel() const { return model; }
    /**
     * Usually a maximum packet length for in and outgoing packets is determined
     * by the underlying implementation and usb specification. E.g. the roll buffer
     * mode uses the maximum in length for transfer. Some devices do not support
     * that much data though and need an artification restriction.
     */
    inline void overwriteInPacketLength( unsigned len ) { inPacketLength = len; }

  protected:
    int claimInterface( const libusb_interface_descriptor *interfaceDescriptor );

    // Device model data
    DSOModel *model;

    // Libusb specific variables
    struct libusb_device_descriptor descriptor;
    libusb_device *device; ///< The USB handle for the oscilloscope
    libusb_device_handle *handle = nullptr;
    unsigned findIteration;
    const UniqueUSBid uniqueUSBdeviceID;
    int nInterface;
    unsigned outPacketLength; ///< Packet length for the OUT endpoint
    unsigned inPacketLength;  ///< Packet length for the IN endpoint

  private:
    /// \brief Bulk transfer to/from the oscilloscope.
    /// \param endpoint Endpoint number, also sets the direction of the transfer.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \param timeout The timeout in ms.
    /// \return Number of transferred bytes on success, libusb error code on error.
    int bulkTransfer( unsigned char endpoint, const unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS,
                      unsigned int timeout = HANTEK_TIMEOUT );

    /// \brief Bulk write to the oscilloscope.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    inline int bulkWrite( const unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS ) {
        return bulkTransfer( HANTEK_EP_OUT, data, length, attempts );
    }

    /// \brief Bulk read from the oscilloscope.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    template < class T > inline int bulkRead( const T *command, int attempts = HANTEK_ATTEMPTS ) {
        return bulkTransfer( HANTEK_EP_IN, command->data(), command->size(), attempts );
    }

    bool realHW = true;
    bool stopTransfer = false;
    bool disconnected = true;
    QString serialNumber = "0000";

  signals:
    void deviceDisconnected(); ///< The device has been disconnected
};

extern int verboseLevel;
