//*-- Author :    Robert Michaels   05/08/2002

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
// Defines the tree and histogram output for THaAnalyzer.  
// This class reads a file 'output.def' (an example is in /examples)
// to define which global variables, including arrays, and formulas
// (THaFormula's), and histograms go to the ROOT output.
//
// author:  R. Michaels    Sept 2002
//
//
//////////////////////////////////////////////////////////////////////////

#include "THaOutput.h"
#include "TROOT.h"
#include "THaFormula.h"
#include "THaVform.h"
#include "THaVhist.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "THaCut.h"
#include "TH1.h"
#include "TH2.h"
//#include "TTree.h"
#include "TFile.h"
#include "TRegexp.h"
#include "TError.h"
#include "THaScalerGroup.h"
#include "THaScaler.h"
#include "THaEvData.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>

using namespace std;
typedef vector<THaOdata*>::size_type Vsiz_t;
typedef vector<THaVform*>::size_type Vsiz_f;
typedef vector<THaVhist*>::size_type Vsiz_h;
typedef vector<THaString*>::size_type Vsiz_s;

Int_t THaOutput::fgVerbose = 1;

//_____________________________________________________________________________
THaOutput::THaOutput() :
   fNvar(0), fVar(NULL), fEpicsVar(0), fTree(NULL), 
   fEpicsTree(NULL), fInit(false)
{
  // Constructor
}

//_____________________________________________________________________________
THaOutput::~THaOutput() 
{
  // Destructor

  // Delete Trees and histograms only if ROOT system is initialized.
  // ROOT will report being uninitialized if we're called from the TSystem
  // destructor, at which point the trees already have been deleted.
  // FIXME: Trees would also be deleted if deleting the output file, right?
  // Can we use this here?
  Bool_t alive = TROOT::Initialized();
  if( alive ) {
    if (fTree) delete fTree;
    if (fEpicsTree) delete fEpicsTree;
  }
  if (fVar) delete [] fVar;
  if (fEpicsVar) delete [] fEpicsVar;
  if( alive ) {
    for (std::map<string, TTree*>::iterator mt = fScalTree.begin();
	 mt != fScalTree.end(); mt++) delete mt->second;
  }
  for (std::vector<THaOdata* >::iterator od = fOdata.begin();
       od != fOdata.end(); od++) delete *od;
  for (std::vector<THaVform* >::iterator itf = fFormulas.begin();
       itf != fFormulas.end(); itf++) delete *itf;
  for (std::vector<THaVform* >::iterator itf = fCuts.begin();
       itf != fCuts.end(); itf++) delete *itf;
  for (std::vector<THaVhist* >::iterator ith = fHistos.begin();
       ith != fHistos.end(); ith++) delete *ith;
}

//_____________________________________________________________________________
Int_t THaOutput::Init( const char* filename ) 
{
  // Initialize output system. Required before anything can happen.
  //
  // Only re-attach to variables and re-compile cuts and formulas
  // upon re-initialization (eg: continuing analysis with another file)
  if( fInit ) {
    cout << "\nTHaOutput::Init: Info: THaOutput cannot be completely"
	 << " re-initialized. Keeping existing definitions." << endl;
    cout << "Global Variables are being re-attached and formula/cuts"
	 << " are being re-compiled." << endl;
    
    // Assign pointers and recompile stuff reliant on pointers.

    if ( Attach() ) return -4;

    Print();

    return 1;
  }

  if( !gHaVars ) return -2;

  fTree = new TTree("T","Hall A Analyzer Output DST");

  fOpenEpics  = kFALSE;
  fOpenScal   = kFALSE;
  fFirstEpics = kTRUE; 
  fFirstScal  = kTRUE;
  fScalBank   = "nothing";
  fScalRC     = kRate;

  Int_t err = LoadFile( filename );
  if( err == -1 )
    return 0;       // No error if file not found, but please
                    // read the instructions.
  else if( err != 0 ) {
    delete fTree; fTree = NULL;
    return -3;
  }

  fNvar = fVarnames.size();  // this gets reassigned below
  fArrayNames.clear();
  fVNames.clear();

  THaVar *pvar;
  for (Int_t ivar = 0; ivar < fNvar; ivar++) {
    pvar = gHaVars->Find(fVarnames[ivar].c_str());
    if (pvar) {
      if (pvar->IsArray()) {
	fArrayNames.push_back(fVarnames[ivar]);
        fOdata.push_back(new THaOdata());
      } else {
	fVNames.push_back(fVarnames[ivar]);
      }
    } else {
      cout << "\nTHaOutput::Init: WARNING: Global variable ";
      cout << fVarnames[ivar] << " does not exist. "<< endl;
      cout << "There is probably a typo error... "<<endl;
    }
  }
  string tinfo;
  Int_t status;
  for (Vsiz_f iform = 0; iform < fFormnames.size(); iform++) {
    tinfo = Form("f%d",iform);
    fFormulas.push_back(new THaVform("formula",
        fFormnames[iform].c_str(), fFormdef[iform].c_str()));
    status = fFormulas[iform]->Init();
    if ( status != 0) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << fFormnames[iform] << endl;
      cout << "There is probably a typo error... " << endl;
      if( fgVerbose<=2 )
	fFormulas[iform]->ErrPrint(status);
      else
	fFormulas[iform]->LongPrint();  // for debug
    } else {
      fFormulas[iform]->SetOutput(fTree);
// Add variables (i.e. those var's used by the formula) to tree.
// Reason is that TTree::Draw() may otherwise fail with ERROR 26 
      vector<THaString> avar = fFormulas[iform]->GetVars();
      for (Vsiz_s k = 0; k < avar.size(); k++) {
        THaString svar = StripBracket(avar[k]);
        pvar = gHaVars->Find(svar.c_str());
        if (pvar) {
          if (pvar->IsArray()) {
            fArrayNames.push_back(svar);
            fOdata.push_back(new THaOdata());
          } else {
   	    fVNames.push_back(svar);
	  }
	}
      }
    }
  }
  for (Vsiz_t k = 0; k < fOdata.size(); k++)
    fOdata[k]->AddBranches(fTree, fArrayNames[k]);
  fNvar = fVNames.size();
  fVar = new Double_t[fNvar];
  for (Int_t k = 0; k < fNvar; k++) {
    tinfo = fVNames[k] + "/D";
    fTree->Branch(fVNames[k].c_str(), &fVar[k], tinfo.c_str(), kNbout);
  }
  for (Vsiz_f icut = 0; icut < fCutnames.size(); icut++) {
    fCuts.push_back(new THaVform("cut",
        fCutnames[icut].c_str(), fCutdef[icut].c_str()));
    status = fCuts[icut]->Init(); 
    if ( status != 0 ) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << fCutnames[icut] << endl;
      cout << "There is probably a typo error... " << endl;
      fCuts[icut]->ErrPrint(status);
    } else {
      fCuts[icut]->SetOutput(fTree);
    }
    if( fgVerbose>2 )
      fCuts[icut]->LongPrint();  // for debug
  }
  for (Vsiz_h ihist = 0; ihist < fHistos.size(); ihist++) {
// After initializing formulas and cuts, must sort through
// histograms and potentially reassign variables.  
// A histogram variable or cut is either a string (which can 
// encode a formula) or an externally defined THaVform. 
    sfvarx = fHistos[ihist]->GetVarX();
    sfvary = fHistos[ihist]->GetVarY();
    for (Vsiz_f iform = 0; iform < fFormulas.size(); iform++) {
      THaString stemp(fFormulas[iform]->GetName());
      if (sfvarx.CmpNoCase(stemp) == 0) { 
	fHistos[ihist]->SetX(fFormulas[iform]);
      }
      if (sfvary.CmpNoCase(stemp) == 0) { 
	fHistos[ihist]->SetY(fFormulas[iform]);
      }
    }
    if (fHistos[ihist]->HasCut()) {
      scut   = fHistos[ihist]->GetCutStr();
      for (Vsiz_f icut = 0; icut < fCuts.size(); icut++) {
        THaString stemp(fCuts[icut]->GetName());
        if (scut.CmpNoCase(stemp) == 0) { 
	  fHistos[ihist]->SetCut(fCuts[icut]);
        }
      }
    }
    fHistos[ihist]->Init();
  }

  if (fEpicsKey.size() > 0) {
    fEpicsVar = new Double_t[fEpicsKey.size()+1];
    for (UInt_t i = 0; i < fEpicsKey.size(); i++) {
      tinfo = fEpicsKey[i] + "/D";
      fTree->Branch(fEpicsKey[i].c_str(), &fEpicsVar[i], 
        tinfo.c_str(), kNbout);
      fEpicsTree->Branch(fEpicsKey[i].c_str(), &fEpicsVar[i], 
        tinfo.c_str(), kNbout);
    }
    fEpicsTree->Branch("timestamp",&fEpicsVar[fEpicsKey.size()],
		       "timestamp/D", kNbout);
  }
  for (UInt_t i = 0; i < fScalerKey.size(); i++) {
    fScalerKey[i]->AddBranches(fTree);
    for (std::map<string, TTree*>::iterator mt = fScalTree.begin();
	 mt != fScalTree.end(); mt++) {
           THaString thisbank(mt->first);
           TTree *sctree = mt->second;
           if ( thisbank.CmpNoCase(fScalerKey[i]->GetBank()) != 0) 
                continue;
           fScalerKey[i]->AddBranches(sctree,"noprefix");
    }
  }

  Print();

  fInit = true;

  if ( Attach() ) 
    return -4;

  for (Vsiz_f icut=0; icut<fCuts.size(); icut++) 
      fCuts[icut]->SetOutput(fTree);

  for (Vsiz_f iform=0; iform<fFormulas.size(); iform++) 
      fFormulas[iform]->SetOutput(fTree);

  return 0;
}

void THaOutput::BuildList(std::vector<THaString > vdata) 
{
  // Build list of EPICS variables and
  // SCALER variables to add to output.

    if (vdata.size() == 0) return;
    if (vdata[0].CmpNoCase("begin") == 0) {
      if (vdata.size() < 2) return;
      if (vdata[1].CmpNoCase("epics") == 0) fOpenEpics = kTRUE;
      if (vdata[1].CmpNoCase("scalers") == 0) fOpenScal = kTRUE;
    }
    if (vdata[0].CmpNoCase("end") == 0) {
      if (vdata.size() < 2) return;
      if (vdata[1].CmpNoCase("epics") == 0) fOpenEpics = kFALSE;
      if (vdata[1].CmpNoCase("scalers") == 0) fOpenScal = kFALSE;
    }
    if (fOpenEpics && fOpenScal) {
       cout << "THaOutput::ERROR: Syntax error in output.def"<<endl;
       cout << "Must 'begin' and 'end' before 'begin' again."<<endl;
       cout << "e.g. 'begin epics' ..... 'end epics'"<<endl;
       return ;
    }
    if (fOpenEpics) {
       if (fFirstEpics) {
           if (!fEpicsTree)
              fEpicsTree = new TTree("E","Hall A Epics Data");
           fFirstEpics = kFALSE;
           return;
       } else {
           fEpicsKey.push_back(vdata[0]);
       }
    } else {
       fFirstEpics = kTRUE;
    }
    if (fOpenScal) {
       if (fFirstScal) {
         fFirstScal = kFALSE;
         if (vdata.size() < 3) {
           cout << "THaOutput: ERROR: need to specify scaler bank."<<endl;
           cout << "Syntax is 'begin scaler right' for right bank."<<endl;
           return;
	 }
         fScalBank = vdata[2].ToLower();  
         std::string desc = "Hall A Scalers on " + fScalBank;
         fScalTree.insert(make_pair(fScalBank,
          new TTree(fScalBank.ToUpper().c_str(),desc.c_str())));
         return;
       } else {
  	 fScalRC = kRate;
         if (vdata.size() >= 3) {
// This is like 'mypulser 4 5' for slot 4, chan 5
// or 'mypulser 4 5 rate' or 'mupulser 4 5 counts'
             if (vdata.size() >= 4) {
	       if ((vdata[3].CmpNoCase("count") == 0) || 
                   (vdata[3].CmpNoCase("counts") == 0)) {
		 fScalRC = kCount;
	       }
	     }
             AddScaler(vdata[0], fScalBank, 0,
		    atoi(vdata[1].c_str()),atoi(vdata[2].c_str()));
	 } else {
// Default scalers = normalization scalers, like charge and triggers.
	   if (vdata[0].CmpNoCase("default") == 0) {
	     DefScaler();  return;
	   } else if (vdata[0].CmpNoCase("default_helicity") == 0) {
             DefScaler(1);  return;
	   } else {
  	     fScalRC = kRate;
             if (vdata.size() >= 2) {
	       if ((vdata[1].CmpNoCase("count") == 0) || 
                   (vdata[1].CmpNoCase("counts") == 0)) {
		 fScalRC = kCount;
	       }
	     }
     	     AddScaler(vdata[0],fScalBank);           
           }
	 }
       } 
    } else {
      fFirstScal = kTRUE;
    }
    return;
}

//_____________________________________________________________________________
void THaOutput::DefScaler(Int_t hel) {
// Set up a default list of normalization scalers.
// This is what most users care about.
// hel = helicity flag.  hel=0 --> not gated by helicity.
// hel=1 --> load the two helicity gated scaler banks.

// Note: All the dashes ("-") get converted to underbar ("_")
// in the tree variable names, since "-" is interpreted as
// the minus operation in TTree::Draw().  Sorry.

  THaString norm_scaler[] = {"trigger-1", "trigger-2", "trigger-3",
		  	    "trigger-4", "trigger-5", "trigger-6",
			    "trigger-7", "trigger-8", "trigger-9",
			    "trigger-10", "trigger-11", "trigger-12",
			    "bcm_u1", "bcm_u3", "bcm_u10",
			    "bcm_d1", "bcm_d3", "bcm_d10",
                            "clock", "TS-accept", "edtpulser",
                            "strobe" };

  Int_t nscal = 1;
  Int_t jhel = 0;
  if (hel == 1) {
     nscal = 2;  jhel = -1;
  }
  Int_t numchan = sizeof(norm_scaler)/sizeof(THaString);

  for (Int_t i = 0; i < nscal; i++) {
     jhel = jhel + 2*i; // 0, or (-1,1)
     for (Int_t j = 0; j < numchan; j++) {
        AddScaler(norm_scaler[j], fScalBank, jhel);
     }
  }

}


//_____________________________________________________________________________
void THaOutput::AddScaler(THaString name, 
       THaString bank, Int_t helicity, Int_t slot, Int_t chan) 
{
  Int_t scal_rc = 0;  // default: data are rates.
  if (fScalRC == kCount) scal_rc = 1;  // data are counts.
  fScalerKey.push_back(new THaScalerKey(name, bank, scal_rc, 
               helicity, slot, chan));
} 


//_____________________________________________________________________________
Int_t THaOutput::Attach() 
{
  // Get the pointers for the global variables
  // Also, sets the size of the fVariables and fArrays vectors
  // according to the size of the related names array
  
  if( !gHaVars ) return -2;

  THaVar *pvar;
  Int_t NAry = fArrayNames.size();
  Int_t NVar = fVNames.size();

  fVariables.resize(NVar);
  fArrays.resize(NAry);
  
  // simple variable-type names
  for (Int_t ivar = 0; ivar < NVar; ivar++) {
    pvar = gHaVars->Find(fVNames[ivar].c_str());
    if (pvar) {
      if ( !pvar->IsArray() ) {
	fVariables[ivar] = pvar;
      } else {
	cout << "\tTHaOutput::Attach: ERROR: Global variable " << fVNames[ivar]
	     << " changed from simple to array!! Leaving empty space for variable"
	     << endl;
	fVariables[ivar] = 0;
      }
    } else {
      cout << "\nTHaOutput::Attach: WARNING: Global variable ";
      cout << fVarnames[ivar] << " NO LONGER exists (it did before). "<< endl;
      cout << "This is not supposed to happen... "<<endl;
    }
  }

  // arrays
  for (Int_t ivar = 0; ivar < NAry; ivar++) {
    pvar = gHaVars->Find(fArrayNames[ivar].c_str());
    if (pvar) {
      if ( pvar->IsArray() ) {
	fArrays[ivar] = pvar;
      } else {
	cout << "\tTHaOutput::Attach: ERROR: Global variable " << fVNames[ivar]
	     << " changed from ARRAY to Simple!! Leaving empty space for variable"
	     << endl;
	fArrays[ivar] = 0;
      }
    } else {
      cout << "\nTHaOutput::Attach: WARNING: Global variable ";
      cout << fVarnames[ivar] << " NO LONGER exists (it did before). "<< endl;
      cout << "This is not supposed to happen... "<<endl;
    }
  }

  // Reattach formulas, cuts, histos

  for (Vsiz_f iform=0; iform<fFormulas.size(); iform++) {
    fFormulas[iform]->ReAttach();
  }

  for (Vsiz_f icut=0; icut<fCuts.size(); icut++) {
    fCuts[icut]->ReAttach(); 
  }

  for (Vsiz_h ihist = 0; ihist < fHistos.size(); ihist++) {
    fHistos[ihist]->ReAttach();
  }
     
  return 0;

}


//_____________________________________________________________________________
Int_t THaOutput::ProcEpics(THaEvData *evdata) 
{
  // Process the EPICS data, this fills the trees.
  // This function is called by THaAnalyzer.

  if ( !evdata->IsEpicsEvent() 
    || fEpicsKey.size()==0 
    || !fEpicsTree ) return 0;
  fEpicsVar[fEpicsKey.size()] = -1e32;
  for (UInt_t i = 0; i < fEpicsKey.size(); i++) {
    if (evdata->IsLoadedEpics(fEpicsKey[i].c_str())) {
      fEpicsVar[i] = 
         evdata->GetEpicsData(fEpicsKey[i].c_str());
 // fill time stamp (once is ok since this is an EPICS event)
      fEpicsVar[fEpicsKey.size()] =
 	 evdata->GetEpicsTime(fEpicsKey[i].c_str());
    } else {
      fEpicsVar[i] = -1e32;  // data not yet found
    }
  }
  if (fEpicsTree != 0) fEpicsTree->Fill();  
  return 1;
}

//_____________________________________________________________________________
Int_t THaOutput::ProcScaler(THaScalerGroup *scagrp) 
{
  // Process the scaler data, this fills the trees.
  // This function is called by THaAnalyzer.

  Bool_t did_fill = kFALSE;
  THaScaler *scaler = scagrp->GetScalerObj();
  if ( !scaler->IsRenewed()) return 0;
  THaString thisbank(scaler->GetName());

  for (UInt_t i = 0; i < fScalerKey.size(); i++) {

    if ( thisbank.CmpNoCase(fScalerKey[i]->GetBank()) != 0) continue;

    did_fill = kTRUE;

    if (fScalerKey[i]->SlotDefined()) {

      if (fScalerKey[i]->IsRate()) {
         fScalerKey[i]->Fill(scaler->GetScalerRate(
               fScalerKey[i]->GetSlot(), fScalerKey[i]->GetChan()));
      } else {
         fScalerKey[i]->Fill(scaler->GetScaler(
               fScalerKey[i]->GetSlot(), fScalerKey[i]->GetChan()));
      }     

    } else {

      if (fScalerKey[i]->IsRate()) {
         fScalerKey[i]->Fill(
                  scaler->GetNormRate(fScalerKey[i]->GetHelicity(), 
                        fScalerKey[i]->GetChanName().c_str()));
      } else {
         fScalerKey[i]->Fill(
                  scaler->GetNormData(fScalerKey[i]->GetHelicity(), 
                        fScalerKey[i]->GetChanName().c_str(), 0));
      }

    }

// DEBUG
// fScalerKey[i]->Print();

  }
			  
  
  TTree *sctree = fScalTree[thisbank.ToLower().c_str()];
  if (did_fill && sctree) sctree->Fill();

  return 1;
}

//_____________________________________________________________________________
Int_t THaOutput::Process() 
{
  // Process the variables, formulas, and histograms.
  // This is called by THaAnalyzer.

  for (Vsiz_f iform = 0; iform < fFormulas.size(); iform++) 
    if (fFormulas[iform]) fFormulas[iform]->Process();
  for (Vsiz_f icut = 0; icut < fCuts.size(); icut++) 
    if (fCuts[icut]) fCuts[icut]->Process();
  THaVar *pvar;
  for (Int_t ivar = 0; ivar < fNvar; ivar++) {
    pvar = fVariables[ivar];
    if (pvar) fVar[ivar] = pvar->GetValue();
  }
  for (Vsiz_t k = 0; k < fOdata.size(); k++) { 
    fOdata[k]->Clear();
    pvar = fArrays[k];
    if ( pvar == 0) continue;
    for (Int_t i = 0; i < pvar->GetLen(); i++) {
      if (fOdata[k]->Fill(i,pvar->GetValue(i)) != 1) {
	if( fgVerbose>0 )
	  cerr << "THaOutput::ERROR: storing too much variable sized data: " 
	       << pvar->GetName() <<"  "<<pvar->GetLen()<<endl;
      }
    }
  }
  for (Vsiz_h ihist = 0; ihist < fHistos.size(); ihist++) 
    fHistos[ihist]->Process();
  if (fTree != 0) fTree->Fill();  
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::End() 
{
  if (fTree != 0) fTree->Write();
  if (fEpicsTree != 0) fEpicsTree->Write();
  if( fgVerbose>1 )
    cout << "End:: fScalTree size "<<fScalTree.size()<<endl;
  for (std::map<string, TTree*>::iterator mt = fScalTree.begin();
       mt != fScalTree.end(); mt++) {
      TTree* tree = mt->second;
      if (tree) tree->Write();
  }
  for (Vsiz_h ihist = 0; ihist < fHistos.size(); ihist++) 
      fHistos[ihist]->End();
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::LoadFile( const char* filename ) 
{
  // Process the file that defines the output
  
  Int_t idx;
  if( !filename || !*filename || strspn(filename," ") == strlen(filename) ) {
    ::Error( "THaOutput::LoadFile", "invalid file name, "
	     "no output definition loaded" );
    return -1;
  }
  THaString loadfile(filename);
  ifstream* odef = new ifstream(loadfile.c_str());
  if ( ! (*odef) ) {
    ErrFile(-1, loadfile);
    return -1;
  }
  const string comment = "#";
  const string whitespace( " \t" );
  string::size_type pos;
  vector<THaString> strvect;
  THaString sline;
  while (getline(*odef,sline)) {
    // Blank line or comment line?
    if( sline.empty()
	|| (pos = sline.find_first_not_of( whitespace )) == string::npos
	|| comment.find(sline[pos]) != string::npos )
      continue;
    // Get rid of trailing comments
    if( (pos = sline.find_first_of( comment )) != string::npos )
      sline.erase(pos);
    // Split the line into tokens separated by whitespace
    strvect = sline.Split();
    BuildList(strvect);
    if (strvect.size() < 2) {
      ErrFile(0, sline);
      continue;
    }
    Int_t ikey = FindKey(strvect[0]);
    THaString sname = StripBracket(strvect[1]);
    switch (ikey) {
      case kVar:
  	  fVarnames.push_back(sname);
          break;
      case kForm:
          if (strvect.size() < 3) {
	    ErrFile(ikey, sline);
            continue;
	  }
          fFormnames.push_back(sname);
          fFormdef.push_back(strvect[2]);
          break;
      case kCut:
          if (strvect.size() < 3) {
	    ErrFile(ikey, sline);
            continue;
	  }
          fCutnames.push_back(sname);
          fCutdef.push_back(strvect[2]);
          break;
      case kH1f:
      case kH1d:
      case kH2f:
      case kH2d:
          if( ChkHistTitle(ikey, sline) != 1) {
	    ErrFile(ikey, sline);
            continue;
	  }
          fHistos.push_back(
	     new THaVhist(strvect[0],sname,stitle));
          idx = (long)fHistos.size()-1;
// Tentatively assign variables and cuts as strings. 
// Later will check if they are actually THaVform's.
          fHistos[idx]->SetX(nx, xlo, xhi, sfvarx);
          if (ikey == kH2f || ikey == kH2d) {
            fHistos[idx]->SetY(ny, ylo, yhi, sfvary);
	  }
          if (iscut != fgNocut) fHistos[idx]->SetCut(scut);
          break;
      case kBlock:
	  BuildBlock(sname);
	  break;
      case kBegin:
      case kEnd:
          break;
      default:
        if (!fOpenEpics && !fOpenScal)
          cout << "Warning: keyword "<<strvect[0]<<" undefined "<<endl;
    }
  }
  delete odef;

  // sort thru fVarnames, removing identical entries
  if( fVarnames.size() > 1 ) {
    sort(fVarnames.begin(),fVarnames.end());
    vector<THaString>::iterator Vi = fVarnames.begin();
    while ( (Vi+1)!=fVarnames.end() ) {
      if ( *Vi == *(Vi+1) ) {
	fVarnames.erase(Vi+1);
      } else {
	Vi++;
      }
    }
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::FindKey(const THaString& key) const
{
  // Return integer flag corresponding to
  // case-insensitive keyword "key" if it exists
  
  // Map of keywords to internal logical type numbers
  struct KeyMap {
    const char* name;
    EId keyval;
  };
  static const KeyMap keymap[] = { 
    { "variable", kVar },
    { "formula",  kForm },
    { "cut",      kCut },
    { "th1f",     kH1f },
    { "th1d",     kH1d },
    { "th2f",     kH2f },
    { "th2d",     kH2d },
    { "block",    kBlock },
    { "begin",    kBegin },
    { "end",      kEnd },
    { 0 }
  };

  if( const KeyMap* it = keymap ) {
    while( it->name ) {
      if( key.CmpNoCase( it->name ) == 0 )
	return it->keyval;
      it++;
    }
  }
  return -1;
}


//_____________________________________________________________________________
THaString THaOutput::StripBracket(THaString& var) const
{
// If the string contains "[anything]", we strip
// it away.  In practice this should not be fatal 
// because your variable will still show up in the tree.
  string::size_type pos1,pos2;
  THaString open_brack = "[";
  THaString close_brack = "]";
  THaString result = "";
  pos1 = var.find(open_brack,0);
  pos2 = var.find(close_brack,0);
  if ((pos1 != string::npos) &&
      (pos2 != string::npos)) {
      result = var.substr(0,pos1);
      result += var.substr(pos2+1,var.length());    
//      cout << "THaOutput:WARNING:: Stripping away";
//      cout << "unwanted brackets from "<<var<<endl;
  } else {
      result = var;
  }
  return result;
}


//_____________________________________________________________________________
void THaOutput::ErrFile(Int_t iden, const THaString& sline) const
{
  // Print error messages about the output definition file.
  if (iden == -1) {
    cerr << "<THaOutput::LoadFile> WARNING: file " << sline;
    cerr << " does not exist." << endl;
    cerr << "See $ANALYZER/examples/output.def for an example.\n";
    cerr << "Output will only contain event objects "
      "(this may be all you want).\n";
    return;
  }
  if (fOpenEpics || fOpenScal) return;  // No error
  cerr << "THaOutput::ERROR: Syntax error in output definition file."<<endl;
  cerr << "The offending line is :\n"<<sline<<endl<<endl;
  switch (iden) {
     case kVar:
       cerr << "For variables, the syntax is: "<<endl;
       cerr << "    variable  variable-name"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    variable   R.vdc.v2.nclust"<<endl;;
     case kCut:
     case kForm:
       cerr << "For formulas or cuts, the syntax is: "<<endl;
       cerr << "    formula(or cut)  formula-name  formula-expression"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    formula targetX 1.464*B.bpm4b.x-0.464*B.bpm4a.x"<<endl;
     case kH1f:
     case kH1d:
       cerr << "For 1D histograms, the syntax is: "<<endl;
       cerr << "  TH1F(or TH1D) name  'title' ";
       cerr << "variable nbin xlo xhi [cut-expr]"<<endl;
       cerr << "Example: "<<endl;
       cerr << "  TH1F  tgtx 'target X' targetX 100 -2 2"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       cerr << "optionally can impose THaCut expression 'cut-expr'"<<endl;
       break;
     case kH2f:
     case kH2d:
       cerr << "For 2D histograms, the syntax is: "<<endl;
       cerr << "  TH2F(or TH2D)  name  'title'  varx  vary";
       cerr << "  nbinx xlo xhi  nbiny ylo yhi [cut-expr]"<<endl;
       cerr << "Example: "<<endl;
       cerr << "  TH2F t12 't1 vs t2' D.timeroc1  D.timeroc2";
       cerr << "  100 0 20000 100 0 35000"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       cerr << "optionally can impose THaCut expression 'cut-expr'"<<endl;
       break;
     default:
       cerr << "See the documentation or ask Bob Michaels"<<endl;
       break;
  }
}

//_____________________________________________________________________________
void THaOutput::Print() const
{
  // Printout the definitions. Amount printed depends on verbosity
  // level, set with SetVerbosity().
  typedef vector<THaString>::const_iterator Iter;
  if( fgVerbose > 0 ) {
    if( fVarnames.size() == 0 && fFormulas.size() == 0 &&
	fCuts.size() == 0 && fHistos.size() == 0 ) {
      ::Warning("THaOutput", "no output defined");
    } else {
      cout << endl << "THaOutput definitions: " << endl;
      if( fVarnames.size()>0 ) {
	cout << "=== Number of variables "<<fVarnames.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  for (Vsiz_s i = 0; i < fVarnames.size(); i++) {
	    cout << "Variable # "<<i<<" =  "<<fVarnames[i]<<endl;
	  }
	}
      }
      if( fFormulas.size()>0 ) {
	cout << "=== Number of formulas "<<fFormulas.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  for (Vsiz_f i = 0; i < fFormulas.size(); i++) {
	    cout << "Formula # "<<i<<endl;
	    if( fgVerbose>2 )
	      fFormulas[i]->LongPrint();
	    else
	      fFormulas[i]->ShortPrint();
	  }
	}
      }
      if( fCuts.size()>0 ) {
	cout << "=== Number of cuts "<<fCuts.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  for (Vsiz_f i = 0; i < fCuts.size(); i++) {
	    cout << "Cut # "<<i<<endl;
	    if( fgVerbose>2 )
	      fCuts[i]->LongPrint();
	    else
	      fCuts[i]->ShortPrint();
	  }
	}
      }
      if( fHistos.size()>0 ) {
	cout << "=== Number of histograms "<<fHistos.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  for (Vsiz_h i = 0; i < fHistos.size(); i++) {
	    cout << "Histogram # "<<i<<endl;
	    fHistos[i]->Print();
	  }
	}
      }
      cout << endl;
    }
  }
}

//_____________________________________________________________________________
Int_t THaOutput::ChkHistTitle(Int_t iden, const THaString& sline)
{
// Parse the string that defines the histogram.  
// The title must be enclosed in single quotes (e.g. 'my title').  
// Ret value 'result' means:  -1 == error,  1 == everything ok.
  Int_t result = -1;
  stitle = "";   sfvarx = "";  sfvary  = "";
  iscut = fgNocut;  scut = "";
  nx = 0; ny = 0; xlo = 0; xhi = 0; ylo = 0; yhi = 0;
  THaString::size_type pos1 = sline.find_first_of("'");
  THaString::size_type pos2 = sline.find_last_of("'");
  if (pos1 != THaString::npos && pos2 > pos1) {
    stitle = sline.substr(pos1+1,pos2-pos1-1);
  }
  THaString ctemp = sline.substr(pos2+1,sline.size()-pos2);
  vector<THaString> stemp = ctemp.Split();
  if (stemp.size() > 1) {
     sfvarx = stemp[0];
     Int_t ssize = stemp.size();
     if (ssize == 4 || ssize == 5) {
       sscanf(stemp[1].c_str(),"%d",&nx);
       sscanf(stemp[2].c_str(),"%f",&xlo);
       sscanf(stemp[3].c_str(),"%f",&xhi);
       if (ssize == 5) {
         iscut = 1; scut = stemp[4];
       }
       result = 1;
     }
     if (ssize == 8 || ssize == 9) {
       sfvary = stemp[1];
       sscanf(stemp[2].c_str(),"%d",&nx);
       sscanf(stemp[3].c_str(),"%f",&xlo);
       sscanf(stemp[4].c_str(),"%f",&xhi);
       sscanf(stemp[5].c_str(),"%d",&ny);
       sscanf(stemp[6].c_str(),"%f",&ylo);
       sscanf(stemp[7].c_str(),"%f",&yhi);
       if (ssize == 9) {
         iscut = 1; scut = stemp[8]; 
       }
       result = 2;
     }
  }
  if (result != 1 && result != 2) return -1;
  if ((iden == kH1f || iden == kH1d) && 
       result == 1) return 1;  // ok
  if ((iden == kH2f || iden == kH2d) && 
       result == 2) return 1;  // ok
  return -1;
}

//_____________________________________________________________________________
Int_t THaOutput::BuildBlock(const THaString& blockn)
{
  // From the block name, identify and save a specific grouping
  // of global variables by adding them to the fVarnames list.
  //
  // For efficiency, at the end of building the list we should
  // ensure that variables are listed only once.
  //
  // Eventually, we can have some specially named blocks,
  // but for now we simply will use pattern matching, such that
  //   block L.*
  // would save all variables from the left spectrometer.


  TRegexp re(blockn.c_str(),kTRUE);
  TIter next(gHaVars);
  TObject *obj;

  Int_t nvars=0;
  while ((obj = next())) {
    TString s = obj->GetName();
    if ( s.Index(re) != kNPOS ) {
      s.Append('\0');
      THaString vn(s.Data());
      fVarnames.push_back(vn);
      nvars++;
    }
  }
  return nvars;
}

//_____________________________________________________________________________
void THaOutput::SetVerbosity( Int_t level )
{
  // Set verbosity level for debug messages

  fgVerbose = level;
}

//_____________________________________________________________________________
ClassImp(THaOdata)
ClassImp(THaOutput)
