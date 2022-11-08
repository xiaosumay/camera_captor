/**
 * This file is part of the vtn-player.
 * Copyright © Iflytek Co., Ltd.
 * All Rights Reserved.
 *
 * @author  zrmei
 * @date    2022-03-10
 *
 * @brief
 */
#ifndef VTN_PLAYER_RECORDER_H
#define VTN_PLAYER_RECORDER_H

#include <string>
#include <vector>

struct AudioListener
{
    virtual void onAudio(const char *data, int len) = 0;
    virtual int channelCount() const = 0;
    virtual int sampleSize() const = 0;
    virtual int framesPerBuffer() const = 0;
    virtual void onInit() = 0;
};

class AudioCaptor
{
public:
    AudioCaptor(const std::string &soundName, AudioListener *audioListener);
    ~AudioCaptor();

    int start();

    static std::vector<std::string> getDevices();

private:
    typedef void PaStream;
    PaStream *stream = nullptr;
};

#endif    //VTN_PLAYER_RECORDER_H
