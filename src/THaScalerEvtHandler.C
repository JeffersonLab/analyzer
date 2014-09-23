////////////////////////////////////////////////////////////////////
//
//   THaScalerEvtHandler
//
//   Prototype event handler for scalers.
//   R. Michaels,  Sept, 2014
//
//   This class does the following
//      For a particular set of event types (here, just 140)
//      decode the scalers and put some variables into global variables.
//      The global variables can then appear in the Podd output tree T.
//      In addition, a tree "TS" is created by this class; it contains 
//      just the scaler data by itself.  Note, the "fName" is concatenated
//      with "TS" to ensure the tree is unqiue; further, "fName" is
//      concatenated with the name of the global variables, for uniqueness. 
//      The list of global variables and how they are tied to the 
//      scaler module and channels is defined here; eventually this
//      will be modified to use a scaler.map file
//
/////////////////////////////////////////////////////////////////////

#include "THaEvtTypeHandler.h"
#include "THaScalerEvtHandler.h"
#include "GenScaler.h"
#include "Scaler3800.h"
#include "Scaler3801.h"
#include "Scaler1151.h"
#include "Scaler560.h"
#include "THaCodaData.h"
#include "THaEvData.h"
#include "TNamed.h"
#include "TMath.h"
#include "TString.h"
#include <iostream>
#include <sstream>

using namespace std;

THaScalerEvtHandler::THaScalerEvtHandler(const char *name, const char* description) : THaEvtTypeHandler(name,description) {
  rdata = new Int_t[MAXTEVT];
  fDebugFile = 0;
  fScalerTree = 0;
  evcount = 0;
  ifound = 0;
}

THaScalerEvtHandler::~THaScalerEvtHandler() { 
  delete [] rdata;
  if (fDebugFile) {
    fDebugFile->close();
    delete fDebugFile;
  }
  if (fScalerTree) {
    delete fScalerTree;
  }
}

Int_t THaScalerEvtHandler::End( THaRunBase* r) {
  if (fScalerTree) fScalerTree->Write();   
}

Int_t THaScalerEvtHandler::Analyze(THaEvData *evdata) {

  Int_t ldebug=1;
  Int_t lfirst=1;

  if ( !IsMyEvent(evdata->GetEvType()) ) return -1;

  if (fDebugFile) {
     *fDebugFile << endl << "---------------------------------- "<<endl<<endl;
     *fDebugFile << "\nEnter THaScalerEvtHandler  for fName = "<<fName<<endl;
     Dump(evdata);
  }

  if (lfirst && !fScalerTree) {

    lfirst = 0; // Can't do this in Init for some reason

    cout << "THaScalerEvtHandler   name = "<<fName<<endl;

    TString sname1 = "TS";
    TString sname2 = sname1 + fName;
    TString sname3 = fName + "  Scaler Data  =================================";

    if (fDebugFile) {
        *fDebugFile << "\nAnalyze 1st time for fName = "<<fName<<endl;
        *fDebugFile << sname2 << "      " <<sname3<<endl;
    }

    fScalerTree = new TTree(sname2.Data(),sname3.Data());
    fScalerTree->SetAutoSave(200000000);

    TString name, tinfo;

    name = "evcount";
    tinfo = name + "/D";
    fScalerTree->Branch(name.Data(), &evcount, tinfo.Data(), 4000);

    for (Int_t i = 0; i < scalerloc.size(); i++) {
      name = scalerloc[i]->name;
      tinfo = name + "/D";
      fScalerTree->Branch(name.Data(), &dvars[i], tinfo.Data(), 4000);
    }

  }  // if (lfirst && !fScalerTree)


// Parse the data, load local data arrays.

  Int_t ndata = evdata->GetEvLength();
  if (ndata >= MAXTEVT) {
    cout << "THaScalerEvtHandler:: ERROR: Event length crazy "<<endl;
    ndata = MAXTEVT-1;
  }

  if (fDebugFile) *fDebugFile<<"\n\nTHaScalerEvtHandler :: Debugging event type "<<dec<<evdata->GetEvType()<<endl<<endl;

  // local copy of data

  for (Int_t i=0; i<ndata; i++) rdata[i] = evdata->GetRawData(i);

  Int_t nskip=0;
  Int_t *p = rdata;
  Int_t *pstop = rdata+ndata;
  int j=0;
  
  ifound = 0;

  while (p < pstop && j < ndata) {
    if (fDebugFile) *fDebugFile << "p  and  pstop  "<<j++<<"   "<<p<<"   "<<pstop<<"   "<<hex<<*p<<"   "<<dec<<endl;   
    nskip = 1;
    for (Int_t j=0; j<scalers.size(); j++) {
       nskip = scalers[j]->Decode(p);
       if (fDebugFile && nskip > 1) {
	 *fDebugFile << "\n===== Scaler # "<<j<<"     fName = "<<fName<<"   nskip = "<<nskip<<endl;
          scalers[j]->DebugPrint(fDebugFile);
       }
       if (nskip > 1) {
	 ifound = 1;
         break;
       }
    }
    p = p + nskip;
  }

  if (*fDebugFile) {
    *fDebugFile << "Finished with decoding.  "<<endl;
    *fDebugFile << "   Found flag   =  "<<ifound<<endl;
  }

// L-HRS has headers which are different from R-HRS, but both are
// event type 140 and come here.  If you found no headers, it was
// the other arms event type.  (The arm is fName).

  if (!ifound) return 0;   

  // The correspondance between dvars and the scaler and the channel
  // will be driven by a scaler.map file  -- later

  for (Int_t i = 0; i < scalerloc.size(); i++) {
    Int_t ivar = scalerloc[i]->ivar;
    Int_t isca = scalerloc[i]->iscaler;
    Int_t ichan = scalerloc[i]->ichan;
    if (fDebugFile) *fDebugFile << "Debug dvars "<<i<<"   "<<ivar<<"  "<<isca<<"  "<<ichan<<endl;    if ((ivar >= 0 && ivar < scalerloc.size()) &&
        (isca >= 0 && isca < scalers.size()) &&
        (ichan >= 0 && ichan < MAXCHAN)) {
          if (scalerloc[ivar]->ikind == ICOUNT) dvars[ivar] = scalers[isca]->GetData(ichan);
          if (scalerloc[ivar]->ikind == IRATE)  dvars[ivar] = scalers[isca]->GetRate(ichan);
          if (fDebugFile) *fDebugFile << "   dvars  "<<scalerloc[ivar]->ikind<<"  "<<dvars[ivar]<<endl;
    } else {
      cout << "THaScalerEvtHandler:: ERROR:: incorrect index "<<ivar<<"  "<<isca<<"  "<<ichan<<endl;
    }
  }

  evcount = evcount + 1.0;

  for (Int_t j=0; j<scalers.size(); j++) scalers[j]->Clear("");

  if (fDebugFile) *fDebugFile << "scaler tree ptr  "<<fScalerTree<<endl;

  if (fScalerTree) fScalerTree->Fill();

  return 1;
}

THaAnalysisObject::EStatus THaScalerEvtHandler::Init(const TDatime& dt, Int_t idebug) {

  eventtypes.push_back(140);  // what events to look for

  TString dfile;
  dfile = fName + "scaler.txt";

  if (idebug) {
    fDebugFile = new ofstream();
    fDebugFile->open(dfile.Data());
  }

  ifstream mapfile;
  TString sname = "scalevt.map";
  TString sname1;
  sname1 = fName+sname;
  mapfile.open(sname1.Data());
  if (!mapfile) {
     cout << "THaScalerEvtHandler:: Cannot find scaler.map file "<<sname1<<endl;
     return kInitError;
  }
  string sinput;
  size_t minus1 = -1;
  size_t pos1;
  string scomment = "#";
  string svariable = "variable";
  string smap = "map";
  vector<string> dbline;
  while( getline(mapfile, sinput)) {
    dbline = vsplit(sinput);
    if (dbline.size() > 0) {
      pos1 = FindNoCase(dbline[0],scomment);
      if (pos1 != minus1) continue;
      pos1 = FindNoCase(dbline[0],svariable);
      if (pos1 != minus1 && dbline.size()>4) {
	string sdesc = "";
        for (UInt_t j=5; j<dbline.size(); j++) sdesc = sdesc+" "+dbline[j];
        Int_t isca = atoi(dbline[1].c_str());
        Int_t ichan = atoi(dbline[2].c_str());
        Int_t ikind = atoi(dbline[3].c_str());
        *fDebugFile << "add var "<<dbline[0]<<"   desc = "<<sdesc<<"   "<<isca<<"  "<<ichan<<"  "<<ikind<<endl;
        TString tsname(dbline[0].c_str());
        TString tsdesc(sdesc.c_str());
        AddVars(tsname,tsdesc,isca,ichan,ikind);
      }
      pos1 = FindNoCase(dbline[0],smap);
      if (pos1 != minus1 && dbline.size()>7) {
        Int_t imodel = atoi(dbline[1].c_str());
        Int_t icrate = atoi(dbline[2].c_str());
        Int_t slot = atoi(dbline[3].c_str());
        Int_t header = atoi(dbline[4].c_str());
        Int_t mask = atoi(dbline[5].c_str());
        Int_t inorm  = atoi(dbline[6].c_str());
        Int_t clkchan = -1;
        Double_t clkfreq = 1;
        if (inorm == 0 && dbline.size()>8) {
          clkchan = atoi(dbline[7].c_str());
	  chkfreq = 1.0*atoi(dbline[8].c_str());
	}

  // print everything ...

        switch (imodel) {
  	   case 560:
	     scalers.push_back(new Scaler560(icrate, islot));
           case 1151:
	     scalers.push_back(new Scaler1151(icrate, islot));
  	   case 3800:
	     scalers.push_back(new Scaler3800(icrate, islot));
  	   case 3801:
	     scalers.push_back(new Scaler3801(icrate, islot));
	}
        if (scalers.size() > 0) {
	  UInt_t idx = scalers.size()-1;
          scalers[idx]->SetHeader(header, mask);       
          if (clkchan >= 0) scalers[idx]->SetClock(defaultDT, clkchan, clkfreq);

 // need to do LoadNormScaler after scalers created and if inrom >= 0

	}
      }
    }
  }
  


  cout << "We are initializing THaScalerEvtHandler !!   name =   "<<fName<<endl;

#if HARDCODED
  // This will be driven by a scaler.map and will be a kind of loop.
  if (fName == "Left") {
      AddVars("TSbcmu1", "BCM x1 counts", 1, 4, ICOUNT);
      AddVars("TSbcmu1r","BCM x1 rate",  1, 4, IRATE);
      AddVars("TSbcmu3", "BCM u3 counts", 1, 5, ICOUNT);
      AddVars("TSbcmu3r", "BCM u3 rate",  1, 5, IRATE);
  } else {
      AddVars("TSbcmu1", "BCM x1 counts", 0, 4, ICOUNT);
      AddVars("TSbcmu1r","BCM x1 rate",  0, 4, IRATE);
      AddVars("TSbcmu3", "BCM u3 counts", 0, 5, ICOUNT);
      AddVars("TSbcmu3r", "BCM u3 rate",  0, 5, IRATE);
  }
#endif


  DefVars();



#if HARDCODED
// This should be driven by a scaler.map.  Hard-coded for now.
  if (fName == "Left") {
    scalers.push_back(new Scaler1151(1,0));
    scalers.push_back(new Scaler3800(1,1));
    scalers.push_back(new Scaler3800(1,2));
    scalers.push_back(new Scaler3800(1,3));
    scalers[0]->SetHeader(0xabc00000, 0xffff0000);
    scalers[1]->SetHeader(0xabc10000, 0xffff0000);
    scalers[2]->SetHeader(0xabc20000, 0xffff0000);
    scalers[3]->SetHeader(0xabc30000, 0xffff0000);
    scalers[0]->LoadNormScaler(scalers[1]);
    scalers[1]->SetClock(4, 7, 1024);
    scalers[2]->LoadNormScaler(scalers[1]);
    scalers[3]->LoadNormScaler(scalers[1]);
  } else {
    scalers.push_back(new Scaler3800(2,0));
    scalers.push_back(new Scaler3800(2,0));
    scalers.push_back(new Scaler1151(2,1));
    scalers.push_back(new Scaler1151(2,2));
    scalers[0]->SetHeader(0xceb00000, 0xffff0000);
    scalers[1]->SetHeader(0xceb10000, 0xffff0000);
    scalers[2]->SetHeader(0xceb20000, 0xffff0000);
    scalers[3]->SetHeader(0xceb30000, 0xffff0000);
    scalers[0]->SetClock(4, 7, 1024);
    scalers[1]->LoadNormScaler(scalers[0]);
    scalers[2]->LoadNormScaler(scalers[0]);
    scalers[3]->LoadNormScaler(scalers[0]);
  }
#endif


  if(fDebugFile) *fDebugFile << "THaScalerEvtHandler:: Name of scaler bank "<<fName<<endl;
  for (Int_t i=0; i<scalers.size(); i++) {
    if(fDebugFile) {
         *fDebugFile << "Scaler  #  "<<i<<endl;
         scalers[i]->DebugPrint(fDebugFile);
    }
  }
  return kOK;
}

void THaScalerEvtHandler::AddVars(TString name, TString desc, Int_t iscal, Int_t ichan, Int_t ikind) {
// need to add fName here to make it a unique variable.  (Left vs Right HRS, for example)
  TString name1 = fName + name;
  TString desc1 = fName + desc;
  ScalerLoc *loc = new ScalerLoc(name1, desc1, iscal, ichan, ikind);
  loc->ivar = scalerloc.size();  // ivar will be the pointer to the dvars array.
  scalerloc.push_back(loc);
}

void THaScalerEvtHandler::DefVars() {
// called after AddVars has finished being called.
  Nvars = scalerloc.size();
  if (Nvars == 0) return;
  dvars = new Double_t[Nvars];  // dvars is a member of this class 
  if (gHaVars) { 
    if(fDebugFile) *fDebugFile << "THaScalerEVtHandler:: Have gHaVars "<<gHaVars<<endl;
  } else {
    cout << "No gHaVars ?!  Well, that's a problem !!"<<endl;
    return;
  }
  if(fDebugFile) *fDebugFile << "THaScalerEvtHandler:: scalerloc size "<<scalerloc.size()<<endl;
  const Int_t* count = 0;
  for (UInt_t i = 0; i < scalerloc.size(); i++) {
    gHaVars->DefineByType(scalerloc[i]->name.Data(), scalerloc[i]->description.Data(),  
                  &dvars[i], kDouble, count);    
  }
}

vector<string> THaScalerEvtHandler::vsplit(const string& s) {
// split a string into whitespace-separated strings
  vector<string> ret;
  typedef string::size_type string_size;
  string_size i = 0;
  while ( i != s.size()) {
    while (i != s.size() && isspace(s[i])) ++i;
      string_size j = i;
      while (j != s.size() && !isspace(s[j])) ++j;
      if (i != j) {
         ret.push_back(s.substr(i, j-i));
         i = j;
      }
  }
  return ret;
};

size_t THaScalerEvtHandler::FindNoCase(const string sdata, const string skey) {
// Find iterator of word "sdata" where "skey" starts.  Case insensitive.
  string sdatalc, skeylc;
  sdatalc = "";  skeylc = "";
  for (string::const_iterator p = 
   sdata.begin(); p != sdata.end(); p++) {
      sdatalc += tolower(*p);
  } 
  for (string::const_iterator p = 
   skey.begin(); p != skey.end(); p++) {
      skeylc += tolower(*p);
  } 
  if (sdatalc.find(skeylc,0) == string::npos) return -1;
  return sdatalc.find(skeylc,0);
};

ClassImp(THaScalerEvtHandler)
