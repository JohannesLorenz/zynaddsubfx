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
class ScratchString;

// TODO: check public vs protected vs private etc everywhere
class ModulatorNote
{
    /*
     * data
     */
    FMTYPE FMEnabled;

    unsigned char FMFreqFixed;

    //! Modulation voice, -1 if disabled
    int FMVoice;

    //! Voice Output used by other voices if we use this as modulator
    float *VoiceOut;

    //! Wave of the Voice
    float *FMSmp;

    smooth_float FMVolume;
    float FMDetune;  //in cents

    Envelope *FMFreqEnvelope;
    Envelope *FMAmpEnvelope;

    //! integer part (skip) of the Modullator
    unsigned int *oscposhiFM, *oscfreqhiFM;

    //! fractional part (skip) of the Modullator
    float *oscposloFM, *oscfreqloFM;

    float FMoldamplitude, FMnewamplitude;

    //used by Frequency Modulation (for integration)
    float *FMoldsmp;

public:
    /*
     * getters
     */
    float getFMDetune() const { return FMDetune; }
    int getFMVoice() const { return FMVoice; }
    FMTYPE getFMEnabled() const { return FMEnabled; }
    //float getFMvoicebasefreq(float voicebasefreq) const;
    float getFMOscilFreq(int nvoice, const OscilGen *FMSmp) const;

    /*
     * setup functions
     */
    // to be called at object construction
    void setup(const ModulatorParameters& param, Allocator &memory, int unison);
    // to be called at object construction and when UI params change (RT thread, noteout)
    void setupDetune(const ModulatorParameters& voicePar,
                     unsigned char globalDetuneType);
    void setFMVoice(const ModulatorParameters& voicePar);
    // to be called at object construction and when UI params change (RT thread, noteout)
    void setupVoiceMod(const ModulatorParameters &param, const ModulatorParameters &FMVoicePar, // TODO: pack into updateVoiceMod?
            const SYNTH_T &synth, Allocator& memory, bool first_run,
            bool isSoundType, float oscilFreq,
            unsigned char Hrandgrouping, int unison_size, const int *oscposhi);
    void setupVoiceMod2(const ModulatorParameters &param, float voiceBaseFreq,
            float velocity);
    void setupVoiceMod3(const ModulatorParameters &param,
            const SYNTH_T &synth, Allocator& memory, const Controller &ctl, WatchManager *wm, int nvoice,
            float basefreq, const ScratchString &pre);
    /*
     * misc functions
     */
    void initModulationPars(const ModulatorParameters& pars, const ModulatorParameters &FMVoicePar, const SYNTH_T &synth, const Controller& ctl, unsigned char Hrandgrouping);
    void computeCurrentParameters(const SYNTH_T &synth, const Controller &ctl, float voicefreq, int unison_size, float *unison_freq_rap);


private:
    void setfreqFM(const SYNTH_T &synth, float in_freq, int unison_size, float *unison_freq_rap);
public:
    /** Computes the Oscillator samples with mixing.
     * updates tmpwave_unison */
    void ComputeVoiceOscillatorMix(const SYNTH_T &synth, ModulatorNote& fmVoice, int unison_size, float **tmpwave_unison);
    /** Computes the Ring Modulated Oscillator. */
    void ComputeVoiceOscillatorRingModulation(const SYNTH_T &synth, ModulatorNote &fmVoice, int unison_size, float **tmpwave_unison);
    /** Computes the Frequency Modulated Oscillator.
     *  @param FMmode modulation type 0=Phase 1=Frequency */
    void ComputeVoiceOscillatorFrequencyModulation(// input params
            const SYNTH_T &synth, ModulatorNote& fmVoice, int unison_size, float **tmpwave_unison, const float *OscilSmp, int phase_offset,
            // in/out params
            int *oscposhi, int *oscfreqhi, float *oscposlo, float *oscfreqlo,
            // FM type
            FMTYPE FMmode);
    //  inline void ComputeVoiceOscillatorFrequencyModulation(int nvoice);
    /**TODO*/
    void ComputeVoiceOscillatorPitchModulation(int nvoice);

    void putSamplesIntoVoiceOut(const SYNTH_T &synth, float* tmpwavel, float* tmpwaver, bool stereo);
    void kill(Allocator &memory, const SYNTH_T &synth);
protected:
    void releasekey();
public:
    void killVoiceOut(Allocator &memory);
    void kill0(Allocator &memory);
    void allocVoiceOut(const SYNTH_T &synth, Allocator &memory);
    void disableFMVoice() { FMVoice = -1; }
    void initVoiceOut(const SYNTH_T &synth);
};

}

#endif // MODULATORNOTE_H
