// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "hantekdsocontrol.h"

class Capturing : public QThread {
    Q_OBJECT
  public:
    Capturing( HantekDsoControl *hdc );

  private:
    void run() override;
    void capture();
    void getRealSamples();
    void getDemoSamples();
    void xferSamples();
    HantekDsoControl *hdc;
    unsigned channels = 0;
    double effectiveSamplerate = 0;
    bool realSlow = false;
    double samplerate = 0;
    unsigned oversampling = 0;
    unsigned rawSamplesize = 0;
    unsigned gainValue[ 2 ] = {0, 0}; // 1,2,5,10,..
    unsigned gainIndex[ 2 ] = {0, 0}; // index 0..7
    unsigned tag = 0;
    bool valid = true;
    bool rollMode = false;
    std::vector< unsigned char > data;
    std::vector< unsigned char > *dp = &data;
};
