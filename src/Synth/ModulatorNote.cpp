// TODO: header

#include "ModulatorNote.h"

#include <cmath>

#include "OscilGen.h"
#include "../Misc/Allocator.h"
#include "../Misc/Util.h"

namespace zyn {

void ModulatorNote::setup(const ModulatorParameters &param)
{
    FMSmp    = nullptr;
    VoiceOut = nullptr;

    FMVoice = param.PFMVoice;

    FMFreqEnvelope = nullptr;
    FMAmpEnvelope  = nullptr;
}

void ModulatorNote::setupDetune(const ModulatorParameters &voicePar, unsigned char globalDetune)
{
    if(voicePar.PFMDetuneType != 0)
        FMDetune = getdetune(
                voicePar.PFMDetuneType,
                voicePar.PFMCoarseDetune,
                voicePar.PFMDetune);
    else
        FMDetune = getdetune(
                globalDetune,
                voicePar.PFMCoarseDetune,
                voicePar.PFMDetune);
}

void ModulatorNote::setupVoiceMod(
    const ModulatorParameters &param, const ModulatorParameters &FMVoicePar,
        const SYNTH_T &synth, Allocator &memory,
        bool first_run,
        bool isSoundType, float oscilFreq, unsigned char Hrandgrouping,
        float voiceBaseFreq, float velocity, int unison_size)
{
  //  auto &param = pars.VoicePar[nvoice];
    auto &voice = *this; // TODO
    float FMVolume;

    if (!isSoundType)
        voice.FMEnabled = FMTYPE::NONE;
    else
        voice.FMEnabled = param.PFMEnabled;

    voice.FMFreqFixed  = param.PFMFixedFreq;

    //Triggers when a user enables modulation on a running voice
    if(!first_run && voice.FMEnabled != FMTYPE::NONE && voice.FMSmp == NULL && voice.FMVoice < 0) {
        param.FMSmp->newrandseed(prng());
        voice.FMSmp = memory.valloc<float>(synth.oscilsize + OSCIL_SMP_EXTRA_SAMPLES);
        memset(voice.FMSmp, 0, sizeof(float)*(synth.oscilsize + OSCIL_SMP_EXTRA_SAMPLES));

        if(!Hrandgrouping)
            FMVoicePar.FMSmp->newrandseed(prng());

        for(int k = 0; k < unison_size; ++k)
            voice.oscposhiFM[k] = (voice.oscposhi[k]
                    + FMVoicePar.FMSmp->get(
                        voice.FMSmp, oscilFreq))
                % synth.oscilsize;

        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
            voice.FMSmp[synth.oscilsize + i] = voice.FMSmp[i];
        int oscposhiFM_add =
            (int)((param.PFMoscilphase
                        - 64.0f) / 128.0f * synth.oscilsize
                    + synth.oscilsize * 4);
        for(int k = 0; k < unison_size; ++k) {
            voice.oscposhiFM[k] += oscposhiFM_add;
            voice.oscposhiFM[k] %= synth.oscilsize;
        }
    }


    //Compute the Voice's modulator volume (incl. damping)
    float fmvoldamp = powf(440.0f / voiceBaseFreq,
            param.PFMVolumeDamp / 64.0f - 1.0f);
    const float fmvolume_ = param.FMvolume / 100.0f;
    switch(voice.FMEnabled) {
        case FMTYPE::PHASE_MOD:
        case FMTYPE::PW_MOD:
            fmvoldamp = powf(440.0f / voiceBaseFreq,
                    param.PFMVolumeDamp / 64.0f);
            FMVolume = (expf(fmvolume_ * FM_AMP_MULTIPLIER) - 1.0f)
                * fmvoldamp * 4.0f;
            break;
        case FMTYPE::FREQ_MOD:
            FMVolume = (expf(fmvolume_ * FM_AMP_MULTIPLIER) - 1.0f)
                * fmvoldamp * 4.0f;
            break;
        default:
            if(fmvoldamp > 1.0f)
                fmvoldamp = 1.0f;
            FMVolume = fmvolume_ * fmvoldamp;
            break;
    }

    //Voice's modulator velocity sensing
    voice.FMVolume = FMVolume * VelF(velocity, param.PFMVelocityScaleFunction);





    //Compute the Voice's modulator volume (incl. damping)
    float fmvoldamp = powf(440.0f / getvoicebasefreq(nvoice),
                           pars.VoicePar[nvoice].PFMVolumeDamp / 64.0f
                           - 1.0f);

    switch(voice.FMEnabled) {
        case FMTYPE::PHASE_MOD:
        case FMTYPE::PW_MOD:
            fmvoldamp =
                powf(440.0f / getvoicebasefreq(
                         nvoice), pars.VoicePar[nvoice].PFMVolumeDamp
                     / 64.0f);
            FMVolume =
                (expf(pars.VoicePar[nvoice].FMvolume / 100.0f
                      * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
            break;
        case FMTYPE::FREQ_MOD:
            FMVolume =
                (expf(pars.VoicePar[nvoice].FMvolume / 100.0f
                      * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
            break;
        default:
            if(fmvoldamp > 1.0f)
                fmvoldamp = 1.0f;
            FMVolume =
                pars.VoicePar[nvoice].FMvolume
                / 100.0f * fmvoldamp;
            break;
    }

    //Voice's modulator velocity sensing
    voice.FMVolume = FMVolume *
        VelF(velocity,
             pars.VoicePar[nvoice].PFMVelocityScaleFunction);
}




}
