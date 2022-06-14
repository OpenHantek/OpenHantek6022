// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDoubleSpinBox>
#include <QStringList>

#include "utils/printutils.h"

/// \brief A spin box with SI prefix support.
/// This spin box supports the SI prefixes (k/M/G/T) after its value and allows
/// floating point values. The step size is increasing in an exponential way, to
/// keep the percentual difference between the steps at equal levels.
class SiSpinBox : public QDoubleSpinBox {
    Q_OBJECT

  public:
    explicit SiSpinBox( QWidget *parent = nullptr );
    SiSpinBox( Unit unit, QWidget *parent = nullptr );
    ~SiSpinBox() override;

    QValidator::State validate( QString &input, int &pos ) const override;
    double valueFromText( const QString &text ) const override;
    QString textFromValue( double val ) const override;
    void fixup( QString &input ) const override;
    void stepBy( int steps ) override;
    bool setUnit( Unit unit );
    void setUnitPostfix( const QString &postfix );
    void setSteps( const QList< double > &steps );
    void setMode( const int mode );

  private:
    void init();
    void setBackground();

    Unit unit;             ///< The SI unit used for this spin box
    QString unitPostfix;   ///< Shown after the unit
    QList< double > steps; ///< The steps, begins from start after last element
    int mode;              ///< The mode, fixed or constant

    bool steppedTo; ///< true, if the current value was reached using stepBy
    int stepId;     ///< The index of the last step reached using stepBy

  signals:

  private slots:
    void resetSteppedTo();
};
