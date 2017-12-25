// SPDX-License-Identifier: GPL-2.0+

#include <QWidget>

#include "settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigFilesPage                                      configpages.h
/// \brief Config page for file loading/saving.
class DsoConfigFilesPage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigFilesPage(DsoSettings *settings, QWidget *parent = 0);

  public slots:
    void saveSettings();

  private:
    DsoSettings *settings;

    QVBoxLayout *mainLayout;

    QGroupBox *configurationGroup;
    QVBoxLayout *configurationLayout;
    QCheckBox *saveOnExitCheckBox;
    QPushButton *saveNowButton;

    QGroupBox *exportGroup;
    QGridLayout *exportLayout;
    QCheckBox *screenColorCheckBox;
    QLabel *imageWidthLabel;
    QSpinBox *imageWidthSpinBox;
    QLabel *imageHeightLabel;
    QSpinBox *imageHeightSpinBox;
};
