// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "dsosamples.h"
#include "processor.h"

#include <memory>
#include <vector>

#include <QDebug>
#include <QObject>
#include <QThread>

/**
 * Manages all post processing processors. Register another processor with `registerProcessor(p)`.
 * All processors, in the order of insertion, will process the input data, given by `input(data)`.
 * The final result will be made available via the `processingFinished` signal.
 */
class PostProcessing : public QObject {
    Q_OBJECT

  public:
    explicit PostProcessing( ChannelID channelCount, int verboseLevel = 0 );
    /**
     * Adds a new processor that is called when a new input arrived. The order of the processors is
     * imporant. The first added processor will be called first. This class does not take ownership
     * of the processors.
     * @param processor
     */
    void registerProcessor( Processor *processor );
    void stop() { processing = false; }


  private:
    /// A new `PPresult` is created for each new input. We need to know the channel size.
    const unsigned channelCount;
    /// The list of processors. Processors are not memory managed by this class.
    std::vector< Processor * > processors;
    ///
    std::unique_ptr< PPresult > currentData;
    static void convertData( const DSOsamples *source, PPresult *destination );
    bool processing = true;
    int verboseLevel = 0;

  public slots:
    /**
     * Start processing new data. The actual data may be processed in another thread if you have moved
     * this class object into another thread.
     * @param data
     */
    void input( const DSOsamples *data );

  signals:
    void processingFinished( std::shared_ptr< PPresult > result );
};

Q_DECLARE_METATYPE( std::shared_ptr< PPresult > )
