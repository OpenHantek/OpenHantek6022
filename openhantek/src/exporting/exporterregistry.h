// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <memory>
#include <set>
#include <vector>

// Post processing forwards
class Processor;
class PPresult;

// Settings forwards
class DsoSettings;
namespace Dso {
struct ControlSpecification;
}

// Exporter forwards
class ExporterInterface;

class ExporterRegistry : public QObject {
    Q_OBJECT

  public:
    explicit ExporterRegistry( const Dso::ControlSpecification *deviceSpecification, DsoSettings *settings,
                               QObject *parent = nullptr );

    // Sample input. This will probably be performed in the post processing
    // thread context. Do not open GUI dialogs or interrupt the control flow.
    void addRawSamples( PPresult *data );
    void input( std::shared_ptr< PPresult > data );

    void registerExporter( ExporterInterface *exporter );
    void setExporterEnabled( ExporterInterface *exporter, bool enabled );

    void checkForWaitingExporters();

    // Iterate over this class object
    std::vector< ExporterInterface * >::const_iterator begin();
    std::vector< ExporterInterface * >::const_iterator end();

    /// Device specifications
    const Dso::ControlSpecification *deviceSpecification;
    const DsoSettings *settings;

  private:
    /// List of all available exporters
    std::vector< ExporterInterface * > exporters;
    /// List of exporters that collect samples at the moment
    std::list< ExporterInterface * > enabledExporters;
    /// List of exporters that wait to be called back by the user to save their work
    std::set< ExporterInterface * > waitToSaveExporters;

    /// Process data from addRawSamples() or input() in the given exporter. Add the
    /// exporter to waitToSaveExporters if it finishes.
    ///
    /// @return Return true if the exporter has finished and want to be removed from the
    ///     enabledExporters list.
    bool processData( std::shared_ptr< PPresult > &data, ExporterInterface *const &exporter );

  signals:
    void exporterStatusChanged( const QString &exporterName, const QString &status );
    void exporterProgressChanged();
};
