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
#include "THaVform.h"
#include "THaVhist.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaTextvars.h"
#include "THaGlobals.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"
#include "TRegexp.h"
#include "TError.h"
#include "THaScalerGroup.h"
#include "THaScaler.h"
#include "THaEvData.h"
#include "THaString.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>
//#include <iterator>

#include "THaBenchmark.h"

using namespace std;
using namespace THaString;

typedef vector<THaOdata*>::iterator Iter_o_t;
typedef vector<THaVform*>::iterator Iter_f_t;
typedef vector<THaVhist*>::iterator Iter_h_t;
typedef vector<string>::iterator Iter_s_t;

Int_t THaOutput::fgVerbose = 1;
//FIXME: these should be member variables
static Bool_t fgDoBench = kFALSE;
static THaBenchmark fgBench;

//_____________________________________________________________________________
class THaEpicsKey {
// Utility class used by THaOutput to store a list of
// 'keys' to access EPICS data 'string=num' assignments
public:
  THaEpicsKey(const std::string &nm) : fName(nm) 
     { fAssign.clear(); }
  void AddAssign(const std::string& input) {
// Add optional assignments.  The input must
// be of the form "epicsvar=number" 
// epicsvar can be a string with spaces if
// and only if its enclosed in single spaces.
// E.g. "Yes=1" "No=2", "'RF On'", etc
   typedef map<string, double>::value_type valType;
   string::size_type pos;
   string sdata,temp1,temp2;
   sdata = findQuotes(input);
   pos = sdata.find("=");
   if (pos != string::npos) {
      temp1.assign(sdata.substr(0,pos));
      temp2.assign(sdata.substr(pos+1,sdata.length()));
// In case someone used "==" instead of "="
      if (temp2.find("=") != string::npos) 
            temp2.assign
              (sdata.substr(pos+2,sdata.length()));
      if (strlen(temp2.c_str()) > 0) {
        double tdat = atof(temp2.c_str());
        fAssign.insert(valType(temp1,tdat));
      } else {  // In case of "epicsvar="
        fAssign.insert(valType(temp1,0));
     }
   }
  };
  string findQuotes(const string& input) {
    string result,temp1;
    string::size_type pos1,pos2;
    result = input;
    pos1 = input.find("'");
    if (pos1 != string::npos) {
     temp1.assign(input.substr(pos1+1,input.length()));
     pos2 = temp1.find("'");
     if (pos2 != string::npos) {
      result.assign(temp1.substr(0,pos2));
      result += temp1.substr(pos2+1,temp1.length());
     }
    }
    return result;
  };
  Bool_t IsString() { return !fAssign.empty(); };
  Double_t Eval(const string& input) {
    if (fAssign.empty()) return 0;
    for (map<string,Double_t>::iterator pm = fAssign.begin();
	 pm != fAssign.end(); ++pm) {
      if (input == pm->first) {
        return pm->second;
      }
    }
    return 0;  
  };
  Double_t Eval(const TString& input) {
    return Eval(string(input.Data()));
  }
  const string& GetName() { return fName; };
private:
  string fName;
  map<string,Double_t> fAssign;
};

//_____________________________________________________________________________
class THaScalerKey {
// Utility class used by THaOutput to store a list
// of 'keys' to access scaler data for output.
public:
  THaScalerKey(const string& nm, const string& bk, Int_t rc,
	       Int_t hel, Int_t sl, Int_t ch) : 
    fData(0.0), fName(nm), fBank(bk), fRC(rc), fHelicity(hel),
    fSlot(sl), fChan(ch) {}
  THaScalerKey(const string& nm, const string& bk) : 
    fData(0.0), fName(nm), fBank(bk), fRC(0), fHelicity(0),
    fSlot(-1), fChan(-1) {}
  virtual ~THaScalerKey() {}
  void AddBranches(TTree *T, Bool_t addprefix = kTRUE);
  void Fill(Double_t dat) { fData = dat; };
  void Print() const;
  Double_t GetData() { return fData; };
  const string& GetChanName() { return fName; };
  const string& GetBank() { return fBank; };
  Bool_t SlotDefined() { return (fSlot >= 0); };
  Bool_t IsRate() { return (fRC == 0); };
  Bool_t IsCounts() { return !IsRate(); };
  Int_t GetHelicity() { return fHelicity; }; // 0 = none, +1,-1 = helicity
  Int_t GetSlot()  { return fSlot; };
  Int_t GetChan() { return fChan; };

private:

  THaScalerKey(const THaScalerKey& key);
  THaScalerKey& operator=(const THaScalerKey& key);
  static string DashToUbar(const string& var);
  Double_t fData;
  string fName, fBank;
  Int_t fRC, fHelicity, fSlot, fChan;
  //  ClassDef(THaScalerKey,0)  // Scaler data for output
};

//_____________________________________________________________________________
void THaScalerKey::AddBranches(TTree *T, Bool_t addprefix ) 
{
  string name;
  if (addprefix) name = fBank + "_";  
  if (GetHelicity() > 0) name += "P_"; 
  if (GetHelicity() < 0) name += "M_";
  name += fName;
  name = DashToUbar(name); // Can't have "-", replace with "_".
  string tinfo = name + "/D";
  T->Branch(name.c_str(), &fData, tinfo.c_str(), 4000);
}

//_____________________________________________________________________________
void THaScalerKey::Print() const 
{
  cout << "Scaler key name = "<<fName<<"   bank = "<<fBank;
  cout << "  sl = "<<fSlot;
  cout << "  ch = "<<fChan<<"  RC = "<<fRC<<"  hel = "<<fHelicity;
  cout << "  data = "<<fData<<endl;
}

//_____________________________________________________________________________
string THaScalerKey::DashToUbar(const string& var)
{
  // Replace "-" with "_" because "-" does not work in
  // a tree variable name: Draw() thinks it's minus.
  int pos0=0;
  string output = var;
  while (1) {
    int pos = output.find("-");
    if (pos > pos0) {
      output.replace(pos, 1, "_"); pos0 = pos;
    } else { break; }
  }
  return output;
}

//_____________________________________________________________________________
THaOdata::THaOdata( const THaOdata& other )
  : tree(other.tree), name(other.name), nsize(other.nsize) 
{
  data = new Double_t[nsize]; ndata = other.ndata;
  memcpy( data, other.data, nsize*sizeof(Double_t));
}

//_____________________________________________________________________________
THaOdata& THaOdata::operator=(const THaOdata& rhs )
{ 
  if( this != &rhs ) {
    tree = rhs.tree; name = rhs.name;
    if( nsize < rhs.nsize ) {
      nsize = rhs.nsize; delete [] data; data = new Double_t[nsize];
    }
    ndata = rhs.ndata; memcpy( data, rhs.data, nsize*sizeof(Double_t));
  }
  return *this;   
}

//_____________________________________________________________________________
void THaOdata::AddBranches( TTree* _tree, string _name )
{
  name = _name;
  tree = _tree;
  string sname = "Ndata." + name;
  string leaf = sname;
  tree->Branch(sname.c_str(),&ndata,(leaf+"/I").c_str());
  // FIXME: defined this way, ROOT always thinks we are variable-size
  leaf = "data["+leaf+"]/D";
  tree->Branch(name.c_str(),data,leaf.c_str());
}

//_____________________________________________________________________________
Bool_t THaOdata::Resize(Int_t i)
{
  static const Int_t MAX = 4096;
  if( i > MAX ) return true;
  Int_t newsize = nsize;
  while ( i >= newsize ) { newsize *= 2; } 
  Double_t* tmp = new Double_t[newsize];
  memcpy( tmp, data, nsize*sizeof(Double_t) );
  delete [] data; data = tmp; nsize = newsize;
  if( tree )
    tree->SetBranchAddress( name.c_str(), data );
  return false;
}

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
    for (map<string, TTree*>::iterator mt = fScalTree.begin();
	 mt != fScalTree.end(); ++mt) delete mt->second;
  }
  for (Iter_o_t od = fOdata.begin();
       od != fOdata.end(); ++od) delete *od;
  for (Iter_f_t itf = fFormulas.begin();
       itf != fFormulas.end(); ++itf) delete *itf;
  for (Iter_f_t itf = fCuts.begin();
       itf != fCuts.end(); ++itf) delete *itf;
  for (Iter_h_t ith = fHistos.begin();
       ith != fHistos.end(); ++ith) delete *ith;
  for (vector<THaScalerKey* >::iterator isca = fScalerKey.begin();
       isca != fScalerKey.end(); ++isca) delete *isca;
  for (vector<THaEpicsKey* >::iterator iep = fEpicsKey.begin();
       iep != fEpicsKey.end(); ++iep) delete *iep;
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

  if( fgDoBench ) fgBench.Begin("Init");

  fTree = new TTree("T","Hall A Analyzer Output DST");
  fTree->SetAutoSave(200000000);
  fOpenEpics  = kFALSE;
  fOpenScal   = kFALSE;
  fFirstEpics = kTRUE; 
  fFirstScal  = kTRUE;
  fScalBank   = "nothing";
  fScalRC     = kRate;

  Int_t err = LoadFile( filename );
  if( fgDoBench && err != 0 ) fgBench.Stop("Init");
    
  if( err == -1 ) {
    return 0;       // No error if file not found, but please
  }                    // read the instructions.
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
  Int_t k = 0;
  for (Iter_s_t inam = fFormnames.begin(); inam != fFormnames.end(); ++inam, ++k) {
    tinfo = Form("f%d",k);
    // FIXME: avoid duplicate formulas
    // FIXME: create and Init() first, then add to array?
    fFormulas.push_back(new THaVform("formula",inam->c_str(),fFormdef[k].c_str()));
    THaVform* pform = fFormulas[k];
    status = pform->Init();
    if ( status != 0) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << *inam << endl;
      cout << "There is probably a typo error... " << endl;
      if( fgVerbose<=2 )
	pform->ErrPrint(status);
      else
	pform->LongPrint();  // for debug
    } else {
      pform->SetOutput(fTree);
// Add variables (i.e. those var's used by the formula) to tree.
// Reason is that TTree::Draw() may otherwise fail with ERROR 26 
      vector<string> avar = pform->GetVars();
      for (Iter_s_t it = avar.begin(); it != avar.end(); ++it) {
        string svar = StripBracket(*it);
        pvar = gHaVars->Find(svar.c_str());
        if (pvar) {
          if (pvar->IsArray()) {
	    Iter_s_t it = find(fArrayNames.begin(),fArrayNames.end(),svar);
	    if( it == fArrayNames.end() ) {
	      fArrayNames.push_back(svar);
	      fOdata.push_back(new THaOdata());
	    }
          } else {
	    Iter_s_t it = find(fVNames.begin(),fVNames.end(),svar);
	    if( it == fVNames.end() )
	      fVNames.push_back(svar);
	  }
	}
      }
    }
  }
  k = 0;
  for(Iter_o_t iodat = fOdata.begin(); iodat != fOdata.end(); ++iodat, ++k)
    (*iodat)->AddBranches(fTree, fArrayNames[k]);
  fNvar = fVNames.size();
  fVar = new Double_t[fNvar];
  for (k = 0; k < fNvar; ++k) {
    tinfo = fVNames[k] + "/D";
    fTree->Branch(fVNames[k].c_str(), &fVar[k], tinfo.c_str(), kNbout);
  }
  k = 0;
  for (Iter_s_t inam = fCutnames.begin(); inam != fCutnames.end(); ++inam, ++k ) {
    // FIXME: avoid duplicate cuts
    // FIXME: create and Init() first, then add to array?
    fCuts.push_back(new THaVform("cut", inam->c_str(), fCutdef[k].c_str()));
    THaVform* pcut = fCuts[k];
    status = pcut->Init(); 
    if ( status != 0 ) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << *inam << endl;
      cout << "There is probably a typo error... " << endl;
      pcut->ErrPrint(status);
    } else {
      pcut->SetOutput(fTree);
    }
    if( fgVerbose>2 )
      pcut->LongPrint();  // for debug
  }
  for (Iter_h_t ihist = fHistos.begin(); ihist != fHistos.end(); ++ihist) {
// After initializing formulas and cuts, must sort through
// histograms and potentially reassign variables.  
// A histogram variable or cut is either a string (which can 
// encode a formula) or an externally defined THaVform. 
    sfvarx = (*ihist)->GetVarX();
    sfvary = (*ihist)->GetVarY();
    for (Iter_f_t iform = fFormulas.begin(); iform != fFormulas.end(); ++iform) {
      string stemp((*iform)->GetName());
      if (CmpNoCase(sfvarx,stemp) == 0) { 
	(*ihist)->SetX(*iform);
      }
      if (CmpNoCase(sfvary,stemp) == 0) { 
	(*ihist)->SetY(*iform);
      }
    }
    if ((*ihist)->HasCut()) {
      scut   = (*ihist)->GetCutStr();
      for (Iter_f_t icut = fCuts.begin(); icut != fCuts.end(); ++icut) {
        string stemp((*icut)->GetName());
        if (CmpNoCase(scut,stemp) == 0) { 
	  (*ihist)->SetCut(*icut);
        }
      }
    }
    (*ihist)->Init();
  }

  if (!fEpicsKey.empty()) {
    vector<THaEpicsKey*>::size_type siz = fEpicsKey.size();
    fEpicsVar = new Double_t[siz+1];
    UInt_t i = 0;
    for (vector<THaEpicsKey*>::iterator it = fEpicsKey.begin(); 
	 it != fEpicsKey.end(); ++it, ++i) {
      fEpicsVar[i] = -1e32;
      string epicsbr = CleanEpicsName((*it)->GetName());
      tinfo = epicsbr + "/D";
      fTree->Branch(epicsbr.c_str(), &fEpicsVar[i], 
        tinfo.c_str(), kNbout);
      fEpicsTree->Branch(epicsbr.c_str(), &fEpicsVar[i], 
        tinfo.c_str(), kNbout);
    }
    fEpicsVar[siz] = -1e32;
    fEpicsTree->Branch("timestamp",&fEpicsVar[siz],"timestamp/D", kNbout);
  }
  for (vector<THaScalerKey*>::iterator it = fScalerKey.begin(); 
       it != fScalerKey.end(); ++it) {
    THaScalerKey* pkey(*it);
    pkey->AddBranches(fTree);
    for (map<string, TTree*>::iterator mt = fScalTree.begin();
	 mt != fScalTree.end(); ++mt) {
           string thisbank(mt->first);
           TTree *sctree = mt->second;
           if ( CmpNoCase(thisbank,pkey->GetBank()) != 0) 
	     continue;
           pkey->AddBranches(sctree,kFALSE);
    }
  }

  Print();

  fInit = true;

  if( fgDoBench ) fgBench.Stop("Init");

  if( fgDoBench ) fgBench.Begin("Attach");
  Int_t st = Attach();
  if( fgDoBench ) fgBench.Stop("Attach");
  if ( st )
    return -4;

  for (Iter_f_t icut=fCuts.begin(); icut!=fCuts.end(); ++icut) 
      (*icut)->SetOutput(fTree);

  for (Iter_f_t iform=fFormulas.begin(); iform!=fFormulas.end(); ++iform) 
      (*iform)->SetOutput(fTree);

  return 0;
}

void THaOutput::BuildList( const vector<string>& vdata) 
{
  // Build list of EPICS variables and
  // SCALER variables to add to output.

    if (vdata.empty()) return;
    if (CmpNoCase(vdata[0],"begin") == 0) {
      if (vdata.size() < 2) return;
      if (CmpNoCase(vdata[1],"epics") == 0) fOpenEpics = kTRUE;
      if (CmpNoCase(vdata[1],"scalers") == 0) fOpenScal = kTRUE;
    }
    if (CmpNoCase(vdata[0],"end") == 0) {
      if (vdata.size() < 2) return;
      if (CmpNoCase(vdata[1],"epics") == 0) fOpenEpics = kFALSE;
      if (CmpNoCase(vdata[1],"scalers") == 0) fOpenScal = kFALSE;
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
	  fEpicsKey.push_back(new THaEpicsKey(vdata[0]));
          if (vdata.size() > 1) {
            std::vector<string> esdata = reQuote(vdata);
            for (int k = 1; k < (int)esdata.size(); k++) {
              fEpicsKey[fEpicsKey.size()-1]->AddAssign(esdata[k]);
	    }
	  }
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
         fScalBank = ToLower( vdata[2] );
         string desc = "Hall A Scalers on " + fScalBank;
         fScalTree.insert
	   ( make_pair(fScalBank,
		       new TTree( ToUpper(fScalBank).c_str(), desc.c_str() )
		       )
	     );
         return;
       } else {
  	 fScalRC = kRate;
         if (vdata.size() >= 3) {
// This is like 'mypulser 4 5' for slot 4, chan 5
// or 'mypulser 4 5 rate' or 'mupulser 4 5 counts'
             if (vdata.size() >= 4) {
	       if ((CmpNoCase(vdata[3],"count") == 0) || 
                   (CmpNoCase(vdata[3],"counts") == 0)) {
		 fScalRC = kCount;
	       }
	     }
             AddScaler(vdata[0], fScalBank, 0,
		    atoi(vdata[1].c_str()),atoi(vdata[2].c_str()));
	 } else {
	   // Default scalers = normalization scalers, like charge and triggers
	   fScalRC = kRate;
	   if (vdata.size() >= 2) {
	     if ((CmpNoCase(vdata[1],"count") == 0) || 
		 (CmpNoCase(vdata[1],"counts") == 0)) {
	       fScalRC = kCount;
	     }
	   }
	   if (CmpNoCase(vdata[0],"default") == 0) {
	     DefScaler();
	     return;
	   } else if (CmpNoCase(vdata[0],"default_helicity") == 0) {
	     DefScaler(1);
	     return;
	   } else {
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

  string norm_scaler[] = {"trigger-1", "trigger-2", "trigger-3",
			  "trigger-4", "trigger-5", "trigger-6",
			  "trigger-7", "trigger-8", "trigger-9",
			  "trigger-10", "trigger-11", "trigger-12",
			  "bcm_u1", "bcm_u3", "bcm_u10",
			  "bcm_d1", "bcm_d3", "bcm_d10",
			  "clock", "TS-accept", "edtpulser",
			  "strobe", "dclock" };

  Int_t nscal = 1;
  Int_t jhel = 0;
  if (hel == 1) {
     nscal = 2;  jhel = -1;
  }
  Int_t numchan = sizeof(norm_scaler)/sizeof(string);

  for (Int_t i = 0; i < nscal; i++) {
     jhel = jhel + 2*i; // 0, or (-1,1)
     for (Int_t j = 0; j < numchan; j++) {
        AddScaler(norm_scaler[j], fScalBank, jhel);
     }
  }

}


//_____________________________________________________________________________
void THaOutput::AddScaler(const string& name, const string& bank,
			  Int_t helicity, Int_t slot, Int_t chan) 
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

  for (Iter_f_t iform=fFormulas.begin(); iform!=fFormulas.end(); ++iform) {
    (*iform)->ReAttach();
  }

  for (Iter_f_t icut=fCuts.begin(); icut!=fCuts.end(); ++icut) {
    (*icut)->ReAttach(); 
  }

  for (Iter_h_t ihist = fHistos.begin(); ihist != fHistos.end(); ++ihist) {
    (*ihist)->ReAttach();
  }
     
  return 0;

}


//_____________________________________________________________________________
Int_t THaOutput::ProcEpics(THaEvData *evdata) 
{
  // Process the EPICS data, this fills the trees.
  // This function is called by THaAnalyzer.

  if ( !evdata->IsEpicsEvent() 
       || fEpicsKey.empty() || !fEpicsTree ) return 0;
  if( fgDoBench ) fgBench.Begin("EPICS");
  fEpicsVar[fEpicsKey.size()] = -1e32;
  for (UInt_t i = 0; i < fEpicsKey.size(); i++) {
    if (evdata->IsLoadedEpics(fEpicsKey[i]->GetName().c_str())) {
      //      cout << "EPICS name "<<fEpicsKey[i]->GetName()<<"    val "<< evdata->GetEpicsString(fEpicsKey[i]->GetName().c_str())<<endl;
      if (fEpicsKey[i]->IsString()) {
        fEpicsVar[i] = fEpicsKey[i]->Eval(
          evdata->GetEpicsString(
             fEpicsKey[i]->GetName().c_str()));
      } else {
        fEpicsVar[i] = 
           evdata->GetEpicsData(
              fEpicsKey[i]->GetName().c_str());
      }
 // fill time stamp (once is ok since this is an EPICS event)
      fEpicsVar[fEpicsKey.size()] =
 	 evdata->GetEpicsTime(
           fEpicsKey[i]->GetName().c_str());
    } else {
      fEpicsVar[i] = -1e32;  // data not yet found
    }
  }
  if (fEpicsTree != 0) fEpicsTree->Fill();  
  if( fgDoBench ) fgBench.Stop("EPICS");
  return 1;
}

//_____________________________________________________________________________
Int_t THaOutput::ProcScaler(THaScalerGroup *scagrp) 
{
  // Process the scaler data, this fills the trees.
  // This function is called by THaAnalyzer.
  THaScaler *scaler = scagrp->GetScalerObj();
  if (!scaler || !scaler->IsRenewed()) return 0;
  if( fgDoBench ) fgBench.Begin("Scalers");

  Bool_t did_fill = kFALSE;
  string thisbank(scaler->GetName());

  for (vector<THaScalerKey*>::iterator it = fScalerKey.begin(); 
       it != fScalerKey.end(); ++it) {
    THaScalerKey* pkey(*it);

    if (CmpNoCase( thisbank,pkey->GetBank()) != 0) continue;

    did_fill = kTRUE;

    if (pkey->SlotDefined()) {

      if (pkey->IsRate()) {
	pkey->Fill(scaler->GetScalerRate(pkey->GetSlot(), pkey->GetChan()));
      } else {
	pkey->Fill(scaler->GetScaler(pkey->GetSlot(), pkey->GetChan()));
      }     

    } else {
      if (pkey->GetChanName()=="dclock") {
	pkey->Fill(scaler->GetNormData(pkey->GetHelicity(),
				       "clock",0) -
		   scaler->GetNormData(pkey->GetHelicity(),
				       "clock",1) );
      } else 
	if (pkey->IsRate()) {
	  pkey->Fill(scaler->GetNormRate(pkey->GetHelicity(), 
					  pkey->GetChanName().c_str()));
	} else {
	  pkey->Fill(scaler->GetNormData(pkey->GetHelicity(), 
					  pkey->GetChanName().c_str(), 0));
	}

    }

// DEBUG
// pkey->Print();

  }
			  
  
  string key = ToLower(thisbank);
  TTree *sctree = fScalTree[key];
  if (did_fill && sctree) sctree->Fill();

  if( fgDoBench ) fgBench.Stop("Scalers");
  return 1;
}

//_____________________________________________________________________________
Int_t THaOutput::Process() 
{
  // Process the variables, formulas, and histograms.
  // This is called by THaAnalyzer.

  if( fgDoBench ) fgBench.Begin("Formulas");
  for (Iter_f_t iform = fFormulas.begin(); iform != fFormulas.end(); ++iform)
    if (*iform) (*iform)->Process();
  if( fgDoBench ) fgBench.Stop("Formulas");

  if( fgDoBench ) fgBench.Begin("Cuts");
  for (Iter_f_t icut = fCuts.begin(); icut != fCuts.end(); ++icut)
    if (*icut) (*icut)->Process();
  if( fgDoBench ) fgBench.Stop("Cuts");

  if( fgDoBench ) fgBench.Begin("Variables");
  THaVar *pvar;
  for (Int_t ivar = 0; ivar < fNvar; ivar++) {
    pvar = fVariables[ivar];
    if (pvar) fVar[ivar] = pvar->GetValue();
  }
  Int_t k = 0;
  for (Iter_o_t it = fOdata.begin(); it != fOdata.end(); ++it, ++k) { 
    THaOdata* pdat(*it);
    pdat->Clear();
    pvar = fArrays[k];
    if ( pvar == NULL ) continue;
    // Fill array in reverse order so that fOdata[k] gets resized just once
    Int_t i = pvar->GetLen();
    bool first = true;
    while( i-- > 0 ) {
      // FIXME: for better efficiency, should use pointer to data and 
      // Fill(int n,double* data) method in case of a contiguous array
      if (pdat->Fill(i,pvar->GetValue(i)) != 1) {
	if( fgVerbose>0 && first ) {
	  cerr << "THaOutput::ERROR: storing too much variable sized data: " 
	       << pvar->GetName() <<"  "<<pvar->GetLen()<<endl;
	  first = false;
	}
      }
    }
  }
  if( fgDoBench ) fgBench.Stop("Variables");

  if( fgDoBench ) fgBench.Begin("Histos");
  for ( Iter_h_t it = fHistos.begin(); it != fHistos.end(); ++it )
    (*it)->Process();
  if( fgDoBench ) fgBench.Stop("Histos");

  if( fgDoBench ) fgBench.Begin("TreeFill");
  if (fTree != 0) fTree->Fill();  
  if( fgDoBench ) fgBench.Stop("TreeFill");

  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::End() 
{
  if( fgDoBench ) fgBench.Begin("End");

  if (fTree != 0) fTree->Write();
  if (fEpicsTree != 0) fEpicsTree->Write();
  if( fgVerbose>1 )
    cout << "End:: fScalTree size "<<fScalTree.size()<<endl;
  for (map<string, TTree*>::iterator mt = fScalTree.begin();
       mt != fScalTree.end(); ++mt) {
      TTree* tree = mt->second;
      if (tree) tree->Write();
  }
  for (Iter_h_t ihist = fHistos.begin(); ihist != fHistos.end(); ++ihist)
    (*ihist)->End();
  if( fgDoBench ) fgBench.Stop("End");

  if( fgDoBench ) {
    cout << "Output timing summary:" << endl;
    fgBench.Print("Init");
    fgBench.Print("Attach");
    fgBench.Print("Variables");
    fgBench.Print("Formulas");
    fgBench.Print("Cuts");
    fgBench.Print("Histos");
    fgBench.Print("TreeFill");
    fgBench.Print("EPICS");
    fgBench.Print("Scalers");
    fgBench.Print("End");
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::LoadFile( const char* filename ) 
{
  // Process the file that defines the output
  
  if( !filename || !*filename || strspn(filename," ") == strlen(filename) ) {
    ::Error( "THaOutput::LoadFile", "invalid file name, "
	     "no output definition loaded" );
    return -1;
  }
  string loadfile(filename);
  ifstream odef(loadfile.c_str());
  if ( !odef ) {
    ErrFile(-1, loadfile);
    return -1;
  }
  const string comment = "#";
  const string whitespace( " \t" );
  string::size_type pos;
  vector<string> strvect;
  string sline;
  while (getline(odef,sline)) {
    // Blank line or comment line?
    if( sline.empty()
	|| (pos = sline.find_first_not_of( whitespace )) == string::npos
	|| comment.find(sline[pos]) != string::npos )
      continue;
    // Get rid of trailing comments
    if( (pos = sline.find_first_of( comment )) != string::npos )
      sline.erase(pos);
    // Substitute text variables
    vector<string> lines( 1, sline );
    if( gHaTextvars->Substitute(lines) )
      continue;
    for( Iter_s_t it = lines.begin(); it != lines.end(); ++it ) {
      // Split the line into tokens separated by whitespace
      const string& str = *it;
      strvect = Split(str);
      bool special_before = (fOpenEpics || fOpenScal);
      BuildList(strvect);
      bool special_now = (fOpenEpics || fOpenScal);
      if( special_before || special_now )
	continue; // strvect already processed
      if (strvect.size() < 2) {
	ErrFile(0, str);
	continue;
      }
      Int_t ikey = FindKey(strvect[0]);
      string sname = StripBracket(strvect[1]);
      switch (ikey) {
      case kVar:
	fVarnames.push_back(sname);
	break;
      case kForm:
	if (strvect.size() < 3) {
	  ErrFile(ikey, str);
	  continue;
	}
	fFormnames.push_back(sname);
	fFormdef.push_back(strvect[2]);
	break;
      case kCut:
	if (strvect.size() < 3) {
	  ErrFile(ikey, str);
	  continue;
	}
	fCutnames.push_back(sname);
	fCutdef.push_back(strvect[2]);
	break;
      case kH1f:
      case kH1d:
      case kH2f:
      case kH2d:
	if( ChkHistTitle(ikey, str) != 1) {
	  ErrFile(ikey, str);
	  continue;
	}
	fHistos.push_back(
			  new THaVhist(strvect[0],sname,stitle));
	// Tentatively assign variables and cuts as strings. 
	// Later will check if they are actually THaVform's.
	fHistos.back()->SetX(nx, xlo, xhi, sfvarx);
	if (ikey == kH2f || ikey == kH2d) {
	  fHistos.back()->SetY(ny, ylo, yhi, sfvary);
	}
	if (iscut != fgNocut) fHistos.back()->SetCut(scut);
	break;
      case kBlock:
	// Do not strip brackets for block regexps: use strvect[1] not sname
	if( BuildBlock(strvect[1]) == 0 ) {
	  cout << "\nTHaOutput::Init: WARNING: Block ";
	  cout << strvect[1] << " does not match any variables. " << endl;
	  cout << "There is probably a typo error... "<<endl;
	}
	break;
      case kBegin:
      case kEnd:
	break;
      default:
	cout << "Warning: keyword "<<strvect[0]<<" undefined "<<endl;
      }
    }
  }

  // sort thru fVarnames, removing identical entries
  if( fVarnames.size() > 1 ) {
    sort(fVarnames.begin(),fVarnames.end());
    vector<string>::iterator Vi = fVarnames.begin();
    while ( (Vi+1)!=fVarnames.end() ) {
      if ( *Vi == *(Vi+1) ) {
	fVarnames.erase(Vi+1);
      } else {
	++Vi;
      }
    }
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::FindKey(const string& key) const
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
      if( CmpNoCase( key, it->name ) == 0 )
	return it->keyval;
      it++;
    }
  }
  return -1;
}

//_____________________________________________________________________________
string THaOutput::StripBracket(const string& var) const
{
// If the string contains "[anything]", we strip
// it away.  In practice this should not be fatal 
// because your variable will still show up in the tree.
  string::size_type pos1,pos2;
  string open_brack("[");
  string close_brack("]");
  string result;
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
std::vector<string> THaOutput::reQuote(const std::vector<string>& input) const {
  // Specialist private function needed by EPICs line parser:
  // The problem is that the line was split by white space, so
  // a line like "'RF On'=42"  must be repackaged into
  // one string, i.e. "'RF" and "On'=42" put back together.
  std::vector<string> result;
  result.clear();
  int first_quote = 1;
  int to_add = 0;
  string temp1,temp2,temp3;
  string::size_type pos1,pos2;
  for (vector<string>::const_iterator str = input.begin(); str !=
	 input.end(); ++str ) {
    temp1 = *str;
    pos1 = temp1.find("'");
    if (pos1 != string::npos) {
      if (first_quote) {
        temp2.assign(temp1.substr(pos1,temp1.length()));
// But there might be a 2nd "'" with no spaces
// like "'Yes'" (silly, but understandable & allowed)
        temp3.assign(temp1.substr(pos1+1,temp1.length()));
        pos2 = temp3.find("'");
        if (pos2 != string::npos) {
          temp1.assign(temp3.substr(0,pos2));
          temp2.assign(temp3.substr
		       (pos2+1,temp3.length()));
	  temp3 = temp1+temp2;
          result.push_back(temp3);
          continue;
        }
        first_quote = 0;
        to_add = 1;
      } else {
        temp2 = temp2 + " " + temp1;
        result.push_back(temp2);
        temp2.clear();
        first_quote = 1;
        to_add = 0;
      }
    } else {
      if (to_add) {
	temp2 = temp2 + " " + temp1;
      } else {
        result.push_back(temp1);
      }
    }
  }
  return result;
}

//_____________________________________________________________________________
string THaOutput::CleanEpicsName(const string& input) const
{
// To clean up EPICS variable names that contain
// bad characters like ":" and arithmetic operations
// that confuse TTree::Draw().
// Replace all 'badchar' with 'goodchar'

  static const char badchar[]=":+-*/=";
  static const string goodchar = "_";
  int numbad = sizeof(badchar)/sizeof(char) - 1;

  string output = input;

  for (int i = 0; i < numbad; i++) {
     string sbad(&badchar[i]);
     sbad.erase(1,sbad.size());
     string::size_type pos = input.find(sbad,0);
     while (pos != string::npos) {
       output.replace(pos,1,goodchar);
       pos = input.find(sbad,pos+1);
     }
  }
  return output;
}


//_____________________________________________________________________________
void THaOutput::ErrFile(Int_t iden, const string& sline) const
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
       cerr << "Illegal line: " << sline << endl;
       cerr << "See the documentation or ask Bob Michaels"<<endl;
       break;
  }
}

//_____________________________________________________________________________
void THaOutput::Print() const
{
  // Printout the definitions. Amount printed depends on verbosity
  // level, set with SetVerbosity().

  typedef vector<string>::const_iterator Iterc_s_t;
  typedef vector<THaVform*>::const_iterator Iterc_f_t;
  typedef vector<THaVhist*>::const_iterator Iterc_h_t;

  if( fgVerbose > 0 ) {
    if( fVarnames.size() == 0 && fFormulas.size() == 0 &&
	fCuts.size() == 0 && fHistos.size() == 0 ) {
      ::Warning("THaOutput", "no output defined");
    } else {
      cout << endl << "THaOutput definitions: " << endl;
      if( !fVarnames.empty() ) {
	cout << "=== Number of variables "<<fVarnames.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  
	  UInt_t i = 0;
	  for (Iterc_s_t ivar = fVarnames.begin(); ivar != fVarnames.end(); 
	       i++, ivar++ ) {
	    cout << "Variable # "<<i<<" =  "<<(*ivar)<<endl;
	  }
	}
      }
      if( !fFormulas.empty() ) {
	cout << "=== Number of formulas "<<fFormulas.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  UInt_t i = 0;
	  for (Iterc_f_t iform = fFormulas.begin(); 
	       iform != fFormulas.end(); i++, iform++ ) {
	    cout << "Formula # "<<i<<endl;
	    if( fgVerbose>2 )
	      (*iform)->LongPrint();
	    else
	      (*iform)->ShortPrint();
	  }
	}
      }
      if( !fCuts.empty() ) {
	cout << "=== Number of cuts "<<fCuts.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  UInt_t i = 0;
	  for (Iterc_f_t icut = fCuts.begin(); icut != fCuts.end();
	       i++, icut++ ) {
	    cout << "Cut # "<<i<<endl;
	    if( fgVerbose>2 )
	      (*icut)->LongPrint();
	    else
	      (*icut)->ShortPrint();
	  }
	}
      }
      if( !fHistos.empty() ) {
	cout << "=== Number of histograms "<<fHistos.size()<<endl;
	if( fgVerbose > 1 ) {
	  cout << endl;
	  UInt_t i = 0;
	  for (Iterc_h_t ihist = fHistos.begin(); ihist != fHistos.end(); 
	       i++, ihist++) {
	    cout << "Histogram # "<<i<<endl;
	    (*ihist)->Print();
	  }
	}
      }
      cout << endl;
    }
  }
}

//_____________________________________________________________________________
Int_t THaOutput::ChkHistTitle(Int_t iden, const string& sline)
{
// Parse the string that defines the histogram.  
// The title must be enclosed in single quotes (e.g. 'my title').  
// Ret value 'result' means:  -1 == error,  1 == everything ok.
  Int_t result = -1;
  stitle = "";   sfvarx = "";  sfvary  = "";
  iscut = fgNocut;  scut = "";
  nx = 0; ny = 0; xlo = 0; xhi = 0; ylo = 0; yhi = 0;
  string::size_type pos1 = sline.find_first_of("'");
  string::size_type pos2 = sline.find_last_of("'");
  if (pos1 != string::npos && pos2 > pos1) {
    stitle = sline.substr(pos1+1,pos2-pos1-1);
  }
  string ctemp = sline.substr(pos2+1,sline.size()-pos2);
  vector<string> stemp = Split(ctemp);
  if (stemp.size() > 1) {
     sfvarx = stemp[0];
     Int_t ssize = stemp.size();
     if (ssize == 4 || ssize == 5) {
       sscanf(stemp[1].c_str(),"%8d",&nx);
       sscanf(stemp[2].c_str(),"%16f",&xlo);
       sscanf(stemp[3].c_str(),"%16f",&xhi);
       if (ssize == 5) {
         iscut = 1; scut = stemp[4];
       }
       result = 1;
     }
     if (ssize == 8 || ssize == 9) {
       sfvary = stemp[1];
       sscanf(stemp[2].c_str(),"%8d",&nx);
       sscanf(stemp[3].c_str(),"%16f",&xlo);
       sscanf(stemp[4].c_str(),"%16f",&xhi);
       sscanf(stemp[5].c_str(),"%8d",&ny);
       sscanf(stemp[6].c_str(),"%16f",&ylo);
       sscanf(stemp[7].c_str(),"%16f",&yhi);
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
Int_t THaOutput::BuildBlock(const string& blockn)
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
      string vn(s.Data());
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
//ClassImp(THaScalerKey)
//ClassImp(THaOdata)
ClassImp(THaOutput)
