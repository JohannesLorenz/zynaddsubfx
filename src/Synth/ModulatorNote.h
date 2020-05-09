/*
  ZynAddSubFX - a software synthesizer

  ModulatorNote.h - Note for TODO
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
    float getFMOscilFreq(int nvoice, const OscilGen *FMSmp) const;

    /*
     * setup functions
     */
    //! "initialize" members
    //! to be called at object construction
    void setup(const ModulatorParameters& param, Allocator &memory, int unison);
    //! set FMDetune value
    //! to be called at object construction and when UI params change (RT thread, noteout)
    void setupDetune(const ModulatorParameters& voicePar,
                     unsigned char globalDetuneType);
    // only required for legatonote
    void setFMVoice(const ModulatorParameters& voicePar);
    // to be called at object construction and when UI params change (RT thread, noteout)
    void setupVoiceMod(
            const ModulatorParameters &param, const ModulatorParameters &FMVoicePar, // TODO: pack into updateVoiceMod?
            const SYNTH_T &synth, Allocator& memory, bool first_run,
            bool isSoundType, float oscilFreq,
            bool Hrandgrouping, int unison_size, const int *oscposhi);
    //! Compute the Voice's modulator volume (incl. damping)
    //! to be called at object construction and when UI params change (RT thread, noteout)
    void setupVoiceFMVol(const ModulatorParameters &param, float voiceBaseFreq,
            float velocity);
    //! To be called by CTOR only
    void setupVoiceMod3(const ModulatorParameters &param,
            const SYNTH_T &synth, Allocator& memory, const Controller &ctl, WatchManager *wm, int nvoice,
            float basefreq, const ScratchString &pre);
    //! Only for ADnote::legatonote
    void setupVoiceModForLegato(const ModulatorParameters& pars, const ModulatorParameters &FMVoicePar, const SYNTH_T &synth, const Controller& ctl, bool Hrandgrouping);

    /*
     * misc functions
     */
    //! To be called at beginning of noteout
    void computeCurrentParameters(const SYNTH_T &synth, const Controller &ctl, float voicefreq, int unison_size, const float *unison_freq_rap);

private:
    //! set oscfreqhi/loFM
    void setfreqFM(const SYNTH_T &synth, float in_freq, int unison_size, const float *unison_freq_rap);
public:
    //! Compute the Oscillator samples with mixing
    void ComputeVoiceOscillatorMix(const SYNTH_T &synth, ModulatorNote& fmVoice, int unison_size, float **tmpwave_unison);
    //! Compute the Ring Modulated Oscillator
    void ComputeVoiceOscillatorRingModulation(const SYNTH_T &synth, ModulatorNote &fmVoice, int unison_size, float **tmpwave_unison);
    //! Compute the Frequency Modulated Oscillator.
    //! @param FMmode modulation type
    void ComputeVoiceOscillatorFrequencyModulation(// input params
            const SYNTH_T &synth, ModulatorNote& fmVoice, int unison_size, float **tmpwave_unison, const float *OscilSmp, int phase_offset,
            // in/out params
            int *oscposhi, int *oscfreqhi, float *oscposlo, float *oscfreqlo,
            // FM type
            FMTYPE FMmode);

    //! Put the samples into VoiceOut
    //! To be called when you computed this voice and want to store it for the next voice
    //! (before applying "global" volume, because this is only used as a modulator)
    void putSamplesIntoVoiceOut(const SYNTH_T &synth, float* tmpwavel, float* tmpwaver, bool stereo);

    //! Cleanup, deallocation
    //! To be called by DTOR or when the note is already finished
    void killMod(Allocator &memory, const SYNTH_T &synth);
protected:
    //! Calls releasekey on the envelopes
    void releasekey();
public:
    //! Allocate and zero-out VoiceOut buffer
    //! Should be called only by CTOR in case this voice
    //! is used by another modulator
    void allocAndInitVoiceOut(const SYNTH_T &synth, Allocator &memory);
    //! Set modulation voice to "none"
    void disableFMVoice() { FMVoice = -1; }
};

}

#endif // MODULATORNOTE_H
