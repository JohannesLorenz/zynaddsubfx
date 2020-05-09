/*
  ZynAddSubFX - a software synthesizer

  ModulationParameters.h - Parameters for TODO
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

class SYNTH_T;
class FFTwrapper;
class Resonance;
class AbsTime;

struct ModulatorParameters
{
    /* Modulator Parameters (0=off,1=Mix,2=RM,3=PM,4=FM..) */
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

    // What external oscil should I use, -1 for internal OscilSmp&FMSmp
    short int PextFMoscil;

    // FM oscillator phase
    unsigned char PFMoscilphase;

    void enable(const SYNTH_T &synth, FFTwrapper *fft,
                const AbsTime *time);
    void defaults();
    void kill();

protected:
    void paste(ModulatorParameters &a);
};

#define MODULATOR_PARAMS_DIRECT \
    rOption(PFMEnabled, rShort("mode"), rOptions(none, mix, ring, phase, \
                frequency, pulse), rDefault(none), "Modulator mode"), \
    rParamI(PFMVoice,                   rShort("voice"), rDefault(-1), \
        "Modulator Oscillator Selection"), \
    rParamF(FMvolume,                   rShort("vol."),  rLinear(0.0, 100.0), \
        rDefault(70.0),                 "Modulator Magnitude"), \
    rParamZyn(PFMVolumeDamp,            rShort("damp."), rDefault(64), \
        "Modulator HF dampening"), \
    rParamZyn(PFMVelocityScaleFunction, rShort("sense"), rDefault(64), \
        "Modulator Velocity Function"), \
    /*nominally -8192..8191*/ \
    rParamI(PFMDetune,                  rShort("fine"), \
            rLinear(0, 16383), rDefault(8192), "Modulator Fine Detune"), \
    rParamI(PFMCoarseDetune,            rShort("coarse"), rDefault(0), \
            "Modulator Coarse Detune"), \
    rParamZyn(PFMDetuneType,            rShort("type"), \
              rOptions(L35cents, L10cents, E100cents, E1200cents), \
              rDefault(L35cents), \
              "Modulator Detune Magnitude"), \
    rToggle(PFMFixedFreq,               rShort("fixed"),  rDefault(false), \
            "Modulator Frequency Fixed"), \
    rToggle(PFMFreqEnvelopeEnabled,  rShort("enable"), rDefault(false), \
            "Modulator Frequency Envelope"), \
    rToggle(PFMAmpEnvelopeEnabled,   rShort("enable"), rDefault(false), \
            "Modulator Amplitude Envelope"), \
    rParamI(PextFMoscil,     rDefault(-1),     rShort("ext."), \
            rMap(min, -1), rMap(max, 16), "External FM Oscillator Selection"), \
    rParamZyn(PFMoscilphase, rShort("phase"),  rDefault(64), \
        "FM Oscillator Phase"), \
    {"PFMVolume::i", rShort("vol.") rLinear(0,127) \
        rDoc("Modulator Magnitude"), NULL, \
        [](const char *msg, RtData &d) \
        { \
            rObject *obj = (rObject *)d.obj; \
            if (!rtosc_narguments(msg)) \
                d.reply(d.loc, "i", (int)roundf(127.0f * obj->FMvolume \
                    / 100.0f)); \
            else \
                obj->FMvolume = 100.0f * rtosc_argument(msg, 0).i / 127.0f; \
        }}, \
    /*weird stuff for PCoarseDetune*/ \
    {"FMdetunevalue:", rMap(unit,cents) rDoc("Get modulator detune"), NULL, [](const char *, RtData &d) \
        { \
            rObject *obj = (rObject *)d.obj; \
            unsigned detuneType = \
            obj->PFMDetuneType == 0 ? *(obj->GlobalPDetuneType) \
            : obj->PFMDetuneType; \
            /*TODO check if this is accurate or if PCoarseDetune is utilized*/ \
            /*TODO do the same for the other engines*/ \
            d.reply(d.loc, "f", getdetune(detuneType, 0, obj->PFMDetune)); \
        }}, \
    {"FMoctave::c:i", rProp(parameter) rShort("octave") rLinear(-8,7) rDoc("Octave note offset for modulator"), NULL, \
        [](const char *msg, RtData &d) \
        { \
            rObject *obj = (rObject *)d.obj; \
            auto get_octave = [&obj](){ \
                int k=obj->PFMCoarseDetune/1024; \
                if (k>=8) k-=16; \
                return k; \
            }; \
            if(!rtosc_narguments(msg)) { \
                d.reply(d.loc, "i", get_octave()); \
            } else { \
                int k=(int) rtosc_argument(msg, 0).i; \
                if (k<0) k+=16; \
                obj->PFMCoarseDetune = k*1024 + obj->PFMCoarseDetune%1024; \
                d.broadcast(d.loc, "i", get_octave()); \
            } \
        }}, \
    {"FMcoarsedetune::c:i", rProp(parameter) rShort("coarse") rLinear(-64,63) \
        rDoc("Coarse note detune for modulator"), \
        NULL, [](const char *msg, RtData &d) \
        { \
            rObject *obj = (rObject *)d.obj; \
            auto get_coarse = [&obj](){ \
                int k=obj->PFMCoarseDetune%1024; \
                if (k>=512) k-=1024; \
                return k; \
            }; \
            if(!rtosc_narguments(msg)) { \
                d.reply(d.loc, "i", get_coarse()); \
            } else { \
                int k=(int) rtosc_argument(msg, 0).i; \
                if (k<0) k+=1024; \
                obj->PFMCoarseDetune = k + (obj->PFMCoarseDetune/1024)*1024; \
                d.broadcast(d.loc, "i", get_coarse()); \
            } \
        }}

#define MODULATOR_PARAMS_RECUR \
    rRecurp(FMFreqEnvelope, "Modulator Frequency Envelope"), \
    rRecurp(FMAmpEnvelope,  "Modulator Amplitude Envelope")

}

#endif // FMCOMMONPARAMETERS_H
