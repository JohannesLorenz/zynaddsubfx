#ifndef MODULATORNOTE_H
#define MODULATORNOTE_H

#include "globals.h"
#include "../Params/ModulatorParameters.h"

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
public:
    void setup();
};

}

#endif // MODULATORNOTE_H
