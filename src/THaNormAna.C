//*-- Author :  R. Michaels, May 2004
//
//////////////////////////////////////////////////////////////////////////
//
// THaNormAna
//
// Code to summarize the information needed 
// to normalize an experiment and obtain a 
// cross section, including
//
// 1. Prescale factors
// 2. Calibration of charge monitor
// 3. Integrated Charge of a run
// 4. Livetime by trigger type, by helicity, and 
//    an approximate average livetime.
// 5. Number of triggers from bit pattern.
//
// This class requires scalers to be enabled
// in the analyzer.  For scalers, see 
// http://hallaweb.jlab.org/equipment/daq/THaScaler.html
//
// At present the run summary is just a printout,
// eventually one could write to a database.
// This needs some work (hopefully a student will do it).
// In addition, some global variables are
// registered for possible Tree analysis, e.g.
// cutting on helicity-correlated deadtime.
//
// How to use this class:
// 
// 1. Do something like this in the analysis script:
//      THaNormAna* norm = 
//        new THaNormAna("N","Normalization Analysis");
//      gHaPhysics->Add(norm);
// 
// 2. Add variables N.* to the output tree.
//
// 3. THaNormAna will produce an end-of-run printout.
//
// 4. Calibrate scaler BCMs versus EPICS (assumes EPICS
//    is accurate & reliable) as follows: 
//    First, before analyzing a run
//       normana->DoBcmCalib().
//    Do a CODA run with several beam currents, staying at
//    each current setting for 5 minutes.
//    Look at N.bcmu3 vs fEvtHdr.fEvtNum to decide cut 
//    intervals of stable beam, and analyze again after
//       normana->SetEventInt(Int_t *event)
//    (see method for documentation).
//    Fit the 2D histograms of bcm vs EPICS in a macro 
//    to get the parameters.   
//
//////////////////////////////////////////////////////////////////////////

//#define WITH_DEBUG 1

#include "THaNormAna.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "THaAnalyzer.h"
#include "THaScalerGroup.h"
#include "THaString.h"
#include "TList.h"
#include "THaEvData.h"
#include "TDatime.h"
#include "THaScaler.h"
#include "TH1.h"
#include "TH2.h"
#include "VarDef.h"
#include <fstream>
#include <iostream>

using namespace std;


//_____________________________________________________________________________
THaNormAna::THaNormAna( const char* name, const char* descript ) :
  THaPhysicsModule( name, descript )
{
  fSetPrescale = kFALSE;
  fHelEnable = kFALSE;
  fDoBcmCalib = kFALSE;
  myscaler = 0;
  normdata = new BNormData();
  // There are 12 triggers
  nhit = new Int_t[12];
  // There are 6 hits possible on a TDC channel
  tdcdata = new Int_t[6*12];
  alive = 0;
  hpos_alive = 0;
  hneg_alive = 0;
  eventint = new Int_t[fgMaxEvInt];
  memset(eventint, 0, fgMaxEvInt*sizeof(Int_t));
}

//_____________________________________________________________________________
THaNormAna::~THaNormAna()
{

  delete normdata;
  delete [] nhit;
  delete [] tdcdata;
  delete [] eventint;
  SetupRawData( NULL, kDelete ); 

}

//_____________________________________________________________________________
Int_t THaNormAna::SetupRawData( const TDatime* run_time, EMode mode )
{
  Int_t retval = 0;

  RVarDef vars[] = {
    { "evtypebits", "event type bit pattern",      "evtypebits" },  
    { "bcmu3",   "bcm upstream with x3 gain",      "bcmu3" },
    { "alive",   "approx average livetime from scaler", "alive" },  
    { "hpos_alive", "helicity positive livetime", "hpos_alive" },  
    { "hneg_alive", "helicity negative livetime", "hneg_alive" },  

    { 0 }
  };

  if( mode != kDefine || !fIsSetup )
    retval = DefineVarsFromList( vars, mode );

  fIsSetup = ( mode == kDefine );
  
  return retval;
}

//_____________________________________________________________________________
Int_t THaNormAna::End( THaRunBase* run ) 
{

  PrintSummary();
  WriteHist();
  return 0;
}


//_____________________________________________________________________________
Int_t THaNormAna::PrintSummary() const
{
// Print the summary.  Eventually want this to go to a
// database or a file.

  if (myscaler == 0) {
    cout << "Sorry, no scalers.  Therefore THaNormAna ";
    cout << "failed to work."<<endl;
    return 0;
  }

  cout << "Trigger      Prescale      Num evt     Num in      Livetime"<<endl;
  cout << "             Factor        CODA file   Scalers"<<endl;

  Int_t nhel=1;
  if (fHelEnable) nhel=3; 
  for (int ihel = 0; ihel < nhel; ihel++) {
    Int_t jhel = normdata->InvHel(ihel);  // jhel = 0, -1, +1
    if (fHelEnable) {
      if (jhel==0) {
        cout << "\nIrrespective of helicity "<<endl;
      } else {
        cout << "\nFor Helicity =  "<<jhel<<endl<<endl;
      }
    }
    for (int itrig = 0; itrig < 12; itrig++) {
// Again be careful about indices.  E.g. GetTrig starts at itrig=1.
      cout << "  "<<itrig+1<<"      "<<normdata->GetPrescale(itrig)<<"      "<<normdata->GetTrigCount(itrig,jhel)<<"       "<<myscaler->GetTrig(jhel,itrig+1)<<"       "<<normdata->GetLive(itrig,jhel)<<endl;

    }
    cout << "\nNumber of events = "<<normdata->GetEvCount(jhel)<<endl;
    cout << "Run-averaged livetime from scalers = ";
    cout << normdata->GetAvgLive(jhel)<<endl;
    cout << "Correction for non-synch of scalers = ";
    cout << normdata->GetCorrfact(jhel)<<endl;
  }

// This repeats what is in THaScaler::PrintSummary()
  printf("\n ----------------   Scaler Summary   ---------------- \n");
  Double_t clockrate = 1024;
  Double_t time_sec = myscaler->GetPulser("clock")/clockrate;
  if (time_sec == 0) {
     cout << "THaScaler: WARNING:  Time of run = ZERO (\?\?)\n"<<endl;
  } else {
   Double_t time_min = time_sec/60;  
   Double_t curr_u1  = ((Double_t)myscaler->GetBcm("bcm_u1")/time_sec - off_u1)/calib_u1;
   Double_t curr_u3  = ((Double_t)myscaler->GetBcm("bcm_u3")/time_sec - off_u3)/calib_u3;
   Double_t curr_u10 = ((Double_t)myscaler->GetBcm("bcm_u10")/time_sec - off_u10)/calib_u10;
   Double_t curr_d1  = ((Double_t)myscaler->GetBcm("bcm_d1")/time_sec - off_d1)/calib_d1;
   Double_t curr_d3  = ((Double_t)myscaler->GetBcm("bcm_d3")/time_sec - off_d3)/calib_d3;
   Double_t curr_d10 = ((Double_t)myscaler->GetBcm("bcm_d10")/time_sec - off_d10)/calib_d10;
   printf("Time of run  %7.2f min \n",time_min);
   printf("Triggers:     1 = %d    2 = %d    3 = %d   4 = %d    5 = %d\n",
         myscaler->GetTrig(1),myscaler->GetTrig(2),myscaler->GetTrig(3),myscaler->GetTrig(4),myscaler->GetTrig(5));
   printf("Accepted triggers:   %d \n",myscaler->GetNormData(0,"TS-accept"));
   printf("Accepted triggers by helicity:    (-) = %d    (+) = %d\n",
         myscaler->GetNormData(-1,"TS-accept"),myscaler->GetNormData(1,"TS-accept"));
   printf("Charge Monitors  (Micro Coulombs)\n");
   printf("Upstream BCM   gain x1 %8.2f     x3 %8.2f     x10 %8.2f\n",
    curr_u1*time_sec,curr_u3*time_sec,curr_u10*time_sec);
   printf("Downstream BCM   gain x1 %8.2f     x3 %8.2f     x10 %8.2f\n",
     curr_d1*time_sec,curr_d3*time_sec,curr_d10*time_sec);
  }

  return 1;
}

//_____________________________________________________________________________
  void THaNormAna::WriteHist()
{
  for (UInt_t i = 0; i < hist.size(); i++) hist[i]->Write();
}

//_____________________________________________________________________________
  void THaNormAna::BookHist()
{
// TDCs used for trigger latch pattern.  Used to tune cuts
// that define "simultaneity" of triggers.  Optionally fill
// histograms for BCM calibration (scalers versus EPICS).
  
  Int_t nbin = 100;
  Float_t xlo = 0;
  Float_t xhi = 4000;
  char cname[50],ctitle[50];
  for (int i = 0; i < 12; i++) {
    sprintf(cname,"Tlatch%d",i+1);
    sprintf(ctitle,"Trigger Latch TDC %d",i+1);
    hist.push_back(new TH1F(cname,ctitle,nbin,xlo,xhi));
  }
  if (fDoBcmCalib) {
// Fit these histograms to get calibration factors.
    hist.push_back(new TH2F("bcmu1","bcm u1 vs hac_bcm_average",
			    200,10,120,200,10000,150000));
    hist.push_back(new TH2F("bcmu3","bcm u3 vs hac_bcm_average",
			    200,10,120,200,40000,500000));
    hist.push_back(new TH2F("bcmu10","bcm u10 vs hac_bcm_average",
			    200,10,120,200,100000,1200000));
    hist.push_back(new TH2F("bcmd1","bcm d1 vs hac_bcm_average",
			    200,10,120,200,10000,150000));
    hist.push_back(new TH2F("bcmd3","bcm d3 vs hac_bcm_average",
			    200,10,120,200,40000,500000));
    hist.push_back(new TH2F("bcmd10","bcm d10 vs hac_bcm_average",
			    200,10,120,200,100000,1200000));
   }
}


//_____________________________________________________________________________
THaAnalysisObject::EStatus THaNormAna::Init( const TDatime& run_time ) 
{

  fStatus = kNotinit;
  MakePrefix();
 
  THaAnalyzer* theAnalyzer = THaAnalyzer::GetInstance();
  TList* scalerList = theAnalyzer->GetScalers();
  TIter next(scalerList);
  while( THaScalerGroup* tscalgrp = static_cast<THaScalerGroup*>( next() )) {
    THaScaler *scaler = tscalgrp->GetScalerObj();
    THaString mybank("Left");
    if (mybank.CmpNoCase(scaler->GetName()) == 0) {
         myscaler = scaler; 
         break;
    }
  }
  
#ifdef TRY1
  // Dont want this.  It illustrates how to use a
  // private THaScaler to process the data, but at 
  // present neither THaApparatus::Decode nor
  // a THaPhysicsModule::Process receive event type 140 
  // However, its probably not too bad an idea to use
  // the THaScaler from THaAnalyzer (see above) anyway.
  myscaler = new THaScaler("Left");
  if (myscaler) {
    if( myscaler->Init(run_time) == -1) {
        cout << "Error init scalers "<<endl;
        myscaler = 0;
    }
  }
#endif

  BookHist();

// Calibration of BCMs.  Probably needs adjustment.
  calib_u1 = 1345;  calib_u3 = 4114;  calib_u10 = 12515; 
  calib_d1 = 1303;  calib_d3 = 4034;  calib_d10 = 12728; 
  off_u1 = 92.07;   off_u3 = 167.06;  off_u10 = 102.62;  
  off_d1 = 72.19;   off_d3 = 81.08;   off_d10 = 199.51;


  return fStatus = static_cast<EStatus>( SetupRawData( &run_time ) );


}



//_____________________________________________________________________________
Int_t THaNormAna::Process(const THaEvData& evdata)
{
// Process the data:  Get prescale factors, get trigger
// latch pattern.  Process scalers.  All sorted by helicity
// if helicity was enabled.

  Int_t i, j;

// bcmu3 is an uncalibrated rate in Hz
  bcmu3 = -1;
  if (myscaler) bcmu3 = myscaler->GetBcmRate("bcm_u3");

  if (fDoBcmCalib) BcmCalib(evdata);

// helicity is -1,0,1.  here, 0 == irrespective of helicity.
// But if helicity disabled its always 0.

  fHelEnable = evdata.HelicityEnabled();
  Int_t helicity = evdata.GetHelicity();

  if (evdata.IsPhysicsTrigger()) normdata->EvCount(helicity);

// If you have ~1000 events you certainly 
// have seen prescale event; kludgy but works.
  if (!fSetPrescale && evdata.GetEvNum() > 1000) {
    fSetPrescale = kTRUE;
// CAREFUL, must GetPrescaleFactor(J) where J = 1,2,3 .. 
// (index starts at 1).  normdata starts at 0.
    for (i = 0; i < 8; i++) 
         normdata->SetPrescale(i,evdata.GetPrescaleFactor(i+1));
    for (i = 8; i < 12; i++) normdata->SetPrescale(i,1);
  }

#ifdef TRY1
  // Example to process the scalers if event type 140
  // came here, which it does not.  See also the TRY1
  // comments elsewhere.
  //  if (evdata.GetEvType() == 140) {
  //     if (myscaler) myscaler->LoadData(evdata);
  //  }
#endif


// TDC for trigger latch pattern.  The crate,slot,startchan 
// might change with experiments (I hope not).

  int crate = 3;
  int slot = 5;
  int startchan = 64;

  for (i = 0; i < 12; i++) {
    for (j = 0; j < 6; j++) tdcdata[j*12+i] = 0;
    nhit[i] = evdata.GetNumHits(crate, slot, startchan+i);
    if (nhit[i] > 6) nhit[i] = 6;  
    for (j = 0; j < nhit[i]; j++) {
      Int_t tdat = evdata.GetData(crate,slot,startchan+i,j);
      tdcdata[j*12+i] = tdat;
      hist[i]->Fill(tdat);
    }        
  }
     
  TrigBits(helicity);

  LiveTime();

  if (fDEBUG) Print();


  return 0;
}


//_____________________________________________________________________________
void THaNormAna::Print( Option_t* opt ) const {
// Print for purpose of debugging.

   normdata->Print();

   PrintSummary();

}


//_____________________________________________________________________________
void THaNormAna::SetEventInt(Int_t *event)
{
// See also comments at top of code.
// Set the event intervals that define stable beam.
// It is necessary to first look at N.bcmu3:fEvtHdr.fEvtNum
// to decide which event intervals have stable beam.
// Then let  N = event[0] = number of intevals (pairs).
// event[1] to event[2] is a stable beam interval  
// up to: event[2N-1] to event[2N]

  if (event == 0) return;
  Int_t N = event[0];
  if (N == 0) return;
  if (2*N >= fgMaxEvInt) N = fgMaxEvInt/2;
  for (int i = 0; i <= 2*N; i++) eventint[i] = event[i];
  return;
}

//_____________________________________________________________________________
Int_t THaNormAna::BcmCalib(const THaEvData& evdata) {
// Fill histograms of scaler BCM versus EPICS BCM.
// This calibrates the scalers and assumes the EPICS
// variables are well calibrated.

  if (!fDoBcmCalib) return 0;
  if (myscaler == 0) return 0;
  if (hist.size() < 18) return 0;
// See SetEventInt() for definition of eventint.
  Int_t N = eventint[0];
  if (N <= 0) return 0;
  Double_t u1  = myscaler->GetBcmRate("bcm_u1");
  Double_t u3  = myscaler->GetBcmRate("bcm_u3");
  Double_t u10 = myscaler->GetBcmRate("bcm_u10");
  Double_t d1  = myscaler->GetBcmRate("bcm_d1");
  Double_t d3  = myscaler->GetBcmRate("bcm_d3");
  Double_t d10 = myscaler->GetBcmRate("bcm_d10");
  Double_t hacbcm = evdata.GetEpicsData("hac_bcm_average");
  for (int i = 1; i <= 2*N; i++) {
    if (evdata.GetEvNum() > eventint[i] && 
        evdata.GetEvNum() < eventint[i+1]) {
      hist[12]->Fill(hacbcm,u1);
      hist[13]->Fill(hacbcm,u3);
      hist[14]->Fill(hacbcm,u10);
      hist[15]->Fill(hacbcm,d1);
      hist[16]->Fill(hacbcm,d3);
      hist[17]->Fill(hacbcm,d10);
      return 0;
    }
  }
  return 0;
}

//_____________________________________________________________________________

void THaNormAna::TrigBits( Int_t helicity ) {
// Figure out which triggers got a hit.  These are 
// multihit TDCs, so we need to sort out which hit 
// we want to take by applying cuts.

  int itrig,jhit,localtrig[12];

// These cuts can be decided by looking at the histograms.
// They importantly define "simultaneous" triggers, and 
// may therefore introduce a (small) systematic error

  static const Int_t cutlo = 700;
  static const Int_t cuthi = 1300;

  evtypebits = 0;
  memset(localtrig,0,12*sizeof(int));
  
  for (itrig = 0; itrig < 12; itrig++) {
    if (fDEBUG) {
           cout << "TDC for trig "<<itrig;
           cout << "   nhit = "<<nhit[itrig]<<endl;
    }
    for (jhit = 0; jhit < nhit[itrig]; jhit++) {
      Int_t tdat = tdcdata[12*jhit + itrig];
// Count at most once per event (don't double count)
// That's the purpose of localtrig
      if (fDEBUG) {
        cout << "    TDC dat for hit ";
        cout << jhit<<"   "<<tdat<<endl;
      }
      if (tdat > cutlo && tdat < cuthi && localtrig[itrig]==0) {
        localtrig[itrig] = 1;
	normdata->TrigCount(itrig,helicity);
        evtypebits |= (2<<itrig);
        if (fDEBUG) {
          cout << "        trigger bit "<< itrig;
          cout << "   helicity "<< helicity << "  count = ";
          cout << normdata->GetTrigCount(itrig,helicity)<<endl;
	}
      }
    }
  }

  if (fDEBUG) {
     cout << "  trig bit pattern "<<evtypebits<<endl;
  }

}


//_____________________________________________________________________________

void THaNormAna::LiveTime() {

// Compute the livetime by trigger type, by
// helicity, and averaged from scalers-only-info.

   if (myscaler == 0) return;

  Int_t itrig, ihel, nhel, ok1;
  Double_t t5corr, t7corr, tcorr;
  Double_t totaltrig, livetime, avglive;
  Double_t numtrig, tsaccept, corrfact;  

  ok1=0;
  nhel = 1;  
  if (fHelEnable) nhel = 3;

  for (ihel = 0; ihel < nhel; ihel++) { 
    Int_t helicity = 0;
    if (ihel==1) helicity = -1;
    if (ihel==2) helicity = 1;

// CAREFUL ABOUT INDEX conventions
// THaScaler::GetTrig starts at itrig=1
// normdata indices all start at 0.

    t5corr = 0;
    if (normdata->GetPrescale(4) != 0) {
      t5corr = 
          (Double_t)myscaler->GetTrig(helicity,5)/
                      normdata->GetPrescale(4);
    }
    t7corr = 0;
    if (normdata->GetPrescale(6) != 0) {
      t7corr = 
          (Double_t)myscaler->GetTrig(helicity,7)/
                      normdata->GetPrescale(6);
    }

    tsaccept = 0;
    totaltrig = 0;
    corrfact = 0;
    ok1 = 0;

    for (itrig = 0; itrig < 12; itrig++) {

      numtrig = (Double_t)myscaler->GetTrig(helicity,itrig+1);

// Livetime by trigger type and by helicity:
// These are exact and physically meaningful, in
// contrast to "average livetime" below.

      if (normdata->GetPrescale(itrig) > 0 && numtrig > 0) {
        livetime = normdata->GetTrigCount(itrig,helicity)*
             normdata->GetPrescale(itrig)/numtrig;
        tsaccept = 
          (Double_t)myscaler->GetNormData(helicity,"TS-accept");

// Correction for the non-synch of scalers and events.
// If correction too far from 1 --> failure to estimate.
// At a normal end-of-run, corrfact should be 1.0

        if (tsaccept > 0) corrfact = 
   	   normdata->GetEvCount(helicity)/tsaccept;
        if (corrfact > 0.5 && corrfact < 2.0) {
          ok1=1;
          livetime = livetime/corrfact;
        } else {
          livetime = -1;  // failure
	}
        normdata->SetLive(itrig, livetime, helicity); 

// Average livetime using only scaler information
// Assumptions, valid for e94107, are:
// Every T5 is a T1 and T3.  Every T7 is a T5 and T6.
// There are other correlations which are ignored,
// hence this is only approximate.

        tcorr = 0; 
        if (itrig == 0 || itrig == 2) tcorr = t5corr;
        if (itrig == 4 || itrig == 5) tcorr = t7corr;
        totaltrig = totaltrig + 
           (numtrig-tcorr)/normdata->GetPrescale(itrig);
      }

      avglive = 0;
      if (totaltrig > 0) avglive = tsaccept/totaltrig;
      normdata->SetAvgLive(avglive,corrfact,helicity);

    }
  }

  alive = normdata->GetAvgLive();
  hpos_alive = normdata->GetAvgLive(1);
  hneg_alive = normdata->GetAvgLive(-1);

}


//_____________________________________________________________________________
ClassImp(THaNormAna)

