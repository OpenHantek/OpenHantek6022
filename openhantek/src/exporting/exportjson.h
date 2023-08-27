// SPDX-License-Identifier: GPL-2.0-or-later
// Sandro Sobczyński <sandro.sobczynski@gmail.com>

#pragma once
#include "exporterdata.h"
#include "exporterinterface.h"

#include <QFile>
#include <QTextStream>

class ExporterJSON : public ExporterInterface {
    Q_DECLARE_TR_FUNCTIONS( LegacyExportDrawer )

  public:
    ExporterJSON();
    void create( ExporterRegistry *registry ) override;
    QString name() override;
    QString format() override;
    Type type() override;
    bool samples( const std::shared_ptr< PPresult > newData ) override;
    bool save() override;
    float progress() override;

  private:
    QFile *getFile();
    void fillData( QTextStream &jsonStream, const ExporterData &dto );
    std::shared_ptr< PPresult > data;
};
