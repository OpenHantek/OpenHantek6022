////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  helper.cpp
//
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


#include <cmath>

#include <QApplication>
#include <QLocale>
#include <QStringList>

#if LIBUSB_VERSION == 0
#include <usb.h>
#define libusb_device usb_device
#else
#include <libusb-1.0/libusb.h>
#endif


#include "helper.h"


namespace Helper {
	/// \brief Returns string representation for libusb errors.
	/// \param error The error code.
	/// \return String explaining the error.
	QString libUsbErrorString(int error) {
		switch(error) {
			case LIBUSB_SUCCESS:
				return QApplication::tr("Success (no error)");
			case LIBUSB_ERROR_IO:
				return QApplication::tr("Input/output error");
			case LIBUSB_ERROR_INVALID_PARAM:
				return QApplication::tr("Invalid parameter");
			case LIBUSB_ERROR_ACCESS:
				return QApplication::tr("Access denied (insufficient permissions)");
			case LIBUSB_ERROR_NO_DEVICE:
				return QApplication::tr("No such device (it may have been disconnected)");
			case LIBUSB_ERROR_NOT_FOUND:
				return QApplication::tr("Entity not found");
			case LIBUSB_ERROR_BUSY:
				return QApplication::tr("Resource busy");
			case LIBUSB_ERROR_TIMEOUT:
				return QApplication::tr("Operation timed out");
			case LIBUSB_ERROR_OVERFLOW:
				return QApplication::tr("Overflow");
			case LIBUSB_ERROR_PIPE:
				return QApplication::tr("Pipe error");
			case LIBUSB_ERROR_INTERRUPTED:
				return QApplication::tr("System call interrupted (perhaps due to signal)");
			case LIBUSB_ERROR_NO_MEM:
				return QApplication::tr("Insufficient memory");
			case LIBUSB_ERROR_NOT_SUPPORTED:
				return QApplication::tr("Operation not supported or unimplemented on this platform");
			default:
				return QApplication::tr("Other error");
		}
	}
	
	/// \brief Converts double to string containing value and (prefix+)unit (Counterpart to Helper::stringToValue).
	/// \param value The value in prefixless units.
	/// \param unit The unit for the value.
	/// \param precision Significant digits, 0 for integer, -1 for auto.
	/// \return String with the value and unit.
	QString valueToString(double value, Unit unit, int precision) {
		char format = (precision < 0) ? 'g' : 'f';
		
		switch(unit) {
			case UNIT_VOLTS: {
				// Voltage string representation
				int logarithm = floor(log10(fabs(value)));
				if(value < 1e-3)
					return QApplication::tr("%L1 \265V").arg(value / 1e-6, 0, format, (precision <= 0) ? precision : qBound(0, precision - 7 - logarithm, precision));
				else if(value < 1.0)
					return QApplication::tr("%L1 mV").arg(value / 1e-3, 0, format, (precision <= 0) ? precision : (precision - 4 - logarithm));
				else
					return QApplication::tr("%L1 V").arg(value, 0, format, (precision <= 0) ? precision : qMax(0, precision - 1 - logarithm));
			}
			case UNIT_DECIBEL:
				// Power level string representation
				return QApplication::tr("%L1 dB").arg(value, 0, format, (precision <= 0) ? precision : qBound(0, precision - 1 - (int) floor(log10(fabs(value))), precision));
			
			case UNIT_SECONDS:
				// Time string representation
				if(value < 1e-9)
					return QApplication::tr("%L1 ps").arg(value / 1e-12, 0, format, (precision <= 0) ? precision : qBound(0, precision - 13 - (int) floor(log10(fabs(value))), precision));
				else if(value < 1e-6)
					return QApplication::tr("%L1 ns").arg(value / 1e-9, 0, format, (precision <= 0) ? precision : (precision - 10 - (int) floor(log10(fabs(value)))));
				else if(value < 1e-3)
					return QApplication::tr("%L1 \265s").arg(value / 1e-6, 0, format, (precision <= 0) ? precision : (precision - 7 - (int) floor(log10(fabs(value)))));
				else if(value < 1.0)
					return QApplication::tr("%L1 ms").arg(value / 1e-3, 0, format, (precision <= 0) ? precision : (precision - 4 - (int) floor(log10(fabs(value)))));
				else if(value < 60)
					return QApplication::tr("%L1 s").arg(value, 0, format, (precision <= 0) ? precision : (precision - 1 - (int) floor(log10(fabs(value)))));
				else if(value < 3600)
					return QApplication::tr("%L1 min").arg(value / 60, 0, format, (precision <= 0) ? precision : (precision - 1 - (int) floor(log10(value / 60))));
				else
					return QApplication::tr("%L1 h").arg(value / 3600, 0, format, (precision <= 0) ? precision : qMax(0, precision - 1 - (int) floor(log10(value / 3600))));
			
			case UNIT_HERTZ: {
				// Frequency string representation
				int logarithm = floor(log10(fabs(value)));
				if(value < 1e3)
					return QApplication::tr("%L1 Hz").arg(value, 0, format, (precision <= 0) ? precision : qBound(0, precision - 1 - logarithm, precision));
				else if(value < 1e6)
					return QApplication::tr("%L1 kHz").arg(value / 1e3, 0, format, (precision <= 0) ? precision : precision + 2 - logarithm);
				else if(value < 1e9)
					return QApplication::tr("%L1 MHz").arg(value / 1e6, 0, format, (precision <= 0) ? precision : precision + 5 - logarithm);
				else
					return QApplication::tr("%L1 GHz").arg(value / 1e9, 0, format, (precision <= 0) ? precision : qMax(0, precision + 8 - logarithm));
			}
			case UNIT_SAMPLES: {
				// Sample count string representation
				int logarithm = floor(log10(fabs(value)));
				if(value < 1e3)
					return QApplication::tr("%L1 S").arg(value, 0, format, (precision <= 0) ? precision : qBound(0, precision - 1 - logarithm, precision));
				else if(value < 1e6)
					return QApplication::tr("%L1 kS").arg(value / 1e3, 0, format, (precision <= 0) ? precision : precision + 2 - logarithm);
				else if(value < 1e9)
					return QApplication::tr("%L1 MS").arg(value / 1e6, 0, format, (precision <= 0) ? precision : precision + 5 - logarithm);
				else
					return QApplication::tr("%L1 GS").arg(value / 1e9, 0, format, (precision <= 0) ? precision : qMax(0, precision + 8 - logarithm));
			}
			default:
				return QString();
		}
	}
	
	/// \brief Converts string containing value and (prefix+)unit to double (Counterpart to Helper::valueToString).
	/// \param text The text containing the value and its unit.
	/// \param unit The base unit of the value.
	/// \param ok Pointer to a success-flag, true on success, false on error.
	/// \return Decoded value.
	double stringToValue(const QString &text, Unit unit, bool *ok) {
		// Check if the text is empty
		int totalSize = text.size();
		if(!totalSize){
			if(ok)
				*ok = false;
			return 0.0;
		}
		
		// Split value and unit apart
		int valueSize = 0;
		QLocale locale;
		bool decimalFound = false;
		bool exponentFound = false;
		if(text[valueSize] == locale.negativeSign())
			++valueSize;
		for(; valueSize < text.size(); ++valueSize) {
			QChar character = text[valueSize];
			
			if(character.isDigit()) {
			}
			else if(character == locale.decimalPoint() && decimalFound == false && exponentFound == false) {
				decimalFound = true;
			}
			else if(character == locale.exponential() && exponentFound == false) {
				exponentFound = true;
				if(text[valueSize + 1] == locale.negativeSign())
					++valueSize;
			}
			else {
				break;
			}
		}
		QString valueString = text.left(valueSize);
		bool valueOk = false;
		double value = valueString.toDouble(&valueOk);
		if(!valueOk) {
			if(ok)
				*ok = false;
			return value;
		}
		QString unitString = text.right(text.size() - valueSize).trimmed();
		
		if(ok)
			*ok = true;
		switch(unit) {
			case UNIT_VOLTS: {
				// Voltage string decoding
				if(unitString.startsWith('\265'))
					return value * 1e-6;
				else if(unitString.startsWith('m'))
					return value * 1e-3;
				else
					return value;
			}
			case UNIT_DECIBEL:
				// Power level string decoding
				return value;
			
			case UNIT_SECONDS:
				// Time string decoding
				if(unitString.startsWith('p'))
					return value * 1e-12;
				else if(unitString.startsWith('n'))
					return value * 1e-9;
				else if(unitString.startsWith('\265'))
					return value * 1e-6;
				else if(unitString.startsWith("min"))
					return value * 60;
				else if(unitString.startsWith('m'))
					return value * 1e-3;
				else if(unitString.startsWith('h'))
					return value * 3600;
				else
					return value;
			
			case UNIT_HERTZ:
				// Frequency string decoding
				if(unitString.startsWith('k'))
					return value * 1e3;
				else if(unitString.startsWith('M'))
					return value * 1e6;
				else if(unitString.startsWith('G'))
					return value * 1e9;
				else
					return value;
			
			case UNIT_SAMPLES:
				// Sample count string decoding
				if(unitString.startsWith('k'))
					return value * 1e3;
				else if(unitString.startsWith('M'))
					return value * 1e6;
				else if(unitString.startsWith('G'))
					return value * 1e9;
				else
					return value;
			
			default:
				if(ok)
					*ok = false;
				return value;
		}
	}
	
#ifdef DEBUG
	/// \brief Returns the hex dump for the given data.
	/// \param data Pointer to the data bytes that should be dumped.
	/// \param length The length of the data array in bytes.
	/// \return String with the hex dump of the data.
	QString hexDump(unsigned char *data, unsigned int length) {
		QString dumpString, byteString;
		
		for(unsigned int index = 0; index < length; ++index)
			dumpString.append(byteString.sprintf(" %02x", data[index]));
		
		return dumpString;
	}
	
	/// \brief Returns the hex dump for the given data.
	/// \param dump The string with the hex dump of the data.
	/// \param data Pointer to the address where the data bytes should be saved.
	/// \param length The maximum length of the data array in bytes.
	/// \return The length of the saved data.
	unsigned int hexParse(const QString dump, unsigned char *data, unsigned int length) {
		QString dumpString = dump;
		dumpString.remove(' ');
		QString byteString;
		unsigned int index;
		
		for(index = 0; index < length; ++index) {
			byteString = dumpString.mid(index * 2, 2);
			
			// Check if we reached the end of the string
			if(byteString.isNull())
				break;
			
			// Check for parsing errors
			bool ok;
			unsigned char byte = (unsigned char) byteString.toUShort(&ok, 16);
			if(!ok)
				break;
			
			data[index] = byte;
		}
		
		return index;
	}
#endif
}
