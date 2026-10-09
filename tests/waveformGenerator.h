#ifndef WAVEFORMGENERATOR_H
#define WAVEFORMGENERATOR_H

#include "Rtypes.h"
#include "RtypesCore.h"
#include "TMath.h"
#include <random>
#include <vector>

#include "../Podd/FadcStandaloneAnalyzer.h"

namespace WaveformUtil {
// Same math as the private spePulse, but now publicly accessible
// Exponentially modified Gaussian (EMG) pulse shape
Double_t spePulse(Double_t t, Double_t t0, Double_t sigma, Double_t tau);
} // namespace WaveformUtil

struct DigitizerParams {
  Double_t sample_rate = 4.0;  // Sampling rate of the digitizer (250 MHz)
  Int_t num_samples = 256;     // #of samples in the sampling window
  Double_t pedestal = 100.0;   // Pedestal in ADC counts
  Double_t noise_sigma = 0.50; // Electronic noise (mV)
  Double_t v_lsb = 1.0;        // Voltage resolution (mV per ADC count)
  Int_t max_adc = 4095;        // Maximum ADC value (12-bit)
  Int_t max_n_pulses = 4;      // Max # of pulses per window
  Int_t tdc_subsamples = 64;   // Number of time subsamples per FADC sample

  // Params needed for the initial algorithm and constant shape pulses
  Int_t init_ped_samples =
      4;               // # of samples needed to calculate initial pedestal
  Int_t ped_sigma = 4; // Pedestal variation in number of samples
};

// Structs for the waveform generator algorithm v2 - keep the safety values
struct WaveformParams {
  Double_t sigma_g = 1.5;    // Gaussian width for EMG pulse shape (ns)
  Double_t tau = 5.0;        // Exponential decay constant for EMG (ns)
  Double_t tts_sigma = 1.5;  // Transit time spread (ns)
  Double_t mean_gain = 10.0; // Mean amplitude per PE (mV)
  Double_t gain_sigma = 2.5; // Gain variation (mV)
  // Double_t pes_per_pulse = 10;   // Number of PEs per pulse

  // After pulse parameters
  Double_t ap_prob = 0.03;       // After pulse generation probability
  Double_t ap_delay_mean = 80.0; // Mean delay for afterpulse (ns)
  Double_t ap_delay_sigma = 5.0; // Spread in afterpulse delay (ns)

  // Secondary pulse parameters (small echo after primary)
  Bool_t sp_enable = false;         // Enable secondary pulse generation
  Double_t sp_delay = 10.0;         // Delay after primary pulse (ns)
  Double_t sp_amplitude_frac = 0.1; // Fraction of primary PE count
  Double_t sp_delay_sigma = 0.0;    // Jitter on delay (ns), 0 = fixed

  // Parameters needed for the initial algorithm
  Int_t pulse_width = 10;      // Pulse width in samples
  Int_t pulse_separation = 15; // Pulse separation in samples
};

// Truth-level pulse info per hit, computed during waveform generation.
// One entry per hit group (not per PE). Values are converted to match
// the units of FADCStandaloneAnalyzer output for direct comparison.
struct TruthPulseInfo {
  std::vector<Double_t>
      pulseIntegral; // Ped-subtracted integral in ADC counts (÷ v_lsb)
  std::vector<Double_t>
      pulseAmplitude;              // Peak in ADC counts (÷ v_lsb + pedestal)
  std::vector<Double_t> pulseTime; // Half-max crossing in 1/64th-sample units
};

// Pulse shape enumerator
enum class PulseShape { SPE, Square, Triangle };

class waveformGenerator {
public:
  waveformGenerator(); // Seed the random number generator. New seed everytime
                       // run.
  virtual ~waveformGenerator() = default;

  // NEW: Detector + Digitizer Setter
  void setDetectorParams(const WaveformParams &params) { fWf = params; }
  void setDigitizerParams(const DigitizerParams &params) { fDaq = params; }

  // Waveform generators
  // Preliminary generators
  std::vector<Int_t> noiseWaveform() const;
  std::vector<Int_t> singlePulseWaveform() const;
  std::vector<Int_t> multiplePulseWaveform() const;

  // PE based generator - pass the enum to the function, default is the SPE
  // pulse shape
  std::vector<Int_t> generateWaveform(PulseShape shape = PulseShape::SPE);

  // Accessors for truth-level pulse info computed during generation
  const TruthPulseInfo &getTruthInfo() const { return fTruthInfo; }
  void clearTruthInfo() { fTruthInfo = {}; }

private:
  TruthPulseInfo fTruthInfo;
  DigitizerParams fDaq;
  WaveformParams fWf;

  // Per-instance RNG — thread-safe (no shared global state)
  mutable std::mt19937 fRng;

  // Helpers for waveform generator v2 - waveform templates
  Double_t spePulse(Double_t t, Double_t t0, Double_t amp) const;
  Double_t squarePulse(Double_t t, Double_t t0, Double_t amp) const;
  Double_t trianglePulse(Double_t t, Double_t t0, Double_t amp) const;

  // Hit-level PE grouping for structured truth extraction
  struct PEHit {
    Double_t hitTime; // Primary hit time (ns)
    std::vector<Double_t>
        peTimes; // All PE times in this hit (incl. afterpulses)
  };
  std::vector<PEHit> generatePEHits() const;
};

#endif // WAVEFORMGENERATOR_H