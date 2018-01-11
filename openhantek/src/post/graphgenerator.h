// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <deque>

#include <QObject>
#include <QVector3D>

#include "hantekdso/enums.h"
#include "hantekprotocol/types.h"
#include "processor.h"

struct DsoSettingsScope;
class PPresult;
namespace Dso {
struct ControlSpecification;
}

/// \brief Generates ready to be used vertex arrays
class GraphGenerator : public QObject, public Processor {
    Q_OBJECT

  public:
    GraphGenerator(const DsoSettingsScope *scope, bool isSoftwareTriggerDevice);
    void generateGraphsXY(PPresult *result, const DsoSettingsScope *scope);

    bool isReady() const;

  private:
    void generateGraphsTYvoltage(PPresult *result);
    void generateGraphsTYspectrum(PPresult *result);

  private:
    bool ready = false;
    const DsoSettingsScope *scope;
    const bool isSoftwareTriggerDevice;

    // Processor interface
    private:
    virtual void process(PPresult *) override;
};
