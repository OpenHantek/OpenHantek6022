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
#include "settings.h"
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
    double stretchBase = (double)(paintDevice->width() - lineHeight * 10) / 4;

    // Print trigger details
    painter.setPen(colorValues->voltage[settings->scope.trigger.source]);
    QString levelString = valueToString(settings->scope.voltage[settings->scope.trigger.source].trigger, UNIT_VOLTS, 3);
    QString pretriggerString = tr("%L1%").arg((int)(settings->scope.trigger.position * 100 + 0.5));
    painter.drawText(QRectF(0, 0, lineHeight * 10, lineHeight),
                     tr("%1  %2  %3  %4")
                         .arg(settings->scope.voltage[settings->scope.trigger.source].name,
                              Dso::slopeString(settings->scope.trigger.slope), levelString, pretriggerString));

    double scopeHeight;

    { // DataAnalyser mutex lock
        // Print sample count
        painter.setPen(colorValues->text);
        painter.drawText(QRectF(lineHeight * 10, 0, stretchBase, lineHeight), tr("%1 S").arg(result->sampleCount()),
                         QTextOption(Qt::AlignRight));
        // Print samplerate
        painter.drawText(QRectF(lineHeight * 10 + stretchBase, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.samplerate, UNIT_SAMPLES) + tr("/s"),
                         QTextOption(Qt::AlignRight));
        // Print timebase
        painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.timebase, UNIT_SECONDS, 0) + tr("/div"),
                         QTextOption(Qt::AlignRight));
        // Print frequencybase
        painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, 0, stretchBase, lineHeight),
                         valueToString(settings->scope.horizontal.frequencybase, UNIT_HERTZ, 0) + tr("/div"),
                         QTextOption(Qt::AlignRight));

        // Draw the measurement table
        stretchBase = (double)(paintDevice->width() - lineHeight * 6) / 10;
        int channelCount = 0;
        for (int channel = settings->scope.voltage.size() - 1; channel >= 0; channel--) {
            if ((settings->scope.voltage[channel].used || settings->scope.spectrum[channel].used) &&
                result->data(channel)) {
                ++channelCount;
                double top = (double)paintDevice->height() - channelCount * lineHeight;

                // Print label
                painter.setPen(colorValues->voltage[channel]);
                painter.drawText(QRectF(0, top, lineHeight * 4, lineHeight), settings->scope.voltage[channel].name);
                // Print coupling/math mode
                if ((unsigned int)channel < deviceSpecification->channels)
                    painter.drawText(QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                                     Dso::couplingString(settings->scope.coupling(channel, deviceSpecification)));
                else
                    painter.drawText(
                        QRectF(lineHeight * 4, top, lineHeight * 2, lineHeight),
                        Dso::mathModeString(Dso::getMathMode(settings->scope.voltage[channel])));

                // Print voltage gain
                painter.drawText(QRectF(lineHeight * 6, top, stretchBase * 2, lineHeight),
                                 valueToString(settings->scope.gain(channel), UNIT_VOLTS, 0) + tr("/div"),
                                 QTextOption(Qt::AlignRight));
                // Print spectrum magnitude
                if (settings->scope.spectrum[channel].used) {
                    painter.setPen(colorValues->spectrum[channel]);
                    painter.drawText(QRectF(lineHeight * 6 + stretchBase * 2, top, stretchBase * 2, lineHeight),
                                     valueToString(settings->scope.spectrum[channel].magnitude, UNIT_DECIBEL, 0) +
                                         tr("/div"),
                                     QTextOption(Qt::AlignRight));
                }

                // Amplitude string representation (4 significant digits)
                painter.setPen(colorValues->text);
                painter.drawText(QRectF(lineHeight * 6 + stretchBase * 4, top, stretchBase * 3, lineHeight),
                                 valueToString(result->data(channel)->computeAmplitude(), UNIT_VOLTS, 4),
                                 QTextOption(Qt::AlignRight));
                // Frequency string representation (5 significant digits)
                painter.drawText(QRectF(lineHeight * 6 + stretchBase * 7, top, stretchBase * 3, lineHeight),
                                 valueToString(result->data(channel)->frequency, UNIT_HERTZ, 5),
                                 QTextOption(Qt::AlignRight));
            }
        }

        // Draw the marker table
        stretchBase = (double)(paintDevice->width() - lineHeight * 10) / 4;
        painter.setPen(colorValues->text);

        // Calculate variables needed for zoomed scope
        double divs = fabs(settings->scope.horizontal.marker[1] - settings->scope.horizontal.marker[0]);
        double time = divs * settings->scope.horizontal.timebase;
        double zoomFactor = DIVS_TIME / divs;
        double zoomOffset = (settings->scope.horizontal.marker[0] + settings->scope.horizontal.marker[1]) / 2;

        if (settings->view.zoom) {
            scopeHeight = (double)(paintDevice->height() - (channelCount + 5) * lineHeight) / 2;
            double top = 2.5 * lineHeight + scopeHeight;

            painter.drawText(QRectF(0, top, stretchBase, lineHeight),
                             tr("Zoom x%L1").arg(DIVS_TIME / divs, -1, 'g', 3));

            painter.drawText(QRectF(lineHeight * 10, top, stretchBase, lineHeight),
                             valueToString(time, UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(lineHeight * 10 + stretchBase, top, stretchBase, lineHeight),
                             valueToString(1.0 / time, UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));

            painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase, lineHeight),
                             valueToString(time / DIVS_TIME, UNIT_SECONDS, 3) + tr("/div"),
                             QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(lineHeight * 10 + stretchBase * 3, top, stretchBase, lineHeight),
                             valueToString(divs * settings->scope.horizontal.frequencybase / DIVS_TIME, UNIT_HERTZ, 3) +
                                 tr("/div"),
                             QTextOption(Qt::AlignRight));
        } else {
            scopeHeight = (double)paintDevice->height() - (channelCount + 4) * lineHeight;
            double top = 2.5 * lineHeight + scopeHeight;

            painter.drawText(QRectF(0, top, stretchBase, lineHeight), tr("Marker 1/2"));

            painter.drawText(QRectF(lineHeight * 10, top, stretchBase * 2, lineHeight),
                             valueToString(time, UNIT_SECONDS, 4), QTextOption(Qt::AlignRight));
            painter.drawText(QRectF(lineHeight * 10 + stretchBase * 2, top, stretchBase * 2, lineHeight),
                             valueToString(1.0 / time, UNIT_HERTZ, 4), QTextOption(Qt::AlignRight));
        }

        // Set DIVS_TIME x DIVS_VOLTAGE matrix for oscillograph
        painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE,
                                  (double)(paintDevice->width() - 1) / 2, (scopeHeight - 1) / 2 + lineHeight * 1.5),
                          false);

        // Draw the graphs
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(Qt::NoBrush);

        for (int zoomed = 0; zoomed < (settings->view.zoom ? 2 : 1); ++zoomed) {
            switch (settings->scope.horizontal.format) {
            case Dso::GraphFormat::TY:
                // Add graphs for channels
                for (ChannelID channel = 0; channel < settings->scope.voltage.size(); ++channel) {
                    if (settings->scope.voltage[channel].used && result->data(channel)) {
                        painter.setPen(QPen(colorValues->voltage[channel], 0));

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor =
                            result->data(channel)->voltage.interval / settings->scope.horizontal.timebase;
                        // How many samples are visible?
                        double centerPosition, centerOffset;
                        if (zoomed) {
                            centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
                        } else {
                            centerPosition = DIVS_TIME / 2 / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / 2;
                        }
                        unsigned int firstPosition = qMax((int)(centerPosition - centerOffset), 0);
                        unsigned int lastPosition = qMin((int)(centerPosition + centerOffset),
                                                         (int)result->data(channel)->voltage.sample.size() - 1);

                        // Draw graph
                        QPointF *graph = new QPointF[lastPosition - firstPosition + 1];

                        for (unsigned int position = firstPosition; position <= lastPosition; ++position)
                            graph[position - firstPosition] = QPointF(position * horizontalFactor - DIVS_TIME / 2,
                                                                      result->data(channel)->voltage.sample[position] /
                                                                              settings->scope.gain(channel) +
                                                                          settings->scope.voltage[channel].offset);

                        painter.drawPolyline(graph, lastPosition - firstPosition + 1);
                        delete[] graph;
                    }
                }

                // Add spectrum graphs
                for (ChannelID channel = 0; channel < settings->scope.spectrum.size(); ++channel) {
                    if (settings->scope.spectrum[channel].used && result->data(channel)) {
                        painter.setPen(QPen(colorValues->spectrum[channel], 0));

                        // What's the horizontal distance between sampling points?
                        double horizontalFactor =
                            result->data(channel)->spectrum.interval / settings->scope.horizontal.frequencybase;
                        // How many samples are visible?
                        double centerPosition, centerOffset;
                        if (zoomed) {
                            centerPosition = (zoomOffset + DIVS_TIME / 2) / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / zoomFactor / 2;
                        } else {
                            centerPosition = DIVS_TIME / 2 / horizontalFactor;
                            centerOffset = DIVS_TIME / horizontalFactor / 2;
                        }
                        unsigned int firstPosition = qMax((int)(centerPosition - centerOffset), 0);
                        unsigned int lastPosition = qMin((int)(centerPosition + centerOffset),
                                                         (int)result->data(channel)->spectrum.sample.size() - 1);

                        // Draw graph
                        QPointF *graph = new QPointF[lastPosition - firstPosition + 1];

                        for (unsigned int position = firstPosition; position <= lastPosition; ++position)
                            graph[position - firstPosition] =
                                QPointF(position * horizontalFactor - DIVS_TIME / 2,
                                        result->data(channel)->spectrum.sample[position] /
                                                settings->scope.spectrum[channel].magnitude +
                                            settings->scope.spectrum[channel].offset);

                        painter.drawPolyline(graph, lastPosition - firstPosition + 1);
                        delete[] graph;
                    }
                }
                break;

            case Dso::GraphFormat::XY:
                break;

            default:
                break;
            }

            // Set DIVS_TIME / zoomFactor x DIVS_VOLTAGE matrix for zoomed
            // oscillograph
            painter.setMatrix(QMatrix((paintDevice->width() - 1) / DIVS_TIME * zoomFactor, 0, 0,
                                      -(scopeHeight - 1) / DIVS_VOLTAGE,
                                      (double)(paintDevice->width() - 1) / 2 -
                                          zoomOffset * zoomFactor * (paintDevice->width() - 1) / DIVS_TIME,
                                      (scopeHeight - 1) * 1.5 + lineHeight * 4),
                              false);
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
        painter.setMatrix(QMatrix((scopeWidth - 1) / DIVS_TIME, 0, 0, -(scopeHeight - 1) / DIVS_VOLTAGE,
                                  (double)(scopeWidth - 1) / 2,
                                  (scopeHeight - 1) * (zoomed + 0.5) + lineHeight * 1.5 + lineHeight * 2.5 * zoomed),
                          false);

        // Grid lines
        painter.setPen(QPen(colorValues->grid, 0));

        if (isPrinter) {
            // Draw vertical lines
            for (int div = 1; div < DIVS_TIME / 2; ++div) {
                for (int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawLine(QPointF((double)-div - 0.02, (double)-dot / 5),
                                     QPointF((double)-div + 0.02, (double)-dot / 5));
                    painter.drawLine(QPointF((double)-div - 0.02, (double)dot / 5),
                                     QPointF((double)-div + 0.02, (double)dot / 5));
                    painter.drawLine(QPointF((double)div - 0.02, (double)-dot / 5),
                                     QPointF((double)div + 0.02, (double)-dot / 5));
                    painter.drawLine(QPointF((double)div - 0.02, (double)dot / 5),
                                     QPointF((double)div + 0.02, (double)dot / 5));
                }
            }
            // Draw horizontal lines
            for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for (int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    painter.drawLine(QPointF((double)-dot / 5, (double)-div - 0.02),
                                     QPointF((double)-dot / 5, (double)-div + 0.02));
                    painter.drawLine(QPointF((double)dot / 5, (double)-div - 0.02),
                                     QPointF((double)dot / 5, (double)-div + 0.02));
                    painter.drawLine(QPointF((double)-dot / 5, (double)div - 0.02),
                                     QPointF((double)-dot / 5, (double)div + 0.02));
                    painter.drawLine(QPointF((double)dot / 5, (double)div - 0.02),
                                     QPointF((double)dot / 5, (double)div + 0.02));
                }
            }
        } else {
            // Draw vertical lines
            for (int div = 1; div < DIVS_TIME / 2; ++div) {
                for (int dot = 1; dot < DIVS_VOLTAGE / 2 * 5; ++dot) {
                    painter.drawPoint(QPointF(-div, (double)-dot / 5));
                    painter.drawPoint(QPointF(-div, (double)dot / 5));
                    painter.drawPoint(QPointF(div, (double)-dot / 5));
                    painter.drawPoint(QPointF(div, (double)dot / 5));
                }
            }
            // Draw horizontal lines
            for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
                for (int dot = 1; dot < DIVS_TIME / 2 * 5; ++dot) {
                    if (dot % 5 == 0) continue; // Already done by vertical lines
                    painter.drawPoint(QPointF((double)-dot / 5, -div));
                    painter.drawPoint(QPointF((double)dot / 5, -div));
                    painter.drawPoint(QPointF((double)-dot / 5, div));
                    painter.drawPoint(QPointF((double)dot / 5, div));
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
