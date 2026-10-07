#include "FadcStandaloneAnalyzer.h"
#include "RtypesCore.h"

#include <Math/Factory.h>
#include <Math/Functor.h>
#include <Math/Minimizer.h>
#include <TROOT.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

#if defined(DEBUG) || defined(_DEBUG) || defined(FADC_DEBUG)
#define FADC_DEBUG_LOG(msg)                                                    \
  do {                                                                         \
    std::cerr << msg;                                                          \
  } while (0)
#else
#define FADC_DEBUG_LOG(msg)                                                    \
  do {                                                                         \
  } while (0)
#endif

FadcStandaloneAnalyzer::FadcStandaloneAnalyzer() {
  fConfig = {/*safety config parameters*/
             4, 50, 2, 5, 2, 3};

  fCalib = {/*safety calibration parameters*/
            0.2441, 4.0 / 50.0};
}

FADCPulse FadcStandaloneAnalyzer::Analyze(const std::vector<Double_t> &samples,
                                          Double_t gain, Double_t tcal) const {
  const Int_t fNSamples = static_cast<Int_t>(samples.size());
  FADCPulse data;

  // Guard: nothing to do
  if (fNSamples <= 0 || fConfig.fNPedestalSamples <= 0)
    return data;

  data.fSampPulseInt.assign(fConfig.fMaxNPulses, 0.0);
  data.fSampPulseAmp.assign(fConfig.fMaxNPulses, 0.0);
  data.fSampPulseTime.assign(fConfig.fMaxNPulses, 0.0);
  data.fSampPulseTOT.assign(fConfig.fMaxNPulses, 0.0);
  data.fSampPulsePileup.assign(fConfig.fMaxNPulses, 0);
  data.fSampPulseIntPedSub.assign(fConfig.fMaxNPulses, 0.0);
  data.fSampPulseIntMOLLERRaw.assign(fConfig.fMaxNPulses, 0.0);
  data.fSampPulseIntMOLLERVal.assign(fConfig.fMaxNPulses, 0.0);

  auto GetIntegral = [&](Int_t lo, Int_t hi) {
    Double_t sum = 0.0;
    lo = TMath::Max(lo, 0);
    hi = TMath::Min(hi, fNSamples - 1);
    for (Int_t i = lo; i <= hi; ++i)
      sum += samples[i];
    return sum;
  };

  // GetSample returns the pedestal-subtracted value of sample i
  auto GetSample = [&](Int_t i) {
    return samples[i] - data.fSampPed / fConfig.fNPedestalSamples;
  };

  data.fHasMulti = kTRUE;
  data.fSampPed = GetIntegral(0, fConfig.fNPedestalSamples - 1);

  Int_t NS = fConfig.fNPedestalSamples - 1;
  data.fNSampPulses = 0;

  if (fConfig.fSampThreshold == 0) {
    data.fSampPulseInt[data.fNSampPulses] =
        GetIntegral(TMath::Max(NS - fConfig.fNSB, 0),
                    TMath::Min((NS + fConfig.fNSA - 1), fNSamples - 1));
    data.fNPeakSamples = TMath::Min((NS + fConfig.fNSA - 1), fNSamples - 1) -
                         TMath::Max(NS - fConfig.fNSB, 0) + 1;
    data.fPeakPedestalRatio =
        1.0 * data.fNPeakSamples / fConfig.fNPedestalSamples;
    data.fSampPulseIntPedSub[data.fNSampPulses] =
        data.fSampPulseInt[data.fNSampPulses] -
        data.fSampPed * data.fPeakPedestalRatio;
    data.fSampPulseIntMOLLERRaw[data.fNSampPulses] =
        data.fSampPulseInt[data.fNSampPulses] * fCalib.ChanTomV *
        fCalib.pC_Conv;
    data.fSampPulseIntMOLLERVal[data.fNSampPulses] =
        data.fSampPulseIntPedSub[data.fNSampPulses] * fCalib.ChanTomV *
        fCalib.pC_Conv * gain;
    data.fSampPulseAmp[data.fNSampPulses] =
        samples[NS]; // raw (no pedestal division)
    data.fSampPulseTime[data.fNSampPulses] = 64 * NS;
    data.fNSampPulses = 1;
  } else {
    Bool_t CheckSampBelowThres = kFALSE;
    Double_t LastTailPed =
        GetIntegral(fNSamples - fConfig.fNPedestalSamples, fNSamples - 1);
    if (LastTailPed < data.fSampPed) {
      data.fSampPed = LastTailPed;
      CheckSampBelowThres = kTRUE;
    }

    while (NS < fNSamples && data.fNSampPulses < fConfig.fMaxNPulses) {
      if (CheckSampBelowThres) {
        if (GetSample(NS) < fConfig.fSampThreshold)
          CheckSampBelowThres = kFALSE;
      } else {
        Int_t ns_found = 0;
        for (Int_t nt = NS; nt < TMath::Min(NS + fConfig.fNSAT, fNSamples);
             ++nt) {
          if (GetSample(nt) > fConfig.fSampThreshold)
            ns_found++;
        }
        if (ns_found == fConfig.fNSAT) {
          data.fSampPulseInt[data.fNSampPulses] =
              GetIntegral(TMath::Max(NS - fConfig.fNSB, 0),
                          TMath::Min(NS + fConfig.fNSA - 1, fNSamples - 1));
          data.fNPeakSamples =
              TMath::Min((NS + fConfig.fNSA - 1), fNSamples - 1) -
              TMath::Max(NS - fConfig.fNSB, 0) + 1;
          data.fPeakPedestalRatio =
              1.0 * data.fNPeakSamples / fConfig.fNPedestalSamples;

          data.fSampPulseIntPedSub[data.fNSampPulses] =
              data.fSampPulseInt[data.fNSampPulses] -
              data.fSampPed * data.fPeakPedestalRatio;

          data.fSampPulseIntMOLLERRaw[data.fNSampPulses] =
              data.fSampPulseInt[data.fNSampPulses] * fCalib.ChanTomV *
              fCalib.pC_Conv;

          data.fSampPulseIntMOLLERVal[data.fNSampPulses] =
              data.fSampPulseIntPedSub[data.fNSampPulses] * fCalib.ChanTomV *
              fCalib.pC_Conv * gain;

          // Peak finding: scan NSA window for the highest sample
          // Use -1 as sentinel (0 is a valid bin index)
          Int_t PeakBin = -1;
          Double_t PeakVal = GetSample(NS);
          for (Int_t nt = NS + 1;
               nt < TMath::Min((NS + fConfig.fNSA), fNSamples); ++nt) {
            if (GetSample(nt) < PeakVal && PeakBin == -1) {
              PeakBin = nt - 1;
            } else if (PeakBin == -1) {
              PeakVal = GetSample(nt);
            }
          }
          if (PeakBin > -1) {
            data.fSampPulseAmp[data.fNSampPulses] =
                samples[PeakBin]; // raw sample
            Int_t Time = NS * 64;
            Double_t VMid = (GetSample(PeakBin)) / 2.;
            for (Int_t nt = TMath::Max(NS - fConfig.fNSB, 0);
                 nt < TMath::Min(PeakBin, int(fNSamples - 1)); nt++) {
              if (VMid >= GetSample(nt) && VMid < GetSample(nt + 1)) {
                Time = 64 * nt + int(64 * (VMid - GetSample(nt)) /
                                     (GetSample(nt + 1) - GetSample(nt)));
              }
            }
            data.fSampPulseTime[data.fNSampPulses] = Time;
          } else {
            data.fSampPulseAmp[data.fNSampPulses] = samples[NS]; // raw sample
            data.fSampPulseTime[data.fNSampPulses] = 64 * NS;
          }

          // Pileup calculations
          Int_t hi = TMath::Min((NS + fConfig.fNSA - 1), fNSamples - 1);
          Int_t tot = 0;
          for (Int_t nt = NS; nt <= hi; nt++) {
            if (GetSample(nt) > fConfig.fSampThreshold)
              tot++;
          }
          data.fSampPulseTOT[data.fNSampPulses] = 64.0 * tot;

          Int_t pileup = 0;
          for (Int_t nt = NS + 1; nt < hi; nt++) {
            if (GetSample(nt) > fConfig.fSampThreshold &&
                GetSample(nt) > GetSample(nt - 1) &&
                GetSample(nt) > GetSample(nt + 1)) {
              pileup = 1;
              break;
            }
          }
          data.fSampPulsePileup[data.fNSampPulses] = pileup;

          data.fNSampPulses++;
          NS = NS + fConfig.fNSA;
          CheckSampBelowThres = kTRUE;
        }
      }
      NS++;
    }
  }
  return data;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Second-derivative peak finder — fully template-free analysis path
// ═══════════════════════════════════════════════════════════════════════════════

// ---------------------------------------------------------------------------
// gaussianSmooth — 1-D Gaussian convolution with reflect boundary.
// Kernel sigma = halfWidth / 2.0 samples; kernel extends ±halfWidth samples.
// ---------------------------------------------------------------------------
std::vector<Double_t>
FadcStandaloneAnalyzer::gaussianSmooth(const std::vector<Double_t> &wf,
                                       Int_t halfWidth) {
  Int_t N = static_cast<Int_t>(wf.size());
  if (halfWidth <= 0 || N <= 1)
    return wf;

  // Build normalised Gaussian kernel
  Double_t sigma = halfWidth / 2.0;
  Int_t kW = 2 * halfWidth + 1;
  std::vector<Double_t> kernel(kW);
  Double_t kSum = 0.0;
  for (Int_t j = 0; j < kW; ++j) {
    Double_t x = static_cast<Double_t>(j - halfWidth);
    kernel[j] = std::exp(-0.5 * x * x / (sigma * sigma));
    kSum += kernel[j];
  }
  for (Int_t j = 0; j < kW; ++j)
    kernel[j] /= kSum;

  // Convolve with reflect boundary
  std::vector<Double_t> out(N);
  for (Int_t i = 0; i < N; ++i) {
    Double_t val = 0.0;
    for (Int_t j = 0; j < kW; ++j) {
      Int_t idx = i + j - halfWidth;
      // Reflect at boundaries
      if (idx < 0)
        idx = -idx;
      if (idx >= N)
        idx = 2 * (N - 1) - idx;
      idx = std::max(0, std::min(idx, N - 1)); // safety clamp
      val += wf[idx] * kernel[j];
    }
    out[i] = val;
  }
  return out;
}

// ---------------------------------------------------------------------------
// evalRawEMG — Evaluates the raw (unnormalized) EMG pulse shape at time offset
// dt, with Gaussian sigma and exponential tau, both in sample units. Uses
// erfc(-erf_arg) to prevent catastrophic cancellation when erf_arg < -6.
// ---------------------------------------------------------------------------
Double_t FadcStandaloneAnalyzer::evalRawEMG(Double_t dt, Double_t sigma,
                                            Double_t tau) {
  if (sigma <= 0.0 || tau <= 0.0)
    return 0.0;
  Double_t s2_2t2 = (sigma * sigma) / (2.0 * tau * tau);
  Double_t erf_arg = (dt - (sigma * sigma) / tau) / (sigma * std::sqrt(2.0));

  if (erf_arg > 700.0)
    return 0.0; // overflow guard

  // Use erfc(-erf_arg) instead of (1.0 + erf(erf_arg)) to prevent precision
  // cancellation
  Double_t val = std::exp(s2_2t2 - (dt / tau)) * std::erfc(-erf_arg);
  return (val > 1e-12) ? val : 0.0;
}

// ---------------------------------------------------------------------------
// findEMGPeak — Fast golden-section search for the exact peak location and
// value of evalRawEMG(dt, sigma, tau). The mode of an EMG is always near 0 (in
// [-sigma, max(3*sigma, tau)]).
// ---------------------------------------------------------------------------
void FadcStandaloneAnalyzer::findEMGPeak(Double_t sigma, Double_t tau,
                                         Double_t &peakOffset,
                                         Double_t &peakVal) {
  if (sigma <= 0.0 || tau <= 0.0) {
    peakOffset = 0.0;
    peakVal = 0.0;
    return;
  }
  Double_t a = -sigma;
  Double_t b = std::max(3.0 * sigma, tau);
  const Double_t gr = (std::sqrt(5.0) + 1.0) / 2.0;
  Double_t c = b - (b - a) / gr;
  Double_t d = a + (b - a) / gr;
  while (std::abs(b - a) > 1e-4) {
    if (evalRawEMG(c, sigma, tau) > evalRawEMG(d, sigma, tau))
      b = d;
    else
      a = c;
    c = b - (b - a) / gr;
    d = a + (b - a) / gr;
  }
  peakOffset = (b + a) / 2.0;
  peakVal = evalRawEMG(peakOffset, sigma, tau);
  if (peakVal < 1e-12)
    peakVal = 1.0;
}

// ---------------------------------------------------------------------------
// evalNormalizedEMG — Evaluates the EMG pulse shape normalised so that its peak
// is 1.0.
// ---------------------------------------------------------------------------
Double_t FadcStandaloneAnalyzer::evalNormalizedEMG(Double_t dt, Double_t sigma,
                                                   Double_t tau) {
  if (sigma <= 0.0 || tau <= 0.0)
    return 0.0;
  Double_t peakOffset = 0.0, peakVal = 1.0;
  findEMGPeak(sigma, tau, peakOffset, peakVal);
  return evalRawEMG(dt, sigma, tau) / peakVal;
}

static inline Double_t evalRawEMG(Double_t dt, Double_t sigma, Double_t tau) {
  return FadcStandaloneAnalyzer::evalRawEMG(dt, sigma, tau);
}

static inline void findEMGPeak(Double_t sigma, Double_t tau,
                               Double_t &peakOffset, Double_t &peakVal) {
  FadcStandaloneAnalyzer::findEMGPeak(sigma, tau, peakOffset, peakVal);
}

// ---------------------------------------------------------------------------
// EMGFitFunctor — chi2 callable for Stage-2 Minuit fit.
// Parameters: [0]=A (amplitude), [1]=b (baseline), [2]=t0,
//             [3]=sigma (sample units), [4]=tau (sample units).
// ---------------------------------------------------------------------------
struct EMGFitFunctor {
  const std::vector<Double_t> *wfResidual;
  Int_t fitLo;
  Int_t fitHi;
  Double_t minSigma;
  Double_t adcPerMV;
  Double_t noiseSigma;

  Double_t operator()(const Double_t *par) const {
    Double_t A = par[0];
    Double_t b = par[1];
    Double_t t0 = par[2];
    Double_t sigma = par[3];
    Double_t tau = par[4];

    Double_t chi2 = 0.0;
    for (Int_t i = fitLo; i <= fitHi; ++i) {
      Double_t dt = static_cast<Double_t>(i) - t0;
      Double_t f = evalRawEMG(dt, sigma, tau);
      Double_t expected = b + A * f;
      Double_t obs = (*wfResidual)[i];
      Double_t s_shot = std::sqrt(std::abs(obs) * adcPerMV / 2.0) / adcPerMV;
      Double_t s_rel = 0.03 * std::abs(obs); // 3% relative systematic model
                                             // tolerance for large amplitudes
      Double_t sig =
          std::sqrt(s_shot * s_shot + noiseSigma * noiseSigma + s_rel * s_rel);
      if (sig < minSigma)
        sig = minSigma;
      Double_t diff = obs - expected;
      if (obs >= 1850.0 && expected >= obs) {
        diff = (expected - obs) * 0.1; // soften penalty for saturated clipping
      }
      Double_t residual = diff / sig;
      chi2 += residual * residual;
    }
    return chi2;
  }
};

// ---------------------------------------------------------------------------
// computeSecondDerivative
// Smooths the waveform with a Gaussian kernel, then computes the
// central-difference second derivative: d2[i] = y[i-1] - 2*y[i] + y[i+1].
// Edge values are copied from the nearest interior value.
// ---------------------------------------------------------------------------
std::vector<Double_t> FadcStandaloneAnalyzer::computeSecondDerivative(
    const std::vector<Double_t> &wfPS, Int_t smoothHalfWidth) {
  Int_t N = static_cast<Int_t>(wfPS.size());
  if (N < 3)
    return std::vector<Double_t>(N, 0.0);

  // Gaussian pre-smoothing
  std::vector<Double_t> smoothed;
  const std::vector<Double_t> *yPtr = &wfPS;
  if (smoothHalfWidth > 0) {
    smoothed = gaussianSmooth(wfPS, smoothHalfWidth);
    yPtr = &smoothed;
  }
  const std::vector<Double_t> &y = *yPtr;

  // Central-difference second derivative
  std::vector<Double_t> d2(N, 0.0);
  for (Int_t i = 1; i < N - 1; ++i) {
    d2[i] = y[i - 1] - 2.0 * y[i] + y[i + 1];
  }
  // Edge values: copy from nearest interior value
  d2[0] = d2[1];
  d2[N - 1] = d2[N - 2];

  return d2;
}

struct EMGSeeds {
  Double_t sigma;
  Double_t tau;
};

// ---------------------------------------------------------------------------
// estimateEMGSeeds — estimate σ and τ from the smoothed waveform shape
// around a D2-identified peak, before Minuit runs.
//
// Modified: Valley-aware backward/forward searches to prevent pileup tails
// from skewing the baseline. Physically clamped output.
// ---------------------------------------------------------------------------
static EMGSeeds estimateEMGSeeds(const std::vector<Double_t> &wfSmooth,
                                 Double_t peakPosition, Double_t peakAmplitude,
                                 Int_t nPulsesInCluster) {
  const Double_t defaultSigma = 1.25; // σ = 5 ns / 4 ns/sample
  const Double_t defaultTau = 2.5;    // τ = 10 ns / 4 ns/sample

  Int_t N = static_cast<Int_t>(wfSmooth.size());
  Int_t peakBin = static_cast<Int_t>(std::round(peakPosition));
  peakBin = std::max(1, std::min(peakBin, N - 2));

  EMGSeeds seeds = {defaultSigma, defaultTau};

  if (peakAmplitude < 1e-6)
    return seeds; // no signal — use defaults

  // σ estimate: Valley-aware 10%–90% rise time

  Int_t leftValleyBin = 0;
  Double_t leftMinVal = wfSmooth[peakBin];
  for (Int_t i = peakBin - 1; i >= 0; --i) {
    if (wfSmooth[i] <= leftMinVal) {
      leftMinVal = wfSmooth[i];
      leftValleyBin = i;
    } else {
      leftValleyBin = i + 1; // hit rising edge of earlier pulse or baseline
      break;
    }
  }

  Double_t heightLeft = wfSmooth[peakBin] - leftMinVal;
  if (heightLeft > 4.0 && peakBin > leftValleyBin) {
    Double_t level10 = leftMinVal + 0.10 * heightLeft;
    Double_t level90 = leftMinVal + 0.90 * heightLeft;

    for (Int_t i = peakBin; i > leftValleyBin; --i) {
      if (wfSmooth[i] <= level90 && wfSmooth[i + 1] > level90) {
        Double_t frac90 =
            (level90 - wfSmooth[i]) / (wfSmooth[i + 1] - wfSmooth[i]);
        Double_t t90 = static_cast<Double_t>(i) + frac90;

        for (Int_t j = i; j > leftValleyBin; --j) {
          if (wfSmooth[j] <= level10 && wfSmooth[j + 1] > level10) {
            Double_t frac10 =
                (level10 - wfSmooth[j]) / (wfSmooth[j + 1] - wfSmooth[j]);
            Double_t t10 = static_cast<Double_t>(j) + frac10;
            Double_t tRise = t90 - t10;
            if (tRise > 0.0)
              seeds.sigma = tRise / 2.56;
            break;
          }
        }
        break;
      }
    }
  }

  // Clamping σ to Physical Limits
  seeds.sigma = std::max(0.3, std::min(seeds.sigma, 2.5));

  // τ estimate: Valley-aware peak to 1/e decay

  Int_t maxForward = N - 1 - peakBin;
  if (nPulsesInCluster > 1) {
    maxForward = std::max(1, maxForward / 2);
  }
  Int_t searchEnd = std::min(N - 1, peakBin + maxForward);

  // Walk forward to find the local valley
  Int_t fwdValleyBin = peakBin;
  for (Int_t i = peakBin + 1; i <= searchEnd; ++i) {
    if (wfSmooth[i] > wfSmooth[i - 1])
      break; // Hit a valley
    fwdValleyBin = i;
  }

  Double_t fwdBase = wfSmooth[fwdValleyBin];
  Double_t effDecayAmp = peakAmplitude - fwdBase;

  if (effDecayAmp > 1e-6) {
    Double_t level1e = fwdBase + effDecayAmp / std::exp(1.0);

    for (Int_t i = peakBin; i < fwdValleyBin; ++i) {
      if (wfSmooth[i] >= level1e && wfSmooth[i + 1] < level1e) {
        Double_t frac =
            (wfSmooth[i] - level1e) / (wfSmooth[i] - wfSmooth[i + 1]);
        Double_t tDecay =
            (static_cast<Double_t>(i) + frac) - static_cast<Double_t>(peakBin);
        if (tDecay > 0.0)
          seeds.tau = tDecay;
        break;
      }
    }
  }

  // Clamping τ to Physical Limits
  seeds.tau = std::max(1.0, std::min(seeds.tau, 6.0));

  return seeds;
}

// File-local struct for D2 peak candidates
struct D2Peak {
  Double_t
      position; // sub-sample peak position (parabolic-refined on smoothed wf)
  Double_t
      d2Value; // d2 value at the minimum (negative = concave down = pulse peak)
  Int_t clusterID;           // which ROI cluster this peak belongs to
  Double_t seedSigma = 1.25; // per-pulse σ estimate from waveform edges
  Double_t seedTau = 2.5;    // per-pulse τ estimate from waveform edges
};

// Return type for findD2Peaks
struct D2FindResult {
  std::vector<D2Peak> peaks;
  std::vector<D2Cluster> clusters;
};

// ---------------------------------------------------------------------------
// findD2Peaks — identify ROIs and pulse candidates from the d2 trace.
//
// Algorithm:
//   1. Scan d2 for regions where d2[i] < -d2Threshold (ROIs).
//   2. Within each ROI, find local minima of d2 separated by at least
//      minPeakSep samples.  Each minimum marks one pulse candidate.
//   3. For each d2 minimum, find the corresponding waveform peak (local max
//      of wfSmooth) and refine its position with parabolic interpolation.
//   4. Group peaks into D2Cluster structs with ROI boundaries and pulse count.
// ---------------------------------------------------------------------------
static D2FindResult findD2Peaks(const std::vector<Double_t> &d2,
                                const std::vector<Double_t> &wfSmooth,
                                Double_t d2Threshold, Int_t minPeakSep,
                                Int_t nPedSamples) {
  Int_t N = static_cast<Int_t>(d2.size());
  D2FindResult result;

  // Find contiguous ROIs where d2 < -threshold
  struct ROI {
    Int_t start;
    Int_t end;
  };
  std::vector<ROI> rois;

  Bool_t inROI = kFALSE;
  Int_t roiStart = 0;

  for (Int_t i = 0; i < N; ++i) {
    if (!inROI && d2[i] < -d2Threshold) {
      inROI = kTRUE;
      roiStart = i;
    } else if (inROI && d2[i] >= -d2Threshold) {
      inROI = kFALSE;
      rois.push_back({roiStart, i - 1});
    }
  }
  if (inROI)
    rois.push_back({roiStart, N - 1});

  // Process each ROI into a cluster
  for (size_t ci = 0; ci < rois.size(); ++ci) {
    const ROI &roi = rois[ci];
    D2Cluster cluster;
    cluster.roiStart = roi.start;
    cluster.roiEnd = roi.end;
    cluster.nPulses = 0;

    Int_t searchStart = std::max(0, roi.start);
    Int_t searchEnd = std::min(N - 1, roi.end);

    // Find all local minima of d2 within the ROI
    std::vector<Int_t> minBins;
    for (Int_t i = searchStart; i <= searchEnd; ++i) {
      bool is_left_lower = (i == 0 || d2[i] <= d2[i - 1]);
      bool is_right_lower = (i == N - 1 || d2[i] <= d2[i + 1]);
      if (is_left_lower && is_right_lower) {
        // Enforce minimum separation — keep the deeper minimum
        Bool_t merged = kFALSE;
        for (size_t k = 0; k < minBins.size(); ++k) {
          if (std::abs(i - minBins[k]) < minPeakSep) {
            if (d2[i] < d2[minBins[k]])
              minBins[k] = i;
            merged = kTRUE;
            break;
          }
        }
        if (!merged)
          minBins.push_back(i);
      }
    }

    // Fallback: if no strict local minimum was found, use the global minimum
    if (minBins.empty()) {
      Int_t bestBin = searchStart;
      for (Int_t i = searchStart + 1; i <= searchEnd; ++i) {
        if (d2[i] < d2[bestBin])
          bestBin = i;
      }
      minBins.push_back(bestBin);
    }

    // For each d2 minimum, refine the peak position on the smoothed waveform
    for (Int_t mb : minBins) {
      Int_t sLo = std::max(0, mb - minPeakSep);
      Int_t sHi = std::min(N - 1, mb + minPeakSep);
      Int_t wfPeakBin = mb;
      Double_t wfPeakVal = wfSmooth[mb];
      for (Int_t i = sLo; i <= sHi; ++i) {
        if (wfSmooth[i] > wfPeakVal) {
          wfPeakVal = wfSmooth[i];
          wfPeakBin = i;
        }
      }

      // Parabolic sub-sample interpolation on the smoothed waveform
      Double_t pos = static_cast<Double_t>(wfPeakBin);
      if (wfPeakBin > 0 && wfPeakBin < N - 1) {
        Double_t denom =
            2.0 * (wfSmooth[wfPeakBin - 1] - 2.0 * wfSmooth[wfPeakBin] +
                   wfSmooth[wfPeakBin + 1]);
        if (std::abs(denom) > 1e-9) {
          pos = wfPeakBin -
                (wfSmooth[wfPeakBin + 1] - wfSmooth[wfPeakBin - 1]) / denom;
        }
      }

      D2Peak peak;
      peak.position = pos;
      peak.d2Value = d2[mb];
      peak.clusterID = static_cast<Int_t>(ci);

      // Determine how many pulses share this cluster to constrain the tau
      // search
      Int_t nPulsesInCluster = static_cast<Int_t>(minBins.size());
      EMGSeeds seeds = estimateEMGSeeds(wfSmooth, peak.position, wfPeakVal,
                                        nPulsesInCluster);
      peak.seedSigma = seeds.sigma;
      peak.seedTau = seeds.tau;

      result.peaks.push_back(peak);
      cluster.nPulses++;
    }

    result.clusters.push_back(cluster);
  }

  return result;
}

// ---------------------------------------------------------------------------
// fitEMGDirect — template-free EMG Minuit fit for the D2 path.
//
// wfPeakBin is the D2-identified waveform-peak sample position.
// The fit seeds t0 so that the initial EMG peaks near wfPeakBin:
//   t0_init = wfPeakBin - sigma_init²/tau_init
//
// Post-convergence, the raw Minuit amplitude A_raw is converted to the
// physical peak ADC count:  amplitude = A_raw * max(emgRaw) over the fit
// window.
// ---------------------------------------------------------------------------
static FitPulseResult fitEMGDirect(const std::vector<Double_t> &wfResidual,
                                   Double_t wfPeakBin, Int_t fitLo, Int_t fitHi,
                                   const TemplateAnalysisConfig &cfg,
                                   Double_t seedSigma = 1.25,
                                   Double_t seedTau = 2.5) {
  Int_t N = static_cast<Int_t>(wfResidual.size());
  fitLo = std::max(fitLo, 0);
  fitHi = std::min(fitHi, N - 1);
  Int_t nPts = fitHi - fitLo + 1;

  FitPulseResult best;
  best.valid = kFALSE;
  best.chi2ndf = std::numeric_limits<Double_t>::max();
  best.timeSample = wfPeakBin;
  best.timeHalfMax = wfPeakBin;
  best.amplitude = 0.0;
  best.integral = 0.0;
  best.baseline = 0.0;
  best.mfAmplitude = 0.0;
  best.fitStage = 2; // always EMG in D2 path
  best.fittedSigma = 0.0;
  best.fittedTau = 0.0;
  best.clusterID = -1;
  best.nPulsesInCluster = 1;

  std::unique_ptr<ROOT::Math::Minimizer> min(
      ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad"));
  if (!min)
    return best;

  min->SetMaxFunctionCalls(1000);
  min->SetTolerance(0.1);
  min->SetPrintLevel(0);

  EMGFitFunctor chi2Functor{&wfResidual,   fitLo,         fitHi,
                            cfg.fMinSigma, cfg.fAdcPerMV, cfg.fNoiseSigmaADC};
  ROOT::Math::Functor functor(chi2Functor, 5);
  min->SetFunction(functor);

  // Rough amplitude seed from the residual window
  Double_t maxVal = 0.0;
  for (Int_t i = fitLo; i <= fitHi; ++i)
    if (wfResidual[i] > maxVal)
      maxVal = wfResidual[i];

  // Dynamic seeds from waveform shape (or defaults if not estimated)
  const Double_t initSigma = seedSigma;
  const Double_t initTau = seedTau;

  // Seed t0 so the initial EMG peaks near the D2-identified waveform peak.
  Double_t initPeakOffset = 0.0, initPeakVal = 1.0;
  findEMGPeak(initSigma, initTau, initPeakOffset, initPeakVal);
  Double_t initT0 = wfPeakBin - initPeakOffset;

  min->SetLimitedVariable(0, "Amplitude", maxVal, maxVal * 0.1, 0.0,
                          maxVal * 20.0);
  Double_t maxB = std::max(cfg.fMaxBaselineAbs, cfg.fMaxBaselineFrac * maxVal);
  min->SetLimitedVariable(1, "Baseline", 0.0, 0.5, -maxB, maxB);
  min->SetLimitedVariable(2, "t0", initT0, 0.5, initT0 - cfg.fScanHalf,
                          initT0 + cfg.fScanHalf);
  min->SetLimitedVariable(3, "Sigma", initSigma, 0.1, 0.15, 4.0);
  Double_t maxTau = 10.0;

  // If pulses are clustered in pileup, prevent Tau from expanding into the
  // neighbor
  if ((fitHi - static_cast<Int_t>(wfPeakBin)) <= 4 || seedTau < 2.0) {
    maxTau = std::max(2.0, seedTau * 1.5);
  }
  min->SetLimitedVariable(4, "Tau", initTau, 0.1, 0.18, maxTau);

  min->Minimize();

  if (min->Status() == 0 || min->Status() == 1) {
    const Double_t *xs = min->X();
    Double_t A_raw = xs[0];
    Double_t t0 = xs[2];
    Double_t sigma = xs[3];
    Double_t tau = xs[4];

    // Convert raw amplitude to physical peak ADC count
    Double_t peakOffset = 0.0, emgPeakRaw = 1.0;
    findEMGPeak(sigma, tau, peakOffset, emgPeakRaw);

    best.amplitude = A_raw * emgPeakRaw;
    best.baseline = xs[1];
    best.timeSample = t0;
    best.fittedSigma = sigma;
    best.fittedTau = tau;

    Double_t chi2 = min->MinValue();
    Int_t ndf = std::max(nPts - 5, 1);
    best.chi2ndf = chi2 / ndf;
    best.valid = kTRUE;
  }

  return best;
}

// ---------------------------------------------------------------------------
// addEMGContribution — add or subtract a fitted EMG pulse from a waveform.
// sign = +1.0 to add back (restore), -1.0 to subtract (peel).
// ---------------------------------------------------------------------------
static void addEMGContribution(std::vector<Double_t> &wf,
                               const FitPulseResult &pulse, Double_t sign) {
  Int_t N = static_cast<Int_t>(wf.size());
  for (Int_t i = 0; i < N; ++i) {
    Double_t f = FadcStandaloneAnalyzer::evalNormalizedEMG(
        static_cast<Double_t>(i) - pulse.timeSample, pulse.fittedSigma,
        pulse.fittedTau);
    wf[i] += sign * pulse.amplitude * f;
  }
}

// ---------------------------------------------------------------------------
// computePulseDerivedQuantitiesEMG — template-free derived quantities.
// Computes the pulse integral and half-max rising-edge timing. The integration
// window is derived from the fitted sigma and tau.
// ---------------------------------------------------------------------------
static void
computePulseDerivedQuantitiesEMG(FitPulseResult &fit,
                                 const std::vector<Double_t> &wfForTiming,
                                 const TemplateAnalysisConfig &cfg) {
  Int_t N = static_cast<Int_t>(wfForTiming.size());

  // Integration window: [-3σ, peak + 5τ] centred on the EMG origin.
  Double_t sigma = (fit.fittedSigma > 0.0) ? fit.fittedSigma : 1.25;
  Double_t tau = (fit.fittedTau > 0.0) ? fit.fittedTau : 2.5;

  Double_t emgPeakOffset = 0.0, emgPeakRaw = 1.0;
  findEMGPeak(sigma, tau, emgPeakOffset, emgPeakRaw);

  Int_t intLo = static_cast<Int_t>(std::floor(-3.0 * sigma));
  Int_t intHi = static_cast<Int_t>(std::ceil(emgPeakOffset + 5.0 * tau));

  Double_t emgAreaRaw = 0.0;
  for (Int_t i = intLo; i <= intHi; ++i) {
    emgAreaRaw += evalRawEMG(static_cast<Double_t>(i), sigma, tau);
  }

  fit.integral = fit.amplitude * emgAreaRaw / emgPeakRaw;

  Int_t wfPeak = static_cast<Int_t>(std::round(fit.timeSample + emgPeakOffset));
  wfPeak = std::max(0, std::min(wfPeak, N - 1));
  Int_t hLo = std::max(0, wfPeak - cfg.fFitMargin);
  Int_t hHi = std::min(N - 1, wfPeak + cfg.fFitMargin);
  Double_t locPeak = 0.0;
  Int_t locBin = wfPeak;
  for (Int_t i = hLo; i <= hHi; ++i) {
    if (wfForTiming[i] > locPeak) {
      locPeak = wfForTiming[i];
      locBin = i;
    }
  }
  Double_t vMid = locPeak / 2.0;
  Double_t halfMax = static_cast<Double_t>(locBin); // fallback: peak bin
  for (Int_t i = hLo; i < locBin; ++i) {
    if (wfForTiming[i] <= vMid && wfForTiming[i + 1] > vMid) {
      halfMax =
          i + (vMid - wfForTiming[i]) / (wfForTiming[i + 1] - wfForTiming[i]);
      break;
    }
  }
  fit.timeHalfMax = halfMax;
}

// ---------------------------------------------------------------------------
// AnalyzeWithEMG — second-derivative based, fully template-free analysis.
//
// Pipeline:
//   1. Pedestal estimation (identical to hcana PedestalEstimation)
//   2. Second derivative computation (d2)
//   3. Auto-scaled d2 threshold from pedestal noise
//   4. ROI detection and peak identification via findD2Peaks
//   5. Direct EMG Minuit fit per peak (no template)
//   6. Iterative coordinate-descent re-fitting for pileup resolution
//   7. Chronological sort of output pulses
// ---------------------------------------------------------------------------
FADCPulseWF
FadcStandaloneAnalyzer::AnalyzeWithEMG(const std::vector<Double_t> &samples,
                                       const TemplateAnalysisConfig &cfg,
                                       Long64_t eventNum) const {
  FADCPulseWF result;
  result.nPulses = 0;

  Int_t N = static_cast<Int_t>(samples.size());
  if (N < 3 || cfg.fNPedSamples <= 0) {
    if (eventNum >= 0) {
      std::cerr << "[AnalyzeWithEMG] Event " << eventNum
                << ": Event skipped from processing\n";
    } else {
      std::cerr << "Event skipped from processing\n";
    }
    return result;
  }

  // 1. Pedestal — noise-aware front vs tail selection ________________________
  Int_t nPed = std::min(cfg.fNPedSamples, N);

  // Front pedestal
  Double_t ped = 0.0;
  for (Int_t i = 0; i < nPed; ++i)
    ped += samples[i];
  ped /= nPed;
  Double_t pedVar = 0.0;
  for (Int_t i = 0; i < nPed; ++i) {
    Double_t d = samples[i] - ped;
    pedVar += d * d;
  }
  pedVar /= (nPed > 1) ? (nPed - 1) : 1;

  // Tail pedestal
  Double_t ped2 = 0.0;
  for (Int_t i = N - nPed; i < N; ++i)
    ped2 += samples[i];
  ped2 /= nPed;
  Double_t ped2Var = 0.0;
  for (Int_t i = N - nPed; i < N; ++i) {
    Double_t d = samples[i] - ped2;
    ped2Var += d * d;
  }
  ped2Var /= (nPed > 1) ? (nPed - 1) : 1;

  Double_t pedNoiseSigma = std::sqrt((ped <= ped2) ? pedVar : ped2Var);
  Double_t pedestal = (pedVar <= ped2Var) ? ped : ped2;
  result.pedestal = pedestal;

  // Pedestal-subtracted waveform
  std::vector<Double_t> wfPS(N);
  for (Int_t i = 0; i < N; ++i)
    wfPS[i] = samples[i] - result.pedestal;
  result.wfPS = wfPS; // store for diagnostics

  // Auto-estimate electronic noise for the per-sample error model
  TemplateAnalysisConfig localCfg = cfg;
  if (localCfg.fNoiseSigmaADC < 0.0) {
    localCfg.fNoiseSigmaADC = pedNoiseSigma;
  }

  // 2. Second derivative calculation ______________________________________
  Int_t effectiveHalfWidth = localCfg.fSmoothHalfWidth;
  if (effectiveHalfWidth < 0) {
    // Measure maximum 1-sample rising slope relative to peak amplitude
    Double_t maxSlope = 0.0;
    Double_t maxAmp = 0.0;
    for (Int_t i = 1; i < N; ++i) {
      Double_t slope = wfPS[i] - wfPS[i - 1];
      if (slope > maxSlope)
        maxSlope = slope;
      if (wfPS[i] > maxAmp)
        maxAmp = wfPS[i];
    }

    if (maxAmp > 10.0 && (maxSlope / maxAmp) > 0.60) {
      effectiveHalfWidth = 1;
    } else {
      effectiveHalfWidth = 2;
    }
  }

  // Smoothing for visualization purposes and d2 peak finding. D2 calculation
  // has its own smoothing.
  std::vector<Double_t> wfSmooth = gaussianSmooth(wfPS, effectiveHalfWidth);
  result.wfSmooth = wfSmooth;

  std::vector<Double_t> d2 = computeSecondDerivative(wfPS, effectiveHalfWidth);
  result.d2Out = d2;

  // 3. Auto-scale d2 threshold from pedestal noise ________________________
  //       Estimate σ_d2 from the pedestal region of the d2 trace.
  Int_t d2PedStart, d2PedEnd;

  // Use variance to find the quieter region
  if (pedVar <= ped2Var) {
    // Front is quieter
    d2PedStart = 1; // skip edge copy
    d2PedEnd = std::min(d2PedStart + nPed, N - 1);
  } else {
    // Tail is quieter
    d2PedEnd = N - 1;
    d2PedStart = std::max(1, d2PedEnd - nPed);
  }
  Int_t d2PedN = d2PedEnd - d2PedStart;
  if (d2PedN < 1)
    d2PedN = 1;

  Double_t d2Mean = 0.0;
  for (Int_t i = d2PedStart; i < d2PedEnd; ++i)
    d2Mean += d2[i];
  d2Mean /= d2PedN;

  Double_t d2Var = 0.0;
  for (Int_t i = d2PedStart; i < d2PedEnd; ++i) {
    Double_t dd = d2[i] - d2Mean;
    d2Var += dd * dd;
  }
  d2Var /= (d2PedN > 1) ? (d2PedN - 1) : 1;
  Double_t d2Sigma = (pedNoiseSigma > 0.05) ? (pedNoiseSigma * 0.595) : 0.6;
  Double_t d2Threshold = std::max(0.5, localCfg.fD2ThresholdNSigma * d2Sigma);

  // 4. Find D2 peaks and clusters ______________________________________
  D2FindResult d2Result =
      findD2Peaks(d2, wfSmooth, d2Threshold, localCfg.fD2MinPeakSep, nPed);
  result.clusters = d2Result.clusters;

  // Sort peaks by |d2| value (most negative = strongest peak first)
  std::sort(
      d2Result.peaks.begin(), d2Result.peaks.end(),
      [](const D2Peak &a, const D2Peak &b) { return a.d2Value < b.d2Value; });

  // 5. Fit each peak with direct EMG ______________________________________
  std::vector<Double_t> wfResidual = wfPS;
  std::vector<Double_t> fittedPositions;

  for (const auto &peak : d2Result.peaks) {
    if (result.nPulses >= localCfg.fMaxPulses)
      break;

    // Skip if too close to an already-fitted pulse
    Bool_t tooClose = kFALSE;
    for (Double_t pos : fittedPositions) {
      if (std::abs(peak.position - pos) <
          static_cast<Double_t>(localCfg.fD2MinPeakSep)) {
        tooClose = kTRUE;
        break;
      }
    }
    if (tooClose)
      continue;

    Int_t effectiveFitMargin = localCfg.fFitMargin;
    if (d2Result.peaks.size() >= 2) {
      // Check distance to closest neighboring candidate
      Double_t minNeighborDist = 999.0;
      for (const auto &other : d2Result.peaks) {
        if (&other == &peak)
          continue;
        Double_t dist = std::abs(peak.position - other.position);
        if (dist < minNeighborDist)
          minNeighborDist = dist;
      }
      if (minNeighborDist < 8.0) {
        effectiveFitMargin =
            std::max(2, static_cast<Int_t>(minNeighborDist / 2.0));
      }
    }

    // Fit window around the D2-identified peak
    Int_t fitLo =
        std::max(0, static_cast<Int_t>(peak.position) - effectiveFitMargin);

    // Dynamically scale the right fit margin
    Int_t tailMargin = std::max(
        localCfg.fFitMargin, static_cast<Int_t>(std::ceil(3.0 * peak.seedTau)));
    // Neighbor check: clamp tailMargin BEFORE reaching the neighboring peak
    for (const auto &other : d2Result.peaks) {
      if (other.position > peak.position) {
        Double_t dist = other.position - peak.position;
        Int_t maxAllowed = std::max(1, static_cast<Int_t>(dist - 1.0));
        if (tailMargin > maxAllowed) {
          tailMargin = maxAllowed;
        }
      }
    }
    Int_t fitHi =
        std::min(N - 1, static_cast<Int_t>(peak.position) + tailMargin);

    Int_t minPts = 2 * localCfg.fFitMargin;
    if (fitHi - fitLo < minPts) {
      if (fitLo == 0)
        fitHi = std::min(N - 1, fitLo + minPts);
      else if (fitHi == N - 1)
        fitLo = std::max(0, fitHi - minPts);
    }

    FitPulseResult fit = fitEMGDirect(wfResidual, peak.position, fitLo, fitHi,
                                      localCfg, peak.seedSigma, peak.seedTau);

    fit.mfAmplitude = std::abs(peak.d2Value); // store |d2| as diagnostic
    fit.clusterID = peak.clusterID;
    fit.nPulsesInCluster =
        (peak.clusterID >= 0 &&
         peak.clusterID < static_cast<Int_t>(d2Result.clusters.size()))
            ? d2Result.clusters[peak.clusterID].nPulses
            : 1;

    if (!fit.valid || fit.amplitude < localCfg.fAmpCutADC) {
      FADC_DEBUG_LOG("[AnalyzeWithD2] Peak at sample "
                     << peak.position << " rejected (amp=" << fit.amplitude
                     << " < cut=" << localCfg.fAmpCutADC
                     << "), treating as noise.\n");
      continue;
    }

    computePulseDerivedQuantitiesEMG(fit, wfResidual, localCfg);

    // Subtract this pulse from the residual for subsequent fits
    addEMGContribution(wfResidual, fit, -1.0);

    Double_t pOffset = 0.0, pVal = 1.0;
    findEMGPeak(fit.fittedSigma, fit.fittedTau, pOffset, pVal);
    fittedPositions.push_back(fit.timeSample + pOffset);
    result.pulses.push_back(fit);
    result.nPulses++;
  }

  // 6. Iterative re-fitting (coordinate descent for pileup resolution)
  //       Each pulse is added back, re-fitted against the clean residual
  //       (all other pulses subtracted), then the new fit is subtracted.
  if (localCfg.fRefitMaxIter > 0 && result.nPulses >= 2) {
    FADC_DEBUG_LOG("[AnalyzeWithD2] Waveform "
                   << "pileup detected (" << result.nPulses
                   << " pulses), starting iterative re-fit.\n");

    Bool_t converged = kFALSE;
    for (Int_t iter = 0; iter < localCfg.fRefitMaxIter; ++iter) {
      Double_t prevTotalChi2 = 0.0;
      for (const auto &p : result.pulses)
        prevTotalChi2 += p.chi2ndf;

      // Process pulses strongest-first
      std::vector<Int_t> order(result.nPulses);
      for (Int_t i = 0; i < result.nPulses; ++i)
        order[i] = i;
      std::sort(order.begin(), order.end(), [&](Int_t a, Int_t b) {
        return result.pulses[a].amplitude > result.pulses[b].amplitude;
      });

      for (Int_t idx : order) {
        FitPulseResult &pulse = result.pulses[idx];

        // Add back this pulse's contribution to the residual
        addEMGContribution(wfResidual, pulse, +1.0);

        // Fit window from the current pulse's peak position
        Double_t pOffset = 0.0, pVal = 1.0;
        findEMGPeak(pulse.fittedSigma, pulse.fittedTau, pOffset, pVal);
        Double_t peakPos = pulse.timeSample + pOffset;
        Int_t fitLo =
            std::max(0, static_cast<Int_t>(peakPos) - localCfg.fFitMargin);

        Int_t tailMargin =
            std::max(localCfg.fFitMargin,
                     static_cast<Int_t>(std::ceil(3.0 * pulse.fittedTau)));

        Int_t fitHi = std::min(N - 1, static_cast<Int_t>(peakPos) + tailMargin);

        // Re-fit against the clean residual
        FitPulseResult refit =
            fitEMGDirect(wfResidual, peakPos, fitLo, fitHi, localCfg,
                         pulse.fittedSigma, pulse.fittedTau);

        refit.mfAmplitude = pulse.mfAmplitude;
        refit.clusterID = pulse.clusterID;
        refit.nPulsesInCluster = pulse.nPulsesInCluster;

        if (refit.valid && refit.amplitude >= localCfg.fAmpCutADC) {
          computePulseDerivedQuantitiesEMG(refit, wfResidual, localCfg);
          pulse = refit;
        }

        // Subtract the (possibly updated) pulse from the residual
        addEMGContribution(wfResidual, pulse, -1.0);
      }

      // Convergence check
      Double_t newTotalChi2 = 0.0;
      for (const auto &p : result.pulses)
        newTotalChi2 += p.chi2ndf;

      if (prevTotalChi2 > 0.0 &&
          std::abs(newTotalChi2 - prevTotalChi2) / prevTotalChi2 <
              localCfg.fRefitConvergence) {
        converged = kTRUE;
        break;
      }
    }
    if (!converged && result.nPulses >= 2) {
      FADC_DEBUG_LOG(
          "[AnalyzeWithD2] Waveform: chi2 re-fit did NOT converge after "
          << localCfg.fRefitMaxIter << " iterations\n");
    }
  }

  std::vector<FitPulseResult> cleanPulses;
  for (const auto &p : result.pulses) {
    // Quality cut on decoupled chi2/ndf:
    // Scale threshold gracefully for large pulses and saturated waveforms
    Double_t effMaxChi2 = localCfg.fMaxChi2NDF;
    bool isSaturated = (p.amplitude >= 1200.0);
    if (isSaturated) {
      effMaxChi2 =
          std::max(effMaxChi2, 1000.0); // preserve physical saturated pulses
    } else if (p.amplitude > 50.0) {
      effMaxChi2 = localCfg.fMaxChi2NDF + 0.15 * (p.amplitude - 50.0);
    }

    if (p.chi2ndf > effMaxChi2) {
      FADC_DEBUG_LOG("[AnalyzeWithD2] Pulse at sample "
                     << p.timeSample << " rejected: chi2/ndf=" << p.chi2ndf
                     << " > max=" << effMaxChi2 << "\n");
      continue;
    }

    // Baseline offset sanity
    if (std::abs(p.baseline) > std::max(localCfg.fMaxBaselineFrac * p.amplitude,
                                        localCfg.fMaxBaselineAbs)) {
      FADC_DEBUG_LOG("[AnalyzeWithD2] Pulse at sample "
                     << p.timeSample << " rejected: baseline=" << p.baseline
                     << " > max frac*amp or max abs\n");
      continue;
    }

    // Boundary pegging cut — reject unphysical delta spikes or upper limit
    // pegging.
    if (p.fittedSigma < 0.10 || p.fittedTau < 0.17 || p.fittedSigma >= 3.90) {
      FADC_DEBUG_LOG("[AnalyzeWithD2] Pulse at sample "
                     << p.timeSample << " rejected: pegging (sigma="
                     << p.fittedSigma << " tau=" << p.fittedTau << ")\n");
      continue;
    }

    // Physical shape cut: reject unphysical phantom spikes where slow Gaussian
    // rise (sigma >= 2.0 samples = 8 ns) drastically exceeds the exponential
    // decay tail.
    if (!isSaturated && p.fittedSigma >= 2.0 &&
        p.fittedSigma > 2.0 * p.fittedTau) {
      FADC_DEBUG_LOG("[AnalyzeWithD2] Pulse at sample "
                     << p.timeSample
                     << " rejected: unphysical shape (sigma=" << p.fittedSigma
                     << " > 2*tau=" << 2.0 * p.fittedTau << ")\n");
      continue;
    }

    cleanPulses.push_back(p);
  }
  result.pulses = cleanPulses;
  result.nPulses = static_cast<Int_t>(cleanPulses.size());

  // 7. Sort results chronologically for output _________________________
  std::sort(result.pulses.begin(), result.pulses.end(),
            [](const FitPulseResult &a, const FitPulseResult &b) {
              return a.timeSample < b.timeSample;
            });

  if (result.nPulses < 1) {
    if (eventNum >= 0) {
      std::cerr << "[AnalyzeWithEMG] Event " << eventNum
                << ": No pulse found\n";
    } else {
      std::cerr << "[AnalyzeWithEMG] No pulse found\n";
    }
  }

  return result;
}