// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <list>
#include <string>

#include <QString>

#include "definitions.h"
#include "states.h"
#include "utils/dataarray.h"

namespace Hantek {

class BulkCommand : public DataArray<uint8_t> {
protected:
    BulkCommand(unsigned size): DataArray<uint8_t>(size) {}
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetFilter                                        hantek/types.h
/// \brief The BULK::SETFILTER builder.
class BulkSetFilter : public BulkCommand {
  public:
    BulkSetFilter();
    BulkSetFilter(bool channel1, bool channel2, bool trigger);

    bool getChannel(unsigned int channel);
    void setChannel(unsigned int channel, bool filtered);
    bool getTrigger();
    void setTrigger(bool filtered);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetTriggerAndSamplerate                          hantek/types.h
/// \brief The BulkCode::SETTRIGGERANDSAMPLERATE builder.
class BulkSetTriggerAndSamplerate : public BulkCommand {
  public:
    BulkSetTriggerAndSamplerate();
    BulkSetTriggerAndSamplerate(uint16_t downsampler, uint32_t triggerPosition, uint8_t triggerSource = 0,
                                uint8_t recordLength = 0, uint8_t samplerateId = 0, bool downsamplingMode = true,
                                uint8_t usedChannels = 0, bool fastRate = false, uint8_t triggerSlope = 0);

    uint8_t getTriggerSource();
    void setTriggerSource(uint8_t value);
    uint8_t getRecordLength();
    void setRecordLength(uint8_t value);
    uint8_t getSamplerateId();
    void setSamplerateId(uint8_t value);
    bool getDownsamplingMode();
    void setDownsamplingMode(bool downsampling);
    uint8_t getUsedChannels();
    void setUsedChannels(uint8_t value);
    bool getFastRate();
    void setFastRate(bool fastRate);
    uint8_t getTriggerSlope();
    void setTriggerSlope(uint8_t slope);
    uint16_t getDownsampler();
    void setDownsampler(uint16_t downsampler);
    uint32_t getTriggerPosition();
    void setTriggerPosition(uint32_t position);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkForceTrigger                                     hantek/types.h
/// \brief The BulkCode::FORCETRIGGER builder.
class BulkForceTrigger : public BulkCommand {
  public:
    BulkForceTrigger();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkCaptureStart                                     hantek/types.h
/// \brief The BULK_CAPTURESTART builder.
class BulkCaptureStart : public BulkCommand {
  public:
    BulkCaptureStart();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkTriggerEnabled                                   hantek/types.h
/// \brief The BULK_TRIGGERENABLED builder.
class BulkTriggerEnabled : public BulkCommand {
  public:
    BulkTriggerEnabled();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkGetData                                          hantek/types.h
/// \brief The BulkCode::GETDATA builder.
class BulkGetData : public BulkCommand {
  public:
    BulkGetData();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkGetCaptureState                                  hantek/types.h
/// \brief The BulkCode::GETCAPTURESTATE builder.
class BulkGetCaptureState : public BulkCommand {
  public:
    BulkGetCaptureState();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkResponseGetCaptureState                          hantek/types.h
/// \brief The parser for the BulkCode::GETCAPTURESTATE response.
class BulkResponseGetCaptureState : public BulkCommand {
  public:
    BulkResponseGetCaptureState();

    CaptureState getCaptureState();
    unsigned int getTriggerPoint();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetGain                                          hantek/types.h
/// \brief The BulkCode::SETGAIN builder.
class BulkSetGain : public BulkCommand {
  public:
    BulkSetGain();
    BulkSetGain(uint8_t channel1, uint8_t channel2);

    uint8_t getGain(unsigned int channel);
    void setGain(unsigned int channel, uint8_t value);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetLogicalData                                   hantek/types.h
/// \brief The BulkCode::SETLOGICALDATA builder.
class BulkSetLogicalData : public BulkCommand {
  public:
    BulkSetLogicalData();
    BulkSetLogicalData(uint8_t data);

    uint8_t getData();
    void setData(uint8_t data);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkGetLogicalData                                   hantek/types.h
/// \brief The BulkCode::GETLOGICALDATA builder.
class BulkGetLogicalData : public BulkCommand {
  public:
    BulkGetLogicalData();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetChannels2250                                  hantek/types.h
/// \brief The DSO-2250 BULK_BSETFILTER builder.
class BulkSetChannels2250 : public BulkCommand {
  public:
    BulkSetChannels2250();
    BulkSetChannels2250(uint8_t usedChannels);

    uint8_t getUsedChannels();
    void setUsedChannels(uint8_t value);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetTrigger2250                                   hantek/types.h
/// \brief The DSO-2250 BulkCode::CSETTRIGGERORSAMPLERATE builder.
class BulkSetTrigger2250 : public BulkCommand {
  public:
    BulkSetTrigger2250();
    BulkSetTrigger2250(uint8_t triggerSource, uint8_t triggerSlope);

    uint8_t getTriggerSource();
    void setTriggerSource(uint8_t value);
    uint8_t getTriggerSlope();
    void setTriggerSlope(uint8_t slope);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetSamplerate5200                                hantek/types.h
/// \brief The DSO-5200/DSO-5200A BulkCode::CSETTRIGGERORSAMPLERATE builder.
class BulkSetSamplerate5200 : public BulkCommand {
  public:
    BulkSetSamplerate5200();
    BulkSetSamplerate5200(uint16_t samplerateSlow, uint8_t samplerateFast);

    uint8_t getSamplerateFast();
    void setSamplerateFast(uint8_t value);
    uint16_t getSamplerateSlow();
    void setSamplerateSlow(uint16_t samplerate);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetRecordLength2250                              hantek/types.h
/// \brief The DSO-2250 BulkCode::DSETBUFFER builder.
class BulkSetRecordLength2250 : public BulkCommand {
  public:
    BulkSetRecordLength2250();
    BulkSetRecordLength2250(uint8_t recordLength);

    uint8_t getRecordLength();
    void setRecordLength(uint8_t value);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetBuffer5200                                    hantek/types.h
/// \brief The DSO-5200/DSO-5200A BulkCode::DSETBUFFER builder.
class BulkSetBuffer5200 : public BulkCommand {
  public:
    BulkSetBuffer5200();
    BulkSetBuffer5200(uint16_t triggerPositionPre, uint16_t triggerPositionPost, DTriggerPositionUsed usedPre = DTriggerPositionUsed::DTRIGGERPOSITION_OFF,
                      DTriggerPositionUsed usedPost = DTriggerPositionUsed::DTRIGGERPOSITION_OFF, uint8_t recordLength = 0);

    uint16_t getTriggerPositionPre();
    void setTriggerPositionPre(uint16_t value);
    uint16_t getTriggerPositionPost();
    void setTriggerPositionPost(uint16_t value);
    uint8_t getUsedPre();
    void setUsedPre(DTriggerPositionUsed value);
    DTriggerPositionUsed getUsedPost();
    void setUsedPost(DTriggerPositionUsed value);
    uint8_t getRecordLength();
    void setRecordLength(uint8_t value);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetSamplerate2250                                hantek/types.h
/// \brief The DSO-2250 BulkCode::ESETTRIGGERORSAMPLERATE builder.
class BulkSetSamplerate2250 : public BulkCommand {
  public:
    BulkSetSamplerate2250();
    BulkSetSamplerate2250(bool fastRate, bool downsampling = false, uint16_t samplerate = 0);

    bool getFastRate();
    void setFastRate(bool fastRate);
    bool getDownsampling();
    void setDownsampling(bool downsampling);
    uint16_t getSamplerate();
    void setSamplerate(uint16_t samplerate);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetTrigger5200                                   hantek/types.h
/// \brief The DSO-5200/DSO-5200A BulkCode::ESETTRIGGERORSAMPLERATE builder.
class BulkSetTrigger5200 : public BulkCommand {
  public:
    BulkSetTrigger5200();
    BulkSetTrigger5200(uint8_t triggerSource, uint8_t usedChannels, bool fastRate = false, uint8_t triggerSlope = 0,
                       uint8_t triggerPulse = 0);

    uint8_t getTriggerSource();
    void setTriggerSource(uint8_t value);
    uint8_t getUsedChannels();
    void setUsedChannels(uint8_t value);
    bool getFastRate();
    void setFastRate(bool fastRate);
    uint8_t getTriggerSlope();
    void setTriggerSlope(uint8_t slope);
    bool getTriggerPulse();
    void setTriggerPulse(bool pulse);

  private:
    void init();
};

//////////////////////////////////////////////////////////////////////////////
/// \class BulkSetBuffer2250                                    hantek/types.h
/// \brief The DSO-2250 BulkCode::FSETBUFFER builder.
class BulkSetBuffer2250 : public BulkCommand {
  public:
    BulkSetBuffer2250();
    BulkSetBuffer2250(uint32_t triggerPositionPre, uint32_t triggerPositionPost);

    uint32_t getTriggerPositionPost();
    void setTriggerPositionPost(uint32_t value);
    uint32_t getTriggerPositionPre();
    void setTriggerPositionPre(uint32_t value);

  private:
    void init();
};
}
