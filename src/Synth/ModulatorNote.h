#ifndef MODULATORNOTE_H
#define MODULATORNOTE_H

#include "globals.h"
#include "../Params/ModulatorParameters.h"

//Globals

/**FM amplitude tune*/
#define FM_AMP_MULTIPLIER 14.71280603f

#define OSCIL_SMP_EXTRA_SAMPLES 5

namespace zyn {

class Envelope;

// TODO: check public vs protected vs private etc everywhere
class ModulatorNote
{
    FMTYPE FMEnabled;

    unsigned char FMFreqFixed;

    int FMVoice;

    // Voice Output used by other voices if use this as modullator
    float *VoiceOut;

    /* Wave of the Voice */
    float *FMSmp;

    smooth_float FMVolume;
    float FMDetune;  //in cents

    Envelope *FMFreqEnvelope;
    Envelope *FMAmpEnvelope;

    //integer part (skip) of the Modullator
    unsigned int *oscposhiFM, *oscfreqhiFM;

public:
    void setup(const ModulatorParameters& param);
    void setupDetune(const ModulatorParameters& voicePar,
                     unsigned char globalDetune);
    void setupVoiceMod(const ModulatorParameters &param, const ModulatorParameters &FMVoicePar,
            const SYNTH_T &synth, Allocator& memory, bool first_run,
            bool isSoundType, float oscilFreq,
            unsigned char Hrandgrouping, float voiceBaseFreq,
            float velocity, int unison_size);
};

}

#endif // MODULATORNOTE_H
