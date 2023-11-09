// Replay script for integration tests
// For Podd 1.7+
//
// Ole Hansen, ole@jlab.org, 08-Nov-2023

#include "THaRun.h"
#include "THaAnalyzer.h"
#include "THaGlobals.h"
#include "TSystem.h"
#include "TError.h"
#include "Textvars.h"
#include "THaHRS.h"
#include "THaVDC.h"
#include "THaScintillator.h"
#include "THaCherenkov.h"
#include "THaIdealBeam.h"
#include "THaGoldenTrack.h"
#include "THaPrimaryKine.h"
#include "THaReactionPoint.h"

struct IgnoreMessages {
  explicit IgnoreMessages( Int_t level ) : orig_level_{gErrorIgnoreLevel}
  { gErrorIgnoreLevel = level; }
  ~IgnoreMessages() { gErrorIgnoreLevel = orig_level_; }
  Int_t orig_level_;
};

int replay( const char* run_file, const char* out_file = nullptr, int nev = -1 )
{
  static const char* const here = "replay.cxx";
  static const char* const run_description = "Podd integration test data";

  { // Suppress noisy info and warning messages
    IgnoreMessages setignore(kError);

    if( !run_file || !*run_file ) {
      Error(here, "Must specify data file");
      return 1;
    }
    if( gSystem->AccessPathName(run_file) ) {
      Error(here, "Cannot find %s", run_file);
      return 2;
    }
    if( !out_file || !*out_file ) {
      out_file = "/tmp/podd_test.root";
      Info(here, "Using default output file %s", out_file);
    } else
      Info(here, "Writing output to %s", out_file);
  }

  Info(here, "Replaying %s", run_file);

  IgnoreMessages setignore(kError);

  auto* run = new THaRun(run_file, run_description);
  if( nev > 0 )
    run->SetLastEvent(nev);

  // Assume LHRS data
  const char* const arm = "L";
  gHaTextvars->Set("arm", arm);

  auto* hrs = new THaHRS(arm, Form("%sHRS", arm));
  hrs->AddDetector(new THaVDC("vdc", Form("%sHRS Vertical Drift Chambers", arm)));
  hrs->AddDetector(new THaScintillator("s1", Form("%sHRS scintillator 1", arm)));
  hrs->AddDetector(new THaScintillator("s2", Form("%sHRS scintillator 2", arm)));
  hrs->AddDetector(new THaCherenkov("cer", Form("%sHRS gas Cherenkov", arm)));
  gHaApps->Add(hrs);

  gHaApps->Add(new THaIdealBeam("IB", "Ideal beam"));

  gHaPhysics->Add(new THaGoldenTrack(Form("%s.gold", arm),
                                     Form("%sHRS golden track", arm), arm));

  // Assume a carbon-12 target (12 AMU)
  gHaPhysics->Add(new THaPrimaryKine(Form("%s.ekine", arm),
                                     Form("%sHRS electron kinematics", arm),
                                     arm, 0.511e-3, 12 * 0.9315));

  gHaPhysics->Add(new THaReactionPoint(Form("%s.vx", arm),
                                       Form("Vertex %s", arm),
                                       arm, "IB"));

  auto* analyzer = new THaAnalyzer;

  const char* out_dir = gSystem->DirName(out_file);
  gSystem->mkdir(out_dir, true);
  TString summaryfile(out_file);
  if( summaryfile.EndsWith(".root", TString::kIgnoreCase))
    summaryfile.Replace(summaryfile.Length() - 4, 4, "log");
  else
    summaryfile.Append(".log");

  analyzer->SetOutFile( out_file );
  analyzer->SetCutFile( "replay.cdef" );
  analyzer->SetOdefFile( "replay.odef" );
  analyzer->SetVerbosity(gErrorIgnoreLevel < kError ? 2 : 0);
  analyzer->EnableBenchmarks(false);
  analyzer->EnableSlowControl(false);
  analyzer->EnableHelicity(false);
  analyzer->SetSummaryFile(summaryfile);

  analyzer->Process(run);

  analyzer->Close();
  delete analyzer;
  gHaPhysics->Delete();
  gHaApps->Delete();
  delete run;

  return 0;
}
