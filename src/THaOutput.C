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
#include "TString.h"
#include "THaEvData.h"
#include "THaEvtTypeHandler.h"
#include "THaEpicsEvtHandler.h"
#include "THaScalerEvtHandler.h"
#include "THaString.h"
#include "FileInclude.h"

#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>
#include <sstream>
//#include <iterator>

#include "THaBenchmark.h"

using namespace std;
using namespace THaString;
using namespace Podd;

typedef vector<THaOdata*>::iterator Iter_o_t;
typedef vector<THaVform*>::iterator Iter_f_t;
typedef vector<THaVhist*>::iterator Iter_h_t;
typedef vector<string>::iterator Iter_s_t;

Int_t THaOutput::fgVerbose = 1;
//FIXME: these should be member variables
static Bool_t fgDoBench = kFALSE;
static THaBenchmark fgBench;

static const char comment('#');

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
  leaf = name + "[" + leaf + "]/D";
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
THaOutput::THaOutput()
  : fNvar(0), fVar(0), fEpicsVar(0), fTree(0), fEpicsTree(0), fInit(false),
    fExtra(0), fEpicsHandler(0),
    nx(0), ny(0), iscut(0), xlo(0), xhi(0), ylo(0), yhi(0),
    fOpenEpics(false), fFirstEpics(false), fIsScalar(false)
{
  // Constructor
}

//_____________________________________________________________________________
THaOutput::~THaOutput() 
{
  // Destructor

  delete fExtra; fExtra = 0;

  // Delete Trees and histograms only if ROOT system is initialized.
  // ROOT will report being uninitialized if we're called from the TSystem
  // destructor, at which point the trees already have been deleted.
  // FIXME: Trees would also be deleted if deleting the output file, right?
  // Can we use this here?
  if( TROOT::Initialized() ) {
    if (fTree) delete fTree;
    if (fEpicsTree) delete fEpicsTree;
  }
  if (fVar) delete [] fVar;
  if (fEpicsVar) delete [] fEpicsVar;
  for (Iter_o_t od = fOdata.begin();
       od != fOdata.end(); ++od) delete *od;
  for (Iter_f_t itf = fFormulas.begin();
       itf != fFormulas.end(); ++itf) delete *itf;
  for (Iter_f_t itf = fCuts.begin();
       itf != fCuts.end(); ++itf) delete *itf;
  for (Iter_h_t ith = fHistos.begin();
       ith != fHistos.end(); ++ith) delete *ith;
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
  fFirstEpics = kTRUE; 

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
  Int_t k = 0;
  for (Iter_s_t inam = fFormnames.begin(); inam != fFormnames.end(); ++inam, ++k) {
    string tinfo = Form("f%d",k);
    // FIXME: avoid duplicate formulas
    THaVform* pform = new THaVform("formula",inam->c_str(),fFormdef[k].c_str());
    Int_t status = pform->Init();
    if ( status != 0) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << *inam << endl;
      cout << "There is probably a typo error... " << endl;
      pform->ErrPrint(status);
      delete pform;
      --k;
      continue;
    }
    pform->SetOutput(fTree);
    fFormulas.push_back(pform);
    if( fgVerbose > 2 )
      pform->LongPrint();  // for debug
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
  k = 0;
  for(Iter_o_t iodat = fOdata.begin(); iodat != fOdata.end(); ++iodat, ++k)
    (*iodat)->AddBranches(fTree, fArrayNames[k]);
  fNvar = fVNames.size();
  fVar = new Double_t[fNvar];
  for (k = 0; k < fNvar; ++k) {
    string tinfo = fVNames[k] + "/D";
    fTree->Branch(fVNames[k].c_str(), &fVar[k], tinfo.c_str(), kNbout);
  }
  k = 0;
  for (Iter_s_t inam = fCutnames.begin(); inam != fCutnames.end(); ++inam, ++k ) {
    // FIXME: avoid duplicate cuts
    THaVform* pcut = new THaVform("cut", inam->c_str(), fCutdef[k].c_str());
    Int_t status = pcut->Init();
    if ( status != 0 ) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << *inam << endl;
      cout << "There is probably a typo error... " << endl;
      pcut->ErrPrint(status);
      delete pcut;
      --k;
      continue;
    }
    pcut->SetOutput(fTree);
    fCuts.push_back(pcut);
    if( fgVerbose>2 )
      pcut->LongPrint();  // for debug
  }
  for (Iter_h_t ihist = fHistos.begin(); ihist != fHistos.end(); ++ihist) {
    THaVhist* vhist = *ihist;
// After initializing formulas and cuts, must sort through
// histograms and potentially reassign variables.  
// A histogram variable or cut is either a string (which can 
// encode a formula) or an externally defined THaVform. 
    sfvarx = vhist->GetVarX();
    sfvary = vhist->GetVarY();
    for (Iter_f_t iform = fFormulas.begin(); iform != fFormulas.end(); ++iform) {
      string stemp((*iform)->GetName());
      if (CmpNoCase(sfvarx,stemp) == 0) { 
	vhist->SetX(*iform);
      }
      if (CmpNoCase(sfvary,stemp) == 0) { 
	vhist->SetY(*iform);
      }
    }
    if (vhist->HasCut()) {
      scut   = vhist->GetCutStr();
      for (Iter_f_t icut = fCuts.begin(); icut != fCuts.end(); ++icut) {
        string stemp((*icut)->GetName());
        if (CmpNoCase(scut,stemp) == 0) { 
	  vhist->SetCut(*icut);
        }
      }
    }
    vhist->Init();
  }

  if (!fEpicsKey.empty()) {
    vector<THaEpicsKey*>::size_type siz = fEpicsKey.size();
    fEpicsVar = new Double_t[siz+1];
    UInt_t i = 0;
    for (vector<THaEpicsKey*>::iterator it = fEpicsKey.begin(); 
	 it != fEpicsKey.end(); ++it, ++i) {
      fEpicsVar[i] = -1e32;
      string epicsbr = CleanEpicsName((*it)->GetName());
      string tinfo = epicsbr + "/D";
      fTree->Branch(epicsbr.c_str(), &fEpicsVar[i], 
        tinfo.c_str(), kNbout);
      fEpicsTree->Branch(epicsbr.c_str(), &fEpicsVar[i], 
        tinfo.c_str(), kNbout);
    }
    fEpicsVar[siz] = -1e32;
    fEpicsTree->Branch("timestamp",&fEpicsVar[siz],"timestamp/D", kNbout);
  }

  Print();

  fInit = true;

  if( fgDoBench ) fgBench.Stop("Init");

  if( fgDoBench ) fgBench.Begin("Attach");
  Int_t st = Attach();
  if( fgDoBench ) fgBench.Stop("Attach");
  if ( st )
    return -4;

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
    }
    if (CmpNoCase(vdata[0],"end") == 0) {
      if (vdata.size() < 2) return;
      if (CmpNoCase(vdata[1],"epics") == 0) fOpenEpics = kFALSE;
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
    return;
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
Int_t THaOutput::ProcEpics(THaEvData *evdata, THaEpicsEvtHandler *epicshandle)  
{
  // Process the EPICS data, this fills the trees.

  if ( !epicshandle ) return 0;
  if ( !epicshandle->IsMyEvent(evdata->GetEvType()) 
       || fEpicsKey.empty() || !fEpicsTree ) return 0;
  if( fgDoBench ) fgBench.Begin("EPICS");
  fEpicsVar[fEpicsKey.size()] = -1e32;
  for (UInt_t i = 0; i < fEpicsKey.size(); i++) {
    if (epicshandle->IsLoaded(fEpicsKey[i]->GetName().c_str())) {
      //      cout << "EPICS name "<<fEpicsKey[i]->GetName()<<"    val "<< epicshandle->GetString(fEpicsKey[i]->GetName().c_str())<<endl;
      if (fEpicsKey[i]->IsString()) {
        fEpicsVar[i] = fEpicsKey[i]->Eval(
          epicshandle->GetString(
             fEpicsKey[i]->GetName().c_str()));
      } else {
        fEpicsVar[i] = 
           epicshandle->GetData(
              fEpicsKey[i]->GetName().c_str());
      }
 // fill time stamp (once is ok since this is an EPICS event)
      fEpicsVar[fEpicsKey.size()] =
 	 epicshandle->GetTime(
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
    fgBench.Print("End");
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::LoadFile( const char* filename ) 
{
  // Process the file that defines the output
  
  const char* const here = "THaOutput::LoadFile";

  if( !filename || !*filename || strspn(filename," ") == strlen(filename) ) {
    ::Error( here, "invalid file name, no output definition loaded" );
    return -2;
  }
  string loadfile(filename);
  ifstream odef(loadfile.c_str());
  if ( !odef ) {
    ErrFile(-1, loadfile);
    return -1;
  }
  string::size_type pos;
  vector<string> strvect;
  string sline;

  while (getline(odef,sline)) {
    // #include
    if( sline.substr(0,kIncTag.length()) == kIncTag &&
	sline.length() > kIncTag.length() ) {
      string incfilename;
      if( GetIncludeFileName(sline,incfilename) != 0 ) {
	ostringstream ostr;
	ostr << "Error in #include specification: " << sline;
	::Error( here, "%s", ostr.str().c_str() );
	return -3;
      }
      if( CheckIncludeFilePath(incfilename) != 0 ) {
	ostringstream ostr;
	ostr << "Error opening include file: " << sline;
	::Error( here, "%s", ostr.str().c_str() );
	return -3;
      }
      if( incfilename == filename ) {
	// File including itself?
	// FIXME: does not catch including the same file via full pathname or similar
	ostringstream ostr;
	ostr << "File cannot include itself: " << sline;
	::Error( here, "%s", ostr.str().c_str() );
	return -3;
      }
      Int_t ret = LoadFile( incfilename.c_str() );
      if( ret != 0 )
	return ret;
      continue;
    }
    // Blank line or comment line?
    if( sline.empty() ||
	(pos = sline.find_first_not_of(kWhiteSpace)) == string::npos ||
	sline[pos] == comment )
      continue;
    // Get rid of trailing comments
    if( (pos = sline.find(comment)) != string::npos )
      sline.erase(pos);
    // Substitute text variables
    vector<string> lines( 1, sline );
    if( gHaTextvars->Substitute(lines) )
      continue;
    for( Iter_s_t it = lines.begin(); it != lines.end(); ++it ) {
      // Split the line into tokens separated by whitespace
      const string& str = *it;
      strvect = Split(str);
      bool special_before = (fOpenEpics);
      BuildList(strvect);
      bool special_now = (fOpenEpics);
      if( special_before || special_now )
	continue; // strvect already processed
      if (strvect.size() < 2) {
	ErrFile(0, str);
	continue;
      }
      fIsScalar = kFALSE;  // this may be set true by svPrefix
      string svkey = svPrefix(strvect[0]);
      Int_t ikey = FindKey(svkey);
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
	fHistos.push_back(new THaVhist(svkey,sname,stitle));
	// Tentatively assign variables and cuts as strings. 
	// Later will check if they are actually THaVform's.
	fHistos.back()->SetX(nx, xlo, xhi, sfvarx);
	if (ikey == kH2f || ikey == kH2d) {
	  fHistos.back()->SetY(ny, ylo, yhi, sfvary);
	}
	if (iscut != fgNocut) fHistos.back()->SetCut(scut);
// If we know now that its a scalar, inform the histogram to remain that way
// and over-ride its internal rules for self-determining if its a vector.
        if (fIsScalar) fHistos.back()->SetScalarTrue();
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
	cout << "Warning: keyword "<<svkey<<" undefined "<<endl;
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

//_____________________________________________________________________________s
std::string THaOutput::svPrefix(std::string& histtype)
{
// If the arg is a string for a histogram type, we strip the initial 
// "s" or "v".  If the first character is "s" we set fIsScalar true.
  const int ldebug=0;
  fIsScalar = kFALSE;
  string sresult = histtype;
  if (ldebug) cout << "svPrefix  histogram  type = "<<histtype<<"   histtype length  "<<histtype.length()<<endl;
// For histograms `histtype' is of the form "XthNY" 
// with X="s" or "v" and N=1 or 2,  Y = "f" or "d"
// For example, "sth1f".  Note it always has length 5
  if (histtype.length() != 5) return sresult;  
  string sfirst = histtype.substr(0,1); 
  if (ldebug) cout << "sfirst = "<<sfirst<<endl;
  if(CmpNoCase(sfirst,"s")==0) fIsScalar = kTRUE;
  sresult=histtype.substr(1);
// Needs to be a histogram, not something else like block
  if( (CmpNoCase(sresult,"th1f")!=0) &&
      (CmpNoCase(sresult,"th2f")!=0) &&
      (CmpNoCase(sresult,"th1d")!=0) &&
      (CmpNoCase(sresult,"th2d")!=0) ) return histtype;  // return original
  if (ldebug) {
      cout << "result  "<<sresult<< endl;
      if (fIsScalar) cout << "fScalar is TRUE"<<endl;
  }
  return sresult;

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
  if (fOpenEpics) return;  // No error
  cerr << "THaOutput::ERROR: Syntax error in output definition file."<<endl;
  cerr << "The offending line is :\n"<<sline<<endl<<endl;
  switch (iden) {
     case kVar:
       cerr << "For variables, the syntax is: "<<endl;
       cerr << "    variable  variable-name"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    variable   R.vdc.v2.nclust"<<endl;;
       break;
     case kCut:
     case kForm:
       cerr << "For formulas or cuts, the syntax is: "<<endl;
       cerr << "    formula(or cut)  formula-name  formula-expression"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    formula targetX 1.464*B.bpm4b.x-0.464*B.bpm4a.x"<<endl;
       break;
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
//ClassImp(THaOdata)
ClassImp(THaOutput)
