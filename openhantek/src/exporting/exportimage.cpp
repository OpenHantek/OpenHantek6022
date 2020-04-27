// SPDX-License-Identifier: GPL-2.0+

#include "exportimage.h"
#include "dsosettings.h"
#include "exporterregistry.h"
#include "iconfont/QtAwesome.h"
#include "legacyexportdrawer.h"
#include "post/ppresult.h"
#include "viewsettings.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QPrinter>

ExporterImage::ExporterImage() {}

void ExporterImage::create( ExporterRegistry *newRegistry ) {
    this->registry = newRegistry;
    data.reset();
}

int ExporterImage::faIcon() { return fa::image; }

QString ExporterImage::name() { return QCoreApplication::tr( "Export &Image/PDF .." ); }

ExporterInterface::Type ExporterImage::type() { return Type::SnapshotExport; }

bool ExporterImage::samples( const std::shared_ptr< PPresult > newData ) {
    data = std::move( newData );
    return false;
}

bool ExporterImage::save() {
    QStringList filters;
    filters << QCoreApplication::tr( "Portable Document Format (*.pdf)" ) << QCoreApplication::tr( "Image (*.png *.xpm *.jpg)" );

    QFileDialog fileDialog( nullptr, QCoreApplication::tr( "Export file .." ), QString(), filters.join( ";;" ) );
    fileDialog.setFileMode( QFileDialog::AnyFile );
    fileDialog.setAcceptMode( QFileDialog::AcceptSave );
    if ( fileDialog.exec() != QDialog::Accepted )
        return false;

    bool isPdf = filters.indexOf( fileDialog.selectedNameFilter() ) == 0;
    std::unique_ptr< QPaintDevice > paintDevice;

    const DsoSettingsColorValues *colorValues = &( registry->settings->view.print );

    QString fileName = fileDialog.selectedFiles().first();

    if ( !isPdf ) {
        if ( !fileName.endsWith( ".png" ) && !fileName.endsWith( ".xpm" ) && !fileName.endsWith( ".jpg" ) ) {
            fileName += ".png";
        }
        // We need a QPixmap for image-export
        QPixmap *qPixmap = new QPixmap( registry->settings->exporting.imageSize );
        qPixmap->fill( colorValues->background );
        paintDevice = std::unique_ptr< QPaintDevice >( qPixmap );
    } else {
        if ( !fileName.endsWith( ".pdf" ) ) {
            fileName += ".pdf";
        }
        // We need a QPrinter for printing, pdf- and ps-export
        std::unique_ptr< QPrinter > printer = std::unique_ptr< QPrinter >( new QPrinter( QPrinter::HighResolution ) );
        printer->setOrientation( registry->settings->view.zoom ? QPrinter::Portrait : QPrinter::Landscape );
        printer->setPageMargins( 20, 20, 20, 20, QPrinter::Millimeter );
        printer->setOutputFileName( fileName );
        printer->setOutputFormat( QPrinter::PdfFormat );
        paintDevice = std::move( printer );
    }

    if ( !paintDevice )
        return false;

    LegacyExportDrawer::exportSamples( data.get(), paintDevice.get(), registry->deviceSpecification, registry->settings,
                                       colorValues );

    if ( !isPdf )
        static_cast< QPixmap * >( paintDevice.get() )->save( fileName );
    return true;
}

float ExporterImage::progress() { return data ? 1.0f : 0; }
