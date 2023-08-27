// SPDX-License-Identifier: GPL-2.0-or-later

#include "exportcsv.h"
#include "dsosettings.h"
#include "exporterregistry.h"
#include "post/ppresult.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QLocale>
#include <QMessageBox>
#include <QTextStream>

ExporterCSV::ExporterCSV() {}

void ExporterCSV::create( ExporterRegistry *newRegistry ) {
    registry = newRegistry;
    data.reset();
}

QString ExporterCSV::name() { return tr( "Export &CSV .." ); }

QString ExporterCSV::format() { return "CSV"; }

ExporterInterface::Type ExporterCSV::type() { return Type::SnapshotExport; }

bool ExporterCSV::samples( const std::shared_ptr< PPresult > newData ) {
    data = std::move( newData );
    return false;
}

QFile *ExporterCSV::getFile() {
    QFileDialog fileDialog( nullptr, tr( "Save CSV" ), QString(), tr( "Comma-Separated Values (*.csv)" ) );
    fileDialog.setFileMode( QFileDialog::AnyFile );
    fileDialog.setAcceptMode( QFileDialog::AcceptSave );
    fileDialog.setOption( QFileDialog::DontUseNativeDialog );
    if ( fileDialog.exec() != QDialog::Accepted )
        return nullptr;

    QFile *csvFile = new QFile( fileDialog.selectedFiles().first() );
    if ( !csvFile->open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        QMessageBox::critical( nullptr, QCoreApplication::applicationName(), tr( "Write error\n%1" ).arg( csvFile->fileName() ) );
        return nullptr;
    }
    return csvFile;
}

void ExporterCSV::fillHeaders( QTextStream &csvStream, const ExporterData &dto, const char *sep ) {
    std::vector< const SampleValues * > voltageData = dto.getVoltageData();
    std::vector< const SampleValues * > spectrumData = dto.getSpectrumData();

    csvStream << "\"t / s\"";

    // Channels
    for ( ChannelID channel = 0; channel < dto.getChannelsCount(); ++channel ) {
        if ( voltageData[ channel ] != nullptr ) {
            csvStream << sep << "\"" << registry->settings->scope.voltage[ channel ].name << " / V\"";
        }
    }

    // Spectrums
    if ( dto.isSpectrumUsed() ) {
        csvStream << sep << "\"f / Hz\"";
        for ( ChannelID channel = 0; channel < dto.getChannelsCount(); ++channel ) {
            if ( spectrumData[ channel ] != nullptr ) {
                csvStream << sep << "\"" << registry->settings->scope.spectrum[ channel ].name << " / dB\"";
            }
        }
    }

    csvStream << "\n";
}


void ExporterCSV::fillData( QTextStream &csvStream, const ExporterData &dto, const char *sep ) {
    std::vector< const SampleValues * > voltageData = dto.getVoltageData();
    std::vector< const SampleValues * > spectrumData = dto.getSpectrumData();

    for ( unsigned int row = 0; row < dto.getMaxRow(); ++row ) {

        csvStream << QLocale().toString( dto.getTimeInterval() * row );
        for ( ChannelID channel = 0; channel < dto.getChannelsCount(); ++channel ) {
            if ( voltageData[ channel ] != nullptr ) {
                csvStream << sep;
                if ( row < voltageData[ channel ]->samples.size() ) {
                    csvStream << QLocale().toString( voltageData[ channel ]->samples[ row ] );
                }
            }
        }
        if ( dto.isSpectrumUsed() ) {
            csvStream << sep << QLocale().toString( dto.getFreqInterval() * row );
            for ( ChannelID channel = 0; channel < dto.getChannelsCount(); ++channel ) {
                if ( spectrumData[ channel ] != nullptr ) {
                    csvStream << sep;
                    if ( row < spectrumData[ channel ]->samples.size() ) {
                        csvStream << QLocale().toString( spectrumData[ channel ]->samples[ row ] );
                    }
                }
            }
        }
        csvStream << "\n";
    }
}

bool ExporterCSV::save() {
    QFile *file = getFile();
    if ( file == nullptr )
        return false;

    QTextStream csvStream( file );
    csvStream.setRealNumberNotation( QTextStream::FixedNotation );
    csvStream.setRealNumberPrecision( 10 );

    ExporterData dto = ExporterData( data, registry->settings->scope );

    // use semicolon as data separator if comma is already used as decimal separator - e.g. with german locale
    const char *sep = QLocale().decimalPoint() == ',' ? ";" : ",";

    fillHeaders( csvStream, dto, sep );
    fillData( csvStream, dto, sep );

    file->close();
    delete file;

    return true;
}


float ExporterCSV::progress() { return data ? 1.0f : 0; }
