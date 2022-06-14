// SPDX-License-Identifier: GPL-2.0-or-later

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
/// \class DsoConfigAnalysisPage                                   configpages.h
/// \brief Config page for the data spectral analysis.
class DsoConfigAnalysisPage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigAnalysisPage( DsoSettings *settings, QWidget *parent = nullptr );

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

    QCheckBox *reuseFftPlanCheckBox;
    QCheckBox *showNoteCheckBox;

    QGroupBox *analysisGroup;
    QGridLayout *analysisLayout;

    QCheckBox *dummyLoadCheckbox;
    QSpinBox *dummyLoadSpinBox;
    QLabel *dummyLoadUnitLabel;
    QHBoxLayout *dummyLoadLayout;

    QCheckBox *thdCheckBox;
};
