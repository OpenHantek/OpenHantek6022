#include "postprocessing.h"

PostProcessing::PostProcessing(unsigned channelCount) : channelCount(channelCount) {
    qRegisterMetaType<std::shared_ptr<PPresult>>();
}

void PostProcessing::registerProcessor(Processor *processor) { processors.push_back(processor); }

void PostProcessing::convertData(const DSOsamples *source, PPresult *destination) {
    QReadLocker locker(&source->lock);

    for (ChannelID channel = 0; channel < source->data.size(); ++channel) {
        const std::vector<double> &rawChannelData = source->data.at(channel);

        if (rawChannelData.empty()) { continue; }

        DataChannel *const channelData = destination->modifyData(channel);
        channelData->voltage.interval = 1.0 / source->samplerate;
        channelData->voltage.sample = rawChannelData;
    }
}

void PostProcessing::input(const DSOsamples *data) {
    currentData.reset(new PPresult(channelCount));
    convertData(data, currentData.get());
    for (Processor *p : processors) p->process(currentData.get());
    std::shared_ptr<PPresult> res = std::move(currentData);
    emit processingFinished(res);
}
