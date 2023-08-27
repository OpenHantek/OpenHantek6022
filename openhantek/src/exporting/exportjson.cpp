// SPDX-License-Identifier: GPL-2.0-or-later
// Sandro Sobczyński <sandro.sobczynski@gmail.com>

#include "exportjson.h"
#include "dsosettings.h"
#include "exporterregistry.h"
#include "post/ppresult.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QLocale>
#include <QMessageBox>
#include <QTextStream>

ExporterJSON::ExporterJSON() {}

void ExporterJSON::create( ExporterRegistry *newRegistry ) {
    registry = newRegistry;
    data.reset();
}

QString ExporterJSON::name() { return tr( "Export &JSON .." ); }

QString ExporterJSON::format() { return "JSON"; }

ExporterInterface::Type ExporterJSON::type() { return Type::SnapshotExport; }

bool ExporterJSON::samples( const std::shared_ptr< PPresult > newData ) {
    data = std::move( newData );
    return false;
}

QFile *ExporterJSON::getFile() {
    QFileDialog fileDialog( nullptr, tr( "Save JSON" ), QString(), tr( "Java Script Object Notation (*.json)" ) );
    fileDialog.setFileMode( QFileDialog::AnyFile );
    fileDialog.setAcceptMode( QFileDialog::AcceptSave );
    fileDialog.setOption( QFileDialog::DontUseNativeDialog );
    if ( fileDialog.exec() != QDialog::Accepted )
        return nullptr;

    QFile *jsonFile = new QFile( fileDialog.selectedFiles().first() );
    if ( !jsonFile->open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        QMessageBox::critical( nullptr, QCoreApplication::applicationName(), tr( "Write error\n%1" ).arg( jsonFile->fileName() ) );
        return nullptr;
    }
    return jsonFile;
}

void ExporterJSON::fillData( QTextStream &jsonStream, const ExporterData &dto ) {
    std::vector< const SampleValues * > voltageData = dto.getVoltageData();
    std::vector< const SampleValues * > spectrumData = dto.getSpectrumData();

    jsonStream << "[\n";
    const char *indent = "  ";

    for ( unsigned int row = 0; row < dto.getMaxRow(); ++row ) {
        jsonStream << indent << "{\n";

        QString objInString; // to easily remove latest comma in json object
        QTextStream objInStream( &objInString );
        objInStream.setRealNumberNotation( QTextStream::FixedNotation );
        objInStream.setRealNumberPrecision( 10 );

        objInStream << indent << indent << "\"time\": " << dto.getTimeInterval() * row << ",\n";

        for ( ChannelID channel = 0; channel < dto.getChannelsCount(); ++channel )
            if ( voltageData[ channel ] != nullptr ) {
                objInStream << indent << indent << '\"' << registry->settings->scope.voltage[ channel ].name << "\": ";
                if ( row < voltageData[ channel ]->samples.size() )
                    objInStream << voltageData[ channel ]->samples[ row ];
                else
                    objInStream << "\": null";
                objInStream << ",\n";
            }

        if ( dto.isSpectrumUsed() ) {
            objInStream << indent << indent << "\"freq\": " << dto.getFreqInterval() * row << ",\n";
            for ( ChannelID channel = 0; channel < dto.getChannelsCount(); ++channel ) {
                if ( spectrumData[ channel ] != nullptr ) {
                    objInStream << indent << indent << '\"' << registry->settings->scope.spectrum[ channel ].name << "\": ";
                    if ( row < spectrumData[ channel ]->samples.size() )
                        objInStream << spectrumData[ channel ]->samples[ row ];
                    else
                        objInStream << "null";
                    objInStream << ",\n";
                }
            }
        }

        jsonStream << objInString.mid( 0, objInString.length() - 2 ) << '\n' << indent << '}';
        if ( row != dto.getMaxRow() - 1 )
            jsonStream << ',';
        jsonStream << '\n';
    }
    jsonStream << "]\n";
}

bool ExporterJSON::save() {
    QFile *jsonFile = getFile();
    if ( jsonFile == nullptr )
        return false;

    QTextStream jsonStream( jsonFile );
    jsonStream.setRealNumberNotation( QTextStream::FixedNotation );
    jsonStream.setRealNumberPrecision( 10 );

    ExporterData dto = ExporterData( data, registry->settings->scope );
    fillData( jsonStream, dto );

    jsonFile->close();
    delete jsonFile;

    return true;
}


float ExporterJSON::progress() { return data ? 1.0f : 0; }
