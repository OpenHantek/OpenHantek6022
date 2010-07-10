////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file hantek/device.h
/// \brief Declares the Hantek::Device class.
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


#ifndef HANTEK_DEVICE_H
#define HANTEK_DEVICE_H


#include <QObject>
#include <QStringList>

#if LIBUSB_VERSION == 0
#include <usb.h>
#define libusb_device_handle usb_device_handle
#define libusb_device_descriptor usb_device_descriptor
#else
#include <libusb-1.0/libusb.h>
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
			int bulkTransfer(unsigned char endpoint, unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS_DEFAULT);
#endif
			int bulkWrite(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS_DEFAULT);
			int bulkRead(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS_DEFAULT);
			
			int bulkCommand(Helper::DataArray<unsigned char> *command, int attempts = HANTEK_ATTEMPTS_DEFAULT);
			int bulkReadMulti(unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS_DEFAULT);
			
#if LIBUSB_VERSION != 0
			int controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value, int index, int attempts = HANTEK_ATTEMPTS_DEFAULT);
#endif
			int controlWrite(unsigned char request, unsigned char *data, unsigned int length, int value = 0, int index = 0, int attempts = HANTEK_ATTEMPTS_DEFAULT);
			int controlRead(unsigned char request, unsigned char *data, unsigned int length, int value = 0, int index = 0, int attempts = HANTEK_ATTEMPTS_DEFAULT);
			
			int getConnectionSpeed();
		
		protected:
			// Lists for enums
			QList<unsigned short int> modelIds; ///< Product ID for each #Model
			QStringList modelStrings; ///< The name as QString for each #Model
			
			// Command buffers
			ControlBeginCommand *beginCommandControl;
			
			// Libusb specific variables
#if LIBUSB_VERSION != 0
			libusb_context *context; ///< The usb context used for this device
#endif
			Model model; ///< The model of the connected oscilloscope
			libusb_device_handle *handle; ///< The USB handle for the oscilloscope
			libusb_device_descriptor descriptor; ///< The device descriptor of the oscilloscope
			int interface; ///< The number of the claimed interface
			int error; ///< The libusb error, that happened on initialization
			int outPacketLength; ///< Packet length for the OUT endpoint
			int inPacketLength; ///< Packet length for the IN endpoint
		
		signals:
			void connected();
			void disconnected();
			
		public slots:
			
	};
}


#endif
