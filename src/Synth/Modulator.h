#ifndef MODULATOR_H
#define MODULATOR_H

#include <cstddef>
#include <vector>

#include "globals.h"

namespace zyn
{

enum FMTYPE {
    NONE, MIX, RING_MOD, PHASE_MOD, FREQ_MOD, PW_MOD
};


class Modulator
{
    const class SYNTH_T    &synth;

    /***********************************************************/
    /*                    VOICE PARAMETERS                     */
    /***********************************************************/

    struct VoiceCommon {

        /* preserved for phase mod PWM emulation. */
        int phase_offset;

        int FMVoice;

        // Voice Output used by other voices if use this as modullator
        float *VoiceOut;

        /* Wave of the Voice */
        float *FMSmp;

        /* Waveform of the Voice */
        float *OscilSmp;

        FMTYPE FMEnabled;

        /* if AntiAliasing is enabled */
        bool AAEnabled;
    };


    // TODO: use std::vector everywhere
    //fractional part (skip)
    float *oscposlo[NUM_VOICES], *oscfreqlo[NUM_VOICES];

    //integer part (skip)
    int *oscposhi[NUM_VOICES], *oscfreqhi[NUM_VOICES];
    //fractional part (skip) of the Modullator
    float *oscposloFM[NUM_VOICES], *oscfreqloFM[NUM_VOICES];
        //integer part (skip) of the Modullator
        unsigned int *oscposhiFM[NUM_VOICES], *oscfreqhiFM[NUM_VOICES];

        //used to compute and interpolate the amplitudes of voices and modullators
        float oldamplitude[NUM_VOICES],
              newamplitude[NUM_VOICES],
              FMoldamplitude[NUM_VOICES],
              FMnewamplitude[NUM_VOICES];

    //used by Frequency Modulation (for integration)
    float *FMoldsmp[NUM_VOICES];

        void ComputeVoiceOscillator_SincInterpolation(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon* NoteVoicePar);
        void ComputeVoiceOscillator_LinearInterpolation(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar);
        void ComputeVoiceOscillatorMix(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar);
        void ComputeVoiceOscillatorRingModulation(int nvoice, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar);
        void ComputeVoiceOscillatorFrequencyModulation(int nvoice,
                                                       int FMmode, std::size_t unison_size, float** tmpwave_unison, VoiceCommon *NoteVoicePar);
protected:
        void ComputeVoiceOscillator(int nvoice, std::size_t unison_size, float **tmpwave_unison, VoiceCommon *NoteVoicePar);
        Modulator(const class SYNTH_T    &synth, std::size_t num_voices);
};

}

#endif // MODULATOR_H
