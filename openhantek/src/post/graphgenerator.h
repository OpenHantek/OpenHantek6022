// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>

#include <QObject>
#include <QVector3D>

#include "hantekdso/enums.h"
#include "hantekprotocol/types.h"
#include "processor.h"

struct DsoSettingsScope;
struct DsoSettingsView;
class PPresult;
namespace Dso {
struct ControlSpecification;
}

/// \brief Generates ready to be used vertex arrays
class GraphGenerator : public QObject, public Processor {
    Q_OBJECT

  public:
    GraphGenerator( const DsoSettingsScope *scope, const DsoSettingsView *view );

  private:
    void generateGraphsTYvoltage( PPresult *result );
    void generateGraphsTYspectrum( PPresult *result );
    void generateGraphsXY( PPresult *result );

    bool ready = false;
    const DsoSettingsScope *scope;
    const DsoSettingsView *view;

    void prepareSinc( void );                             // setup the sinc table used for upsampling
    std::vector< double > sinc;                           // sinc function table for convolution
    const unsigned int sincWidth = 2;                     // two periods
    const unsigned int oversample = 5;                    // 5 time oversample
    const unsigned int sincSize = sincWidth * oversample; // size of the table
    std::vector< double > resample;                       // destination for overampled data

    // Processor interface
    void process( PPresult *data ) override;
};
