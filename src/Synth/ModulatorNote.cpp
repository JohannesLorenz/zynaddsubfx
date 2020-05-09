/*
  ZynAddSubFX - a software synthesizer

  ModulatorNote.cpp - Note for TODO
  Copyright (C) 2020-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "ModulatorNote.h"

#include <cmath>

#include "OscilGen.h"
#include "../Misc/Allocator.h"
#include "../Params/Controller.h"
#include "../Synth/Envelope.h"
#include "../Containers/ScratchString.h"
#include "../Misc/Util.h"

namespace zyn {

/*
 * Computes the frequency of an modullator oscillator
 */
void ModulatorNote::setfreqFM(const SYNTH_T& synth, float in_freq, int unison_size, float* unison_freq_rap)
{
    for(int k = 0; k < unison_size; ++k) {
        float freq  = fabsf(in_freq) * unison_freq_rap[k];
        float speed = freq * synth.oscilsize_f / synth.samplerate_f;
        if(speed > synth.samplerate_f)
            speed = synth.samplerate_f;

        F2I(speed, oscfreqhiFM[k]);
        oscfreqloFM[k] = speed - floorf(speed);
    }
}

void ModulatorNote::setup(const ModulatorParameters &param, Allocator& memory, int unison)
{
    FMSmp    = nullptr;
    VoiceOut = nullptr;

    FMVoice = param.PFMVoice;

    FMFreqEnvelope = nullptr;
    FMAmpEnvelope  = nullptr;

    oscfreqhiFM = memory.valloc<unsigned int>(unison);
    oscfreqloFM = memory.valloc<float>(unison);
    oscposhiFM  = memory.valloc<unsigned int>(unison);
    oscposloFM  = memory.valloc<float>(unison);

    for(int k = 0; k < unison; ++k) {
        oscposhiFM[k] = 0;
        oscposloFM[k] = 0.0f;
    }

    FMoldsmp = memory.valloc<float>(unison);
    for(int k = 0; k < unison; ++k)
        FMoldsmp[k] = 0.0f; //this is for FM (integration)
}

void ModulatorNote::allocAndInitVoiceOut(const SYNTH_T &synth, Allocator& memory)
{
    VoiceOut = memory.valloc<float>(synth.buffersize);
    memset(VoiceOut, 0, synth.bufferbytes);
}

void ModulatorNote::setupDetune(const ModulatorParameters &voicePar, unsigned char globalDetuneType)
{
    FMDetune = getdetune(
                         voicePar.PFMDetuneType
                             ? voicePar.PFMDetuneType
                             : globalDetuneType,
                         voicePar.PFMCoarseDetune, voicePar.PFMDetune);
}

void ModulatorNote::setFMVoice(const ModulatorParameters &voicePar)
{
    FMVoice = voicePar.PFMVoice;
}

void ModulatorNote::setupVoiceMod(const ModulatorParameters &param, const ModulatorParameters &FMVoicePar,
        const SYNTH_T &synth, Allocator &memory,
        bool first_run,
        bool isSoundType, float oscilFreq, bool Hrandgrouping,
        int unison_size, const int* oscposhi)
{
    FMEnabled = (isSoundType) ? param.PFMEnabled : FMTYPE::NONE;
    FMFreqFixed = param.PFMFixedFreq;

    //Triggers when a user enables modulation on a running voice
    if(!first_run && FMEnabled != FMTYPE::NONE && FMSmp == NULL && FMVoice < 0) {
        param.FMSmp->newrandseed(prng());
        FMSmp = memory.valloc<float>(synth.oscilsize + OSCIL_SMP_EXTRA_SAMPLES);
        memset(FMSmp, 0, sizeof(float)*(synth.oscilsize + OSCIL_SMP_EXTRA_SAMPLES));

        if(!Hrandgrouping)
            FMVoicePar.FMSmp->newrandseed(prng());

        for(int k = 0; k < unison_size; ++k)
            oscposhiFM[k] = (oscposhi[k]
                    + FMVoicePar.FMSmp->get(FMSmp, oscilFreq))
                % synth.oscilsize;

        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
            FMSmp[synth.oscilsize + i] = FMSmp[i];
        int oscposhiFM_add =
            (int)((param.PFMoscilphase- 64.0f) / 128.0f * synth.oscilsize
                    + synth.oscilsize * 4);
        for(int k = 0; k < unison_size; ++k) {
            oscposhiFM[k] += oscposhiFM_add;
            oscposhiFM[k] %= synth.oscilsize;
        }
    }
}

void ModulatorNote::setupVoiceFMVol(
    const ModulatorParameters &param,
        float voiceBaseFreq, float velocity)
{
    float FMVolumeTmp;

    //Compute the Voice's modulator volume (incl. damping)
    float fmvoldamp = powf(440.0f / voiceBaseFreq,
            param.PFMVolumeDamp / 64.0f - 1.0f);
    const float fmvolume_ = param.FMvolume / 100.0f;
    switch(FMEnabled) {
        case FMTYPE::PHASE_MOD:
        case FMTYPE::PW_MOD:
            fmvoldamp = powf(440.0f / voiceBaseFreq,
                    param.PFMVolumeDamp / 64.0f);
            FMVolumeTmp = (expf(fmvolume_ * FM_AMP_MULTIPLIER) - 1.0f)
                * fmvoldamp * 4.0f;
            break;
        case FMTYPE::FREQ_MOD:
            FMVolumeTmp = (expf(fmvolume_ * FM_AMP_MULTIPLIER) - 1.0f)
                * fmvoldamp * 4.0f;
            break;
        default:
            if(fmvoldamp > 1.0f)
                fmvoldamp = 1.0f;
            FMVolumeTmp = fmvolume_ * fmvoldamp;
            break;
    }

    //Voice's modulator velocity sensing
    FMVolume = FMVolumeTmp * VelF(velocity, param.PFMVelocityScaleFunction);
}

void ModulatorNote::setupVoiceMod3(const ModulatorParameters &param,
        const SYNTH_T &synth, Allocator& memory, const Controller &ctl, WatchManager *wm, int nvoice, float basefreq, const ScratchString& pre)
{
    if(param.PFMFreqEnvelopeEnabled)
        FMFreqEnvelope = memory.alloc<Envelope>(*param.FMFreqEnvelope,
                basefreq, synth.dt(), wm,
                (pre+"VoicePar"+nvoice+"/FMFreqEnvelope/").c_str);

    FMnewamplitude = FMVolume * ctl.fmamp.relamp;

    if(param.PFMAmpEnvelopeEnabled) {
        FMAmpEnvelope =
            memory.alloc<Envelope>(*param.FMAmpEnvelope,
                    basefreq, synth.dt(), wm,
                    (pre+"VoicePar"+nvoice+"/FMAmpEnvelope/").c_str);
        FMnewamplitude *= FMAmpEnvelope->envout_dB();
    }
}

void ModulatorNote::setupVoiceMod4(const ModulatorParameters &pars, const ModulatorParameters &FMVoicePar, const SYNTH_T &synth, const Controller &ctl, bool Hrandgrouping)
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

    FMnewamplitude = FMVolume * ctl.fmamp.relamp;

    if(pars.PFMAmpEnvelopeEnabled && FMAmpEnvelope)
        FMnewamplitude *= FMAmpEnvelope->envout_dB();
}

void ModulatorNote::computeCurrentParameters(const SYNTH_T& synth, const Controller &ctl, float voicefreq, int unison_size, float* unison_freq_rap)
{
    if(FMEnabled != FMTYPE::NONE) {
        float FMfreq, FMrelativepitch;
        FMrelativepitch = FMDetune / 100.0f;
        if(FMFreqEnvelope)
            FMrelativepitch += FMFreqEnvelope->envout() / 100.0f;
        if (FMFreqFixed)
            FMfreq = powf(2.0f, FMrelativepitch / 12.0f) * 440.0f;
        else
            FMfreq = powf(2.0f, FMrelativepitch / 12.0f) * voicefreq;
        setfreqFM(synth, FMfreq, unison_size, unison_freq_rap);

        FMoldamplitude = FMnewamplitude;
        FMnewamplitude = FMVolume * ctl.fmamp.relamp;
        if(FMAmpEnvelope)
            FMnewamplitude *= FMAmpEnvelope->envout_dB();
    }
}

/*
 * Computes the Oscillator (Mixing)
 */
void ModulatorNote::ComputeVoiceOscillatorMix(const SYNTH_T &synth, ModulatorNote& fmVoice, int unison_size, float** tmpwave_unison)
{
    ModulatorNote& vce = *this;
    if(vce.FMnewamplitude > 1.0f)
        vce.FMnewamplitude = 1.0f;
    if(vce.FMoldamplitude > 1.0f)
        vce.FMoldamplitude = 1.0f;

    if(vce.FMVoice >= 0) {
        //if I use VoiceOut[] as modullator
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(vce.FMoldamplitude,
                                            vce.FMnewamplitude,
                                            i,
                                            synth.buffersize);
                tw[i] = tw[i]
                    * (1.0f - amp) + amp * fmVoice.VoiceOut[i];
            }
        }
    }
    else
        for(int k = 0; k < unison_size; ++k) {
            int    poshiFM  = vce.oscposhiFM[k];
            float  posloFM  = vce.oscposloFM[k];
            int    freqhiFM = vce.oscfreqhiFM[k];
            float  freqloFM = vce.oscfreqloFM[k];
            float *tw = tmpwave_unison[k];

            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(vce.FMoldamplitude,
                                            vce.FMnewamplitude,
                                            i,
                                            synth.buffersize);
                tw[i] = tw[i] * (1.0f - amp) + amp
                        * (FMSmp[poshiFM] * (1 - posloFM)
                           + FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth.oscilsize - 1;
            }
            vce.oscposhiFM[k] = poshiFM;
            vce.oscposloFM[k] = posloFM;
        }
}

/*
 * Computes the Oscillator (Ring Modulation)
 */
void ModulatorNote::ComputeVoiceOscillatorRingModulation(const SYNTH_T &synth, ModulatorNote& fmVoice, int unison_size, float** tmpwave_unison)
{
    ModulatorNote& vce = *this;
    if(vce.FMnewamplitude > 1.0f)
        vce.FMnewamplitude = 1.0f;
    if(vce.FMoldamplitude > 1.0f)
        vce.FMoldamplitude = 1.0f;
    if(FMVoice >= 0)
        // if I use VoiceOut[] as modullator
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(vce.FMoldamplitude,
                                            vce.FMnewamplitude,
                                            i,
                                            synth.buffersize);
                int FMVoice = FMVoice;
                tw[i] *= (1.0f - amp) + amp * fmVoice.VoiceOut[i];
            }
        }
    else
        for(int k = 0; k < unison_size; ++k) {
            int    poshiFM  = vce.oscposhiFM[k];
            float  posloFM  = vce.oscposloFM[k];
            int    freqhiFM = vce.oscfreqhiFM[k];
            float  freqloFM = vce.oscfreqloFM[k];
            float *tw = tmpwave_unison[k];

            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(vce.FMoldamplitude,
                                            vce.FMnewamplitude,
                                            i,
                                            synth.buffersize);
                tw[i] *= (FMSmp[poshiFM] * (1.0f - posloFM)
                          + FMSmp[poshiFM + 1] * posloFM) * amp
                         + (1.0f - amp);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth.oscilsize - 1;
            }
            vce.oscposhiFM[k] = poshiFM;
            vce.oscposloFM[k] = posloFM;
        }
}

/*
 * Computes the Oscillator (Phase Modulation or Frequency Modulation)
 */
void ModulatorNote::ComputeVoiceOscillatorFrequencyModulation(const SYNTH_T &synth, ModulatorNote& fmVoice, int unison_size, float** tmpwave_unison, const float* OscilSmp,
                                                              int phase_offset, int* oscposhi, int* oscfreqhi, float *oscposlo, float *oscfreqlo, FMTYPE FMmode)
{
    if(FMVoice >= 0) {
        //if I use VoiceOut[] as modulator
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            const float *smps = fmVoice.VoiceOut;
            if (FMmode == FMTYPE::PW_MOD && (k & 1))
                for (int i = 0; i < synth.buffersize; ++i)
                    tw[i] = -smps[i];
            else
                memcpy(tw, smps, synth.bufferbytes);
        }
    } else {
        //Compute the modulator and store it in tmpwave_unison[][]
        for(int k = 0; k < unison_size; ++k) {
            int    poshiFM  = oscposhiFM[k];
            int    posloFM  = (int)(oscposloFM[k]  * (1<<24));
            int    freqhiFM = oscfreqhiFM[k];
            int    freqloFM = (int)(oscfreqloFM[k] * (1<<24));
            float *tw = tmpwave_unison[k];
            const float *smps = FMSmp;

            for(int i = 0; i < synth.buffersize; ++i) {
                tw[i] = (smps[poshiFM] * ((1<<24) - posloFM)
                         + smps[poshiFM + 1] * posloFM) / (1.0f*(1<<24));
                if (FMmode == FMTYPE::PW_MOD && (k & 1))
                    tw[i] = -tw[i];

                posloFM += freqloFM;
                if(posloFM >= (1<<24)) {
                    posloFM &= 0xffffff;//fmod(posloFM, 1.0f);
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth.oscilsize - 1;
            }
            oscposhiFM[k] = poshiFM;
            oscposloFM[k] = posloFM/((1<<24)*1.0f);
        }
    }
    // Amplitude interpolation
    if(ABOVE_AMPLITUDE_THRESHOLD(FMoldamplitude,
                                 FMnewamplitude)) {
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i)
                tw[i] *= INTERPOLATE_AMPLITUDE(FMoldamplitude,
                                               FMnewamplitude,
                                               i,
                                               synth.buffersize);
        }
    } else {
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i)
                tw[i] *= FMnewamplitude;
        }
    }


    //normalize: makes all sample-rates, oscil_sizes to produce same sound
    if(FMmode == FMTYPE::FREQ_MOD) { //Frequency modulation
        const float normalize = synth.oscilsize_f / 262144.0f * 44100.0f
                          / synth.samplerate_f;
        for(int k = 0; k < unison_size; ++k) {
            float *tw    = tmpwave_unison[k];
            float  fmold = FMoldsmp[k];
            for(int i = 0; i < synth.buffersize; ++i) {
                fmold = fmodf(fmold + tw[i] * normalize, synth.oscilsize);
                tw[i] = fmold;
            }
            FMoldsmp[k] = fmold;
        }
    }
    else {  //Phase or PWM modulation
        const float normalize = synth.oscilsize_f / 262144.0f;
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i)
                tw[i] *= normalize;
        }
    }

    //do the modulation
    for(int k = 0; k < unison_size; ++k) {
        const float *smps   = OscilSmp;
        float *tw     = tmpwave_unison[k];
        int    poshi  = oscposhi[k];
        int    poslo  = (int)(oscposlo[k] * (1<<24));
        int    freqhi = oscfreqhi[k];
        int    freqlo = (int)(oscfreqlo[k] * (1<<24));

        for(int i = 0; i < synth.buffersize; ++i) {
            int FMmodfreqhi = 0;
            F2I(tw[i], FMmodfreqhi);
            float FMmodfreqlo = tw[i]-FMmodfreqhi;//fmod(tw[i] /*+ 0.0000000001f*/, 1.0f);
            if(FMmodfreqhi < 0)
                FMmodfreqlo++;

            //carrier
            int carposhi = poshi + FMmodfreqhi;
            int carposlo = (int)(poslo + FMmodfreqlo);
            if (FMmode == FMTYPE::PW_MOD && (k & 1))
                carposhi += phase_offset;

            if(carposlo >= (1<<24)) {
                carposhi++;
                carposlo &= 0xffffff;//fmod(carposlo, 1.0f);
            }
            carposhi &= (synth.oscilsize - 1);

            tw[i] = (smps[carposhi] * ((1<<24) - carposlo)
                    + smps[carposhi + 1] * carposlo)/(1.0f*(1<<24));

            poslo += freqlo;
            if(poslo >= (1<<24)) {
                poslo &= 0xffffff;//fmod(poslo, 1.0f);
                poshi++;
            }

            poshi += freqhi;
            poshi &= synth.oscilsize - 1;
        }
        oscposhi[k] = poshi;
        oscposlo[k] = (poslo)/((1<<24)*1.0f);
    }
}

void ModulatorNote::putSamplesIntoVoiceOut(const SYNTH_T &synth, float *tmpwavel, float *tmpwaver, bool stereo)
{
    if(VoiceOut) {
        if(stereo)
            for(int i = 0; i < synth.buffersize; ++i)
                VoiceOut[i] = tmpwavel[i] + tmpwaver[i];
        else   //mono
            for(int i = 0; i < synth.buffersize; ++i)
                VoiceOut[i] = tmpwavel[i];
    }
}

void ModulatorNote::killMod(Allocator &memory, const SYNTH_T &synth)
{
    if(VoiceOut)
        memory.dealloc(VoiceOut);

    memory.devalloc(oscfreqhiFM);
    memory.devalloc(oscfreqloFM);
    memory.devalloc(oscposhiFM);
    memory.devalloc(oscposloFM);

    memory.devalloc(FMoldsmp);

    memory.dealloc(FMFreqEnvelope);
    memory.dealloc(FMAmpEnvelope);

    if((FMEnabled != FMTYPE::NONE) && (FMVoice < 0))
        memory.devalloc(FMSmp);

    if(VoiceOut)
        memset(VoiceOut, 0, synth.bufferbytes);
    //the buffer can't be safely deleted as it may be
    //an input to another voice
}

void ModulatorNote::releasekey()
{
    if(FMFreqEnvelope)
        FMFreqEnvelope->releasekey();
    if(FMAmpEnvelope)
        FMAmpEnvelope->releasekey();
}

}
