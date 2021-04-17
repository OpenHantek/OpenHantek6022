// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "exporterdata.h"
#include "exporterinterface.h"

#include <QFile>
#include <QTextStream>

class ExporterCSV : public ExporterInterface {
    Q_DECLARE_TR_FUNCTIONS( LegacyExportDrawer )

  public:
    ExporterCSV();
    void create( ExporterRegistry *registry ) override;
    int faIcon() override;
    QString name() override;
    Type type() override;
    bool samples( const std::shared_ptr< PPresult > newData ) override;
    bool save() override;
    float progress() override;

  private:
    QFile *getFile();
    void fillHeaders( QTextStream &jsonStream, const ExporterData &dto, const char *sep );
    void fillData( QTextStream &jsonStream, const ExporterData &dto, const char *sep );
    std::shared_ptr< PPresult > data;
};
