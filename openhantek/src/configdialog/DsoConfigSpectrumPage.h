// SPDX-License-Identifier: GPL-2.0+

#include <QWidget>

#include "dsosettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigSpectrumPage                                   configpages.h
/// \brief Config page for the data spectral analysis.
class DsoConfigSpectrumPage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigSpectrumPage( DsoSettings *settings, QWidget *parent = nullptr );

  public slots:
    void saveSettings();

  private:
    DsoSettings *settings;

    QVBoxLayout *mainLayout;

    QGroupBox *spectrumGroup;
    QGridLayout *spectrumLayout;
    QLabel *windowFunctionLabel;
    QComboBox *windowFunctionComboBox;

    QLabel *referenceLevelLabel;
    QDoubleSpinBox *referenceLevelSpinBox;
    QLabel *referenceLevelUnitLabel;
    QHBoxLayout *referenceLevelLayout;

    QLabel *minimumMagnitudeLabel;
    QDoubleSpinBox *minimumMagnitudeSpinBox;
    QLabel *minimumMagnitudeUnitLabel;
    QHBoxLayout *minimumMagnitudeLayout;
};
