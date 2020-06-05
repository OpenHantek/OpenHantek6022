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
    HantekDsoControl *hdc;
    unsigned channels = 0;
    double samplerate = 0;
    unsigned oversampling = 0;
    unsigned gainValue[ 2 ] = {0, 0}; // 1,2,5,10,..
    unsigned gainIndex[ 2 ] = {0, 0}; // index 0..7
    bool freeRun = false;
    unsigned tag = 0;
    std::vector< unsigned char > data;
};
