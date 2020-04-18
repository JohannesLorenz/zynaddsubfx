// TODO: header

#include <complex>

#include "../Synth/OscilGen.h"

#include "ModulatorParameters.h"
#include "EnvelopeParams.h"

namespace zyn {

void ModulatorParameters::enable(const zyn::SYNTH_T &synth,
                                 zyn::FFTwrapper *fft,
                                 const zyn::AbsTime *time)
{
    FMSmp    = new OscilGen(synth, fft, NULL);

    FMFreqEnvelope = new EnvelopeParams(0, 0, time);
    FMFreqEnvelope->init(ad_voice_fm_freq);
    FMAmpEnvelope = new EnvelopeParams(64, 1, time);
    FMAmpEnvelope->init(ad_voice_fm_amp);
}

void ModulatorParameters::defaults()
{
    PextFMoscil   = -1;
    PFMoscilphase = 64;

    PFMEnabled                = FMTYPE::NONE;
    PFMFixedFreq              = false;

    //I use the internal oscillator (-1)
    PFMVoice = -1;

    FMvolume       = 70.0;
    PFMVolumeDamp   = 64;
    PFMDetune       = 8192;
    PFMCoarseDetune = 0;
    PFMDetuneType   = 0;
    PFMFreqEnvelopeEnabled   = 0;
    PFMAmpEnvelopeEnabled    = 0;
    PFMVelocityScaleFunction = 64;

    FMSmp->defaults();

    FMFreqEnvelope->defaults();
    FMAmpEnvelope->defaults();
}

void ModulatorParameters::kill()
{
    delete FMSmp;

    delete FMFreqEnvelope;
    delete FMAmpEnvelope;
}

#define copy(x) this->x = a.x
#define RCopy(x) this->x->paste(*a.x)
void ModulatorParameters::paste(ModulatorParameters &a)
{
    copy(PextFMoscil);
    copy(PFMoscilphase);

    copy(PFMEnabled);
    copy(PFMFixedFreq);

    copy(PFMVoice);
    copy(FMvolume);
    copy(PFMVolumeDamp);
    copy(PFMVelocityScaleFunction);

    copy(PFMAmpEnvelopeEnabled);

    RCopy(FMAmpEnvelope);

    copy(PFMDetune);
    copy(PFMCoarseDetune);
    copy(PFMDetuneType);
    copy(PFMFreqEnvelopeEnabled);


    RCopy(FMFreqEnvelope);

    RCopy(FMSmp);
}

#undef RCopy
#undef copy

}

