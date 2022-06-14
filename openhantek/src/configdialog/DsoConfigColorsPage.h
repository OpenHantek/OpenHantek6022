// SPDX-License-Identifier: GPL-2.0-or-later

#include <QWidget>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#include <QDebug>

#include "colorbox.h"
#include "dsosettings.h"


////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigColorsPage                                     configpages.h
/// \brief Config page for the colors.
class DsoConfigColorsPage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigColorsPage( DsoSettings *settings, QWidget *parent = nullptr );

  public slots:
    void saveSettings();

  private:
    DsoSettings *settings;

    QVBoxLayout *mainLayout;

    QGroupBox *colorsGroup;
    QGridLayout *colorsLayout;

    QLabel *screenColorsLabel, *printColorsLabel;
    QLabel *axesLabel, *backgroundLabel, *borderLabel, *gridLabel, *markersLabel, *textLabel;
    ColorBox *axesColorBox, *backgroundColorBox, *borderColorBox, *gridColorBox, *markersColorBox, *textColorBox;

    ColorBox *printAxesColorBox, *printBackgroundColorBox, *printBorderColorBox, *printGridColorBox, *printMarkersColorBox,
        *printTextColorBox;

    QLabel *horizontalLine;

    QLabel *screenChannelLabel, *screenSpectrumLabel, *printChannelLabel, *printSpectrumLabel;
    std::vector< QLabel * > colorLabel;
    std::vector< ColorBox * > screenChannelColorBox;
    std::vector< ColorBox * > screenSpectrumColorBox;
    std::vector< ColorBox * > printChannelColorBox;
    std::vector< ColorBox * > printSpectrumColorBox;
    QCheckBox *screenColorCheckBox;
    QLabel *fontSizeLabel;
    QSpinBox *fontSizeSpinBox;
    QCheckBox *styleFusionCheckBox;
    QLabel *themeLabel;
    QComboBox *themeComboBox;
    QGridLayout *themeLayout;
};
