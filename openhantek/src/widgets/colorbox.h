// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QColor>
#include <QPushButton>

/// \brief A widget for the selection of a color.
class ColorBox : public QPushButton {
    Q_OBJECT

  public:
    ColorBox(QColor color, QWidget *parent = 0);
    ~ColorBox();

    const QColor getColor();

  public slots:
    void setColor(QColor color);
    void waitForColor();

  private:
    QColor color;

  signals:
    void colorChanged(QColor color); ///< The color has been changed
};
