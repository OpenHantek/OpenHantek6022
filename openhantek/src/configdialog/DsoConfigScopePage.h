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
/// \class DsoConfigScopePage                                      configpages.h
/// \brief Config page for the scope screen.
class DsoConfigScopePage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigScopePage(DsoSettings *settings, QWidget *parent = nullptr);

  public slots:
    void saveSettings();

  private:
    DsoSettings *settings;

    QVBoxLayout *mainLayout;

    QGroupBox *graphGroup;
    QGridLayout *graphLayout;
    QLabel *digitalPhosphorDepthLabel;
    QSpinBox *digitalPhosphorDepthSpinBox;
    QLabel *interpolationLabel;
    QComboBox *interpolationComboBox;

    QGroupBox *cursorsGroup;
    QGridLayout *cursorsLayout;
    QLabel *cursorsLabel;
    QComboBox *cursorsComboBox;
};
