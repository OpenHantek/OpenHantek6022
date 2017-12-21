// SPDX-License-Identifier: GPL-2.0+

#include <QMutex>

#include "glgenerator.h"
#include "utils/printutils.h"
#include "settings.h"

GlGenerator::GlGenerator(DsoSettingsScope *scope, DsoSettingsView *view) : settings(scope), view(view) {
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

const std::vector<GLfloat> &GlGenerator::channel(int mode, unsigned channel, unsigned index) const {
    return vaChannel[mode][channel][index];
}

const std::vector<GLfloat> &GlGenerator::grid(int a) const { return vaGrid[a]; }

bool GlGenerator::isReady() const { return ready; }

void GlGenerator::generateGraphs(const DataAnalyzerResult *result) {

    int digitalPhosphorDepth = view->digitalPhosphorDepth;

    // Handle all digital phosphor related list manipulations
    for (int mode = Dso::CHANNELMODE_VOLTAGE; mode < Dso::CHANNELMODE_COUNT; ++mode) {
        // Adapt the number of graphs
        vaChannel[mode].resize(settings->voltage.size());

        for (unsigned int channel = 0; channel < vaChannel[mode].size(); ++channel) {
            // Move the last list element to the front
            vaChannel[mode][channel].push_front(std::vector<GLfloat>());

            // Resize lists for vector array to fit the digital phosphor depth
            vaChannel[mode][channel].resize(digitalPhosphorDepth);
        }
    }

    ready = true;

    unsigned int preTrigSamples = 0;
    unsigned int postTrigSamples = 0;
    switch (settings->horizontal.format) {
    case Dso::GRAPHFORMAT_TY: {
        unsigned int swTriggerStart = 0;
        // check trigger point for software trigger
        if (settings->trigger.mode == Dso::TRIGGERMODE_SOFTWARE && settings->trigger.source <= 1) {
            unsigned int channel = settings->trigger.source;
            if (settings->voltage[channel].used && result->data(channel) &&
                !result->data(channel)->voltage.sample.empty()) {
                double value;
                double level = settings->voltage[channel].trigger;
                unsigned int sampleCount = result->data(channel)->voltage.sample.size();
                double timeDisplay = settings->horizontal.timebase * 10;
                double samplesDisplay = timeDisplay * settings->horizontal.samplerate;
                if (samplesDisplay >= sampleCount) {
                    // For sure not enough samples to adjust for jitter.
                    // Following options exist:
                    //    1: Decrease sample rate
                    //    2: Change trigger mode to auto
                    //    3: Ignore samples
                    // For now #3 is chosen
                    timestampDebug(QString("Too few samples to make a steady "
                                           "picture. Decrease sample rate"));
                    return;
                }
                preTrigSamples = (settings->trigger.position * samplesDisplay);
                postTrigSamples = sampleCount - (samplesDisplay - preTrigSamples);

                if (settings->trigger.slope == Dso::SLOPE_POSITIVE) {
                    double prev = INT_MAX;
                    for (unsigned int i = preTrigSamples; i < postTrigSamples; i++) {
                        value = result->data(channel)->voltage.sample[i];
                        if (value > level && prev <= level) {
                            int rising = 0;
                            for (unsigned int k = i + 1; k < i + 11 && k < sampleCount; k++) {
                                if (result->data(channel)->voltage.sample[k] >= value) { rising++; }
                            }
                            if (rising > 7) {
                                swTriggerStart = i;
                                break;
                            }
                        }
                        prev = value;
                    }
                } else if (settings->trigger.slope == Dso::SLOPE_NEGATIVE) {
                    double prev = INT_MIN;
                    for (unsigned int i = preTrigSamples; i < postTrigSamples; i++) {
                        value = result->data(channel)->voltage.sample[i];
                        if (value < level && prev >= level) {
                            int falling = 0;
                            for (unsigned int k = i + 1; k < i + 11 && k < sampleCount; k++) {
                                if (result->data(channel)->voltage.sample[k] < value) { falling++; }
                            }
                            if (falling > 7) {
                                swTriggerStart = i;
                                break;
                            }
                        }
                        prev = value;
                    }
                }
            }
            if (swTriggerStart == 0) {
                timestampDebug(QString("Trigger not asserted. Data ignored"));
                return;
            }
        }

        // Add graphs for channels
        for (int mode = Dso::CHANNELMODE_VOLTAGE; mode < Dso::CHANNELMODE_COUNT; ++mode) {
            for (int channel = 0; channel < (int)settings->voltage.size(); ++channel) {
                // Check if this channel is used and available at the data analyzer
                if (((mode == Dso::CHANNELMODE_VOLTAGE) ? settings->voltage[channel].used
                                                        : settings->spectrum[channel].used) &&
                    result->data(channel) && !result->data(channel)->voltage.sample.empty()) {
                    // Check if the sample count has changed
                    size_t sampleCount = (mode == Dso::CHANNELMODE_VOLTAGE)
                                             ? result->data(channel)->voltage.sample.size()
                                             : result->data(channel)->spectrum.sample.size();
                    if (mode == Dso::CHANNELMODE_VOLTAGE) sampleCount -= (swTriggerStart - preTrigSamples);
                    size_t neededSize = sampleCount * 2;

#if 0
                    for(unsigned int index = 0; index < digitalPhosphorDepth; ++index) {
                        if(vaChannel[mode][channel][index].size() != neededSize)
                            vaChannel[mode][channel][index].clear(); // Something was changed, drop old traces
                    }
#endif

                    // Set size directly to avoid reallocations
                    vaChannel[mode][(size_t)channel].front().resize(neededSize);

                    // Iterator to data for direct access
                    std::vector<GLfloat>::iterator glIterator = vaChannel[mode][(size_t)channel].front().begin();

                    // What's the horizontal distance between sampling points?
                    double horizontalFactor;
                    if (mode == Dso::CHANNELMODE_VOLTAGE)
                        horizontalFactor = result->data(channel)->voltage.interval / settings->horizontal.timebase;
                    else
                        horizontalFactor =
                            result->data(channel)->spectrum.interval / settings->horizontal.frequencybase;

                    // Fill vector array
                    if (mode == Dso::CHANNELMODE_VOLTAGE) {
                        std::vector<double>::const_iterator dataIterator =
                            result->data(channel)->voltage.sample.begin();
                        const double gain = settings->voltage[channel].gain;
                        const double offset = settings->voltage[channel].offset;
                        const double invert = settings->voltage[channel].inverted ? -1.0 : 1.0;

                        std::advance(dataIterator, swTriggerStart - preTrigSamples);

                        for (unsigned int position = 0; position < sampleCount; ++position) {
                            *(glIterator++) = position * horizontalFactor - DIVS_TIME / 2;
                            *(glIterator++) = *(dataIterator++) / gain * invert + offset;
                        }
                    } else {
                        std::vector<double>::const_iterator dataIterator =
                            result->data(channel)->spectrum.sample.begin();
                        const double magnitude = settings->spectrum[channel].magnitude;
                        const double offset = settings->spectrum[channel].offset;

                        for (unsigned int position = 0; position < sampleCount; ++position) {
                            *(glIterator++) = position * horizontalFactor - DIVS_TIME / 2;
                            *(glIterator++) = *(dataIterator++) / magnitude + offset;
                        }
                    }
                } else {
                    // Delete all vector arrays
                    for (unsigned index = 0; index < (unsigned)digitalPhosphorDepth; ++index)
                        vaChannel[mode][channel][index].clear();
                }
            }
        }
    } break;

    case Dso::GRAPHFORMAT_XY:
        for (int channel = 0; channel < settings->voltage.size(); ++channel) {
            // For even channel numbers check if this channel is used and this and the
            // following channel are available at the data analyzer
            if (channel % 2 == 0 && channel + 1 < settings->voltage.size() && settings->voltage[channel].used &&
                result->data(channel) && !result->data(channel)->voltage.sample.empty() && result->data(channel + 1) &&
                !result->data(channel + 1)->voltage.sample.empty()) {
                // Check if the sample count has changed
                const unsigned sampleCount = qMin(result->data(channel)->voltage.sample.size(),
                                                  result->data(channel + 1)->voltage.sample.size());
                const unsigned neededSize = sampleCount * 2;
                for (unsigned index = 0; index < (unsigned)digitalPhosphorDepth; ++index) {
                    if (vaChannel[Dso::CHANNELMODE_VOLTAGE][(size_t)channel][index].size() != neededSize)
                        vaChannel[Dso::CHANNELMODE_VOLTAGE][(size_t)channel][index]
                            .clear(); // Something was changed, drop old traces
                }

                // Set size directly to avoid reallocations
                vaChannel[Dso::CHANNELMODE_VOLTAGE][(size_t)channel].front().resize(neededSize);

                // Iterator to data for direct access
                std::vector<GLfloat>::iterator glIterator =
                    vaChannel[Dso::CHANNELMODE_VOLTAGE][channel].front().begin();

                // Fill vector array
                unsigned int xChannel = channel;
                unsigned int yChannel = channel + 1;
                std::vector<double>::const_iterator xIterator = result->data(xChannel)->voltage.sample.begin();
                std::vector<double>::const_iterator yIterator = result->data(yChannel)->voltage.sample.begin();
                const double xGain = settings->voltage[xChannel].gain;
                const double yGain = settings->voltage[yChannel].gain;
                const double xOffset = settings->voltage[xChannel].offset;
                const double yOffset = settings->voltage[yChannel].offset;
                const double xInvert = settings->voltage[xChannel].inverted ? -1.0 : 1.0;
                const double yInvert = settings->voltage[yChannel].inverted ? -1.0 : 1.0;

                for (unsigned int position = 0; position < sampleCount; ++position) {
                    *(glIterator++) = *(xIterator++) / xGain * xInvert + xOffset;
                    *(glIterator++) = *(yIterator++) / yGain * yInvert + yOffset;
                }
            } else {
                // Delete all vector arrays
                for (unsigned int index = 0; index < (unsigned)digitalPhosphorDepth; ++index)
                    vaChannel[Dso::CHANNELMODE_VOLTAGE][(size_t)channel][index].clear();
            }

            // Delete all spectrum graphs
            for (unsigned int index = 0; index < (unsigned)digitalPhosphorDepth; ++index)
                vaChannel[Dso::CHANNELMODE_SPECTRUM][(size_t)channel][index].clear();
        }
        break;

    default:
        break;
    }

    emit graphsGenerated();
}
