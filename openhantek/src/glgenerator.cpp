// SPDX-License-Identifier: GPL-2.0+

#include <QMutex>
#include <QDebug>

#include "glgenerator.h"
#include "utils/printutils.h"
#include "settings.h"
#include "analyse/dataanalyzerresult.h"
#include "viewconstants.h"
#include "hantekdso/softwaretrigger.h"

static const SampleValues& useSamplesOf(Dso::ChannelMode mode, ChannelID channel, const DataAnalyzerResult *result, const DsoSettingsScope *scope) {
    static SampleValues emptyDefault;
    if (mode == Dso::ChannelMode::Voltage) {
        if (!scope->voltage[channel].used || !result->data(channel)) return emptyDefault;
        return result->data(channel)->voltage;
    } else {
        if (!scope->spectrum[channel].used || !result->data(channel)) return emptyDefault;
        return result->data(channel)->spectrum;
    }
}

GlGenerator::GlGenerator() {
    // Grid
    const int DIVS_TIME_S2 = (int)DIVS_TIME - 2;
    const int DIVS_VOLTAGE_S2 = (int)DIVS_VOLTAGE - 2;
    const int vaGrid0Size = (int) ((DIVS_TIME * DIVS_SUB - 2) * DIVS_VOLTAGE_S2 +
                                   (DIVS_VOLTAGE * DIVS_SUB - 2) * DIVS_TIME_S2 -
                                   (DIVS_TIME_S2 * DIVS_VOLTAGE_S2)) * 2;

    vaGrid[0].resize(vaGrid0Size);
    std::vector<GLfloat>::iterator glIterator = vaGrid[0].begin();
    // Draw vertical lines
    for (int div = 1; div < DIVS_TIME / 2; ++div) {
        for (int dot = 1; dot < DIVS_VOLTAGE / 2 * DIVS_SUB; ++dot) {
            float dotPosition = (float)dot / DIVS_SUB;
            *(glIterator++) = -div;
            *(glIterator++) = -dotPosition;
            *(glIterator++) = -div;
            *(glIterator++) = dotPosition;
            *(glIterator++) = div;
            *(glIterator++) = -dotPosition;
            *(glIterator++) = div;
            *(glIterator++) = dotPosition;
        }
    }
    // Draw horizontal lines
    for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
        for (int dot = 1; dot < DIVS_TIME / 2 * DIVS_SUB; ++dot) {
            if (dot % DIVS_SUB == 0) continue; // Already done by vertical lines
            float dotPosition = (float)dot / DIVS_SUB;
            *(glIterator++) = -dotPosition;
            *(glIterator++) = -div;
            *(glIterator++) = dotPosition;
            *(glIterator++) = -div;
            *(glIterator++) = -dotPosition;
            *(glIterator++) = div;
            *(glIterator++) = dotPosition;
            *(glIterator++) = div;
        }
    }

    // Axes
    vaGrid[1].resize((int)(2 + (DIVS_TIME * DIVS_SUB - 2) + (DIVS_VOLTAGE * DIVS_SUB - 2)) * 4);
    glIterator = vaGrid[1].begin();
    // Horizontal axis
    *(glIterator++) = -DIVS_TIME / 2;
    *(glIterator++) = 0;
    *(glIterator++) = DIVS_TIME / 2;
    *(glIterator++) = 0;
    // Vertical axis
    *(glIterator++) = 0;
    *(glIterator++) = -DIVS_VOLTAGE / 2;
    *(glIterator++) = 0;
    *(glIterator++) = DIVS_VOLTAGE / 2;
    // Subdiv lines on horizontal axis
    for (int line = 1; line < DIVS_TIME / 2 * DIVS_SUB; ++line) {
        float linePosition = (float)line / DIVS_SUB;
        *(glIterator++) = linePosition;
        *(glIterator++) = -0.05f;
        *(glIterator++) = linePosition;
        *(glIterator++) = 0.05f;
        *(glIterator++) = -linePosition;
        *(glIterator++) = -0.05f;
        *(glIterator++) = -linePosition;
        *(glIterator++) = 0.05f;
    }
    // Subdiv lines on vertical axis
    for (int line = 1; line < DIVS_VOLTAGE / 2 * DIVS_SUB; ++line) {
        float linePosition = (float)line / DIVS_SUB;
        *(glIterator++) = -0.05f;
        *(glIterator++) = linePosition;
        *(glIterator++) = 0.05f;
        *(glIterator++) = linePosition;
        *(glIterator++) = -0.05f;
        *(glIterator++) = -linePosition;
        *(glIterator++) = 0.05f;
        *(glIterator++) = -linePosition;
    }

    // Border
    vaGrid[2].resize(4 * 2);
    glIterator = vaGrid[2].begin();
    *(glIterator++) = -DIVS_TIME / 2;
    *(glIterator++) = -DIVS_VOLTAGE / 2;
    *(glIterator++) = DIVS_TIME / 2;
    *(glIterator++) = -DIVS_VOLTAGE / 2;
    *(glIterator++) = DIVS_TIME / 2;
    *(glIterator++) = DIVS_VOLTAGE / 2;
    *(glIterator++) = -DIVS_TIME / 2;
    *(glIterator++) = DIVS_VOLTAGE / 2;
}

const std::vector<GLfloat> &GlGenerator::channel(Dso::ChannelMode mode, ChannelID channel, unsigned index) const {
    return vaChannel[(unsigned)mode][channel][index];
}

const std::vector<GLfloat> &GlGenerator::grid(int a) const { return vaGrid[a]; }

bool GlGenerator::isReady() const { return ready; }

bool GlGenerator::generateGraphs(const DataAnalyzerResult *result, unsigned digitalPhosphorDepth,
                                 const DsoSettingsScope *scope, unsigned physicalChannels) {

    // Handle all digital phosphor related list manipulations
    for (Dso::ChannelMode mode: Dso::ChannelModeEnum) {
        DrawLinesWithHistoryPerChannel& d = vaChannel[(unsigned)mode];
        // Resize to the number of channels
        d.resize(scope->voltage.size());

        for (ChannelID channel = 0; channel < vaChannel[(unsigned)mode].size(); ++channel) {
            DrawLinesWithHistory& drawLinesHistory = d[channel];
            // Move the last list element to the front
            if (digitalPhosphorDepth > 1 && drawLinesHistory.size())
                drawLinesHistory.push_front(drawLinesHistory.back());

            // Resize lists for vector array to fit the digital phosphor depth
            drawLinesHistory.resize(digitalPhosphorDepth);
        }
    }

    ready = true;

    unsigned preTrigSamples=0;
    unsigned postTrigSamples=0;
    unsigned swTriggerStart=0;
    bool triggered = false;

    switch (scope->horizontal.format) {
    case Dso::GraphFormat::TY:
        std::tie(preTrigSamples, postTrigSamples, swTriggerStart) = SoftwareTrigger::computeTY(result, scope, physicalChannels);
        triggered = postTrigSamples > preTrigSamples;

        // Add graphs for channels
        for (Dso::ChannelMode mode: Dso::ChannelModeEnum) {
            DrawLinesWithHistoryPerChannel& dPerChannel = vaChannel[(unsigned)mode];
            for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
                DrawLinesWithHistory& withHistory = dPerChannel[channel];
                const SampleValues& samples = useSamplesOf(mode, channel, result, scope);

                // Check if this channel is used and available at the data analyzer
                if (samples.sample.empty()) {
                    // Delete all vector arrays
                    for (unsigned index = 0; index < digitalPhosphorDepth; ++index)
                        withHistory[index].clear();
                    continue;
                }
                // Check if the sample count has changed
                size_t sampleCount = samples.sample.size();
                if (sampleCount>500000) {
                    qWarning() << "Sample count too high!";
                    throw new std::runtime_error("Sample count too high!");
                }
                if (mode == Dso::ChannelMode::Voltage)
                    sampleCount -= (swTriggerStart - preTrigSamples);
                size_t neededSize = sampleCount * 2;

#if 0
                for(unsigned int index = 0; index < digitalPhosphorDepth; ++index) {
                    if(vaChannel[mode][channel][index].size() != neededSize)
                        vaChannel[mode][channel][index].clear(); // Something was changed, drop old traces
                }
#endif

                // Set size directly to avoid reallocations
                withHistory.front().resize(neededSize);

                // Iterator to data for direct access
                DrawLines::iterator glIterator = withHistory.front().begin();

                // What's the horizontal distance between sampling points?
                float horizontalFactor;
                if (mode == Dso::ChannelMode::Voltage)
                    horizontalFactor = (float)(samples.interval / scope->horizontal.timebase);
                else
                    horizontalFactor = (float)(samples.interval / scope->horizontal.frequencybase);

                // Fill vector array
                if (mode == Dso::ChannelMode::Voltage) {
                    std::vector<double>::const_iterator dataIterator = samples.sample.begin();
                    const float gain = (float) scope->gain(channel);
                    const float offset = (float) scope->voltage[channel].offset;
                    const float invert = scope->voltage[channel].inverted ? -1.0f : 1.0f;

                    std::advance(dataIterator, swTriggerStart - preTrigSamples);

                    for (unsigned int position = 0; position < sampleCount; ++position) {
                        *(glIterator++) = position * horizontalFactor - DIVS_TIME / 2;
                        *(glIterator++) = (float)*(dataIterator++) / gain * invert + offset;
                    }
                } else {
                    std::vector<double>::const_iterator dataIterator = samples.sample.begin();
                    const float magnitude = (float)scope->spectrum[channel].magnitude;
                    const float offset = (float)scope->spectrum[channel].offset;

                    for (unsigned int position = 0; position < sampleCount; ++position) {
                        *(glIterator++) = position * horizontalFactor - DIVS_TIME / 2;
                        *(glIterator++) = (float)*(dataIterator++) / magnitude + offset;
                    }
                }
            }
        }
        break;

    case Dso::GraphFormat::XY:
        triggered = true;
        for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
            // For even channel numbers check if this channel is used and this and the
            // following channel are available at the data analyzer
            if (channel % 2 == 0 && channel + 1 < scope->voltage.size() && scope->voltage[channel].used &&
                    result->data(channel) && !result->data(channel)->voltage.sample.empty() && result->data(channel + 1) &&
                    !result->data(channel + 1)->voltage.sample.empty()) {
                // Check if the sample count has changed
                const size_t sampleCount = std::min(result->data(channel)->voltage.sample.size(),
                                                  result->data(channel + 1)->voltage.sample.size());
                const size_t neededSize = sampleCount * 2;
                DrawLinesWithHistory& withHistory = vaChannel[(unsigned)Dso::ChannelMode::Voltage][(size_t)channel];
                for (unsigned index = 0; index < digitalPhosphorDepth; ++index) {
                    if (withHistory[index].size() != neededSize)
                        withHistory[index].clear(); // Something was changed, drop old traces
                }

                // Set size directly to avoid reallocations
                DrawLines& drawLines = withHistory.front();
                drawLines.resize(neededSize);

                // Iterator to data for direct access
                std::vector<GLfloat>::iterator glIterator = drawLines.begin();

                // Fill vector array
                unsigned int xChannel = channel;
                unsigned int yChannel = channel + 1;
                std::vector<double>::const_iterator xIterator = result->data(xChannel)->voltage.sample.begin();
                std::vector<double>::const_iterator yIterator = result->data(yChannel)->voltage.sample.begin();
                const double xGain = scope->gain(xChannel);
                const double yGain = scope->gain(yChannel);
                const double xOffset = scope->voltage[xChannel].offset;
                const double yOffset = scope->voltage[yChannel].offset;
                const double xInvert = scope->voltage[xChannel].inverted ? -1.0 : 1.0;
                const double yInvert = scope->voltage[yChannel].inverted ? -1.0 : 1.0;

                for (unsigned int position = 0; position < sampleCount; ++position) {
                    *(glIterator++) = (GLfloat)( *(xIterator++) / xGain * xInvert + xOffset);
                    *(glIterator++) = (GLfloat)( *(yIterator++) / yGain * yInvert + yOffset);
                }
            } else {
                // Delete all vector arrays
                for (unsigned index = 0; index < digitalPhosphorDepth; ++index)
                    vaChannel[(unsigned)Dso::ChannelMode::Voltage][channel][index].clear();
            }

            // Delete all spectrum graphs
            for (unsigned index = 0; index < digitalPhosphorDepth; ++index)
                vaChannel[(unsigned)Dso::ChannelMode::Spectrum][channel][index].clear();
        }
        break;
    }

    emit graphsGenerated();

    return triggered;
}
