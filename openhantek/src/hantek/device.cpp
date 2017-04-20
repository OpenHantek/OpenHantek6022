////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  hantek/device.cpp
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010 - 2012  Oliver Haag
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
		this->modelIds << 0x2090 << 0x2150 << 0x2250 << 0x5200 << 0x520A << 0x6022;
		this->modelStrings << "DSO-2090" << "DSO-2150" << "DSO-2250" << "DSO-5200" << "DSO-5200A" << "DSO-6022BE";
		this->model = MODEL_UNKNOWN;
		
		this->beginCommandControl = new ControlBeginCommand();
		
		this->handle = 0;
		this->interface = -1;
		
		this->outPacketLength = 0;
		this->inPacketLength = 0;

		this->error = LIBUSB_SUCCESS;
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
			return tr("Can't search for Hantek oscilloscopes: %1").arg(Helper::libUsbErrorString(this->error));
		
		QString message;
		QString deviceAddress;
		int errorCode = LIBUSB_SUCCESS;

		libusb_device **deviceList;
		libusb_device *device;
		
		if(this->handle)
			libusb_close(this->handle);
		
		ssize_t deviceCount = libusb_get_device_list(this->context, &deviceList);
		if(deviceCount < 0)
			return tr("Failed to get device list: %1").arg(Helper::libUsbErrorString(errorCode));
		
		// Iterate through all usb devices
		this->model = MODEL_UNKNOWN;
		for(ssize_t deviceIterator = 0; deviceIterator < deviceCount; ++deviceIterator) {
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
				for(int interfaceIndex = 0; interfaceIndex < (int) configDescriptor->bNumInterfaces; ++interfaceIndex) {
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
							for (int endpoint = 0; endpoint < interfaceDescriptor->bNumEndpoints; ++endpoint) {
								endpointDescriptor = &(interfaceDescriptor->endpoint[endpoint]);
								switch(endpointDescriptor->bEndpointAddress) {
									case HANTEK_EP_OUT:
										this->outPacketLength = endpointDescriptor->wMaxPacketSize;
										break;
									case HANTEK_EP_IN:
										if (this->getModel() == MODEL_DSO6022BE)
											this->inPacketLength = 16384;
										else
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

	/// \brief Bulk transfer to/from the oscilloscope.
	/// \param endpoint Endpoint number, also sets the direction of the transfer.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \param timeout The timeout in ms.
	/// \return Number of transferred bytes on success, libusb error code on error.
	int Device::bulkTransfer(unsigned char endpoint, unsigned char *data, unsigned int length, int attempts, unsigned int timeout) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = LIBUSB_ERROR_TIMEOUT;
		int transferred;
		for(int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
			errorCode = libusb_bulk_transfer(this->handle, endpoint, data, length, &transferred, timeout);
		
		if(errorCode == LIBUSB_ERROR_NO_DEVICE)
			this->disconnect();
		if(errorCode < 0)
			return errorCode;
		else
			return transferred;
	}

	/// \brief Bulk write to the oscilloscope.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return Number of sent bytes on success, libusb error code on error.
	int Device::bulkWrite(unsigned char *data, unsigned int length, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = this->getConnectionSpeed();
		if(errorCode < 0)
			return errorCode;

		return this->bulkTransfer(HANTEK_EP_OUT, data, length, attempts);
	}
	
	/// \brief Bulk read from the oscilloscope.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return Number of received bytes on success, libusb error code on error.
	int Device::bulkRead(unsigned char *data, unsigned int length, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = this->getConnectionSpeed();
		if(errorCode < 0)
			return errorCode;

		return this->bulkTransfer(HANTEK_EP_IN, data, length, attempts);
	}
	
	/// \brief Send a bulk command to the oscilloscope.
	/// \param command The command, that should be sent.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return Number of sent bytes on success, libusb error code on error.
	int Device::bulkCommand(Helper::DataArray<unsigned char> *command, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;

		// don't send bulk command if dso6022be
		if (this->getModel() == MODEL_DSO6022BE)
			return 0;

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
	/// \return Number of received bytes on success, libusb error code on error.
	int Device::bulkReadMulti(unsigned char *data, unsigned int length, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = 0;
		
		errorCode = this->getConnectionSpeed();
		if(errorCode < 0)
			return errorCode;
		
		errorCode = this->inPacketLength;
		unsigned int packet, received = 0;
		for(packet = 0; received < length && errorCode == this->inPacketLength; ++packet) {
			errorCode = this->bulkTransfer(HANTEK_EP_IN, data + packet * this->inPacketLength, qMin(length - received, (unsigned int) this->inPacketLength), attempts, HANTEK_TIMEOUT_MULTI);
			if(errorCode > 0)
				received += errorCode;
		}
		
		if(received > 0)
			return received;
		else
			return errorCode;
	}
	
	/// \brief Control transfer to the oscilloscope.
	/// \param type The request type, also sets the direction of the transfer.
	/// \param request The request field of the packet.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length field of the packet.
	/// \param value The value field of the packet.
	/// \param index The index field of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return Number of transferred bytes on success, libusb error code on error.
	int Device::controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		int errorCode = LIBUSB_ERROR_TIMEOUT;
		for(int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
			errorCode = libusb_control_transfer(this->handle, type, request, value, index, data, length, HANTEK_TIMEOUT);
		
		if(errorCode == LIBUSB_ERROR_NO_DEVICE)
			this->disconnect();
		return errorCode;
	}
	
	/// \brief Control write to the oscilloscope.
	/// \param request The request field of the packet.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length field of the packet.
	/// \param value The value field of the packet.
	/// \param index The index field of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return Number of sent bytes on success, libusb error code on error.
	int Device::controlWrite(unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		return this->controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT, request,  data, length, value, index, attempts);
	}
	
	/// \brief Control read to the oscilloscope.
	/// \param request The request field of the packet.
	/// \param data Buffer for the sent/recieved data.
	/// \param length The length field of the packet.
	/// \param value The value field of the packet.
	/// \param index The index field of the packet.
	/// \param attempts The number of attempts, that are done on timeouts.
	/// \return Number of received bytes on success, libusb error code on error.
	int Device::controlRead(unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts) {
		if(!this->handle)
			return LIBUSB_ERROR_NO_DEVICE;
		
		return this->controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN, request, data, length, value, index, attempts);
	}
	
	/// \brief Gets the speed of the connection.
	/// \return The ::ConnectionSpeed of the USB connection.
	int Device::getConnectionSpeed() {
		int errorCode;
		ControlGetSpeed response;
		
		errorCode = this->controlRead(CONTROL_GETSPEED, response.data(), response.getSize());
		if(errorCode < 0)
			return errorCode;
		
		return response.getSpeed();
	}
	
	/// \brief Gets the maximum size of one packet transmitted via bulk transfer.
	/// \return The maximum packet size in bytes, -1 on error.
	int Device::getPacketSize() {
		switch(this->getConnectionSpeed()) {
			case CONNECTION_FULLSPEED:
				return 64;
				break;
			case CONNECTION_HIGHSPEED:
				return 512;
				break;
			default:
				return -1;
				break;
		}
	}
	
	/// \brief Get the oscilloscope model.
	/// \return The ::Model of the connected Hantek DSO.
	Model Device::getModel() {
		return this->model;
	}
}
