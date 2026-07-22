#include "waveformGenerator.h"
#include "RtypesCore.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

waveformGenerator::waveformGenerator() : fRng(std::random_device{}()) {}

std::vector<Int_t> waveformGenerator::noiseWaveform() const {
    int min = fDaq.pedestal - fDaq.ped_sigma / 2.0;
    int max = fDaq.pedestal + fDaq.ped_sigma / 2.0 + 1;
    std::uniform_real_distribution<double> rndm(0.0, 1.0);
    std::vector<Int_t> wave;
    for( int i = 0; i < fDaq.num_samples; i++ ) {
    wave.push_back(min + (max - min) * rndm(fRng));
    }
    return wave;
}

std::vector<Int_t> waveformGenerator::singlePulseWaveform() const {
    int min_ped = fDaq.pedestal - fDaq.ped_sigma / 2.0;
    int max_ped = fDaq.pedestal + fDaq.ped_sigma / 2.0 + 1;
    std::uniform_real_distribution<double> rndm(0.0, 1.0);
    std::vector<Int_t> wave;

    int pulse_min = fDaq.init_ped_samples;
    int pulse_max = fDaq.num_samples - 1;
    int random_idx = pulse_min + int(pulse_max - pulse_min) * rndm(fRng);
    for( int i = 0; i < fDaq.num_samples; i++ ) {
        if(i >= random_idx && i < random_idx + fWf.pulse_width) {
            int j = i - random_idx;
            wave.push_back(min_ped + fWf.mean_gain/TMath::Power(2, j));
        } else {
            wave.push_back(min_ped + (max_ped - min_ped) * rndm(fRng));
        }
    }
    return wave;
}

std::vector<Int_t> waveformGenerator::multiplePulseWaveform() const {
    int min_ped = fDaq.pedestal - fDaq.ped_sigma / 2.0;
    int max_ped = fDaq.pedestal + fDaq.ped_sigma / 2.0 + 1;
    std::uniform_real_distribution<double> rndm(0.0, 1.0);
    std::vector<Int_t> wave;
    std::vector<std::pair<Int_t, Int_t>> pulse_info;

    int num_pulses = 2 + int((fDaq.max_n_pulses - 2) * rndm(fRng));

    // Maximum pulse space required
    int max_space = (num_pulses * fWf.pulse_width) + 
                ((num_pulses - 1) * fWf.pulse_separation);

    // Maximum available space for pulse start
    int max_first_start = fDaq.num_samples - max_space;

    if(max_first_start < fDaq.init_ped_samples) {
    max_first_start = fDaq.init_ped_samples;
    }

    // Current starting point
    int current_start = fDaq.init_ped_samples + 
                    int((max_first_start - fDaq.init_ped_samples) * rndm(fRng));

    // Random amps and separations
    for(int p = 0; p < num_pulses; p++) {
    int min_amp = fWf.mean_gain / 2;
    int current_amp = min_amp + int((fWf.mean_gain - min_amp) * rndm(fRng));

    pulse_info.push_back({current_start, current_amp});

    int current_separation = fWf.pulse_separation / 2 + 
                                int((fWf.pulse_separation - fWf.pulse_separation / 2.) * rndm(fRng));
    current_start += fWf.pulse_width + current_separation;
    }

    // Build the waveform
    for( int i = 0; i < fDaq.num_samples; i++) {
    Bool_t in_pulse = false;
    Int_t pulse_relative_idx = 0;
    Int_t active_amp = 0;

    for(auto& p : pulse_info) {
        if(i >= p.first && i < p.first + fWf.pulse_width) {
            in_pulse = true;
            pulse_relative_idx = i - p.first;
            active_amp = p.second;
            break;
        }
    }
    if(in_pulse) {
        wave.push_back(min_ped + active_amp/TMath::Power(2, pulse_relative_idx));
    } else {
        wave.push_back(min_ped + (max_ped - min_ped) * rndm(fRng));
    }
    }
    return wave;
}

// PE based waveform generators
// Waveform generator v2 related functions - now the parameters are read from the object state
Double_t waveformGenerator::spePulse(Double_t t, Double_t t0, Double_t amp) const {
    if(t < t0) return 0.0;
    return amp * (std::exp(-(t - t0) / fWf.tau_f) - std::exp(-(t - t0) / fWf.tau_r));
}

Double_t waveformGenerator::squarePulse(Double_t t, Double_t t0, Double_t amp) const {
    Double_t width_ns = fWf.pulse_width * fDaq.sample_rate; // Convert samples -> ns
    if(t < t0 || t > t0 + width_ns) return 0.0;
    return amp;
}

Double_t waveformGenerator::trianglePulse(Double_t t, Double_t t0, Double_t amp) const {
    Double_t width_ns = fWf.pulse_width * fDaq.sample_rate;
    Int_t dt = fDaq.sample_rate;
    t0 = round(t0 / dt) * dt;
    Double_t half_w = width_ns / 2.0;
    if(t < t0 - half_w || t > t0 + half_w) return 0.0;

    if(t < t0) {
        return amp * (t - (t0 - half_w)) / (half_w);
    } else {
        return amp * ((t0 + half_w) - t) / (half_w);
    }
}

std::vector<waveformGenerator::PEHit> waveformGenerator::generatePEHits() const {
    // Distributions for event and after pulse gen
    std::uniform_int_distribution<Int_t> num_pulses_dist(1, fDaq.max_n_pulses);
    std::uniform_real_distribution<Double_t> window_dist(10.0, fDaq.num_samples * fDaq.sample_rate - 50.0);
    std::poisson_distribution<Int_t> pe_per_hit_dist(15); // Avg 15 primary PEs per hit
    std::uniform_real_distribution<Double_t> ap_chance(0.0, 1.0); // Prob for after pulse
    std::normal_distribution<Double_t> ap_time_dist(fWf.ap_delay_mean, fWf.ap_delay_sigma); // After pulse time dist

    std::vector<PEHit> hits;
    Int_t num_pulses = num_pulses_dist(fRng);

    for (Int_t p = 0; p < num_pulses; p++) {
        PEHit hit;
        hit.hitTime = window_dist(fRng);
        Int_t num_pes = pe_per_hit_dist(fRng);

        for (Int_t i = 0; i < num_pes; i++) {
            hit.peTimes.push_back(hit.hitTime);

            // After pulse probability roll
            // if(ap_chance(fRng) < fWf.ap_prob) {
            //     Double_t delayed_time = hit.hitTime + ap_time_dist(fRng);
            //     hit.peTimes.push_back(delayed_time);
            // }
        }
        // Sort PE times within this hit
        std::sort(hit.peTimes.begin(), hit.peTimes.end());
        hits.push_back(hit);
    }
    // Sort hits by arrival time
    std::sort(hits.begin(), hits.end(),
              [](const PEHit& a, const PEHit& b) { return a.hitTime < b.hitTime; });
    return hits;
}

std::vector<Int_t> waveformGenerator::generateWaveform() {
    auto hits = generatePEHits();
    fTruthInfo = {};  // Auto-clear truth info

    // Distributions
    std::normal_distribution<Double_t> tts_dist(0.0, fWf.tts_sigma);
    std::normal_distribution<Double_t> gain_dist(fWf.mean_gain, fWf.gain_sigma);
    std::normal_distribution<Double_t> noise_dist(0.0, fDaq.noise_sigma);

    std::vector<Double_t> analog_wf(fDaq.num_samples, 0.0);

    for(auto& hit : hits) {
        // Per-hit analog contribution for truth extraction
        std::vector<Double_t> hit_contrib(fDaq.num_samples, 0.0);

        for(Double_t pe_time : hit.peTimes) {
            Double_t smeared_time = pe_time + tts_dist(fRng);
            Double_t smeared_gain = gain_dist(fRng);
            if(smeared_gain < 0.0) smeared_gain = 0.0;

            for(Int_t i = 0; i < fDaq.num_samples; i++) {
                Double_t current_time = i * fDaq.sample_rate;
                // Optimization
                if(current_time > smeared_time - 5.0 && current_time < smeared_time + 50) {
                    Double_t val = spePulse(current_time, smeared_time, smeared_gain);
                    hit_contrib[i] += val;
                    analog_wf[i] += val;
                }
            }
        }

        // Extract truth features from per-hit contribution
        // Find peak amplitude and bin
        Double_t peakAmp = 0.0;
        Int_t peakBin = 0;
        for(Int_t i = 0; i < fDaq.num_samples; i++) {
            if(hit_contrib[i] > peakAmp) {
                peakAmp = hit_contrib[i];
                peakBin = i;
            }
        }

        // Integral: sum over non-negligible samples (>1% of peak),
        // converted to ADC counts (pedestal-subtracted)
        Double_t integral = 0.0;
        Double_t threshold = peakAmp * 0.01;
        for(Int_t i = 0; i < fDaq.num_samples; i++) {
            if(hit_contrib[i] > threshold) {
                integral += hit_contrib[i];
            }
        }
        integral /= fDaq.v_lsb;  // mV -> ADC counts

        // Amplitude: convert to ADC with pedestal (matches fSampPulseAmp)
        Double_t ampADC = peakAmp / fDaq.v_lsb + fDaq.pedestal;

        // Time: half-max crossing on rising edge, in 1/64th-sample units
        // (matches analyzer's fSampPulseTime interpolation)
        Double_t vMid = peakAmp / 2.0;
        Double_t timeVal = peakBin * 64.0;  // fallback: peak bin
        for(Int_t i = 0; i < peakBin; i++) {
            if(hit_contrib[i] <= vMid && hit_contrib[i + 1] > vMid) {
                timeVal = 64.0 * i + 64.0 * (vMid - hit_contrib[i]) /
                          (hit_contrib[i + 1] - hit_contrib[i]);
                break;
            }
        }

        fTruthInfo.pulseTime.push_back(timeVal);
        fTruthInfo.pulseAmplitude.push_back(ampADC);
        fTruthInfo.pulseIntegral.push_back(integral);
    }

    // Add noise and digitize
    std::vector<Int_t> adc_wf(fDaq.num_samples, 0);
    for(Int_t i = 0; i < fDaq.num_samples; i++) {
        Double_t noisy_voilage = analog_wf[i] + noise_dist(fRng);
        Int_t adc_count = static_cast<Int_t>(std::floor(noisy_voilage/fDaq.v_lsb) + fDaq.pedestal);
        adc_count = std::max(0, std::min(adc_count, fDaq.max_adc));
        adc_wf[i] = adc_count;
    }
    return adc_wf;
}

std::vector<Int_t> waveformGenerator::squareWaveform() {
    auto hits = generatePEHits();
    fTruthInfo = {};  // Auto-clear truth info

    std::vector<Double_t> square_wf(fDaq.num_samples, 0.0);

    for(auto& hit : hits) {
        // Per-hit analog contribution for truth extraction
        std::vector<Double_t> hit_contrib(fDaq.num_samples, 0.0);

        for(Double_t pe_time : hit.peTimes) {
            for(Int_t i = 0; i < fDaq.num_samples; i++) {
                Double_t current_time = i * fDaq.sample_rate;
                // Optimization
                if(current_time > pe_time - 5.0 && current_time < pe_time + 50) {
                    Double_t val = squarePulse(current_time, pe_time, fWf.mean_gain);
                    hit_contrib[i] += val;
                    square_wf[i] += val;
                }
            }
        }

        // Extract truth features from per-hit contribution
        // Find peak amplitude and bin
        Double_t peakAmp = 0.0;
        Int_t peakBin = 0;
        for(Int_t i = 0; i < fDaq.num_samples; i++) {
            if(hit_contrib[i] > peakAmp) {
                peakAmp = hit_contrib[i];
                peakBin = i;
            }
        }

        // Integral: sum over non-negligible samples (>1% of peak),
        // converted to ADC counts (pedestal-subtracted)
        Double_t integral = 0.0;
        Double_t threshold = peakAmp * 0.01;
        for(Int_t i = 0; i < fDaq.num_samples; i++) {
            if(hit_contrib[i] > threshold) {
                integral += hit_contrib[i];
            }
        }
        integral /= fDaq.v_lsb;  // mV -> ADC counts

        // Amplitude: convert to ADC with pedestal (matches fSampPulseAmp)
        Double_t ampADC = peakAmp / fDaq.v_lsb + fDaq.pedestal;

        // Time: half-max crossing on rising edge, in 1/64th-sample units
        Double_t vMid = peakAmp / 2.0;
        Double_t timeVal = peakBin * 64.0;  // fallback: peak bin
        for(Int_t i = 0; i < peakBin; i++) {
            if(hit_contrib[i] <= vMid && hit_contrib[i + 1] > vMid) {
                timeVal = 64.0 * i + 64.0 * (vMid - hit_contrib[i]) /
                          (hit_contrib[i + 1] - hit_contrib[i]);
                break;
            }
        }

        fTruthInfo.pulseTime.push_back(timeVal);
        fTruthInfo.pulseAmplitude.push_back(ampADC);
        fTruthInfo.pulseIntegral.push_back(integral);
    }

    // Digitize
    std::vector<Int_t> adc_wf(fDaq.num_samples, 0);
    for(Int_t i = 0; i < fDaq.num_samples; i++) {
        Int_t adc_count = static_cast<Int_t>(std::floor(square_wf[i]/fDaq.v_lsb) + fDaq.pedestal);
        adc_count = std::max(0, std::min(adc_count, fDaq.max_adc));
        adc_wf[i] = adc_count;
    }
    return adc_wf;
}

std::vector<Int_t> waveformGenerator::triangleWaveform() {
    auto hits = generatePEHits();
    fTruthInfo = {};  // Auto-clear truth info

    std::vector<Double_t> triangle_wf(fDaq.num_samples, 0.0);

    for(auto& hit : hits) {
        // Per-hit analog contribution for truth extraction
        std::vector<Double_t> hit_contrib(fDaq.num_samples, 0.0);

        for(Double_t pe_time : hit.peTimes) {
            for(Int_t i = 0; i < fDaq.num_samples; i++) {
                Double_t current_time = i * fDaq.sample_rate;
                // Optimization
                if(current_time > pe_time - 5.0 && current_time < pe_time + 50) {
                    Double_t val = trianglePulse(current_time, pe_time, fWf.mean_gain);
                    hit_contrib[i] += val;
                    triangle_wf[i] += val;
                }
            }
        }

        // Extract truth features from per-hit contribution
        // Find peak amplitude and bin
        Double_t peakAmp = 0.0;
        Int_t peakBin = 0;
        for(Int_t i = 0; i < fDaq.num_samples; i++) {
            if(hit_contrib[i] > peakAmp) {
                peakAmp = hit_contrib[i];
                peakBin = i;
            }
        }

        // Integral: sum over non-negligible samples (>1% of peak),
        // converted to ADC counts (pedestal-subtracted)
        Double_t integral = 0.0;
        Double_t threshold = peakAmp * 0.01;
        for(Int_t i = 0; i < fDaq.num_samples; i++) {
            if(hit_contrib[i] > threshold) {
                integral += hit_contrib[i];
            }
        }
        integral /= fDaq.v_lsb;  // mV -> ADC counts

        // Amplitude: convert to ADC with pedestal (matches fSampPulseAmp)
        Double_t ampADC = peakAmp / fDaq.v_lsb + fDaq.pedestal;

        // Time: half-max crossing on rising edge, in 1/64th-sample units
        Double_t vMid = peakAmp / 2.0;
        Double_t timeVal = peakBin * 64.0;  // fallback: peak bin
        for(Int_t i = 0; i < peakBin; i++) {
            if(hit_contrib[i] <= vMid && hit_contrib[i + 1] > vMid) {
                timeVal = 64.0 * i + 64.0 * (vMid - hit_contrib[i]) /
                          (hit_contrib[i + 1] - hit_contrib[i]);
                break;
            }
        }

        fTruthInfo.pulseTime.push_back(timeVal);
        fTruthInfo.pulseAmplitude.push_back(ampADC);
        fTruthInfo.pulseIntegral.push_back(integral);
    }

    // Digitize
    std::vector<Int_t> adc_wf(fDaq.num_samples, 0);
    for(Int_t i = 0; i < fDaq.num_samples; i++) {
        Int_t adc_count = static_cast<Int_t>(std::floor(triangle_wf[i]/fDaq.v_lsb) + fDaq.pedestal);
        adc_count = std::max(0, std::min(adc_count, fDaq.max_adc));
        adc_wf[i] = adc_count;
    }
    return adc_wf;
}