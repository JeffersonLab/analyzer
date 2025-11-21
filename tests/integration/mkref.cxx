// Save reference data (run metadata, replay histograms) extracted from
// replay output to a small summary ROOT file for Podd integration tests.
//
// Ole Hansen, ole@jlab.org, 13-Nov-2023

#include "TFile.h"
#include "TError.h"
#include "TH1F.h"
#include "TTree.h"
#include "TStyle.h"
#include "TPad.h"
#include "TROOT.h"
#include "THaRunBase.h"
#include "THaRunParameters.h"
#include "RunInfo.h"

static const char* const here = "mkref.cxx";

//_____________________________________________________________________________
int mkref( const char* root_file, const char* ref_file )
{
  gROOT->SetBatch();

  //------ Open ROOT files
  if( !root_file || !*root_file || !ref_file || !*ref_file ) {
    Error(here, R"(Usage: verify("root_file","ref_file"))");
    return 1;
  }

  // Analyzed data
  TFile in_f(root_file, "READ");
  if( in_f.IsZombie() )
    return 2;        // Error printed by TFile

  auto* T = in_f.Get<TTree>("T");
  if( !T ) {
    Error(here, "Cannot load tree T");
    return 2;
  }
  Info(here, "Tree loaded OK");

  auto* run = in_f.Get<THaRunBase>("Run_Data");
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

  // Reference data (output)
  TFile out_f(ref_file, "CREATE", "g2p run 3132 reference data");
  if( out_f.IsZombie() )
    return 3;
  // Use maximal compression
  out_f.SetCompressionAlgorithm(ROOT::ECompressionAlgorithm::kLZMA);
  out_f.SetCompressionLevel(9);

  Int_t len = 0;  // Total number of bytes written

  // Fill and write run metadata
  RunInfo ifo;
  ifo.num = run->GetNumber();
  ifo.date = run->GetDate();
  ifo.nevt = run->GetNumAnalyzed();
  ifo.vers = run->GetDataVersion();
  ifo.beamE = par->GetBeamE();
  ifo.beamM = par->GetBeamM();
  Int_t n = par->GetPrescales().GetSize();
  constexpr auto n_ps = Int_t(sizeof(ifo.ps)/sizeof(*ifo.ps));
  if( n > n_ps ) n = n_ps;
  for( Int_t i = 0; i < n; ++i )
    ifo.ps[i] = par->GetPrescales()[i];
  len += out_f.WriteObjectAny(&ifo, "RunInfo", "Run_Info");
  if( len <= 0 ) {
    Error(here, "Cannot write RunInfo. Something odd is going on.");
    return 7;
  }
  Info(here, "RunInfo written OK");

  // Fill and write select replay results
  auto* evlen    = new TH1F("h_evlen",    "fEvtHdr.fEvtLen", 1000, 0, 5000);
  auto* s1lac3   = new TH1F("h_s1lac3",   "L.s1.la_c[3]",    2000, 0, 1000);
  auto* s1rac3   = new TH1F("h_s1rac3",   "L.s1.ra_c[3]",    2000, 0, 1000);
  auto* s2yt3    = new TH1F("h_s2yt3",    "L.s2.y_t[3]",     500, -0.5, 0.5);
  auto* cersum   = new TH1F("h_cer_asumc","L.cer.asum_c",    500, -1000, 9000);
  auto* v1time   = new TH1F("h_v1time",   "L.vdc.v1.time",   800,-50e-9,350e-9);
  auto* v1dist   = new TH1F("h_v1dist",   "L.vdc.v1.dist",   500,-2e-3,2e-2);
  auto* u1nclust = new TH1F("h_u1nclust", "L.vdc.u1.nclust", 12,0,12);
  auto* u2clpos  = new TH1F("h_u2clpos",  "L.vdc.u2.clpos",  500,-.75,.75);
  auto* trn      = new TH1F("h_trn",      "L.tr.n",          10, 0, 10);
  auto* dp       = new TH1F("h_dp",       "L.tr.tg_dp",      1000,0,12e-3);
  auto* s1trdx   = new TH1F("h_s1trdx",   "L.s1.trdx",       500,-.5,.5);
  auto* goldph   = new TH1F("h_goldph",   "L.gold.ph",       500,-.05,.05);
  auto* goldth   = new TH1F("h_goldth",   "L.gold.th",       500,-.1,.1);
  auto* ek_angle = new TH1F("h_ek_angle", "L.ekine.angle",   500,2e-2,20e-2);
  auto* ek_W2    = new TH1F("h_ek_W2",    "L.ekine.W2",      4000,124,128);
  auto* vxz      = new TH1F("h_vxz",      "L.vx.z",          500, -0.5, 0.5);

  T->Draw("fEvtHdr.fEvtLen>>h_evlen",    "", "goff");
  T->Draw("L.s1.la_c[3]>>h_s1lac3",      "", "goff");
  T->Draw("L.s1.ra_c[3]>>h_s1rac3",      "", "goff");
  T->Draw("L.s2.y_t[3]>>h_s2yt3",        "", "goff");
  T->Draw("L.cer.asum_c>>h_cer_asumc",   "", "goff");
  T->Draw("L.vdc.v1.time>>h_v1time",     "", "goff");
  T->Draw("L.vdc.v1.dist>>h_v1dist",     "", "goff");
  T->Draw("L.vdc.u1.nclust>>h_u1nclust", "", "goff");
  T->Draw("L.vdc.u2.clpos>>h_u2clpos",   "", "goff");
  T->Draw("L.tr.n>>h_trn",               "", "goff");
  T->Draw("L.tr.tg_dp>>h_dp",            "", "goff");
  T->Draw("L.s1.trdx>>h_s1trdx",         "L.s1.trdx<100", "goff");
  T->Draw("L.gold.ph>>h_goldph",         "", "goff");
  T->Draw("L.gold.th>>h_goldth",         "", "goff");
  T->Draw("L.ekine.angle>>h_ek_angle",   "", "goff");
  T->Draw("L.ekine.W2>>h_ek_W2",         "", "goff");
  T->Draw("L.vx.z>>h_vxz",               "", "goff");

  // This block is needed to save the histogram statistics box.
  // Not strictly needed, but useful.
  gStyle->SetOptStat(111111);
  evlen->Draw();     gPad->Update();
  s1lac3->Draw();    gPad->Update();
  s1rac3->Draw();    gPad->Update();
  s2yt3->Draw();     gPad->Update();
  cersum->Draw();    gPad->Update();
  v1time->Draw();    gPad->Update();
  v1dist->Draw();    gPad->Update();
  u1nclust->Draw();  gPad->Update();
  u2clpos->Draw();   gPad->Update();
  trn->Draw();       gPad->Update();
  dp->Draw();        gPad->Update();
  s1trdx->Draw();    gPad->Update();
  goldph->Draw();    gPad->Update();
  goldth->Draw();    gPad->Update();
  ek_angle->Draw();  gPad->Update();
  ek_W2->Draw();     gPad->Update();
  vxz->Draw();       gPad->Update();

  Int_t len_prev = len;
  len += evlen->Write();
  len += s1lac3->Write();
  len += s1rac3->Write();
  len += s2yt3->Write();
  len += cersum->Write();
  len += v1time->Write();
  len += v1dist->Write();
  len += u1nclust->Write();
  len += u2clpos->Write();
  len += trn->Write();
  len += dp->Write();
  len += s1trdx->Write();
  len += goldph->Write();
  len += goldth->Write();
  len += ek_angle->Write();
  len += ek_W2->Write();
  len += vxz->Write();

  if( len > len_prev )
    Info(here,"Histograms written OK");
  else
    Error(here, "No histogram data written. Something is wrong.");

  out_f.Close();
  in_f.Close();

  Info(here, "Successfully saved %d bytes of payload data to %s",
       len, ref_file);

  // Pop down the now-blank default canvas
  auto* c = gROOT->FindObject("c1");
  delete c;

  return 0;
}