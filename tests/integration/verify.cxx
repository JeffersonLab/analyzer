// Verify results from integration tests
// For Podd 1.7+
//
// Ole Hansen, ole@jlab.org, 10-Nov-2023

#include "TFile.h"
#include "TError.h"
#include "TH1F.h"
#include "TTree.h"
#include "TMath.h"
#include "RunInfo.h"
#include "THaRunBase.h"
#include "THaRunParameters.h"

//using namespace std;

constexpr Double_t ref_eps  = 1e-6;
constexpr Double_t ref_pval = 0.1;

static const char* const here = "verify.cxx";

//_____________________________________________________________________________
bool check(TFile& f, const char* hname, const char* desc, const TH1* tocheck)
{
  // Check if 1D histogram 'tocheck' matches 'hname' in file 'f'

  auto* href = f.Get<TH1>(hname);
  if( !href ) {
    Error(here, "Cannot retrieve reference histogram %s", hname);
    return 0;
  }
  auto pval = tocheck->Chi2Test(href);
  if( pval < ref_pval ) {
    Error(here, "%s data differ = %lf", desc, pval);
    return false;
  }
  Info(here, "%s data match = %lf", desc, pval);
  return true;
}

//_____________________________________________________________________________
int verify( const char* root_file, const char* ref_file )
{
  //------ Open ROOT file
  if( !root_file || !*root_file || !ref_file || !*ref_file ) {
    Error(here, R"(Usage: verify("root_file","ref_file"))");
    return 1;
  }
  // Analyzed data
  TFile anaf(root_file, "READ");
  if( anaf.IsZombie() )
    // Error printed by TFile
    return 2;
  // Reference data
  TFile tstf(ref_file, "READ");
  if( tstf.IsZombie() )
    return 3;

  //------ Retrieve tree, run data, reference histograms
  auto* T = anaf.Get<TTree>("T");
  if( !T ) {
    Error(here, "Cannot load tree T");
    return 4;
  }
  Info(here, "Tree loaded OK");

  auto* run = anaf.Get<THaRunBase>("Run_Data");
  if( !run ) {
    Error(here, "Cannot load Run_Data");
    return 5;
  }
  Info(here, "Run_Data loaded OK");

  auto* par = run->GetParameters();
  if( !par ) {
    Error(here, "Cannot load run parameters");
    return 6;
  }
  Info(here, "Run parameters loaded OK");

  const auto* pRunInfo = tstf.Get<RunInfo>("Run_Info");
  if( !pRunInfo ) {
    Error(here, "Cannot retrieve reference run info");
    return 7;
  }
  const RunInfo& ifo = *pRunInfo;
  Info(here, "Reference run info loaded OK");

  //------ Compare metadata
  if( run->GetNumber() != ifo.num ) {
    Error(here, "Run number mismatch, got %u, expected %u",
          run->GetNumber(), ifo.num);
    return 10;
  }
  Info(here, "Run number matches = %u", ifo.num);

  TDatime rd = run->GetDate();
  if( rd != ifo.date ) {
    Error(here, "Run date mismatch, got %s, expected %s",
          rd.AsString(), ifo.date.AsString());
    return 11;
  }
  Info(here, "Run date matches = %s", ifo.date.AsString());

  if( run->GetDataVersion() != ifo.vers ) {
    Error(here, "Data version mismatch, got %d, expected %d",
          run->GetDataVersion(), ifo.vers);
    return 12;
  }
  Info(here, "Run data version matches = %d", ifo.vers);

  if( run->GetNumAnalyzed() != ifo.nevt ) {
    Error(here, "Number of events mismatch, got %u, expected %u",
          run->GetNumAnalyzed(), ifo.nevt);
    return 13;
  }
  Info(here, "Number of analyzed events matches = %u", ifo.nevt);

  if( TMath::Abs(par->GetBeamE() - ifo.beamE) > ref_eps ) {
    Error(here, "Run parameters: beam energy mismatch, "
                "got %lf, expected %lf", par->GetBeamE(), ifo.beamE);
    return 14;
  }
  Info(here, "Beam energy run parameter matches = %lf", ifo.beamE);

  if( TMath::Abs(par->GetBeamM() - ifo.beamM) > ref_eps ) {
    Error(here, "Run parameters: beam particle mass mismatch, "
                "got %lf, expected %lf", par->GetBeamM(), ifo.beamM);
    return 15;
  }
  Info(here, "Beam particle mass run parameter matches = %lf", ifo.beamM);

  const auto& ps = par->GetPrescales();
  int n_ps = int(sizeof(ifo.ps) / sizeof(*ifo.ps));
  int n_ps2 = ps.GetSize();
  if( n_ps != n_ps2 ) {
    Error(here, "Run parameters: number of prescale factors differs, "
                "got %d, expected %d", n_ps2, n_ps);
    return 16;
  }
  for( int i = 0; i < n_ps; ++i ) {
    if( ps[i] != ifo.ps[i] ) {
      Error(here, "Run parameters: prescale factor[%d] "
                  "mismatch, got %d, expected %d", i, ps[i], ifo.ps[i]);
      return 17;
    }
  }
  TString pstr;
  for( int i = 0; i < n_ps; ++i ) {
    pstr += ifo.ps[i]; if( i+1 != n_ps ) pstr += '/';
  }
  Info(here, "Prescale factors match: %s", pstr.Data());

  //------ Compare reconstructed data
  auto* evlen    = new TH1F("evlen",    "fEvtHdr.fEvtLen", 1000, 0, 5000);
  auto* s1lac3   = new TH1F("s1lac3",   "L.s1.la_c[3]",    2000, 0, 1000);
  auto* s1rac3   = new TH1F("s1rac3",   "L.s1.ra_c[3]",    2000, 0, 1000);
  auto* s2yt3    = new TH1F("s2yt3",    "L.s2.y_t[3]",     500, -0.5, 0.5);
  auto* cersum   = new TH1F("cersum",   "L.cer.asum_c",    500, -1000, 9000);
  auto* v1time   = new TH1F("v1time",   "L.vdc.v1.time",   800,-50e-9,350e-9);
  auto* v1dist   = new TH1F("v1dist",   "L.vdc.v1.dist",   500,-2e-3,2e-2);
  auto* u1nclust = new TH1F("u1nclust", "L.vdc.u1.nclust", 12,0,12);
  auto* u2clpos  = new TH1F("u2clpos",  "L.vdc.u2.clpos",  500,-.75,.75);
  auto* trn      = new TH1F("trn",      "L.tr.n",          10, 0, 10);
  auto* dp       = new TH1F("dp",       "L.tr.tg_dp",      1000,0,12e-3);
  auto* s1trdx   = new TH1F("s1trdx",   "L.s1.trdx",       500,-.5,.5);
  auto* goldph   = new TH1F("goldph",   "L.gold.ph",       500,-.05,.05);
  auto* goldth   = new TH1F("goldth",   "L.gold.th",       500,-.1,.1);
  auto* ek_angle = new TH1F("ek_angle", "L.ekine.angle",   500,2e-2,20e-2);
  auto* ek_W2    = new TH1F("ek_W2",    "L.ekine.W2",      4000,124,128);
  auto* vxz      = new TH1F("vxz",      "L.vx.z",          500, -0.5, 0.5);

  T->Draw("fEvtHdr.fEvtLen>>evlen",    "", "goff");
  T->Draw("L.s1.la_c[3]>>s1lac3",      "", "goff");
  T->Draw("L.s1.ra_c[3]>>s1rac3",      "", "goff");
  T->Draw("L.s2.y_t[3]>>s2yt3",        "", "goff");
  T->Draw("L.cer.asum_c>>cersum",      "", "goff");
  T->Draw("L.vdc.v1.time>>v1time",     "", "goff");
  T->Draw("L.vdc.v1.dist>>v1dist",     "", "goff");
  T->Draw("L.vdc.u1.nclust>>u1nclust", "", "goff");
  T->Draw("L.vdc.u2.clpos>>u2clpos",   "", "goff");
  T->Draw("L.tr.n>>trn",               "", "goff");
  T->Draw("L.tr.tg_dp>>dp",            "", "goff");
  T->Draw("L.s1.trdx>>s1trdx",         "L.s1.trdx<100", "goff");
  T->Draw("L.gold.ph>>goldph",         "", "goff");
  T->Draw("L.gold.th>>goldth",         "", "goff");
  T->Draw("L.ekine.angle>>ek_angle",   "", "goff");
  T->Draw("L.ekine.W2>>ek_W2",         "", "goff");
  T->Draw("L.vx.z>>vxz",               "", "goff");

  if( !check(tstf, "h_evlen", "Event length (fEvtHdr.fEvtLen)", evlen) )
    return 20;
  if( !check(tstf, "h_s1lac3",
             "S1 calibrated ADC left PMT paddle 3 (L.s1.la_c[3])", s1lac3) )
    return 21;
  if( !check(tstf, "h_s1rac3",
             "S1 calibrated ADC right PMT paddle 3 (L.s1.ra_c[3])", s1rac3) )
    return 22;
  if( !check(tstf, "h_s2yt3",
             "S2 y-pos from PMT TDC time diff paddle 3 (L.s2.y_t[3])", s2yt3) )
    return 23;
  if( !check(tstf, "h_cer_asumc",
             "Cherenkov calibrated ADC amplitude sum (L.cer.asum_c)", cersum) )
    return 24;
  if( !check(tstf, "h_v1time",
             "VDC V1 plane calibrated drift time (L.vdc.v1.time)", v1time) )
    return 25;
  if( !check(tstf, "h_v1dist",
             "VDC V1 plane drift distance (L.vdc.v1.dist)", v1dist) )
    return 26;
  if( !check(tstf, "h_u1nclust",
             "VDC U2 plane number of clusters (L.vdc.u2.nclust)", u1nclust) )
    return 27;
  if( !check(tstf, "h_u2clpos",
             "VDC U1 plane cluster position (L.vdc.u1.clpos)", u2clpos) )
    return 28;
  if( !check(tstf, "h_trn", "Number of reconstructed tracks (L.tr.n)", trn) )
    return 29;
  if( !check(tstf, "h_dp",
             "Momentum delta at target per track (L.tr.tg_dp)", dp) )
    return 30;
  if( !check(tstf, "h_s1trdx",
             "Track crossing x in scintillator S1 (L.s1.trdx)", s1trdx) )
    return 31;
  if( !check(tstf, "h_goldph",
             "Golden track in-plane angle (L.gold.ph)", goldph) )
    return 32;
  if( !check(tstf, "h_goldth",
             "Golden track out-of-plane angle (L.gold.th)", goldth) )
    return 33;
  if( !check(tstf, "h_ek_angle",
             "Electron scattering angle wrt beam (L.ekine.angle)", ek_angle) )
    return 34;
  if( !check(tstf, "h_ek_W2",
             "Invariant mass square (L.ekine.W2)", ek_W2) )
    return 35;
  if( !check(tstf, "h_vxz", "Vertex z (L.vx.z)", vxz) )
    return 36;

  anaf.Close();
  tstf.Close();

  return 0;
}