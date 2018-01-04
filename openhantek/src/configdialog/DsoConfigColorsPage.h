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
#include <QSpinBox>
#include <QVBoxLayout>

#include "colorbox.h"

////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigColorsPage                                     configpages.h
/// \brief Config page for the colors.
class DsoConfigColorsPage : public QWidget {
    Q_OBJECT

  public:
    DsoConfigColorsPage(DsoSettings *settings, QWidget *parent = 0);

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

    ColorBox *printAxesColorBox, *printBackgroundColorBox, *printBorderColorBox, *printGridColorBox,
        *printMarkersColorBox, *printTextColorBox;

    QLabel *graphLabel;

    QLabel *screenChannelLabel, *screenSpectrumLabel, *printChannelLabel, *printSpectrumLabel;
    std::vector<QLabel *> colorLabel;
    std::vector<ColorBox *> screenChannelColorBox;
    std::vector<ColorBox *> screenSpectrumColorBox;
    std::vector<ColorBox *> printChannelColorBox;
    std::vector<ColorBox *> printSpectrumColorBox;
};
