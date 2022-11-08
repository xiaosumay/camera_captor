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
#include "recorder.h"
#include "portaudio/portaudio.h"

#include "common.h"

#include <stdexcept>
using namespace std;

Q_LOGGING_CATEGORY(AudioCaptorLog, "aiui.AudioCaptor")

static int portaudio_cb(const void *input,
                        void *output,
                        unsigned long frameCount,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    AudioListener *audioListener = (AudioListener *)(userData);

    audioListener->onAudio((const char *)input, frameCount * audioListener->sampleSize()
                                                    * audioListener->channelCount());

    //WATCH_OUT(qCDebug(AudioCaptorLog), "portaudio_cb", 40);

    return paContinue;
}

AudioCaptor::AudioCaptor(const std::string &soundName, AudioListener *audioListener)
{
    int ret = -1;
    PaError err;
    do {
        err = Pa_Initialize();
        if (err != paNoError) {
            ret = -1;
            break;
        }

        PaStreamParameters inputParameters;

        int numDevices = Pa_GetDeviceCount();
        if (numDevices < 0) {
            ret = -2;
            break;
        }

        inputParameters.device = Pa_GetDefaultInputDevice();

        std::string name;

        for (int i = 0; i < numDevices && !soundName.empty(); i++) {
            const PaDeviceInfo *di = Pa_GetDeviceInfo(i);

            if (di->maxInputChannels == 0                        // 输出设备
                || di->maxInputChannels > 20                     //模拟设备
                || Pa_GetHostApiInfo(di->hostApi)->type != paMME //MME协议
            )
                continue;

            name = di->name;

            if (((!soundName.empty() && name.find(soundName) != string::npos) ||
#if defined(_WIN32)
                 name.find("Source/Sink") != string::npos ||    //
#elif defined(__linux__)
                 name.find("Bothlent UAC Dongle") != string::npos ||
#endif
                 name.find("AIUI-USB-MIC") != string::npos) &&
                di->maxInputChannels >= audioListener->channelCount()) {
                inputParameters.device = i;
                inputParameters.suggestedLatency = di->defaultLowInputLatency;
                break;
            }
        }

        if (inputParameters.device == paNoDevice) {
            ret = -3;
            break;
        }

        inputParameters.channelCount = audioListener->channelCount();
        inputParameters.sampleFormat = audioListener->sampleSize() == 2 ? paInt16 : paInt32;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_IsFormatSupported(&inputParameters, NULL, 16000);

        if (err != paNoError) {
            break;
        }

        err = Pa_OpenStream(&stream,
                            &inputParameters,
                            NULL,
                            16000,
                            audioListener->framesPerBuffer(),
                            paClipOff,
                            portaudio_cb,
                            audioListener);

        if (err != paNoError) {
            ret = -4;
            break;
        }

        audioListener->onInit();
        ret = 0;
    } while (false);

    if (ret != 0) {
        throw std::invalid_argument(std::to_string(ret));
    }
}

AudioCaptor::~AudioCaptor()
{
    Pa_AbortStream(stream);
    Pa_CloseStream(stream);

    stream = nullptr;
    Pa_Terminate();
}

int AudioCaptor::start()
{
    if (!stream)
        return -1;

    PaError err = Pa_StartStream(stream);

    return err;
}

std::vector<string> AudioCaptor::getDevices()
{
    int ret = 0;
    PaError err;
    std::vector<string> results;

    do {
        err = Pa_Initialize();
        if (err != paNoError) {
            ret = -1;
            break;
        }

        PaStreamParameters inputParameters;

        int numDevices = Pa_GetDeviceCount();
        if (numDevices < 0) {
            ret = -2;
            break;
        }

        inputParameters.device = Pa_GetDefaultInputDevice();

        for (int i = 0; i < numDevices; i++) {
            const PaDeviceInfo *di = Pa_GetDeviceInfo(i);

            if (di->maxInputChannels == 0                        // 输出设备
                || di->maxInputChannels > 20                     //模拟设备
                || Pa_GetHostApiInfo(di->hostApi)->type != paMME //MME协议
            )
                continue;

            results.emplace_back(std::string(di->name));
        }

    } while (false);

    if (err == paNoError)
        Pa_Terminate();

    return results;
}
