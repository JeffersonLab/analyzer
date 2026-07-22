#ifndef WAVEFORMGENERATOR_H
#define WAVEFORMGENERATOR_H

#include <vector>
#include <random>
#include "Rtypes.h"
#include "RtypesCore.h"
#include "TMath.h"

struct DigitizerParams {
    Double_t sample_rate = 4.0;      // Sampling rate of the digitizer (250 MHz)
    Int_t num_samples = 256;         // #of samples in the sampling window
    Double_t pedestal = 100.0;       // Pedestal in ADC counts
    Double_t noise_sigma = 1.5;      // Electronic noise (mV)
    Double_t v_lsb = 1.0;            // Voltage resolution (mV per ADC count)
    Int_t max_adc = 4095;            // Maximum ADC value (12-bit)
    Int_t max_n_pulses = 4;          // Max # of pulses per window

    // Params needed for the initial algorithm and constant shape pulses
    Int_t init_ped_samples = 4;     // # of samples needed to calculate initial pedestal
    Int_t ped_sigma = 4;            // Pedestal variation in number of samples
};

// Structs for the waveform generator algorithm v2 - keep the safety values
struct WaveformParams {
    Double_t tau_r = 2.0;           // Rise time (ns)
    Double_t tau_f = 5.0;          // Decay time (ns)
    Double_t tts_sigma = 1.5;       // Transit time spread (ns)
    Double_t mean_gain = 10.0;       // Mean amplitude per PE (mV)
    Double_t gain_sigma = 1.0;      // Gain variation (mV)

    // After pulse parameters
    Double_t ap_prob = 0.03;         // After pulse generation probability
    Double_t ap_delay_mean = 80.0;   // Mean delay for afterpulse (ns)
    Double_t ap_delay_sigma = 5.0;   // Spread in afterpulse delay (ns)

    // Parameters needed for the initial algorithm
    Int_t pulse_width = 3;          // Pulse width in samples
    Int_t pulse_separation = 10;    // Pulse separation in samples
};

// Truth-level pulse info per hit, computed during waveform generation.
// One entry per hit group (not per PE). Values are converted to match
// the units of FADCStandaloneAnalyzer output for direct comparison.
struct TruthPulseInfo {
    std::vector<Double_t> pulseIntegral;   // Ped-subtracted integral in ADC counts (÷ v_lsb)
    std::vector<Double_t> pulseAmplitude;  // Peak in ADC counts (÷ v_lsb + pedestal)
    std::vector<Double_t> pulseTime;       // Half-max crossing in 1/64th-sample units
};

class waveformGenerator {
    public:
        waveformGenerator(); // Seed the random number generator. New seed everytime run.
        virtual ~waveformGenerator() = default;

        // NEW: Detector + Digitizer Setter
        void setDetectorParams(const WaveformParams& params) {fWf = params;}
        void setDigitizerParams(const DigitizerParams& params) {fDaq = params;}

        // Waveform generators
        // Preliminary generators
        std::vector<Int_t> noiseWaveform() const;
        std::vector<Int_t> singlePulseWaveform() const;
        std::vector<Int_t> multiplePulseWaveform() const;

        // PE based generators
        std::vector<Int_t> generateWaveform();
        std::vector<Int_t> squareWaveform();
        std::vector<Int_t> triangleWaveform();

        // Accessors for truth-level pulse info computed during generation
        const TruthPulseInfo& getTruthInfo() const { return fTruthInfo; }
        void clearTruthInfo() { fTruthInfo = {}; }
        
    private:
        TruthPulseInfo fTruthInfo;

        // Members for waveform generator v2
        DigitizerParams fDaq;
        WaveformParams fWf;

        // Per-instance RNG — thread-safe (no shared global state)
        mutable std::mt19937 fRng;

        // Helpers for waveform generator v2
        // Waveform templates
        Double_t spePulse(Double_t t, Double_t t0, Double_t amp) const;
        Double_t squarePulse(Double_t t, Double_t t0, Double_t amp) const;
        Double_t trianglePulse(Double_t t, Double_t t0, Double_t amp) const;

        // Hit-level PE grouping for structured truth extraction
        struct PEHit {
            Double_t hitTime;                   // Primary hit time (ns)
            std::vector<Double_t> peTimes;      // All PE times in this hit (incl. afterpulses)
        };
        std::vector<PEHit> generatePEHits() const;
};

#endif // WAVEFORMGENERATOR_H