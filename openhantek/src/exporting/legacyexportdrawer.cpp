// SPDX-License-Identifier: GPL-2.0+

#include <algorithm>
#include <cmath>
#include <memory>

#include <QCoreApplication>
#include <QImage>
#include <QPainter>

#include "legacyexportdrawer.h"

#include "controlspecification.h"
#include "post/graphgenerator.h"
#include "post/ppresult.h"
#include "dsosettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"

#define tr(msg) QCoreApplication::translate("Exporter", msg)

bool LegacyExportDrawer::exportSamples(const PPresult *result, QPaintDevice* paintDevice,
                             const Dso::ControlSpecification *deviceSpecification,
                             const DsoSettings *settings, bool isPrinter, const DsoSettingsColorValues *colorValues) {
    // Create a painter for our device
    QPainter painter(paintDevice);

    // Get line height
    QFont font;
    QFontMetrics fontMetrics(font, paintDevice);
    double lineHeight = fontMetrics.height();

    painter.setBrush(Qt::SolidPattern);

    // Draw the settings table
    double stretchBase = double(paintDevice->width()) / 6;

    // Print trigger details
    double pulseWidth1 = result->data( 0 )->pulseWidth1;
    double pulseWidth2 = result->data( 0 )->pulseWidth2;
    painter.setPen(colorValues->voltage[settings->scope.trigger.source]);
    QString levelString = valueToString(settings->scope.voltage[settings->scope.trigger.source].trigger, UNIT_VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg(int(settings->scope.trigger.offset * 100 + 0.5));
    QString pre = Dso::slopeString(settings->scope.trigger.slope); // trigger slope
    QString post = pre; // opposite trigger slope
    if ( settings->scope.trigger.slope == Dso::Slope::Positive )
        post = Dso::slopeString( Dso::Slope:: Negative );
    else if ( settings->scope.trigger.slope == Dso::Slope::Negative )
        post = Dso::slopeString( Dso::Slope:: Positive );
    QString pulseWidthString = bool( pulseWidth1 ) ? pre + valueToString( pulseWidth1, UNIT_SECONDS, 3) + post : "";
    pulseWidthString += bool( pulseWidth2 ) ? valueToString( pulseWidth2, UNIT_SECONDS, 3) + pre : "";
    if ( bool( pulseWidth1 ) && bool( pulseWidth2 ) ) {
        int dutyCyle = int( 0.5 + ( 100.0 * pulseWidth1 ) / ( pulseWidth1 + pulseWidth2 ) );
        pulseWidthString += " (" + QString::number( dutyCyle ) + "%)";
    }
    painter.drawText(QRectF( 0, 0, 2 * stretchBase, lineHeight ),
                     tr( "%1  %2  %3  %4 %5" )
                         .arg( settings->scope.voltage[settings->scope.trigger.source].name,
                               Dso::slopeString(settings->scope.trigger.slope),
                               levelString, pretriggerString, pulseWidthString
                             )
                    );

    double scopeHeight;

    { // DataAnalyser mutex lock
        // Print sample count
        painter.setPen(colorValues->text);
        painter.drawText(QRectF(stretchBase * 2, 0, stretchBase, lineHeight), tr("%1 S on screen").arg(
            int( settings->scope.horizontal.samplerate * settings->scope.horizontal.timebase * DIVS_TIME + 0.99 )
        ),
                         QTextOption(Qt::AlignRight));
        // Print samplerate
        painter.drawText(QRectF(stretchBase * 3, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.samplerate, UNIT_SAMPLES) + tr("/s"),
                         QTextOption(Qt::AlignRight));
        // Print timebase
        painter.drawText(QRectF(stretchBase * 4, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.timebase, UNIT_SECONDS, 0) + tr("/div"),
                         QTextOption(Qt::AlignRight));
        // Print frequencybase
        painter.drawText(QRectF(stretchBase * 5, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.frequencybase, UNIT_HERTZ, 0) + tr("/div"),
                         QTextOption(Qt::AlignRight));

        // Draw the measurement table
        stretchBase = double(paintDevice->width() ) / 30;
        int channelCount = 0;
        for ( int channel = int(settings->scope.voltage.size() - 1); channel >= 0; channel--) {
            if ((settings->scope.voltage[unsigned(channel)].used || settings->scope.spectrum[unsigned(channel)].used) &&
                result->data(unsigned(channel))) {
                ++channelCount;
                double top = double(paintDevice->height()) - channelCount * lineHeight;
                double tPos=0.0, tWidth;
                // Print label
                tWidth = 2;
                painter.setPen(colorValues->voltage[unsigned(channel)]);
                painter.drawText(
                    QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                    settings->scope.voltage[unsigned(channel)].name );
                // Print coupling/math mode
                tPos += tWidth;
                tWidth = 3.5;
                if ( unsigned(channel) < deviceSpecification->channels )
                    painter.drawText(
                        QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                        Dso::couplingString( settings->scope.coupling(unsigned(channel), deviceSpecification) ) );
                else
                    painter.drawText(
                        QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                        Dso::mathModeString(Dso::getMathMode(settings->scope.voltage[unsigned(channel)]) ) );

                // Print voltage gain
                tPos += tWidth;
                tWidth = 3;
                painter.drawText( 
                    QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                        valueToString(settings->scope.gain(unsigned(channel)), UNIT_VOLTS, 0) + tr("/div"),
                        QTextOption(Qt::AlignRight) );
                // Print spectrum magnitude
                tPos += tWidth;
                tWidth = 3;
                if (settings->scope.spectrum[unsigned(channel)].used) {
                    painter.setPen(colorValues->spectrum[unsigned(channel)]);
                    painter.drawText( 
                        QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                            valueToString(settings->scope.spectrum[unsigned(channel)].magnitude, UNIT_DECIBEL, 0) +
                            tr("/div"), QTextOption(Qt::AlignRight) );
                }

                // Vpp Amplitude string representation (3 significant digits)
                tPos += tWidth;
                tWidth = 3;
                painter.setPen(colorValues->text);
                painter.drawText( QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                                 valueToString(result->data(unsigned(channel))->vpp, UNIT_VOLTS, 3) + "pp",
                                 QTextOption(Qt::AlignRight) );
                // RMS Amplitude string representation (3 significant digits)
                tPos += tWidth;
                tWidth = 3.5;
                painter.setPen(colorValues->text);
                painter.drawText(QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                                 valueToString(result->data(unsigned(channel))->rms, UNIT_VOLTS, 3) + "rms",
                                 QTextOption(Qt::AlignRight) );
                // DC Amplitude string representation (3 significant digits)
                tPos += tWidth;
                tWidth = 3;
                painter.setPen(colorValues->text);
                painter.drawText(QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                                 valueToString(result->data(unsigned(channel))->dc, UNIT_VOLTS, 3) + "=",
                                 QTextOption(Qt::AlignRight) );
                // AC Amplitude string representation (3 significant digits)
                tPos += tWidth;
                tWidth = 3;
                painter.setPen(colorValues->text);
                painter.drawText( QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                                 valueToString(result->data(unsigned(channel))->ac, UNIT_VOLTS, 3) + "~",
                                 QTextOption(Qt::AlignRight) );
                // dB Amplitude string representation (3 significant digits)
                tPos += tWidth;
                tWidth = 3;
                painter.setPen(colorValues->text);
                painter.drawText( QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                                 valueToString(result->data(unsigned(channel))->dB, UNIT_DECIBEL, 3),
                                 QTextOption(Qt::AlignRight) );
                // Frequency string representation (4 significant digits)
                tPos += tWidth;
                tWidth = 3;
                painter.drawText( QRectF( stretchBase * tPos, top, stretchBase * tWidth, lineHeight ),
                                 valueToString(result->data(unsigned(channel))->frequency, UNIT_HERTZ, 4),
                                 QTextOption(Qt::AlignRight) );
            }
        }

        // Draw the marker table
        painter.setPen(colorValues->text);

        // Calculate variables needed for zoomed scope
        double m1 = settings->scope.getMarker(0) + DIVS_TIME / 2; // zero at center -> zero at left margin
        double m2 = settings->scope.getMarker(1) + DIVS_TIME / 2; // zero at center -> zero at left margin
        if ( m1 > m2 )
            std::swap( m1, m2 );
        double divs = m2 - m1;
        double zoomFactor = DIVS_TIME / divs;
        double zoomOffset = (m1 + m2) / 2;
        double time1 = ( m1 - DIVS_TIME * settings->scope.trigger.offset ) * settings->scope.horizontal.timebase;
        double time2 = ( m2 - DIVS_TIME * settings->scope.trigger.offset ) * settings->scope.horizontal.timebase;
        double time = divs * settings->scope.horizontal.timebase;
        double freq1 = m1 * settings->scope.horizontal.frequencybase;
        double freq2 = m2 * settings->scope.horizontal.frequencybase;
        double freq = freq2 - freq1;

        if (settings->view.zoom) {
            stretchBase = double( paintDevice->width() ) / 9;
            scopeHeight = double( paintDevice->height() - ( channelCount + 5 ) * lineHeight ) / 2;
            double top = 2.5 * lineHeight + scopeHeight;

            painter.drawText(QRectF(0, top, stretchBase, lineHeight),
                             tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3));

            painter.drawText(QRectF(stretchBase, top, stretchBase, lineHeight),
                             valueToString( time1, UNIT_SECONDS, 4 ), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 2, top, stretchBase, lineHeight),
                             valueToString( time2, UNIT_SECONDS, 4 ), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 3, top, stretchBase, lineHeight),
                             valueToString( time, UNIT_SECONDS, 4 ), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 4, top, stretchBase, lineHeight),
                             valueToString( freq1, UNIT_HERTZ, 4 ), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 5, top, stretchBase, lineHeight),
                             valueToString( freq2, UNIT_HERTZ, 4 ), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 6, top, stretchBase, lineHeight),
                             valueToString( freq, UNIT_HERTZ, 4 ), QTextOption(Qt::AlignRight));

            painter.drawText(QRectF(stretchBase * 7, top, stretchBase, lineHeight),
                             valueToString( time / DIVS_TIME, UNIT_SECONDS, 3 ) + tr("/div"),
                             QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 8, top, stretchBase, lineHeight),
                             valueToString( freq / DIVS_TIME, UNIT_HERTZ, 3 ) +
                                 tr("/div"),
                             QTextOption(Qt::AlignRight));
        } else {
            stretchBase = double( paintDevice->width() ) / 8;
            scopeHeight = double( paintDevice->height() ) - (channelCount + 4) * lineHeight;
            double top = 2.5 * lineHeight + scopeHeight;

            painter.drawText(QRectF(0, top, stretchBase, lineHeight), tr("Marker 1/2"));
            painter.drawText(QRectF(stretchBase, top, stretchBase, lineHeight),
                             "t1: " + valueToString( time1, UNIT_SECONDS, 4 ), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 2, top, stretchBase, lineHeight),
                             "t2: " + valueToString( time2, UNIT_SECONDS, 4 ), QTextOption(Qt::AlignRight));
            if ( bool( time ) ) {
                painter.drawText(QRectF(stretchBase * 3, top, stretchBase, lineHeight),
                             "Δt: " + valueToString( time, UNIT_SECONDS, 4 ), QTextOption(Qt::AlignRight));
                painter.drawText(QRectF(stretchBase * 4, top, stretchBase, lineHeight),
                             " (=" + valueToString( 1/time, UNIT_HERTZ, 4 ) + ")", QTextOption(Qt::AlignLeft));
            }
            painter.drawText(QRectF(stretchBase * 5, top, stretchBase, lineHeight),
                             "f1: " + valueToString( freq1, UNIT_HERTZ, 4 ), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(stretchBase * 6, top, stretchBase, lineHeight),
                             "f2: " + valueToString( freq2, UNIT_HERTZ, 4 ), QTextOption(Qt::AlignRight));
            if ( bool( freq ) )
                painter.drawText(QRectF(stretchBase * 7, top, stretchBase, lineHeight),
                             "Δf: " + valueToString( freq, UNIT_HERTZ, 4 ), QTextOption(Qt::AlignRight));
        }


        // Draw the graphs
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(Qt::NoBrush);

        for (int zoomed = 0; zoomed < (settings->view.zoom ? 2 : 1); ++zoomed) {
            double m11 = paintDevice->width() / DIVS_TIME;
            double m22 = -scopeHeight / DIVS_VOLTAGE;
            double dx = double( paintDevice->width() ) / 2;
            double dy = scopeHeight / 2 + lineHeight * 1.5;
            if ( zoomed ) { // zoom (m11) and shift in x direction (dx), move down (dy)
                m11 = paintDevice->width() / DIVS_TIME * zoomFactor;
                dx = double( paintDevice->width() ) / 2
                   + ( 0.5 - zoomOffset / DIVS_TIME ) * zoomFactor * paintDevice->width();
                dy = scopeHeight * 1.5 + lineHeight * 4;
            }
            // Set DIVS_TIME x DIVS_VOLTAGE matrix for (non-zoomed / zoomed) oscillograph
            // false: don't combine with current matrix -> replace
            painter.setTransform( QTransform( QMatrix ( m11, 0, 0, m22, dx, dy ) ), false );

            switch (settings->scope.horizontal.format) {
            case Dso::GraphFormat::TY:
                // Add graphs for channels
                for (ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel) {
                    if (settings->scope.voltage[unsigned(channel)].used && result->data(channel)) {
                        painter.setPen(QPen(colorValues->voltage[unsigned(channel)], 0));

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor =
                            result->data(channel)->voltage.interval / settings->scope.horizontal.timebase;
                        // How many samples are visible?
                        int dotsOnScreen = int( DIVS_TIME / horizontalFactor + 0.99 ); // round up
                        // align displayed trace with trigger mark on screen ...
                        // ... also if trig pos or time/div was changed on a "frozen" or single trace
                        int preTrigSamples = int( settings->scope.trigger.offset * dotsOnScreen );
                        int leftmostSample = int( result->triggeredPosition ) - preTrigSamples; // 1st sample to show
                        int rightmostSample = leftmostSample + dotsOnScreen;
                        int leftmostPosition = 0; // start position on display
                        if ( leftmostSample < 0 ) { // trig pos or time/div was increased
                            leftmostPosition = -leftmostSample; // trace can't start on left margin
                            leftmostSample = 0; // show as much as we have on left side
                        }
                        int lastPosition = qMin( int( DIVS_TIME / horizontalFactor ),
                                                 int( result->data(channel)->voltage.sample.size() ) );

                        // Draw graph
                        const unsigned binsPerDiv = 50;
                        QPointF *graph = new QPointF[ result->data(channel)->voltage.sample.size() + 1 ];
                        // skip leading samples to show the correct trigger position
                        auto sampleIterator = result->data(channel)->voltage.sample.cbegin() + leftmostSample; // -> visible samples
                        auto displayEnd = result->data(channel)->voltage.sample.cbegin() + rightmostSample;
                        auto sampleEnd = result->data(channel)->voltage.sample.cend();
                        int pointCount = 0;
                        double gain = settings->scope.gain(channel);
                        double offset = settings->scope.voltage[unsigned(channel)].offset;
                        unsigned bins[ int( binsPerDiv * DIVS_VOLTAGE ) ] = { 0 };
                        for ( int position = leftmostPosition;
                              position < dotsOnScreen && position < lastPosition
                              && sampleIterator < sampleEnd && sampleIterator < displayEnd;
                              ++position, ++sampleIterator) {
                            double x = MARGIN_LEFT + position * horizontalFactor;
                            double y = *sampleIterator / gain + offset;
                            if ( !settings->scope.histogram ) {
                                graph[ position - leftmostPosition ] = QPointF( x, y );
                                ++pointCount;
                            } else {
                                int bin = int( round( binsPerDiv * ( y + DIVS_VOLTAGE / 2) ) );
                                if ( bin > 0 && bin < binsPerDiv * DIVS_VOLTAGE ) // if trace is on screen
                                    ++bins[ bin ]; // count value
                                if ( x < MARGIN_RIGHT - 1.1 ) {
                                    graph[ position - leftmostPosition ] = QPointF( x, y );
                                    ++pointCount;
                                }
                            }
                        }
                        painter.drawPolyline( graph, pointCount );
                        delete[] graph;

                        // Draw histogram
                        if ( settings->scope.histogram ) { // scale and display the histogram
                            QPointF *graph = new QPointF[ int( 2 * binsPerDiv * DIVS_VOLTAGE ) ];
                            double max = 0; // find max histo count
                            pointCount = 0;
                            for ( int bin = 0; bin < binsPerDiv * DIVS_VOLTAGE; ++bin ) {
                                if ( bins[ bin ] > max ) {
                                    max = bins[ bin ];
                                }
                            }
                            for ( int bin = 0; bin < binsPerDiv * DIVS_VOLTAGE; ++bin ) {
                                if ( bins[ bin ] ) { // show bar (= start and end point) if value exists
                                    double y = double( bin ) / binsPerDiv - DIVS_VOLTAGE / 2 - double( channel ) / binsPerDiv / 2;
                                    graph[ pointCount++ ] = QPointF( MARGIN_RIGHT, y );
                                    graph[ pointCount++ ] = QPointF( MARGIN_RIGHT - bins[ bin ] / max, y );
                                }
                            }
                            painter.drawLines( graph, pointCount/2 );
                            delete[] graph;
                        }
                    }
                }

                // Add spectrum graphs
                for (ChannelID channel = 0; channel < settings->scope.spectrum.size(); ++channel) {
                    if (settings->scope.spectrum[unsigned(channel)].used && result->data(channel)) {
                        painter.setPen(QPen(colorValues->spectrum[unsigned(channel)], 0));

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor =
                            result->data(channel)->spectrum.interval / settings->scope.horizontal.frequencybase;
                        // How many samples are visible?
                        unsigned int lastPosition = unsigned( qMin( int( DIVS_TIME / horizontalFactor ),
                                                              int(result->data(channel)->spectrum.sample.size() ) - 1 ) );

                        // Draw graph
                        double magnitude = settings->scope.spectrum[unsigned(channel)].magnitude;
                        double offset = settings->scope.spectrum[unsigned(channel)].offset;
                        QPointF *graph = new QPointF[ result->data(channel)->spectrum.sample.size() + 1 ];

                        for (unsigned int position = 0; position <= lastPosition; ++position) {
                            graph[ position ] =
                                QPointF(MARGIN_LEFT + position * horizontalFactor,
                                        result->data(channel)->spectrum.sample[position] / magnitude + offset
                                       );
                        }
                        painter.drawPolyline( graph, int ( lastPosition + 1 ) );
                        delete[] graph;
                    }
                }
                break;

            case Dso::GraphFormat::XY:
                if ( settings->scope.voltage[ 0 ].used && result->data( 0 )
                     && settings->scope.voltage[ 1 ].used && result->data( 1 ) ) {
                    const double xGain = settings->scope.gain( 0 );
                    const double yGain = settings->scope.gain( 1 );
                    const double xOffset = ( settings->scope.trigger.offset - 0.5 ) * DIVS_TIME;
                    const double yOffset = settings->scope.voltage[ 1 ].offset;
                    painter.setPen( QPen( colorValues->voltage[ 1 ], 0) );
                    const unsigned size = unsigned( std::min( int( result->data( 0 )->voltage.sample.size() ),
                                                                  int( result->data( 1 )->voltage.sample.size() )
                                                             )
                                                  );
                    // Draw graph
                    QPointF *graph = new QPointF[ size ];
                    for ( unsigned int index = 0; index < size; ++index ) {
                        graph[ index ] =
                            QPointF( result->data( 0 )->voltage.sample[ index ] / xGain + xOffset,
                                     result->data( 1 )->voltage.sample[ index ] / yGain + yOffset
                                   );
                    }
                    painter.drawPolyline( graph, int( size ) );
                    delete[] graph;
                }
                break;
            }

            if ( !zoomed ) { // draw marker lines and trigger position
                const double trig = DIVS_TIME * ( settings->scope.trigger.offset - 0.5 );
                const double tick = double( DIVS_TIME ) / 250.0;
                const double top = DIVS_VOLTAGE/2;
                const double bottom = -DIVS_VOLTAGE/2;
                const double left = -DIVS_TIME/2;
                //const double right = DIVS_TIME/2;
                painter.setPen( QPen(colorValues->markers, 0) );
                // markers
                painter.drawLine( QLineF( m1 + left, bottom - 4 * tick, m1 + left, top ) );
                painter.drawLine( QLineF( m2 + left, bottom - 4 * tick, m2 + left, top ) );
                // trigger point (t=0)
                painter.drawLine( QLineF( trig - tick, top + 4 * tick, trig, top ) );
                painter.drawLine( QLineF( trig + tick, top + 4 * tick, trig, top ) );
            }
        }
    } // dataanalyser mutex release

    drawGrids(painter, colorValues, lineHeight, scopeHeight, paintDevice->width(),
              isPrinter, settings->view.zoom);
    painter.end();

    return true;
}


void LegacyExportDrawer::drawGrids(QPainter &painter, const DsoSettingsColorValues *colorValues, double lineHeight, double scopeHeight,
                         int scopeWidth, bool isPrinter, bool zoom) {
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (int zoomed = 0; zoomed < (zoom ? 2 : 1); ++zoomed) {
        // Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
        painter.setTransform( QTransform( QMatrix( (scopeWidth - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE,
                                  double(scopeWidth - 1) / 2,
                                  (scopeHeight - 1) * (zoomed + 0.5) + lineHeight * 1.5 + lineHeight * 2.5 * zoomed) ),
                          false);

        // Grid lines
        painter.setPen(QPen(colorValues->grid, 0));

        if (isPrinter) {
            // Draw vertical lines
            for (int div = 1; div < DIVS_TIME / 2; ++div) {
                for (int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawLine(QPointF(double(-div) - 0.02, double(-dot) / 5),
                                     QPointF(double(-div) + 0.02, double(-dot) / 5));
                    painter.drawLine(QPointF(double(-div) - 0.02, double(dot) / 5),
                                     QPointF(double(-div) + 0.02, double(dot) / 5));
                    painter.drawLine(QPointF(double(div) - 0.02, double(-dot) / 5),
                                     QPointF(double(div) + 0.02, double(-dot) / 5));
                    painter.drawLine(QPointF(double(div) - 0.02, double(dot) / 5),
                                     QPointF(double(div) + 0.02, double(dot) / 5));
                }
            }
            // Draw horizontal lines
            for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for (int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    painter.drawLine(QPointF(double(-dot) / 5, double(-div) - 0.02),
                                     QPointF(double(-dot) / 5, double(-div) + 0.02));
                    painter.drawLine(QPointF(double(dot) / 5, double(-div) - 0.02),
                                     QPointF(double(dot) / 5, double(-div) + 0.02));
                    painter.drawLine(QPointF(double(-dot) / 5, double(div) - 0.02),
                                     QPointF(double(-dot) / 5, double(div) + 0.02));
                    painter.drawLine(QPointF(double(dot) / 5, double(div) - 0.02),
                                     QPointF(double(dot) / 5, double(div) + 0.02));
                }
            }
        } else {
            // Draw vertical lines
            for (int div = 1; div < DIVS_TIME / 2; ++div) {
                for (int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawPoint(QPointF(-div, double(-dot) / 5));
                    painter.drawPoint(QPointF(-div, double(dot) / 5));
                    painter.drawPoint(QPointF(div, double(-dot) / 5));
                    painter.drawPoint(QPointF(div, double(dot) / 5));
                }
            }
            // Draw horizontal lines
            for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for (int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    if (dot % 5 == 0) continue; // Already done by vertical lines
                    painter.drawPoint(QPointF(double(-dot) / 5, -div));
                    painter.drawPoint(QPointF(double(dot) / 5, -div));
                    painter.drawPoint(QPointF(double(-dot) / 5, div));
                    painter.drawPoint(QPointF(double(dot) / 5, div));
                }
            }
        }

        // Axes
        painter.setPen(QPen(colorValues->axes, 0));
        painter.drawLine(QPointF(-DIVS_TIME / 2, 0), QPointF(DIVS_TIME / 2, 0));
        painter.drawLine(QPointF(0, -DIVS_VOLTAGE / 2), QPointF(0, DIVS_VOLTAGE / 2));
        for (double div = 0.2; div <= DIVS_TIME / 2; div += 0.2) {
            painter.drawLine(QPointF(div, -0.05), QPointF(div, 0.05));
            painter.drawLine(QPointF(-div, -0.05), QPointF(-div, 0.05));
        }
        for (double div = 0.2; div <= DIVS_VOLTAGE / 2; div += 0.2) {
            painter.drawLine(QPointF(-0.05, div), QPointF(0.05, div));
            painter.drawLine(QPointF(-0.05, -div), QPointF(0.05, -div));
        }

        // Borders
        painter.setPen(QPen(colorValues->border, 0));
        painter.drawRect(QRectF(-DIVS_TIME / 2, -DIVS_VOLTAGE / 2, DIVS_TIME, DIVS_VOLTAGE));
    }
}
