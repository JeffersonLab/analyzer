//*-- Author :    Bob Michaels,  April 2004

//////////////////////////////////////////////////////////////////////////
//
// ParityData
//
// HAPPEX data from spectrometer DAQ.
// 1. New Cavity XYQ Monitors
// 2. HAPPEX electron detectors, alignment.
// 3. UMass Q^2 scanner, alignment.
// 4. Kinematics (Q^2, etc).
//
// Minimal alignment procedure.  output.def :
// a) Alignment histograms (L-arm example):
//   Cut onetrkL L.tr.n==1
//   Cut happexL L.tr.n==1&&P.hapadcl1>1800
//   TH2F Lali1 'Left arm alignment' L.tr.x L.tr.y 100 -0.8 0.8 100 -0.2 0.2  onetrkL
//   TH2F Lali2 'Left arm alignment (HAP det)' L.tr.x L.tr.y 100 -0.8 0.8 100 -0.2 0.2  happexL
// b) Find detector using field too low by ~4%. Whole
//    detector illuminated by rad. tail.
//    --> Defines "box" of detector.
// c) Put field at 0%, check if tracks fit in "box".
//
//////////////////////////////////////////////////////////////////////////

//#define WITH_DEBUG 1

//#define DUMP_SCALER

#include "ParityData.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaScalerGroup.h"
#include "THaScaler.h"
#include "THaAnalyzer.h"
#include "THaString.h"
#include "TDatime.h"
#include "TH1.h"
#include "VarDef.h"
#include <fstream>
#include <iostream>

#define NCAV 6
#define NSTR 12

using namespace std;

typedef vector<BdataLoc*>::iterator Iter_t;

//_____________________________________________________________________________
ParityData::ParityData( const char* name, const char* descript ) : 
  THaApparatus( name, descript )
{
  lscaler = 0;
  rscaler = 0;
  trigcnt = new Int_t[12];
  cped = new Double_t[NCAV];
  cfb  = new Double_t[NCAV];
  sped = new Double_t[NSTR];
  sfb  = new Double_t[NSTR];
  for (int i = 0; i < 12; i++) trigcnt[i]=0;
  for (int i = 0; i < NCAV; i++) {
        cped[i]=0;
        cfb[i]=0;
  }
  for (int i = 0; i < NSTR; i++) {
        sped[i]=0;
        sfb[i]=0;
  }
  Clear();
}

//_____________________________________________________________________________
ParityData::~ParityData()
{
  if (trigcnt) delete [] trigcnt;
  if (cped) delete [] cped;
  if (cfb) delete [] cfb; 
  if (sped) delete [] sped;
  if (sfb) delete [] sfb;
  SetupParData( NULL, kDelete ); 
  for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end(); p++ )  delete *p;
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++ ) delete *p;
}

//_____________________________________________________________________________
void ParityData::Clear( Option_t* opt ) 
{
  evtypebits = 0;
  evtype     = 0;
  hapadcl1 = 0; hapadcl2 = 0;
  hapadcr1 = 0; hapadcr2 = 0;
  haptdcl1 = 0; haptdcl2 = 0;
  haptdcr1 = 0; haptdcr2 = 0;
  profampl = 0; profxl = 0; profyl = 0;
  profampr = 0; profxr = 0; profyr = 0;
  for (int i = 0; i < NCAV; i++) cfb[i]=0;
  for (int i = 0; i < NSTR; i++) sfb[i]=0;
  xcav4a = 0; ycav4a = 0; qcav4a = 0;
  xcav4b = 0; ycav4b = 0; qcav4b = 0;
  xstrip4a = 0; ystrip4a = 0;
  xstrip4b = 0; ystrip4b = 0;
  for( Iter_t p = fWordLoc.begin();  p != fWordLoc.end(); p++)  (*p)->Clear();
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) (*p)->Clear();
}

//_____________________________________________________________________________
Int_t ParityData::SetupParData( const TDatime* run_time, EMode mode )
{

  Int_t retval = 0;

  RVarDef vars[] = {
    { "evtypebits", "event type bit pattern",   "evtypebits" },  
    { "evtype",     "event type from bit pattern", "evtype" },  
    { "hapadcl1",   "HAPPEX ADC Left #1",     "hapadcl1" },  
    { "hapadcl2",   "HAPPEX ADC Left #2",     "hapadcl2" },  
    { "hapadcr1",   "HAPPEX ADC Right #1",    "hapadcr1" },  
    { "hapadcr2",   "HAPPEX ADC Right #2",    "hapadcr2" },  
    { "haptdcl1",   "HAPPEX TDC Left #1",     "haptdcl1" },  
    { "haptdcl2",   "HAPPEX TDC Left #2",     "haptdcl2" },  
    { "haptdcr1",   "HAPPEX TDC Right #1",    "haptdcr1" },  
    { "haptdcr2",   "HAPPEX TDC Right #2",    "haptdcr2" },  
    { "profampl",  "UMass Profile Amp Left",  "profampl" }, 
    { "profxl",    "UMass Profile X Left",    "profxl" }, 
    { "profyl",    "UMass Profile Y Left",    "profyl" }, 
    { "profampr",  "UMass Profire Amp Right",  "profampr" }, 
    { "profxr",    "UMass Profire X Right",    "profxr" }, 
    { "profyr",    "UMass Profire Y Right",    "profyr" }, 
    { "xcav4a",     "X Cavity 4A",             "xcav4a" },  
    { "ycav4a",     "Y Cavity 4A",             "ycav4a" },  
    { "qcav4a",     "Q Cavity 4A",             "qcav4a" },  
    { "xcav4b",     "X Cavity 4B",             "xcav4b" },  
    { "ycav4b",     "Y Cavity 4B",             "ycav4b" },  
    { "qcav4b",     "Q Cavity 4B",             "qcav4b" },  
    { "xstrip4a",   "X Stripline 4A",          "xstrip4a" },  
    { "ystrip4a",   "Y Stripline 4A",          "ystrip4a" },  
    { "xstrip4b",   "X Stripline 4B",          "xstrip4b" },  
    { "ystrip4b",   "Y Stripline 4B",          "ystrip4b" },  
    { "q2L",   "Qsq on Left arm",             "q2L" },  
    { "mmL",   "Missing mass on Left arm",    "mmL" },  
    { "q2R",   "Qsq on Right arm",            "q2R" },  
    { "mmR",   "Missing mass on Right arm",   "mmR" },  
    { 0 }
  };

  Int_t nvar = sizeof(vars)/sizeof(RVarDef);

  if( mode != kDefine || !fIsSetup )
    retval = DefineVarsFromList( vars, mode );

  fIsSetup = ( mode == kDefine );

  if( mode != kDefine ) {   // cleanup the dynamically built list
    for (unsigned int i=0; i<fCrateLoc.size(); i++)
      DefineChannel(fCrateLoc[i],mode);

    for (unsigned int i=0; i<fWordLoc.size(); i++)
      DefineChannel(fWordLoc[i],mode);

    return retval;
  }
  

  fCrateLoc.clear();   
  fWordLoc.clear();   

  ifstream pardatafile;

  const char* const here = "SetupParData()";
  const char* name = GetDBFileName();

  TDatime date;
  if( run_time ) date = *run_time;
  vector<string> fnames = GetDBFileList( name, date, Here(here));
  // always look for 'pardata.map' in the current directory first.
  fnames.insert(fnames.begin(),string("pardata.map"));
  if( !fnames.empty() ) {
    vector<string>::iterator it = fnames.begin();
    do {
#ifdef WITH_DEBUG
      if( fDebug>0 ) {
	cout << "<" << Here(here) << ">: Opening database file " << *it;
      }
#endif
      pardatafile.clear();  // Forget previous failures before attempt
      pardatafile.open((*it).c_str());

#ifdef WITH_DEBUG
      if (pardatafile) {
	cout << "Opening file "<<*it<<endl;
      } else {
        cout << "cannot open file "<<*it<<endl;
      }
      if( fDebug>0 ) 
	if( !pardatafile ) cout << " ... failed" << endl;
	else               cout << " ... ok" << endl;
#endif
    } while ( !pardatafile && ++it != fnames.end() );
  }
  if( fnames.empty() || !pardatafile )
    if (PARDATA_VERBOSE) {
      cout << "WARNING:: ParityData: File db_"<<name<<".dat not found."<<endl;
      cout << "An example of this file should be in the examples directory."<<endl;
      cout << "Will proceed with default mapping for ParityData."<<endl;
      Int_t statm = DefaultMap();
      PrintMap();
      return statm;
    }

  string sinput;
  const string comment = "#";
  while (getline(pardatafile, sinput)) {
#ifdef WITH_DEBUG
    cout << "sinput "<<sinput<<endl;
#endif
    vector<string> strvect( vsplit(sinput) );
    if (strvect.size() < 5 || strvect[0] == comment) continue;
    Bool_t found = kFALSE;
    for (int i = 0; i < nvar; i++) {
      if (vars[i].name && strvect[0] == vars[i].name) found = kTRUE;
    }
// !found may be ok, but might be a typo error too, so I print to warn you.
    if ( !found && PARDATA_VERBOSE ) 
      cout << "ParityData: new variable "<<strvect[0]<<" will become global"<<endl;
    Int_t crate = (Int_t)atoi(strvect[2].c_str());  // crate #
    BdataLoc *b = 0;
    if (strvect[1] == "crate") {  // Crate data ?
      Int_t slot = (Int_t)atoi(strvect[3].c_str());
      Int_t chan = (Int_t)atoi(strvect[4].c_str());
      b = new BdataLoc(strvect[0].c_str(), crate, slot, chan); 
      fCrateLoc.push_back(b);
    } else {         // Data is relative to a header
      UInt_t header = header_str_to_base16(strvect[3].c_str());
      Int_t skip = (Int_t)atoi(strvect[4].c_str());
      b = new BdataLoc(strvect[0].c_str(), crate, header, skip);
      fWordLoc.push_back(b);
    }

    if (!found) {
      // a new variable to add to our dynamic list
      DefineChannel(b,mode);
    }
  }
  PrintMap(1);
  return retval;
}

//_____________________________________________________________________________
void ParityData::PrintMap(Int_t flag) {
  cout << "Map for Parity Data "<<endl;
  if (flag == 1) cout << "Map read from file "<<endl;
  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc *dataloc = *p;
    dataloc->Print();
  }
  for (Iter_t p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
    BdataLoc *dataloc = *p;
    dataloc->Print();
  }  
}

//_____________________________________________________________________________
BdataLoc* ParityData::DefineChannel(BdataLoc *b, EMode mode, const char* desc) {
  string nm(fPrefix + b->name);
  if (mode==kDefine) {
    if (gHaVars) gHaVars->Define(nm.c_str(),desc,b->rdata[0],&(b->ndata));
  } else {
    if (gHaVars) gHaVars->RemoveName(nm.c_str());
  }
  return b;
}
  
//_____________________________________________________________________________
Int_t ParityData::End( THaRunBase* run ) 
{
  WriteHist();
  return 0;
}

//_____________________________________________________________________________
  void ParityData::WriteHist()
{
  for (UInt_t i = 0; i < hist.size(); i++) hist[i]->Write();
}

//_____________________________________________________________________________
  void ParityData::BookHist()
{
  hist.clear();
  hist.push_back(new TH1F("Cav1Q","Cavity1 Q",1200,-800,5200));
  hist.push_back(new TH1F("Cav1X","Cavity1 X",1200,-800,5200));
  hist.push_back(new TH1F("Cav1Y","Cavity1 Y",1200,-800,5200));
  hist.push_back(new TH1F("Cav2Q","Cavity2 Q",1200,-800,5200));
  hist.push_back(new TH1F("Cav2X","Cavity2 X",1200,-800,5200));
  hist.push_back(new TH1F("Cav2Y","Cavity2 Y",1200,-800,5200));

}


//_____________________________________________________________________________
THaAnalysisObject::EStatus ParityData::Init( const TDatime& run_time ) 
{

  fStatus = kNotinit;
  MakePrefix();
  BookHist();
  InitCalib();
  fStatus = static_cast<EStatus>( SetupParData( &run_time ) );
  // Get scalers.  They must be initialized in the
  // analyzer. 
  THaAnalyzer* theAnalyzer = THaAnalyzer::GetInstance();
  TList* scalerList = theAnalyzer->GetScalers();
  TIter next(scalerList);
  while( THaScalerGroup* tscalgrp = static_cast<THaScalerGroup*>( next() )) {
    THaScaler *scaler = tscalgrp->GetScalerObj();
    THaString lbank("Left");
    if (lbank.CmpNoCase(scaler->GetName()) == 0) {
         lscaler = scaler; 
    }
    THaString rbank("Right");
    if (rbank.CmpNoCase(scaler->GetName()) == 0) {
         rscaler = scaler; 
    }
  }
  return fStatus;
}


//_____________________________________________________________________________
Int_t ParityData::InitCalib() {
// Calibration initialization

// 568  518  427  448  1.0  1.0  18.87  1.11  1.07   -0.47   0.67
// 490  426  553  520  1.0  1.0  18.87  0.95  0.99    0.32   0.34
 
//%%%%%%%%%%%  Calibration for Striplines  %%%%%%%%%%%%
//In each line above, first four entries are pedestal.
//5th, 6th entries are alpha_x, alpha_y gain factors.
//By historical convention they are always 1.0 now.
//7th entry is kappa in units of millimeters.
//8th,9th entries are ''attenuation factors'', x_att, y_att
//Last two entries are x_offset,y_offset.  Again millimeters.
//Note, the offset and attenuation is applied after rotation.
//The formulas:
//  xp = xp_adc - xp_ped (attenna signal above ped); similarly xm
//  x_rot = kappa*(xp - alpha_x*xm)/(xp + alpha_x*xm)
//  y_rot  similar
//  x = -1*x_offset + (x_rot - y_rot)*x_att / sqrt(2)
//  y = y_offset + (x_rot + y_rot)*y_att / sqrt(2)
//Sign convention should agree with EPICS

// -------------------------------------------

  
// cavities I,X,Y for 2 monitors
// (One issue: unplugging is different from no signal 
//  from floor of hall.  presumably a ground diff
//  leading to offset due to differential amplifier)
    cped[0] = 550.2;
    cped[1] = 503.6;
    cped[2] = 514.0;
    cped[3] = 514.9;
    cped[4] = 618.6;
    cped[5] = 587.6;

    aqcav4a = 1;
    axcav4a = 1;
    aycav4a = 1;
    aqcav4b = 1;
    axcav4b = 1;
    aycav4b = 1;
    bqcav4a = 0;
    bxcav4a = 0;
    bycav4a = 0;
    bqcav4b = 0;
    bxcav4b = 0;
    bycav4b = 0;

    // Striplines
    alpha4ax = 1.0;
    alpha4ay = 1.0;
    alpha4bx = 1.0;
    alpha4by = 1.0;
    kappa4a  = 18.87;
    kappa4b  = 18.87;
    att4ax   = 1.11;
    att4ay   = 1.07;
    att4bx   = 0.95;
    att4by   = 0.99;
    off4ax   = -0.47;
    off4ay   = 0.67;
    off4bx   = 0.32;
    off4by   = 0.34;
    
    sped[0] = 568;
    sped[1] = 518;
    sped[2] = 427;
    sped[3] = 448;
    sped[4] = 490;
    sped[5] = 426;
    sped[6] = 553;
    sped[7] = 520;
    sped[8] = 0;
    sped[9] = 0;
    sped[10] = 0;
    sped[11] = 0;

    return 0;

}

//_____________________________________________________________________________
Int_t ParityData::DefaultMap() {
// Default setup of mapping of data in 
// this class to locations in the raw data.

// Bit pattern for trigger definition

   for (UInt_t i = 0; i < bits.GetNbits(); i++) {
     fCrateLoc.push_back(new BdataLoc(Form("bit%d",i+1), 3, (Int_t) 5, 64+i));
   }

   fCrateLoc.push_back(new BdataLoc("hapadcl1", 3, 25, 0));
   fCrateLoc.push_back(new BdataLoc("hapadcl2", 3, 25, 2));
   fCrateLoc.push_back(new BdataLoc("haptdcl1", 3, 3, 0));
   fCrateLoc.push_back(new BdataLoc("haptdcl2", 3, 3, 2));

   fCrateLoc.push_back(new BdataLoc("hapadcr1", 1, 25, 0));
   fCrateLoc.push_back(new BdataLoc("hapadcr2", 1, 25, 2));
   fCrateLoc.push_back(new BdataLoc("haptdcr1", 1, 16, 16));
   fCrateLoc.push_back(new BdataLoc("haptdcr2", 1, 16, 18));

   fCrateLoc.push_back(new BdataLoc("profampl", 3, 25, 4));
   fCrateLoc.push_back(new BdataLoc("profampr", 1, 25, 4));

   return 0;
}


//_____________________________________________________________________________
Int_t ParityData::Decode(const THaEvData& evdata)
{
  Int_t i;
  Clear();

  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc *dataloc = *p;
    if ( dataloc->IsSlot() ) {  
      for (i = 0; i < evdata.GetNumHits(dataloc->crate, 
		         dataloc->slot, dataloc->chan); i++) {
	dataloc->Load(evdata.GetData(dataloc->crate, dataloc->slot, 
				     dataloc->chan, i));
      }
    }
  }
  

  for (i = 0; i < evdata.GetEvLength(); i++) {
    for (Iter_t p = fWordLoc.begin(); p != fWordLoc.end(); p++) {
      BdataLoc *dataloc = *p;
      if ( dataloc->DidLoad() || dataloc->IsSlot() ) continue;
      if ( evdata.InCrate(dataloc->crate, i) ) {
        if ((UInt_t)evdata.GetRawData(i) == dataloc->header) {
            dataloc->Load(evdata.GetRawData(i + dataloc->ntoskip));
        }
      }
    }
  }

  evtype = evdata.GetEvType();   // CODA event type 

  for( Iter_t p = fCrateLoc.begin(); p != fCrateLoc.end(); p++) {
    BdataLoc *dataloc = *p;

// bit pattern of triggers
    for (UInt_t i = 0; i < bits.GetNbits(); i++) {
      if ( dataloc->ThisIs(Form("bit%d",i+1)) ) TrigBits(i+1,dataloc);
    }

    if ( dataloc->ThisIs("hapadcl1") ) hapadcl1  = dataloc->Get();
    if ( dataloc->ThisIs("hapadcl2") ) hapadcl2  = dataloc->Get();
    if ( dataloc->ThisIs("hapadcr1") ) hapadcr1  = dataloc->Get();
    if ( dataloc->ThisIs("hapadcr2") ) hapadcr2  = dataloc->Get();

    if ( dataloc->ThisIs("haptdcl1") ) haptdcl1  = dataloc->Get();
    if ( dataloc->ThisIs("haptdcl2") ) haptdcl2  = dataloc->Get();
    if ( dataloc->ThisIs("haptdcr1") ) haptdcr1  = dataloc->Get();
    if ( dataloc->ThisIs("haptdcr2") ) haptdcr2  = dataloc->Get();

    if ( dataloc->ThisIs("profampl") ) profampl  = dataloc->Get();
    if ( dataloc->ThisIs("profampr") ) profampr  = dataloc->Get();

  }

  // Striplines
  Int_t str_roc=2;
  Int_t str_slot=23;
  Int_t str_chan0=0;
  for (int i=0; i<NSTR; i++) {
    sfb[i] = evdata.GetData(str_roc,str_slot,
                 str_chan0+i, 0);         ;
  }

  // Cavities
  Int_t cav_roc=4;
  Int_t cav_slot=20;
  Int_t cav_chan0=48;
  for (int i=0; i<NCAV; i++) {
    cfb[i] = evdata.GetData(cav_roc,cav_slot,
                 cav_chan0+i, 0);          ;
  }

  DoBpm();

  DoKine();

  ProfileXY();

  if (PARDATA_PRINT) Print();

  return 0;
}


//_____________________________________________________________________________
Int_t ParityData::DoBpm( ) {
// The 'stripline' BPM code is redundant to the 
// analyzer.  The 'cavity' BPM is new.

  Double_t x_rot, y_rot;
  static Double_t sqrt2 = 1.41421;
  Int_t loc_debug = 0;

  xp4a = sfb[0]-sped[0];  // "s" = stripline
  xm4a = sfb[1]-sped[1];
  yp4a = sfb[2]-sped[2];
  ym4a = sfb[3]-sped[3];
  xp4b = sfb[4]-sped[4];
  xm4b = sfb[5]-sped[5];
  yp4b = sfb[6]-sped[6];
  ym4b = sfb[7]-sped[7];

  x_rot = kappa4a*(xp4a-alpha4ax*xm4a)/(xp4a+alpha4ax*xm4a);
  y_rot = kappa4a*(yp4a-alpha4ay*ym4a)/(yp4a+alpha4ay*ym4a);
  xstrip4a = -1*off4ax + (x_rot - y_rot)*att4ax / sqrt2;
  ystrip4a = off4ay + (x_rot + y_rot)*att4ay / sqrt2;

  x_rot = kappa4b*(xp4b-alpha4bx*xm4b)/(xp4b+alpha4bx*xm4b);
  y_rot = kappa4b*(yp4b-alpha4by*ym4b)/(yp4b+alpha4by*ym4b);
  xstrip4b = -1*off4bx + (x_rot - y_rot)*att4bx / sqrt2;
  ystrip4b = off4by + (x_rot + y_rot)*att4by / sqrt2;

  // cavity monitor QXY.  Calibration here:
  qcav4a = aqcav4a*(cfb[0]-cped[0])+bqcav4a;
  xcav4a = axcav4a*(cfb[1]-cped[1])+bxcav4a;
  ycav4a = aycav4a*(cfb[2]-cped[2])+bycav4a;
  qcav4b = aqcav4b*(cfb[3]-cped[3])+bqcav4b;
  xcav4b = axcav4b*(cfb[4]-cped[4])+bxcav4b;
  ycav4b = aycav4b*(cfb[5]-cped[5])+bycav4b;

  hist[0]->Fill(qcav4a);
  hist[1]->Fill(xcav4a);
  hist[2]->Fill(ycav4a);
  hist[3]->Fill(qcav4b);
  hist[4]->Fill(xcav4b);
  hist[5]->Fill(ycav4b);

  if (loc_debug) {
    cout << "sfb ---> "<<endl;
    for (int i = 0; i < 8; i++) {
      cout << "   sig = "<<sfb[i]<<"   ped = "<<sped[i]<<endl;
    }
    cout << "calib --> "<<endl;
    cout << "alphas "<<alpha4ax<<"  "<<alpha4ay;
    cout << "  "<<alpha4bx<<"  "<<alpha4by<<endl;
    cout << "offsets "<<off4ax<<"  "<<off4ay;
    cout << "  "<<off4bx<<"  "<<off4by<<endl;
    cout << "kappa "<<kappa4a<<"  "<<kappa4b<<endl;
    cout << "xstrip4a, etc "<<xstrip4a<<"  "<<ystrip4a;
    cout << "  "<<xstrip4b<<"  "<<ystrip4b<<endl;
    cout << "\n\ncfb --> "<<endl;
    for (int i = 0; i < NCAV; i++) {
      cout << "  sig = "<<cfb[i]<<"   ped = "<<cped[i]<<endl;
    }
    cout << "qcav4a = "<<qcav4a<<endl;
    cout << "xcav4a = "<<xcav4a<<endl;
    cout << "ycav4a = "<<ycav4a<<endl;
    cout << "qcav4b = "<<qcav4b<<endl;
    cout << "xcav4b = "<<xcav4b<<endl;
    cout << "ycav4b = "<<ycav4b<<endl;
  }

  return 1;
}


//_____________________________________________________________________________
Int_t ParityData::DoKine( ) {
// Calculate Q^2 and missing mass squared.
// I realized later that this routine obtains
// the same result as THaElectronKine, but that's
// a good check.


  Double_t pL,thL,phiL;
  Double_t pR,thR,phiR;
  THaVar *pvar;

  Double_t ebeam = 3.3176;
  Double_t mprot = 0.938;

  Double_t theta_L = 12.527;  // central angle, degrees
  Double_t theta_R = 12.561;  // central angle, degrees
  Double_t thoff_L = 0;       // offset to play with
  Double_t thoff_R = 0;       // offset "   "    "

  Double_t pscale  = 1.00;    // momentum scale factor
                              // to play with

  Double_t pi = 3.1415926;
  Double_t thL0,thR0,costhL,costhR,sinthL,sinthR;

  thL0 = (theta_L+thoff_L)*pi/180;
  costhL = cos(thL0);
  sinthL = sin(thL0);
  thR0 = (theta_R+thoff_R)*pi/180;
  costhR = cos(thR0);
  sinthR = sin(thR0);

  pL   = -999;
  thL  = -999;
  phiL = -999;
  pR   = -999;
  thR  = -999;
  phiR = -999;
  Int_t okL = 0;  
  Int_t okR = 0;

// Must have the THaGoldenTrack module invoked
  
  pvar = gHaVars->Find("L.gold.ok");
  if (pvar) okL = (Int_t) pvar->GetValue();
  if (okL) {
    pvar = gHaVars->Find("L.gold.p");
    if (pvar) pL = pvar->GetValue();
    pvar = gHaVars->Find("L.gold.th");
    if (pvar) thL = pvar->GetValue();
    pvar = gHaVars->Find("L.gold.ph");
    if (pvar) phiL = pvar->GetValue();
  }

  pvar = gHaVars->Find("R.gold.ok");
  if (pvar) okR = (Int_t) pvar->GetValue();
  if (okR) {
    pvar = gHaVars->Find("R.gold.p");
    if (pvar) pR = pvar->GetValue();
    pvar = gHaVars->Find("R.gold.th");
    if (pvar) thR = pvar->GetValue();
    pvar = gHaVars->Find("R.gold.ph");
    if (pvar) phiR = pvar->GetValue();
  }  

  q2L = -999;
  mmL = -999;

  if (pL > -999 && thL > -999 && phiL > -999 &&
      pL < 1e32 && thL < 1e32 && phiL < 1e32) {
    q2L = 2*ebeam*(pscale*pL)*(1-((costhL-sinthL*phiL)/sqrt(1+thL*thL+phiL*phiL)));
    mmL = 2*mprot*(ebeam-(pscale*pL))-q2L;
  }

  q2R = -999;
  mmR = -999;
  if (pR > -999 && thR > -999 && phiR > -999 &&
      pR < 1e32 && thR < 1e32 && phiR < 1e32) {
// note : sign convention requires costhR + sinthR*phiR 
//                                    not -
    q2R = 2*ebeam*(pscale*pR)*(1-((costhR+sinthR*phiR)/sqrt(1+thR*thR+phiR*phiR)));
    mmR = 2*mprot*(ebeam-(pscale*pR))-q2R;
  }

  return 1;

}


//_____________________________________________________________________________
Int_t ParityData::ProfileXY() {
// Profile scanner data is from scalers.

  if (lscaler == 0 || rscaler == 0) return 0;

  // L-arm X,Y in 31st & up (slot 4, index 30+)
  profxl = lscaler->GetScalerRate(4,30);
  profyl = lscaler->GetScalerRate(4,31);

  // r-arm X,Y in 31st & up (slot 8, index 30+)
  profxr = rscaler->GetScalerRate(8,30);
  profyr = rscaler->GetScalerRate(8,31);

#ifdef DUMP_SCALER
  cout << "\n\nLeft Scaler Dump"<<endl;
  //  lscaler->Print();
  lscaler->PrintSummary();

  cout << "\n\nRight Scaler Dump"<<endl;
  //  rscaler->Print();
  rscaler->PrintSummary();

  cout << "\n\n ================ X,Y  Left  "<<hex<<profxl<<"  "<<profyl<<"  Right "<<profxr<<"  "<<profyr<<dec<<endl;
#endif

  return 0;

}

//_____________________________________________________________________________
void ParityData::Print( Option_t* opt ) const {
// Dump the data for purpose of debugging.
  cout << "Dump of ParityData "<<endl;
  cout << "event pattern bits : ";
  for (UInt_t i = 0; i < bits.GetNbits(); i++) 
    cout << " "<<i<<" = "<< bits.TestBitNumber(i)<<"  | ";
  cout << endl;
  cout << "event types,  CODA = "<<evtype;
  cout << "   bit pattern = "<<evtypebits<<endl;
  cout << "HAPPEX ADC on Left "<<hapadcl1<<"  "<<hapadcl2<<endl;
  cout << "HAPPEX TDC on Left "<<haptdcl1<<"  "<<haptdcl2<<endl;
  cout << "HAPPEX ADC on Right "<<hapadcr1<<"  "<<hapadcr2<<endl;
  cout << "HAPPEX TDC on Right "<<haptdcr1<<"  "<<haptdcr2<<endl;
  cout << "UMass profile scanner amplitude on Left "<<profampl<<endl;
  cout << "UMass profile scanner X on Left "<<profxl<<endl;
  cout << "UMass profile scanner Y on Left "<<profyl<<endl;
  cout << "UMass profile scanner amplitude on Right "<<profampr<<endl;
  cout << "UMass profile scanner X on Right "<<profxr<<endl;
  cout << "UMass profile scanner Y on Right "<<profyr<<endl;
  cout << "Cavity at 4A  X/Y/Q "<<
    xcav4a<<"  "<<ycav4a<<"  "<<qcav4a<<endl;
  cout << "Cavity at 4B  X/Y/Q "<<
    xcav4b<<"  "<<ycav4b<<"  "<<qcav4b<<endl;
  cout << "Stripline at 4A  X/Y "<<
    xstrip4a<<"  "<<ystrip4a<<endl;
  cout << "Stripline at 4B  X/Y "<<
    xstrip4a<<"  "<<ystrip4b<<endl;
  cout << "Left arm q2 "<<q2L<<"   miss mass sq "<<mmL<<endl;
  cout << "Right arm q2 "<<q2R<<"   miss mass sq "<<mmR<<endl;
#ifdef CHECK1
  cout << "Accepted Trigger Counts "<<endl;
  for (Int_t i = 0; i < 12; i++) {
    cout << "     Trig "<<i+1<<"   count = "<<trigcnt[i]<<endl;
  }
#endif
}


//_____________________________________________________________________________
vector<string> ParityData::vsplit(const string& s) {
// split a string into whitespace-separated strings
  vector<string> ret;
  typedef string::size_type ssiz_t;
  ssiz_t i = 0;
  while ( i != s.size()) {
    while (i != s.size() && isspace(s[i])) ++i;
      ssiz_t j = i;
      while (j != s.size() && !isspace(s[j])) ++j;
      if (i != j) {
         ret.push_back(s.substr(i, j-i));
         i = j;
      }
  }
  return ret;
}

//_____________________________________________________________________________
UInt_t ParityData::header_str_to_base16(const char* hdr) {
// Utility to convert string header to base 16 integer
  static const char chex[] = "0123456789abcdef";
  if( !hdr ) return 0;
  const char* p = hdr+strlen(hdr);
  UInt_t result = 0;  UInt_t power = 1;
  while( p-- != hdr ) {
    const char* q = strchr(chex,tolower(*p));
    if( q ) {
      result += (q-chex)*power; 
      power *= 16;
    }
  }
  return result;
};

//_____________________________________________________________________________
void ParityData::TrigBits(UInt_t ibit, BdataLoc *dataloc) {
// Figure out which triggers got a hit.  These are multihit TDCs, so we
// need to sort out which hit we want to take by applying cuts.

  if( ibit >= kBitsPerByte*sizeof(UInt_t) ) return; //Limit of evtypebits
  bits.ResetBitNumber(ibit);

  static const UInt_t cutlo = 400;
  static const UInt_t cuthi = 2500;

  //  cout << "Bit TDC num hits "<<dataloc->NumHits()<<endl;
    for (int ihit = 0; ihit < dataloc->NumHits(); ihit++) {
      //      cout << "TDC data " << ibit<<"  "<<dataloc->Get(ihit)<<endl;

  if (dataloc->Get(ihit) > cutlo && dataloc->Get(ihit) < cuthi) {
      bits.SetBitNumber(ibit);
      evtypebits |= BIT(ibit);
    }
  }

}
//_____________________________________________________________________________
ClassImp(ParityData)

