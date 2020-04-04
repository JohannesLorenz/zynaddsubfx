#include <cassert>
#include <cstring>
#include <cmath>

#include "Modulator.h"

#include "globals.h"

namespace zyn {

#define LENGTHOF(x) ((int)(sizeof(x)/sizeof(x[0])))

Modulator::Modulator(const SYNTH_T &synth)
 : synth(synth)
{

}

/*
 * Computes the Oscillator (Without Modulation) - LinearInterpolation
 */

/* As the code here is a bit odd due to optimization, here is what happens
 * First the current position and frequency are retrieved from the running
 * state. These are broken up into high and low portions to indicate how many
 * samples are skipped in one step and how many fractional samples are skipped.
 * Outside of this method the fractional samples are just handled with floating
 * point code, but that's a bit slower than it needs to be. In this code the low
 * portions are known to exist between 0.0 and 1.0 and it is known that they are
 * stored in single precision floating point IEEE numbers. This implies that
 * a maximum of 24 bits are significant. The below code does your standard
 * linear interpolation that you'll see throughout this codebase, but by
 * sticking to integers for tracking the overflow of the low portion, around 15%
 * of the execution time was shaved off in the ADnote test.
 */
inline void Modulator::ComputeVoiceOscillator_LinearInterpolation(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar)
{
    for(int k = 0; k < unison_size; ++k) {
        int    poshi  = oscposhi[nvoice][k];
        int    poslo  = oscposlo[nvoice][k] * (1<<24);
        int    freqhi = oscfreqhi[nvoice][k];
        int    freqlo = oscfreqlo[nvoice][k] * (1<<24);
        float *smps   = NoteVoicePar[nvoice].OscilSmp;
        float *tw     = tmpwave_unison[k];
        assert(oscfreqlo[nvoice][k] < 1.0f);
        for(int i = 0; i < synth.buffersize; ++i) {
            tw[i]  = (smps[poshi] * ((1<<24) - poslo) + smps[poshi + 1] * poslo)/(1.0f*(1<<24));
            poslo += freqlo;
            poshi += freqhi + (poslo>>24);
            poslo &= 0xffffff;
            poshi &= synth.oscilsize - 1;
        }
        oscposhi[nvoice][k] = poshi;
        oscposlo[nvoice][k] = poslo/(1.0f*(1<<24));
    }
}


/*
 * Computes the Oscillator (Without Modulation) - windowed sinc Interpolation
 */

/* As the code here is a bit odd due to optimization, here is what happens
 * First the current position and frequency are retrieved from the running
 * state. These are broken up into high and low portions to indicate how many
 * samples are skipped in one step and how many fractional samples are skipped.
 * Outside of this method the fractional samples are just handled with floating
 * point code, but that's a bit slower than it needs to be. In this code the low
 * portions are known to exist between 0.0 and 1.0 and it is known that they are
 * stored in single precision floating point IEEE numbers. This implies that
 * a maximum of 24 bits are significant. The below code does your standard
 * linear interpolation that you'll see throughout this codebase, but by
 * sticking to integers for tracking the overflow of the low portion, around 15%
 * of the execution time was shaved off in the ADnote test.
 */
inline void Modulator::ComputeVoiceOscillator_SincInterpolation(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar)
{

    // windowed sinc kernel factor Fs*0.3, rejection 80dB
    const float_t kernel[] = {
        0.0010596256917418426,
        0.004273442181254887,
        0.0035466063043375785,
        -0.014555483937137638,
        -0.04789321342588484,
        -0.050800020978553066,
        0.04679847159974432,
        0.2610646708018185,
        0.4964802251145513,
        0.6000513532962539,
        0.4964802251145513,
        0.2610646708018185,
        0.04679847159974432,
        -0.050800020978553066,
        -0.04789321342588484,
        -0.014555483937137638,
        0.0035466063043375785,
        0.004273442181254887,
        0.0010596256917418426
        };



    for(int k = 0; k < unison_size; ++k) {
        int    poshi  = oscposhi[nvoice][k];
        int    poslo  = oscposlo[nvoice][k] * (1<<24);
        int    freqhi = oscfreqhi[nvoice][k];
        int    freqlo = oscfreqlo[nvoice][k] * (1<<24);
        int    ovsmpfreqhi = oscfreqhi[nvoice][k] / 2;
        int    ovsmpfreqlo = (oscfreqlo[nvoice][k] / 2) * (1<<24);

        int    ovsmpposlo;
        int    ovsmpposhi;
        int    uflow;
        float *smps   = NoteVoicePar[nvoice].OscilSmp;
        float *tw     = tmpwave_unison[k];
        assert(oscfreqlo[nvoice][k] < 1.0f);
        float out = 0;

        for(int i = 0; i < synth.buffersize; ++i) {
            ovsmpposlo  = poslo - (LENGTHOF(kernel)-1)/2 * ovsmpfreqlo;
            uflow = ovsmpposlo>>24;
            ovsmpposhi  = poshi - (LENGTHOF(kernel)-1)/2 * ovsmpfreqhi - ((0x00 - uflow) & 0xff);
            ovsmpposlo &= 0xffffff;
            ovsmpposhi &= synth.oscilsize - 1;
            out = 0;
            for (int l = 0; l<LENGTHOF(kernel); l++) {
                out += kernel[l] * (
                    smps[ovsmpposhi]     * ((1<<24) - ovsmpposlo) +
                    smps[ovsmpposhi + 1] * ovsmpposlo)/(1.0f*(1<<24));
                // advance to next sample
                ovsmpposlo += ovsmpfreqlo;
                ovsmpposhi += ovsmpfreqhi + (ovsmpposlo>>24); // add the 24-bit overflow
                ovsmpposlo &= 0xffffff;
                ovsmpposhi &= synth.oscilsize - 1;

            }

            // advance to next sample
            poslo += freqlo;
            poshi += freqhi + (poslo>>24);
            poslo &= 0xffffff;
            poshi &= synth.oscilsize - 1;

            tw[i] = out;

        }
        oscposhi[nvoice][k] = poshi;
        oscposlo[nvoice][k] = poslo/(1.0f*(1<<24));
    }
}

/*
 * Computes the Oscillator (Mixing)
 */
inline void Modulator::ComputeVoiceOscillatorMix(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar)
{
    ComputeVoiceOscillator_LinearInterpolation(nvoice, unison_size, tmpwave_unison, NoteVoicePar);
    if(FMnewamplitude[nvoice] > 1.0f)
        FMnewamplitude[nvoice] = 1.0f;
    if(FMoldamplitude[nvoice] > 1.0f)
        FMoldamplitude[nvoice] = 1.0f;

    if(NoteVoicePar[nvoice].FMVoice >= 0) {
        //if I use VoiceOut[] as modullator
        int FMVoice = NoteVoicePar[nvoice].FMVoice;
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth.buffersize);
                tw[i] = tw[i]
                    * (1.0f - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    }
    else
        for(int k = 0; k < unison_size; ++k) {
            int    poshiFM  = oscposhiFM[nvoice][k];
            float  posloFM  = oscposloFM[nvoice][k];
            int    freqhiFM = oscfreqhiFM[nvoice][k];
            float  freqloFM = oscfreqloFM[nvoice][k];
            float *tw = tmpwave_unison[k];

            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth.buffersize);
                tw[i] = tw[i] * (1.0f - amp) + amp
                        * (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1 - posloFM)
                           + NoteVoicePar[nvoice].FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth.oscilsize - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
}

/*
 * Computes the Oscillator (Ring Modulation)
 */
inline void Modulator::ComputeVoiceOscillatorRingModulation(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar)
{
    ComputeVoiceOscillator_LinearInterpolation(nvoice, unison_size, tmpwave_unison, NoteVoicePar);
    if(FMnewamplitude[nvoice] > 1.0f)
        FMnewamplitude[nvoice] = 1.0f;
    if(FMoldamplitude[nvoice] > 1.0f)
        FMoldamplitude[nvoice] = 1.0f;
    if(NoteVoicePar[nvoice].FMVoice >= 0)
        // if I use VoiceOut[] as modullator
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth.buffersize);
                int FMVoice = NoteVoicePar[nvoice].FMVoice;
                tw[i] *= (1.0f - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    else
        for(int k = 0; k < unison_size; ++k) {
            int    poshiFM  = oscposhiFM[nvoice][k];
            float  posloFM  = oscposloFM[nvoice][k];
            int    freqhiFM = oscfreqhiFM[nvoice][k];
            float  freqloFM = oscfreqloFM[nvoice][k];
            float *tw = tmpwave_unison[k];

            for(int i = 0; i < synth.buffersize; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth.buffersize);
                tw[i] *= (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1.0f - posloFM)
                          + NoteVoicePar[nvoice].FMSmp[poshiFM
                                                       + 1] * posloFM) * amp
                         + (1.0f - amp);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth.oscilsize - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
}

/*
 * Computes the Oscillator (Phase Modulation or Frequency Modulation)
 */
inline void Modulator::ComputeVoiceOscillatorFrequencyModulation(int nvoice,
                                                              int FMmode, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar)
{
    if(NoteVoicePar[nvoice].FMVoice >= 0) {
        //if I use VoiceOut[] as modulator
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            const float *smps = NoteVoicePar[NoteVoicePar[nvoice].FMVoice].VoiceOut;
            if (FMmode == PW_MOD && (k & 1))
                for (int i = 0; i < synth.buffersize; ++i)
                    tw[i] = -smps[i];
            else
                memcpy(tw, smps, synth.bufferbytes);
        }
    } else {
        //Compute the modulator and store it in tmpwave_unison[][]
        for(int k = 0; k < unison_size; ++k) {
            int    poshiFM  = oscposhiFM[nvoice][k];
            int    posloFM  = oscposloFM[nvoice][k]  * (1<<24);
            int    freqhiFM = oscfreqhiFM[nvoice][k];
            int    freqloFM = oscfreqloFM[nvoice][k] * (1<<24);
            float *tw = tmpwave_unison[k];
            const float *smps = NoteVoicePar[nvoice].FMSmp;

            for(int i = 0; i < synth.buffersize; ++i) {
                tw[i] = (smps[poshiFM] * ((1<<24) - posloFM)
                         + smps[poshiFM + 1] * posloFM) / (1.0f*(1<<24));
                if (FMmode == PW_MOD && (k & 1))
                    tw[i] = -tw[i];

                posloFM += freqloFM;
                if(posloFM >= (1<<24)) {
                    posloFM &= 0xffffff;//fmod(posloFM, 1.0f);
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth.oscilsize - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM/((1<<24)*1.0f);
        }
    }
    // Amplitude interpolation
    if(ABOVE_AMPLITUDE_THRESHOLD(FMoldamplitude[nvoice],
                                 FMnewamplitude[nvoice])) {
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i)
                tw[i] *= INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                               FMnewamplitude[nvoice],
                                               i,
                                               synth.buffersize);
        }
    } else {
        for(int k = 0; k < unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth.buffersize; ++i)
                tw[i] *= FMnewamplitude[nvoice];
        }
    }


    //normalize: makes all sample-rates, oscil_sizes to produce same sound
    if(FMmode == FREQ_MOD) { //Frequency modulation
        const float normalize = synth.oscilsize_f / 262144.0f * 44100.0f
                          / synth.samplerate_f;
        for(int k = 0; k < unison_size; ++k) {
            float *tw    = tmpwave_unison[k];
            float  fmold = FMoldsmp[nvoice][k];
            for(int i = 0; i < synth.buffersize; ++i) {
                fmold = fmod(fmold + tw[i] * normalize, synth.oscilsize);
                tw[i] = fmold;
            }
            FMoldsmp[nvoice][k] = fmold;
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
        float *smps   = NoteVoicePar[nvoice].OscilSmp;
        float *tw     = tmpwave_unison[k];
        int    poshi  = oscposhi[nvoice][k];
        int    poslo  = oscposlo[nvoice][k] * (1<<24);
        int    freqhi = oscfreqhi[nvoice][k];
        int    freqlo = oscfreqlo[nvoice][k] * (1<<24);

        for(int i = 0; i < synth.buffersize; ++i) {
            int FMmodfreqhi = 0;
            F2I(tw[i], FMmodfreqhi);
            float FMmodfreqlo = tw[i]-FMmodfreqhi;//fmod(tw[i] /*+ 0.0000000001f*/, 1.0f);
            if(FMmodfreqhi < 0)
                FMmodfreqlo++;

            //carrier
            int carposhi = poshi + FMmodfreqhi;
            int carposlo = poslo + FMmodfreqlo;
            if (FMmode == PW_MOD && (k & 1))
                carposhi += NoteVoicePar[nvoice].phase_offset;

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
        oscposhi[nvoice][k] = poshi;
        oscposlo[nvoice][k] = (poslo)/((1<<24)*1.0f);
    }
}

void Modulator::ComputeVoiceOscillator(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar)
{
    switch(NoteVoicePar[nvoice].FMEnabled) {
        case MIX:
            ComputeVoiceOscillatorMix(nvoice, unison_size, tmpwave_unison, NoteVoicePar);
            break;
        case RING_MOD:
            ComputeVoiceOscillatorRingModulation(nvoice, unison_size, tmpwave_unison, NoteVoicePar);
            break;
        case FREQ_MOD:
        case PHASE_MOD:
        case PW_MOD:
            ComputeVoiceOscillatorFrequencyModulation(nvoice, NoteVoicePar[nvoice].FMEnabled,
                                                      unison_size, tmpwave_unison, NoteVoicePar);
            break;
        default:
            if(NoteVoicePar[nvoice].AAEnabled) ComputeVoiceOscillator_SincInterpolation(nvoice, unison_size, tmpwave_unison, NoteVoicePar);
            else ComputeVoiceOscillator_LinearInterpolation(nvoice, unison_size, tmpwave_unison, NoteVoicePar);
            //if (config.cfg.Interpolation) ComputeVoiceOscillator_CubicInterpolation(nvoice);
    }
}

}

