#include "../Podd/FadcStandaloneAnalyzer.h"
#include "RtypesCore.h"
#include "waveformGenerator.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Helper: run AnalyzeWithD2 and write results + truth + errors to the stream
static void writeAnalysisResults(std::ofstream &out, const std::string &label,
                                 const std::vector<Int_t> &rawWaveform,
                                 const TruthPulseInfo &truth,
                                 FadcStandaloneAnalyzer &analyzer,
                                 const TemplateAnalysisConfig &tmplCfg) {
  // Convert to Double_t for the analyzer
  std::vector<Double_t> samples(rawWaveform.begin(), rawWaveform.end());

  FADCPulseWF result = analyzer.AnalyzeWithEMG(samples, tmplCfg);

  out << "  --- " << label << " ---\n";

  // Analyzer results
  out << "  Analyzer: pedestal = " << std::fixed << std::setprecision(2)
      << result.pedestal << ", nPulses = " << result.nPulses << "\n";

  for (size_t p = 0; p < result.pulses.size(); p++) {
    const auto &pulse = result.pulses[p];
    out << "    Pulse " << p << ":"
        << " valid=" << pulse.valid << " timeSample=" << std::setprecision(3)
        << pulse.timeSample << " timeHalfMax=" << pulse.timeHalfMax
        << " amplitude=" << std::setprecision(2) << pulse.amplitude
        << " integral=" << pulse.integral << " baseline=" << pulse.baseline
        << " chi2ndf=" << std::setprecision(4) << pulse.chi2ndf
        << " fittedSigma=" << std::setprecision(3) << pulse.fittedSigma
        << " fittedTau=" << pulse.fittedTau << " fitStage=" << pulse.fitStage
        << "\n";
  }

  // Truth info
  out << "  Truth: nPulses = " << truth.pulseIntegral.size() << "\n";
  for (size_t t = 0; t < truth.pulseIntegral.size(); t++) {
    out << "    Pulse " << t << ":"
        << " integral=" << std::setprecision(2) << truth.pulseIntegral[t]
        << " amplitude=" << truth.pulseAmplitude[t]
        << " time=" << std::setprecision(3) << truth.pulseTime[t] << "\n";
  }

  // Error checks
  if (result.nPulses < 1 && !truth.pulseIntegral.empty()) {
    out << "  ERROR: No pulses found by analyzer, expected "
        << truth.pulseIntegral.size() << "\n";
  } else if (static_cast<size_t>(result.nPulses) < truth.pulseIntegral.size()) {
    out << "  ERROR: Missed pulses — found " << result.nPulses << ", expected "
        << truth.pulseIntegral.size() << "\n";
  } else if (static_cast<size_t>(result.nPulses) > truth.pulseIntegral.size()) {
    out << "  WARNING: Extra pulses — found " << result.nPulses << ", expected "
        << truth.pulseIntegral.size() << "\n";
  }

  out << "\n";
}

TEST_CASE("Waveform Cases to Text File", "[waveform_cases][text_output]") {

  // ── Output file ────────────────────────────────────────────────────
  std::ofstream out("waveform_cases_results.txt");
  REQUIRE(out.is_open());

  // ── Shared analyzer & template config ──────────────────────────────
  FadcStandaloneAnalyzer analyzer;
  FADCConfig config = {4, 50, 2, 5, 2, 12};
  analyzer.setConfig(config);

  TemplateAnalysisConfig tmplCfg;
  tmplCfg.fSmoothHalfWidth = 1;
  tmplCfg.fD2ThresholdNSigma = 5.0;
  tmplCfg.fD2MinPeakSep = 1;
  tmplCfg.fAmpCutADC = 5.0;
  tmplCfg.fFitMargin = 5;
  tmplCfg.fMaxBaselineAbs = 8.0;
  tmplCfg.fScanHalf = 2.0;

  // ── Shared digitizer defaults ──────────────────────────────────────
  DigitizerParams daq;
  daq.noise_sigma = 1.5;
  daq.num_samples = 74;

  // ══════════════════════════════════════════════════════════════════
  //  CASE 1 : Empty waveform (flat pedestal, no noise, no pulses)
  // ══════════════════════════════════════════════════════════════════
  {
    out << "======================================================\n";
    out << "  CASE 1 : Empty Waveform (flat pedestal)\n";
    out << "======================================================\n\n";

    std::vector<Int_t> emptyWf(daq.num_samples,
                               static_cast<Int_t>(daq.pedestal));
    TruthPulseInfo emptyTruth;

    writeAnalysisResults(out, "Empty (flat pedestal)", emptyWf, emptyTruth,
                         analyzer, tmplCfg);

    out << "======================================================\n\n";
  }

  // ══════════════════════════════════════════════════════════════════
  //  CASE 2 : Just noise (noiseWaveform)
  // ══════════════════════════════════════════════════════════════════
  {
    out << "======================================================\n";
    out << "  CASE 2 : Just Noise Waveform\n";
    out << "======================================================\n\n";

    waveformGenerator genNoise;
    genNoise.setDigitizerParams(daq);

    std::vector<Int_t> noiseWf = genNoise.noiseWaveform();
    TruthPulseInfo noiseTruth;

    writeAnalysisResults(out, "Noise only", noiseWf, noiseTruth, analyzer,
                         tmplCfg);

    out << "======================================================\n\n";
  }

  // ══════════════════════════════════════════════════════════════════
  //  CASE 3 : 10 single pulses (sigma_g=2.6, tau=7, no secondary)
  // ══════════════════════════════════════════════════════════════════
  {
    out << "======================================================\n";
    out << "  CASE 3 : 10 Single Pulses (sigma_g=2.6, tau=7)\n";
    out << "======================================================\n\n";

    waveformGenerator gen;
    DigitizerParams daqSingle = daq;
    daqSingle.max_n_pulses = 1;

    WaveformParams wf;
    wf.sigma_g = 2.6;
    wf.tau = 7.0;
    wf.sp_enable = false;

    gen.setDigitizerParams(daqSingle);
    gen.setDetectorParams(wf);

    for (int i = 0; i < 10; i++) {
      std::vector<Int_t> waveform = gen.generateWaveform(PulseShape::SPE);
      const TruthPulseInfo &truth = gen.getTruthInfo();

      writeAnalysisResults(out, "Single pulse #" + std::to_string(i), waveform,
                           truth, analyzer, tmplCfg);
    }

    out << "======================================================\n\n";
  }

  // ══════════════════════════════════════════════════════════════════
  //  CASE 4 : 10 pulses with sp_enable (sp_delay=35,
  //           sigma_g=2.6, tau=7)
  // ══════════════════════════════════════════════════════════════════
  {
    out << "======================================================\n";
    out << "  CASE 4 : 10 Pulses with Secondary Pulse\n";
    out << "           (sigma_g=2.6, tau=7, sp_delay=35)\n";
    out << "======================================================\n\n";

    waveformGenerator gen;
    DigitizerParams daqSP = daq;
    daqSP.max_n_pulses = 1;

    WaveformParams wf;
    wf.sigma_g = 2.6;
    wf.tau = 7.0;
    wf.sp_enable = true;
    wf.sp_delay = 35.0;

    gen.setDigitizerParams(daqSP);
    gen.setDetectorParams(wf);

    for (int i = 0; i < 10; i++) {
      std::vector<Int_t> waveform = gen.generateWaveform(PulseShape::SPE);
      const TruthPulseInfo &truth = gen.getTruthInfo();

      writeAnalysisResults(out, "SP pulse #" + std::to_string(i), waveform,
                           truth, analyzer, tmplCfg);
    }

    out << "======================================================\n\n";
  }

  out.close();
  std::cout << "\nResults written to: waveform_cases_results.txt\n";
}
