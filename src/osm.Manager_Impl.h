﻿
#pragma once

#include "OpenSoundMixer.h"
#include "OpenSoundMixerInternal.h"
#include "osm.ReferenceObject.h"

#include <float.h>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace osm {
class Sound_Impl;
class Resampler;

class Manager_Impl : public Manager, public ReferenceObject {
private:
    struct SoundState {

		//! current sampling point in a buffer
        int32_t SamplePos;

		//! playing sound
        Sound_Impl* SoundPtr;

		//! whether is paused?
        bool IsPaused;

		//! volume
        float Volume;

        float FadeGradient;
        float FadeVolume;
        float TargetedFadeVolume;

        bool IsPlaybackSpeedEnabled = false;
        float PlaybackSpeed = 1.0;
        std::shared_ptr<Resampler> ResamplerPtr;

        float PanningPosition = 0.0;
    };

    int32_t m_stateID;
    int32_t m_current;

    std::map<int32_t, SoundState> m_soundStates;

    std::vector<Sample> m_tempSamples;
    std::vector<int32_t> m_tempIDs;

    std::vector<Sample> m_resampleBuf;

    std::recursive_mutex m_mutex;

protected:
    virtual bool InitializeInternal() = 0;
    virtual void FinalizeInternal() = 0;

    int32_t ReadSamples(Sample* samples, int32_t sampleCount);

    //! get mutex to control sound states
    std::recursive_mutex& GetMutex() { return m_mutex; }

public:
    Manager_Impl();
    virtual ~Manager_Impl();

    bool Initialize() override;

    void Finalize() override;

    Sound* CreateSound(const void* data, int32_t size, bool isDecompressed) override;

    int32_t Play(Sound* sound) override;

    bool IsPlaying(int32_t id) override;

    void StopAll() override;

    void Stop(int32_t id) override;

    void Pause(int32_t id) override;

    void Resume(int32_t id) override;

    void SetVolume(int32_t id, float volume) override;

    void FadeIn(int32_t id, float second) override;

    void FadeOut(int32_t id, float second) override;

    void Fade(int32_t id, float second, float targetedVolume) override;

    bool GetIsPlaybackSpeedEnabled(int32_t id) override;

    void SetIsPlaybackSpeedEnabled(int32_t id, bool isPlaybackSpeedEnabled) override;

    float GetPlaybackSpeed(int32_t id) override;

    void SetPlaybackSpeed(int32_t id, float playbackSpeed) override;

    float GetPanningPosition(int32_t id) override;

    void SetPanningPosition(int32_t id, float panningPosition) override;

    float GetPlaybackPercent(int32_t id) override;

public:
    virtual int GetRef() { return ReferenceObject::GetRef(); }
    virtual int AddRef() { return ReferenceObject::AddRef(); }
    virtual int Release() { return ReferenceObject::Release(); }
};
}  // namespace osm
