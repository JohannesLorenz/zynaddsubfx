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

void ModulatorNote::setFMVoice(const ModulatorParameters &voicePar)
{
    FMVoice = voicePar.PFMVoice;
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









            /* Voice Modulation Parameters Init */
        if((vce.FMEnabled != FMTYPE::NONE) && (vce.FMVoice < 0)) {
            param.FMSmp->newrandseed(prng());
            vce.FMSmp = memory.valloc<float>(synth.oscilsize + OSCIL_SMP_EXTRA_SAMPLES);

            //Perform Anti-aliasing only on MIX or RING MODULATION

            const auto& FMVoicePar = pars.getFMVoicePar(nvoice);

            float tmp = 1.0f;
            if((FMVoicePar.FMSmp->Padaptiveharmonics != 0)
               || (vce.FMEnabled == FMTYPE::MIX)
               || (vce.FMEnabled == FMTYPE::RING_MOD))
                tmp = getFMvoicebasefreq(nvoice);

            if(!pars.GlobalPar.Hrandgrouping)
                FMVoicePar.FMSmp->newrandseed(prng());

            for(int k = 0; k < vce.unison_size; ++k)
                vce.oscposhiFM[k] = (vce.oscposhi[k]
                                         + FMVoicePar.FMSmp->get(
                                             vce.FMSmp, tmp))
                                        % synth.oscilsize;

            for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
                vce.FMSmp[synth.oscilsize + i] = vce.FMSmp[i];
            int oscposhiFM_add =
                (int)((param.PFMoscilphase
                       - 64.0f) / 128.0f * synth.oscilsize
                      + synth.oscilsize * 4);
            for(int k = 0; k < vce.unison_size; ++k) {
                vce.oscposhiFM[k] += oscposhiFM_add;
                vce.oscposhiFM[k] %= synth.oscilsize;
            }
        }

        if(param.PFMFreqEnvelopeEnabled)
            vce.FMFreqEnvelope = memory.alloc<Envelope>(*param.FMFreqEnvelope,
                    basefreq, synth.dt(), wm,
                    (pre+"VoicePar"+nvoice+"/FMFreqEnvelope/").c_str);

        vce.FMnewamplitude = vce.FMVolume * ctl.fmamp.relamp;

        if(param.PFMAmpEnvelopeEnabled) {
            vce.FMAmpEnvelope =
                memory.alloc<Envelope>(*param.FMAmpEnvelope,
                        basefreq, synth.dt(), wm,
                        (pre+"VoicePar"+nvoice+"/FMAmpEnvelope/").c_str);
            vce.FMnewamplitude *= vce.FMAmpEnvelope->envout_dB();
        }
}

void ModulatorNote::setupVoiceMod2(
    const ModulatorParameters &param, const ModulatorParameters &FMVoicePar,
        const SYNTH_T &synth, Allocator &memory,
        bool first_run,
        bool isSoundType, float oscilFreq, unsigned char Hrandgrouping,
        float voiceBaseFreq, float velocity, int unison_size)
{
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
}

void ModulatorNote::initModulationPars(const ModulatorParameters &pars, const ModulatorParameters &FMVoicePar, const SYNTH_T &synth, unsigned char Hrandgrouping)
{
    /* Voice Modulation Parameters Init */
    if((FMEnabled != FMTYPE::NONE)
       && (FMVoice < 0)) {
        pars.FMSmp->newrandseed(prng());

        //Perform Anti-aliasing only on MIX or RING MODULATION

        if(!Hrandgrouping)
            FMVoicePar.FMSmp->newrandseed(prng());

        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
            FMSmp[synth.oscilsize + i] = FMSmp[i];
    }

    vce.FMnewamplitude = NoteVoicePar[nvoice].FMVolume
                             * ctl.fmamp.relamp;

    if(pars.VoicePar[nvoice].PFMAmpEnvelopeEnabled
       && NoteVoicePar[nvoice].FMAmpEnvelope)
        vce.FMnewamplitude *=
            NoteVoicePar[nvoice].FMAmpEnvelope->envout_dB();
}




}
