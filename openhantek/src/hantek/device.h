////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file hantek/device.h
/// \brief Declares the Hantek::Device class.
//
/// \copyright (c) 2008, 2009 Oleg Khudyakov <prcoder@potrebitel.ru>
/// \copyright (c) 2010 - 2012 Oliver Haag <oliver.haag@gmail.com>
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


#ifndef HANTEK_DEVICE_H
#define HANTEK_DEVICE_H


#include <QObject>
#include <QStringList>

#if LIBUSB_VERSION == 0
#include <usb.h>
#define libusb_device_handle usb_device_handle
#define libusb_device_descriptor usb_device_descriptor
#else
#include <libusb.h>
#endif


#include "helper.h"
#include "hantek/types.h"


namespace Hantek {
	//////////////////////////////////////////////////////////////////////////////
	/// \class Device                                              hantek/device.h
	/// \brief This class handles the USB communication with the oscilloscope.
	class Device : public QObject {
		Q_OBJECT
		
		public:
			Device(QObject *parent = 0);
			~Device();
			
			QString search();
			void disconnect();
			bool isConnected();
			
			// Various methods to handle USB transfers
#if LIBUSB_VERSION != 0
			int bulkTransfer(unsigned char endpoint, unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS, unsigned int timeout = HANTEK_TIMEOUT);
#endif
			int bulkWrite(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS);
			int bulkRead(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS);
			
			int bulkCommand(Helper::DataArray<unsigned char> *command, int attempts = HANTEK_ATTEMPTS);
			int bulkReadMulti(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS_MULTI);
			
			int controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts = HANTEK_ATTEMPTS);
			int controlWrite(unsigned char request, unsigned char *data, unsigned int length, int value = 0, int index = 0, int attempts = HANTEK_ATTEMPTS);
			int controlRead(unsigned char request, unsigned char *data, unsigned int length, int value = 0, int index = 0, int attempts = HANTEK_ATTEMPTS);
			
			int getConnectionSpeed();
			int getPacketSize();
			Model getModel();
		
		protected:
			// Lists for enums
			QList<unsigned short int> modelIds; ///< Product ID for each ::Model
			QStringList modelStrings; ///< The name as QString for each ::Model
			
			// Command buffers
			ControlBeginCommand *beginCommandControl; ///< Buffer for the CONTROL_BEGINCOMMAND control command
			
			// Libusb specific variables
#if LIBUSB_VERSION != 0
			libusb_context *context; ///< The usb context used for this device
#endif
			Model model; ///< The model of the connected oscilloscope
#if LIBUSB_VERSION == 0
			usb_dev_handle *handle; ///< The USB handle for the oscilloscope
			usb_device_descriptor descriptor; ///< The device descriptor of the oscilloscope
#else
			libusb_device_handle *handle; ///< The USB handle for the oscilloscope
			libusb_device_descriptor descriptor; ///< The device descriptor of the oscilloscope
#endif
			int interface; ///< The number of the claimed interface
			int error; ///< The libusb error, that happened on initialization
			int outPacketLength; ///< Packet length for the OUT endpoint
			int inPacketLength; ///< Packet length for the IN endpoint
		
		signals:
			void connected(); ///< The device has been connected and initialized
			void disconnected(); ///< The device has been disconnected
			
		public slots:
			
	};
}


#endif
