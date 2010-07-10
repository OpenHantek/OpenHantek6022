////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  hantek/device.cpp
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////


#include <QList>


#include "hantek/device.h"

#include "helper.h"
#include "hantek/types.h"


namespace Hantek {
	////////////////////////////////////////////////////////////////////////////////
	// class Hantek::Device
	/// \brief Initializes the usb things and lists.
	/// \param parent The parent widget.
	Device::Device(QObject *parent) : QObject(parent) {
		// Product ids and names for the Model enum
		this->modelIds << 0x2090 << 0x2100 << 0x2150 << 0x2250
				<< 0x5200 << 0x520A;
		this->modelStrings << "DSO-2090" << "DSO-2100" << "DSO-2150" << "DSO-2250"
				<< "DSO-5200" << "DSO-5200A";
		
		this->beginCommandControl = new ControlBeginCommand();
		
		this->handle = 0;
		this->interface = -1;
		this->error = libusb_init(&(this->context));
	}
	
	/// \brief Disconnects the device.
	Device::~Device() {
		this->disconnect();
	}
	
	/// \brief Search for compatible devices.
	/// \return A string with the result of the search.
	QString Device::search() {
		if(this->error)
			return tr("Can't search for Hantek oscilloscopes: ").arg(Helper::libUsbErrorString(this->error));
		QString message;
		
#if LIBUSB_VERSION == 0
		usb_init();
		usb_find_busses();
		usb_find_devices();
	
		struct usb_device *usbDSO = NULL;
		for (struct usb_bus *usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next)
		{
			for (struct usb_device *dev = usb_bus->devices; dev; dev = dev->next)
			{
				if (dev->descriptor.idVendor == deviceVendor)
				{
					for (int i = 0; deviceModelsList[i] != DSO_LAST; i++)
					{
						if (dev->descriptor.idProduct == deviceModelsList[i])
						{
							usbDSO = dev;
							deviceModel = dev->descriptor.idProduct;
							qDebug("Hantek DSO model %4X found", deviceModel);
							break;
						}
					}
				}
			}
		}
	
		if (usbDSO == NULL)
		{
			dsoIOMutex.unlock();
			qDebug("Hantek DSO not found");
			return -1;
		}
	
		if ((deviceModel == DSO_5200) || (deviceModel == DSO_5200A))
		{
			extraBitsData = true;
			qDebug("Using a 9-bita data model");
		}
	
		usbDSOHandle = ::usb_open(usbDSO);
		if (usbDSOHandle == NULL)
		{
			dsoIOMutex.unlock();
			qDebug("Can't open USB device");
			return -2;
		}
		
		struct usb_config_descriptor *usbConfig = usbDSO->config;
		for (int i = 0; i < usbConfig->bNumInterfaces; i++)
		{
			struct usb_interface *usbInterface = &usbConfig->interface[i];
			if (usbInterface->num_altsetting < 1)
				continue;
	
			struct usb_interface_descriptor *usbInterfaceDescr = &usbInterface->altsetting[0];
			if (usbInterfaceDescr->bInterfaceClass == USB_CLASS_VENDOR_SPEC
				&& usbInterfaceDescr->bInterfaceSubClass == 0
				&& usbInterfaceDescr->bInterfaceProtocol == 0
				&& usbInterfaceDescr->bNumEndpoints == 2)
			{
				if (::usb_claim_interface(usbDSOHandle, usbInterfaceDescr->bInterfaceNumber))
				{
					if (::usb_close(usbDSOHandle))
					{
						qDebug("Can't close USB handle");
					}

					dsoIOMutex.unlock();
					qDebug("Not able to claim USB interface");
					return -3;
				}

				interfaceNumber = usbInterfaceDescr->bInterfaceNumber;
				interfaceIsClaimed = true;

				for (int i = 0; i < usbInterfaceDescr->bNumEndpoints; i++)
				{
					usb_endpoint_descriptor *usbEndpointDescr = &usbInterfaceDescr->endpoint[i];
					switch (usbEndpointDescr->bEndpointAddress)
					{
						case 0x02:  // EP OUT
							epOutMaxPacketLen = usbEndpointDescr->wMaxPacketSize;
							qDebug("EP OUT MaxPacketLen = %i", epOutMaxPacketLen);
							break;
						case 0x86:  // EP IN
							epInMaxPacketLen = usbEndpointDescr->wMaxPacketSize;
							qDebug("EP IN MaxPacketLen = %i", epInMaxPacketLen);
							break;
						default:
							qDebug("Unknown endpoint #%02X", usbEndpointDescr->bEndpointAddress);
					}
				}

				break;
			}
		}
		
		if (!interfaceIsClaimed)
		{
			qDebug("Can't find USB interface (Class:0xFF, SubClass:0, Protocol:0) with two endpoints");
			return -4;
		}
		
		dsoIOMutex.unlock();
		
		return 0;
#else
		libusb_device **deviceList;
		libusb_device *device;
		int errorCode = LIBUSB_SUCCESS;
		QString deviceAddress;
		
		if(this->handle)
			libusb_close(this->handle);
		
		ssize_t deviceCount = libusb_get_device_list(this->context, &deviceList);
		if (deviceCount < 0)
			return tr("Failed to get device list: %3").arg(Helper::libUsbErrorString(errorCode));
		
		// Iterate through all usb devices
		for(ssize_t deviceIterator = 0; deviceIterator < deviceCount; deviceIterator++) {
			device = deviceList[deviceIterator];
			// Get device descriptor
			if(libusb_get_device_descriptor(device, &(this->descriptor)) < 0)
				continue;
	
			// Check VID and PID
			if(this->descriptor.idVendor == HANTEK_VENDOR_ID) {
				this->model = (Model) this->modelIds.indexOf(this->descriptor.idProduct);
				if(this->model >= 0)
					break; // Found a compatible device, ignore others
			}
		}
		
		if(this->model >= 0) {
			// Open device
			deviceAddress = QString("%1:%2").arg(libusb_get_bus_number(device), 3, 10, QLatin1Char('0')).arg(libusb_get_device_address(device), 3, 10, QLatin1Char('0'));
			errorCode = libusb_open(device, &(this->handle));
			if(errorCode == LIBUSB_SUCCESS) {
				libusb_config_descriptor *configDescriptor;
				const libusb_interface *interface;
				const libusb_interface_descriptor *interfaceDescriptor;
				
				// Search for the needed interface
				libusb_get_config_descriptor(device, 0, &configDescriptor);
				for(int interfaceIndex = 0; interfaceIndex < (int) configDescriptor->bNumInterfaces; interfaceIndex++) {
					interface = &configDescriptor->interface[interfaceIndex];
					if(interface->num_altsetting < 1)
						continue;
					
					interfaceDescriptor = &interface->altsetting[0];
					if(interfaceDescriptor->bInterfaceClass == LIBUSB_CLASS_VENDOR_SPEC && interfaceDescriptor->bInterfaceSubClass == 0 && interfaceDescriptor->bInterfaceProtocol == 0 && interfaceDescriptor->bNumEndpoints == 2) {
						// That's the interface we need, claim it
						errorCode = libusb_claim_interface(this->handle, interfaceDescriptor->bInterfaceNumber);
						if(errorCode < 0) {
							libusb_close(this->handle);
							this->handle = 0;
							message = tr("Failed to claim interface %1 of device %2: %3").arg(QString::number(interfaceDescriptor->bInterfaceNumber), deviceAddress, Helper::libUsbErrorString(errorCode));
						}
						else {
							this->interface = interfaceDescriptor->bInterfaceNumber;
							
							// Check the maximum endpoint packet size
							const libusb_endpoint_descriptor *endpointDescriptor;
							this->outPacketLength = 0;
							this->inPacketLength = 0;
							for (int endpoint = 0; endpoint < interfaceDescriptor->bNumEndpoints; endpoint++) {
								endpointDescriptor = &(interfaceDescriptor->endpoint[endpoint]);
								switch(endpointDescriptor->bEndpointAddress) {
									case HANTEK_EP_OUT:
										this->outPacketLength = endpointDescriptor->wMaxPacketSize;
										break;
									case HANTEK_EP_IN:
										this->inPacketLength = endpointDescriptor->wMaxPacketSize;
										break;
								}
							}
							message = tr("Device found: Hantek %1 (%2)").arg(this->modelStrings[this->model], deviceAddress);
							emit connected();
						}
					}
				}
				
				libusb_free_config_descriptor(configDescriptor);
			}
			else {
				this->handle = 0;
				message = tr("Couldn't open device %1: %2").arg(deviceAddress, Helper::libUsbErrorString(errorCode));
			}
		}
		else
			message = tr("No Hantek oscilloscope found");
		
		libusb_free_device_list(deviceList, true);
		
		return message;
#endif
	}
	
	/// \brief Disconnect the device.
	void Device::disconnect() {
		if(!this->handle)
			return;
		
		// Release claimed interface
		libusb_release_interface(this->handle, this->interface);
		this->interface = -1;
		
		// Close device handle
		libusb_close(this->handle);
		this->handle = 0;
		
		emit disconnected();
	}
	
	/// \brief Check if the oscilloscope is connected.
	/// \return true, if a connection is up.
	bool Device::isConnected() {
		return this->handle != 0;
	}
	
#if LIBUSB_VERSION != 0
	/// \brief Bulk transfer to the oscilloscope.
	/// \param endpoint Endpoint number, also sets the direction of the transfer.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::bulkTransfer(unsigned char endpoint, unsigned char *data, unsigned int length, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = LIBUSB_ERROR_TIMEOUT;
		int transferred;
		for(int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; attempt++)
			errorCode = libusb_bulk_transfer(this->handle, endpoint, data, length, &transferred, HANTEK_TIMEOUT);
		
		if(errorCode == LIBUSB_ERROR_NO_DEVICE)
			this->disconnect();
		if(errorCode < 0)
			return errorCode;
		else
			return transferred;
	}
#endif
	
	/// \brief Bulk write to the oscilloscope.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::bulkWrite(unsigned char *data, unsigned int length, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = this->getConnectionSpeed();
		if(errorCode < 0)
			return errorCode;
		
#if LIBUSB_VERSION == 0
		int i, rv = -ETIMEDOUT;
		for(i = 0; (rv == -ETIMEDOUT) && (i < attempts); i++)
		{
			rv = ::usb_bulk_write(usbDSOHandle, EP_BULK_OUT | USB_ENDPOINT_OUT, (char*)data, length, timeout);
		}
		
		if (rv < 0)
		{
			qDebug("Usb write bulk returns error %i", rv);
			qDebug("Error: %s", ::usb_strerror());
			return rv;
		}
		
		return 0;
#else
		return this->bulkTransfer(HANTEK_EP_OUT, data, length, attempts);
#endif
	}
	
	/// \brief Bulk read from the oscilloscope.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::bulkRead(unsigned char *data, unsigned int length, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = this->getConnectionSpeed();
		if(errorCode < 0)
			return errorCode;
		
#if LIBUSB_VERSION == 0
		int i, rv = -ETIMEDOUT;
		for(i = 0; (rv == -ETIMEDOUT) && (i < attempts); i++)
		{
			rv = ::usb_bulk_read(usbDSOHandle, EP_BULK_IN | USB_ENDPOINT_IN, (char*)data, length, timeout);
		}
		
		if (rv < 0)
		{
			qDebug("Usb read bulk returns error %i", rv);
			qDebug("Error: %s", ::usb_strerror());
			return rv;
		}
		
		return 0;
#else
		return this->bulkTransfer(HANTEK_EP_IN, data, length, attempts);
#endif
	}
	
	/// \brief Send a bulk command to the oscilloscope.
	/// \param command The command, that should be sent.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::bulkCommand(Helper::DataArray<unsigned char> *command, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		// Send BeginCommand control command
		int errorCode = this->controlWrite(CONTROL_BEGINCOMMAND, this->beginCommandControl->data(), this->beginCommandControl->getSize());
		if(errorCode < 0)
			return errorCode;
		
		// Send bulk command
		return this->bulkWrite(command->data(), command->getSize(), attempts);
	}
	
	/// \brief Multi packet bulk read from the oscilloscope.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length of data contained in the packets.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::bulkReadMulti(unsigned char *data, unsigned int length, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = 0;
		
		errorCode = this->getConnectionSpeed();
		if(errorCode < 0)
			return errorCode;
		
		int packetCount = length / this->inPacketLength;
		
		errorCode = this->inPacketLength;
		int packet;
		for(packet = 0; packet < packetCount && errorCode == this->inPacketLength; packet++)
			errorCode = this->bulkTransfer(HANTEK_EP_IN, data + packet * this->inPacketLength, this->inPacketLength, attempts);
		
		if(errorCode < 0)
			return errorCode;
		else
			return (packet - 1) * this->inPacketLength + errorCode;
	}
	
#if LIBUSB_VERSION != 0
	/// \brief Control transfer to the oscilloscope.
	/// \param type The request type, also sets the direction of the transfer.
	/// \param request The request field of the packet.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length field of the packet.
	/// \param value The value field of the packet.
	/// \param index The index field of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = LIBUSB_ERROR_TIMEOUT;
		for(int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; attempt++)
			errorCode = libusb_control_transfer(this->handle, type, request, value, index, data, length, HANTEK_TIMEOUT);
		
		if(errorCode == LIBUSB_ERROR_NO_DEVICE)
			this->disconnect();
		return errorCode;
	}
#endif
	
	/// \brief Control write to the oscilloscope.
	/// \param request The request field of the packet.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length field of the packet.
	/// \param value The value field of the packet.
	/// \param index The index field of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::controlWrite(unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
#if LIBUSB_VERSION == 0
		int i, rv = -ETIMEDOUT;
		for(i = 0; (rv == -ETIMEDOUT) && (i < attempts); i++)
		{
			rv = ::usb_control_msg(usbDSOHandle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR,
									request, value, index, (char*)data, length, timeout);
		}
		
		if (rv < 0)
		{
			qDebug("Usb write control message %02X returns error %i", request, rv);
			qDebug("Error: %s", ::usb_strerror());
			return rv;
		}
		
		return 0;
#else
		return this->controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT, request,  data, length, value, index,attempts);
#endif
	}
	
	/// \brief Control read to the oscilloscope.
	/// \param request The request field of the packet.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length field of the packet.
	/// \param value The value field of the packet.
	/// \param index The index field of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return 0 on success, libusb error code on error.
	int Device::controlRead(unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
#if LIBUSB_VERSION == 0
		int i, rv = -ETIMEDOUT;
		for(i = 0; (rv == -ETIMEDOUT) && (i < attempts); i++)
		{
			rv = ::usb_control_msg(usbDSOHandle, USB_ENDPOINT_IN | USB_TYPE_VENDOR,
									request, value, index, (char*)data, length, timeout);
		}
		
		if (rv < 0)
		{
			qDebug("Usb read control message %02X returns error %i", request, rv);
			qDebug("Error: %s", ::usb_strerror());
			return rv;
		}
		
		return 0;
#else
		return this->controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN, request, data, length, value, index, attempts);
#endif
	}
	
	/// \brief Gets the speed of the connection.
	/// \return The #ConnectionSpeed of the USB connection.
	int Device::getConnectionSpeed() {
		int errorCode;
		ControlGetSpeed response;
		
		errorCode = this->controlRead(CONTROL_GETSPEED, response.data(), response.getSize());
		if(errorCode < 0)
			return errorCode;
		
		return response.getSpeed();
	}
}
