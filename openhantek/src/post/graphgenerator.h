// SPDX-License-Identifier: GPL-2.0+

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
    GraphGenerator(const DsoSettingsScope *scope, const DsoSettingsView *view);
    void generateGraphsXY(PPresult *result, const DsoSettingsScope *scope);

  private:
    void generateGraphsTYvoltage(PPresult *result, const DsoSettingsView *view);
    void generateGraphsTYspectrum(PPresult *result);

    bool ready = false;
    const DsoSettingsScope *scope;
    const DsoSettingsView *view;
    
    void prepareSinc( void );
    std::vector <double> sinc;
    const unsigned int sincWidth = 5;
    const unsigned int oversample = 10;
    const unsigned int sincSize = sincWidth * oversample;

    // Processor interface
    void process(PPresult *data) override;
};
