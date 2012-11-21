////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  sispinbox.cpp
//
//  Copyright (C) 2012  Oliver Haag
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


#include <cfloat>
#include <cmath>


#include "sispinbox.h"

#include "helper.h"


////////////////////////////////////////////////////////////////////////////////
// class OpenHantekMainWindow
/// \brief Initializes the SiSpinBox internals.
/// \param parent The parent widget.
SiSpinBox::SiSpinBox(QWidget *parent) : QDoubleSpinBox(parent) {
	this->init();
}

/// \brief Initializes the SiSpinBox, allowing the user to choose the unit.
/// \param unit The unit shown for the value in the spin box.
/// \param parent The parent widget.
SiSpinBox::SiSpinBox(Helper::Unit unit, QWidget *parent) : QDoubleSpinBox(parent) {
	this->init();
	
	this->setUnit(unit);
}

/// \brief Cleans up the main window.
SiSpinBox::~SiSpinBox() {
}

/// \brief Validates the text after user input.
/// \param input The content of the text box.
/// \param pos The position of the cursor in the text box.
/// \return Validity of the current text.
QValidator::State SiSpinBox::validate(QString &input, int &pos) const {
	Q_UNUSED(pos);
	
	bool ok;
	double value = Helper::stringToValue(input, this->unit, &ok);
	
	if(!ok)
		return QValidator::Invalid;
	
	if(input == this->textFromValue(value))
		return QValidator::Acceptable;
	return QValidator::Intermediate;
}

/// \brief Parse value from input text.
/// \param text The content of the text box.
/// \return Value in base unit.
double SiSpinBox::valueFromText(const QString &text) const {
	return Helper::stringToValue(text, this->unit);
}

/// \brief Get string representation of value.
/// \param val Value in base unit.
/// \return String representation containing value and (prefix+)unit.
QString SiSpinBox::textFromValue(double val) const {
	return Helper::valueToString(val, this->unit, -1) + this->unitPostfix;
}

/// \brief Fixes the text after the user finished changing it.
/// \param input The content of the text box.
void SiSpinBox::fixup(QString &input) const {
	bool ok;
	double value = Helper::stringToValue(input, this->unit, &ok);
	
	if(!ok)
		value = this->value();
	
	input = this->textFromValue(value);
}

/// \brief Increase/decrease the values in fixed steps.
/// \param steps The number of steps, positive means increase.
void SiSpinBox::stepBy(int steps) {
	double stepsSpan = this->steps.last() / this->steps.first();
	int stepsCount = this->steps.size() - 1;
	
	if(!this->steppedTo) { // No step done directly before this one, so we need to check where we are
		// Get how often the steps have to be fully ran through
		int stepsFully = (int) floor(log(this->value() / this->steps.first()) / log(stepsSpan));
		// And now the remaining multiple
		double stepMultiple = this->value() / pow(stepsSpan, stepsFully);
		// Now get the neighbours of the current value from our steps list
		int remainingSteps = 0;
		for(; remainingSteps <= stepsCount; ++remainingSteps) {
			if(this->steps[remainingSteps] > stepMultiple)
				break;
		}
		if(remainingSteps > 0) // Shouldn't happen, but double may have rounding errors
			--remainingSteps;
		this->stepId = stepsFully * stepsCount + remainingSteps;
		// We need to do one step less down if we are inbetween two of them since our step is lower than the value
		if(steps < 0 && this->steps[remainingSteps] < stepMultiple)
			++this->stepId;
	}
	
	this->stepId += steps;
	int stepsId = this->stepId % stepsCount;
	if(stepsId < 0)
		stepsId += stepsCount;
	double value = pow(stepsSpan, floor((double) this->stepId / stepsCount)) * this->steps[stepsId];
	this->setValue(value);
	value = this->value();
	this->steppedTo = true;
}

/// \brief Set the unit for this spin box.
/// \param unit The unit shown for the value in the spin box.
/// \return true on success, false on invalid unit.
bool SiSpinBox::setUnit(Helper::Unit unit) {
	if((unsigned int) unit >= Helper::UNIT_COUNT)
		return false;
	
	this->unit = unit;
	return true;
}

/// \brief Set the unit postfix for this spin box.
/// \param postfix the string shown after the unit in the spin box.
void SiSpinBox::setUnitPostfix(const QString &postfix) {
	this->unitPostfix = postfix;
}

/// \brief Set the steps the spin box will take.
/// \param steps The steps, will be extended with the ratio from the start after the last element.
void SiSpinBox::setSteps(const QList<double> &steps) {
	this->steps = steps;
}

/// \brief Generic initializations.
void SiSpinBox::init() {
	this->setMinimum(1e-12);
	this->setMaximum(1e12);
	this->setValue(1.0);
	this->setDecimals(DBL_MAX_10_EXP + DBL_DIG); // Disable automatic rounding
	this->unit = unit;
	this->steps << 1.0 << 2.0 << 5.0 << 10.0;
	
	this->steppedTo = false;
	this->stepId = 0;
	
	connect(this, SIGNAL(valueChanged(double)), this, SLOT(resetSteppedTo()));
}

/// \brief Resets the ::steppedTo flag after the value has been changed.
void SiSpinBox::resetSteppedTo() {
	this->steppedTo = false;
}
