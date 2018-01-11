// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "dsosamples.h"
#include "processor.h"

#include <memory>
#include <vector>

#include <QObject>

struct DsoSettingsScope;

/**
 * Manages all post processing processors. Register another processor with `registerProcessor(p)`.
 * All processors, in the order of insertion, will process the input data, given by `input(data)`.
 * The final result will be made available via the `processingFinished` signal.
 */
class PostProcessing : public QObject {
    Q_OBJECT
  public:
    PostProcessing(unsigned channelCount);
    /**
     * Adds a new processor that is called when a new input arrived. The order of the processors is
     * imporant. The first added processor will be called first. This class does not take ownership
     * of the processors.
     * @param processor
     */
    void registerProcessor(Processor *processor);


  private:
    /// A new `PPresult` is created for each new input. We need to know the channel size.
    const unsigned channelCount;
    /// The list of processors. Processors are not memory managed by this class.
    std::vector<Processor *> processors;
    ///
    std::unique_ptr<PPresult> currentData;
    static void convertData(const DSOsamples *source, PPresult *destination);
  public slots:
    /**
     * Start processing new data. The actual data may be processed in another thread if you have moved
     * this class object into another thread.
     * @param data
     */
    void input(const DSOsamples *data);
signals:
    void processingFinished(std::shared_ptr<PPresult> result);
};

Q_DECLARE_METATYPE(std::shared_ptr<PPresult>)
