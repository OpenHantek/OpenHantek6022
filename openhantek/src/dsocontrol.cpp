////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dsocontrol.cpp
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


#include "dsocontrol.h"


////////////////////////////////////////////////////////////////////////////////
// class DsoControl
/// \brief Initialize variables.
DsoControl::DsoControl(QObject *parent) : QThread(parent) {
	this->sampling = false;
}

/// \brief Start sampling process.
void DsoControl::startSampling() {
	this->sampling = true;
	emit samplingStarted();
}

/// \brief Stop sampling process.
void DsoControl::stopSampling() {
	this->sampling = false;
	emit samplingStopped();
}

/// \brief Get a list of the names of the special trigger sources.
const QStringList *DsoControl::getSpecialTriggerSources() {
	return &(this->specialTriggerSources);
}

/// \brief Try to connect to the oscilloscope.
void DsoControl::connectDevice() {
	this->sampling = false;
	this->start();
}

/// \brief Disconnect the oscilloscope.
void DsoControl::disconnectDevice() {
	this->quit();
}
