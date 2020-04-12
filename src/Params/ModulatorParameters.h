#ifndef FMCOMMONPARAMETERS_H
#define FMCOMMONPARAMETERS_H

/****************************
*   MODULATOR PARAMETERS    *
****************************/

namespace zyn {

class EnvelopeParams;
class OscilGen;

enum class FMTYPE {
    NONE, MIX, RING_MOD, PHASE_MOD, FREQ_MOD, PW_MOD
};

struct ModulatorParameters
{
    /* Modulator Parameters (0=off,1=Mix,2=RM,3=PM,4=FM.. */
    FMTYPE PFMEnabled;

    /* Voice that I use as modulator instead of FMSmp.
       It is -1 if I use FMSmp(default).
       It maynot be equal or bigger than current voice */
    short int PFMVoice;

    /* Modulator oscillator */
    OscilGen *FMSmp;

    /* Modulator Volume */
    float FMvolume;

    /* Modulator damping at higher frequencies */
    unsigned char PFMVolumeDamp;

    /* Modulator Velocity Sensing */
    unsigned char PFMVelocityScaleFunction;

    /* Fine Detune of the Modulator */
    unsigned short int PFMDetune;

    /* Coarse Detune of the Modulator */
    unsigned short int PFMCoarseDetune;

    /* The detune type */
    unsigned char PFMDetuneType;

    /* FM base freq fixed at 440Hz */
    unsigned char PFMFixedFreq;

    /* Frequency Envelope of the Modulator */
    unsigned char   PFMFreqEnvelopeEnabled;
    EnvelopeParams *FMFreqEnvelope;

    /* Frequency Envelope of the Modulator */
    unsigned char   PFMAmpEnvelopeEnabled;
    EnvelopeParams *FMAmpEnvelope;
};

}

#endif // FMCOMMONPARAMETERS_H
