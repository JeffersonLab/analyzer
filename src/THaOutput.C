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
//////////////////////////////////////////////////////////////////////////

#define CHECKOUT 1

#include "THaOutput.h"
#include "TObject.h"
#include "THaFormula.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "THaCut.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"
#include "TRegexp.h"
#include "TError.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>

using namespace std;
typedef vector<THaOdata*>::size_type Vsiz_t;

const Double_t THaOutput::kBig = 1.e38;

//_____________________________________________________________________________
THaOutput::THaOutput() :
  fNform(0), fNvar(0), fN1d(0), fN2d(0), fNcut(0),
  fForm(0), fVar(0), fH1vtype(0), fH1form(0), fH2vtypex(0), fH2formx(0),
  fH2vtypey(0), fH2formy(0), fTree(0), fInit(false)
{
  // Constructor
}

//_____________________________________________________________________________
THaOutput::~THaOutput() 
{
  // Destructor

  // FIXME: Does this really clean up everything?
  delete fTree;
  delete [] fForm;
  delete [] fVar;
  delete [] fH1vtype;
  delete [] fH1form;
  delete [] fH2vtypex;
  delete [] fH2formx;
  delete [] fH2vtypey;
  delete [] fH2formy;
  for (std::vector<THaOdata* >::iterator od = fOdata.begin();
       od != fOdata.end(); od++) delete *od;
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
    
    // Assign pointers to desired global variables
    if ( Attach() ) return -4;

    // Re-compile the functions/cuts
    for (Int_t iform=0; iform<fNform; iform++) {
      fFormulas[iform]->Compile();
      if (fFormulas[iform]->IsError()) {
        cout << "THaOutput::Init: WARNING: Error in formula Re-Compilation: ";
        cout << fFormulas[iform]->GetTitle() << endl;
      }
      
    }
    for (Int_t icut=0; icut<fNcut; icut++) {
      fHistCut[icut]->Compile();
      if (fHistCut[icut]->IsError()) {
        cout << "THaOutput::Init: WARNING: Error in cut Re-Compilation: ";
        cout << fHistCut[icut]->GetTitle() << endl;
      }
    }
#ifdef CHECKOUT
    Print();
#endif
    return 1;
  }
    
  if( !gHaVars ) return -2;
  
  fTree = new TTree("T","Hall A Analyzer Output DST");
  
  Int_t err = LoadFile( filename );
  if( err == -1 )
    return 0;       // No error if file not found - then we're just a dummy
  else if( err != 0 ) {
    delete fTree; fTree = NULL;
    return -3;
  }
  
  fNvar = fVarnames.size();  // this gets reassigned below
  fNform = fFormnames.size();
  fN1d = fH1dname.size();
  fN2d = fH2dname.size();
  
  fArrayNames.clear();
  fVNames.clear();
  
  fForm = new Double_t[fNform];
  
  THaVar *pvar;
  string tinfo;
  vector<string> stemp1,stemp2;
  for (Int_t ivar = 0; ivar < fNvar; ivar++) {
    pvar = gHaVars->Find(fVarnames[ivar].c_str());
    if (pvar) {
      if (pvar->IsArray()) {
	fArrayNames.push_back(fVarnames[ivar]);
	//        fArrays.push_back( pvar );
	fOdata.push_back(new THaOdata());
	stemp1.push_back(fVarnames[ivar]);
      } else {
	fVNames.push_back(fVarnames[ivar]);
	//        fVariables.push_back( pvar );
	stemp2.push_back(fVarnames[ivar]);
      }
    } else {
      cout << "\nTHaOutput::Init: WARNING: Global variable ";
      cout << fVarnames[ivar] << " does not exist. "<< endl;
      cout << "There is probably a typo error... "<<endl;
    }
  }
  for (Vsiz_t k = 0; k < fOdata.size(); k++)
    fOdata[k]->AddBranches(fTree, stemp1[k]);
  fNvar = stemp2.size();
  fVar = new Double_t[fNvar];
  for (Int_t k = 0; k < fNvar; k++) {
    tinfo = stemp2[k] + "/D";
    fTree->Branch(stemp2[k].c_str(), &fVar[k], tinfo.c_str(), kNbout);
  }
  for (Int_t iform = 0; iform < fNform; iform++) {
    tinfo = Form("f%d",iform);
    fFormulas.push_back(new THaFormula(tinfo.c_str(),
				       fFormdef[iform].c_str()));
    if (fFormulas[iform]->IsError()) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << fFormnames[iform] << endl;
      cout << "There is probably a typo error... " << endl;
    }
  }
  for (Int_t iform = 0; iform < fNform; iform++) {
    tinfo = fFormnames[iform] + "/D";
    fTree->Branch(fFormnames[iform].c_str(), &fForm[iform], 
		  tinfo.c_str(), kNbout);
  }
  fH1vtype = new Int_t[fN1d];
  fH1form  = new Int_t[fN1d];
  memset(fH1vtype, -1, fN1d*sizeof(Int_t));
  memset(fH1form, -1, fN1d*sizeof(Int_t));
  fNcut = 0;
  for (Int_t ihist = 0; ihist < fN1d; ihist++) {
    if (fH1cutid[ihist] != fgNocut) {
      fHistCut.push_back(new THaCut(fH1cut[ihist].c_str(), 
				    fH1cut[ihist].c_str(), (const char*)"haoutput"));
      if (fHistCut[fNcut]->IsError()) {
	cout << "THaOutput::Init: WARNING: Error in cut definition ";
	cout << fH1cut[ihist] << endl;
      }
      fH1cutid[ihist] = fNcut;
      fNcut++;
    }
    for (Int_t iform = 0; iform < fNform; iform++) {
      if (fH1plot[ihist] == fFormnames[iform]) {
	fH1vtype[ihist] = kForm;
	fH1form[ihist] = iform;  
      }
    } 
    pvar = gHaVars->Find(fH1plot[ihist].c_str());
    if (pvar != 0 && fH1vtype[ihist] != kForm) 
      fH1vtype[ihist] = kVar;
    //    fH1var.push_back(pvar);
    fH1d.push_back( new TH1F (fH1dname[ihist].c_str(), 
			      fH1dtit[ihist].c_str(), 
			      fH1dbin[ihist], fH1dxlo[ihist], fH1dxhi[ihist]));
  }
  fH2vtypex = new Int_t[fN2d];
  fH2vtypey = new Int_t[fN2d];
  fH2formx  = new Int_t[fN2d];
  fH2formy  = new Int_t[fN2d];
  memset(fH2vtypex, -1, fN2d*sizeof(Int_t));
  memset(fH2vtypey, -1, fN2d*sizeof(Int_t));
  memset(fH2formx, -1, fN2d*sizeof(Int_t));
  memset(fH2formy, -1, fN2d*sizeof(Int_t));
  for (Int_t ihist = 0; ihist < fN2d; ihist++) {
    if (fH2cutid[ihist] != fgNocut) {
      fHistCut.push_back(new THaCut(fH2cut[ihist].c_str(), 
				    fH2cut[ihist].c_str(), (const char*)"haoutput"));
      if (fHistCut[fNcut]->IsError()) {
	cout << "THaOutput::Init: WARNING: Error in cut definition ";
	cout << fH2cut[ihist] << endl;
      }
      fH2cutid[ihist] = fNcut;
      fNcut++;
    } 
    for (Int_t iform = 0; iform < fNform; iform++) {
      if (fH2plotx[ihist] == fFormnames[iform]) {
	fH2vtypex[ihist] = kForm;
	fH2formx[ihist] = iform;  
      }
      if (fH2ploty[ihist] == fFormnames[iform]) {
	fH2vtypey[ihist] = kForm;
	fH2formy[ihist] = iform;  
      }  
    }
    pvar = gHaVars->Find(fH2plotx[ihist].c_str());
    if (pvar != 0 && fH2vtypex[ihist] != kForm) 
      fH2vtypex[ihist] = kVar;
    //    fH2varx.push_back(pvar);
    pvar = gHaVars->Find(fH2ploty[ihist].c_str());
    if (pvar != 0 && fH2vtypey[ihist] != kForm) 
      fH2vtypey[ihist] = kVar;
    //    fH2vary.push_back(pvar);
    fH2d.push_back( new TH2F (fH2dname[ihist].c_str(), 
			      fH2dtit[ihist].c_str(), 
			      fH2dbinx[ihist], fH2dxlo[ihist], fH2dxhi[ihist],
			      fH2dbiny[ihist], fH2dylo[ihist], fH2dyhi[ihist]));
  }
  
  fInit = true; // the histograms and branches have been prepared
  
  // Assign pointers to desired global variables
  if ( Attach() ) 
    return -4;
  
  return 0;
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
      cout << "This is not supposed to happend... "<<endl;
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
      cout << "This is not supposed to happend... "<<endl;
    }
  }

  // 1-D histograms

  fH1var.resize(fN1d);
  for (Int_t ihist = 0; ihist < fN1d; ihist++) {
    pvar = gHaVars->Find(fH1plot[ihist].c_str());
    fH1var[ihist] = pvar;
  }

  // 2-D histograms

  fH2varx.resize(fN2d);
  fH2vary.resize(fN2d);
  
  for (Int_t ihist = 0; ihist < fN2d; ihist++) {
    pvar = gHaVars->Find(fH2plotx[ihist].c_str());
    fH2varx[ihist] = pvar;
    pvar = gHaVars->Find(fH2ploty[ihist].c_str());
    fH2vary[ihist] = pvar;
  }

  return 0;
  
}  

//_____________________________________________________________________________
#ifdef IFCANWORK
// Preferred method, but doesn't work, hence turned off with ifdef
Int_t THaOutput::AddToTree(char *name, TObject *tobj) 
{
  // Add a TObject to the tree
  // I would have preferred to use this method, but it did not work !!
  // Instead, it works to add a TObject like this for THaOutput *fOut :
  // if (fOut->TreeDefined()) fOut->GetTree()->Branch(...etc)
  if (fTree != 0 && tobj != 0) {
    fTree->Branch(name,tobj->ClassName(),&tobj);
    return 0;
  }
  return -1;
}
#endif

//_____________________________________________________________________________
Int_t THaOutput::Process() 
{
  static int iev = 0;
  iev++;
  for (Int_t iform = 0; iform < fNform; iform++) {
    if (fFormulas[iform])
      if ( !fFormulas[iform]->IsError() ) {
        fForm[iform] = fFormulas[iform]->Eval();
      } else {
	fForm[iform] = kBig;
      }
    else
      fForm[iform] = kBig;
  } 
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
       if (fOdata[k]->Fill(i,pvar->GetValue(i)) != 1) 
	 cout << "THaOutput::ERROR: storing too much variable sized data: " 
	      << pvar->GetName() <<endl;
    }
  }
  for (Int_t ihist = 0; ihist < fN1d; ihist++) {
    if (fH1cutid[ihist] != fgNocut) {  // histogram cut condition
      if ( !fHistCut[fH1cutid[ihist]]->IsError() ) {
        if ( !fHistCut[fH1cutid[ihist]]->EvalCut() ) continue; 
      }
    }     
    if (fH1vtype[ihist] == kForm) {
      if (fH1form[ihist] >= 0) {
        fH1d[ihist]->Fill(fForm[fH1form[ihist]]);
      }
    } else {
      pvar = fH1var[ihist];    
      if (pvar) {
        if (pvar->IsArray()) {
           for (Int_t i = 0; i < pvar->GetLen(); i++) {
             fH1d[ihist]->Fill(pvar->GetValue(i));
	   }
        } else {
           fH1d[ihist]->Fill(pvar->GetValue());
        }
      }
    }
  }
  for (Int_t ihist = 0; ihist < fN2d; ihist++) {
    if (fH2cutid[ihist] != fgNocut) {  // histogram cut condition
      if ( !fHistCut[fH2cutid[ihist]]->IsError() ) {
        if ( !fHistCut[fH2cutid[ihist]]->EvalCut() ) continue; 
      }
    }   
    Float_t x = 0;  Float_t y = 0;
    if (fH2vtypex[ihist] == kForm) {
      if (fH2formx[ihist] >= 0) {
        x = fForm[fH2formx[ihist]];
      }
    } else {
      pvar = fH2varx[ihist];    
      if (pvar) {
	// FIXME: Put this check into Init()?
        if (pvar->IsArray()) {
          cout << "WARNING: 2d histos w/ arrays not supported"<<endl;
          x = 0; // cannot handle arrays yet
        } else {
          x = pvar->GetValue();
        }
      }
    }
    if (fH2vtypey[ihist] == kForm) {
      if (fH2formy[ihist] >= 0) {
        y = fForm[fH2formy[ihist]];
      }
    } else {
      pvar = fH2vary[ihist];    
      if (pvar) {
        if (pvar->IsArray()) {
          cout << "WARNING: 2d histos w/ arrays not supported"<<endl;
          y = 0; // cannot handle arrays yet
        } else {
          y = pvar->GetValue();
        }
      }
    }
    fH2d[ihist]->Fill(x,y);
  }
  if (fTree != 0) fTree->Fill();  
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::End() 
{
  if (fTree != 0) fTree->Write();
  Int_t ihist;
  for (ihist = 0; ihist < fN1d; ihist++) fH1d[ihist]->Write();
  for (ihist = 0; ihist < fN2d; ihist++) fH2d[ihist]->Write();
  return 0;
}

//_____________________________________________________________________________
Int_t THaOutput::LoadFile( const char* filename ) 
{
  // Process the file that defines the output
  
  if( !filename || !strlen(filename) || strspn(filename," ") == 
      strlen(filename) ) {
    ::Error( "THaOutput::LoadFile()", "invalid file name, "
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
      case kH1:
          if( ParseTitle(sline) != 1) {
	    ErrFile(ikey, sline);
            continue;
	  }
	  fH1dname.push_back(sname);
          fH1dtit.push_back(stitle);
          fH1plot.push_back(sfvar1); 
          fH1dbin.push_back(n1);
	  fH1dxlo.push_back(xl1);
	  fH1dxhi.push_back(xh1);
          fH1cut.push_back(scut);
          fH1cutid.push_back(iscut);
          break;    
      case kH2:                    
   	  if( ParseTitle(sline) != 2 ) {
	    ErrFile(ikey, sline);
	    continue;
	  }
	  fH2dname.push_back(sname);
          fH2dtit.push_back(stitle);  
          fH2plotx.push_back(sfvar1);  
          fH2ploty.push_back(sfvar2);  
          fH2dbinx.push_back(n1);
	  fH2dxlo.push_back(xl1);
	  fH2dxhi.push_back(xh1);
          fH2dbiny.push_back(n2);
	  fH2dylo.push_back(xl2);
	  fH2dyhi.push_back(xh2);
          fH2cut.push_back(scut);
          fH2cutid.push_back(iscut);
          break;          
      case kBlock:
	  BuildBlock(sname);
	  break;
      default:
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

#ifdef CHECKOUT
  Print();
#endif
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
    { "th1f",     kH1 },
    { "th2f",     kH2 },
    { "block",    kBlock },
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
// it away and issue a warning.  In practice this
// should not be fatal because your variable will 
// still show up in the tree (e.g. L.s1.lt[4] will
// show up together with L.s1.lt[0],... etc.).
  UInt_t pos1,pos2;
  THaString open_brack = "[";
  THaString close_brack = "]";
  THaString result = "";
  pos1 = var.find(open_brack,0);
  pos2 = var.find(close_brack,0);
  if ((pos1 != string::npos) &&
      (pos2 != string::npos)) {
      result = var.substr(0,pos1);
      result += var.substr(pos2+1,var.length());    
      cout << "THaOutput:WARNING:: Stripping away ";
      cout << "unwanted brackets from "<<var<<endl;
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
    cerr << "THaOutput::LoadFile ERROR: file " << sline;
    cerr << " does not exist." << endl;
    cerr << "To use THaOutput, you must have an";
    cerr << " output definition file ";
    cerr << "(see ./examples/output.def)"<<endl;
    return;
  }
  cerr << "THaOutput::ERROR: Syntax error in output definition file."<<endl;
  cerr << "The offending line is :\n"<<sline<<endl<<endl;
  switch (iden) {
     case kVar:
       cerr << "For variables, the syntax is: "<<endl;
       cerr << "    variable  variable-name"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    variable   R.vdc.v2.nclust"<<endl;;
     case kForm:
       cerr << "For formulas, the syntax is: "<<endl;
       cerr << "    formula  formula-name  formula-expression"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    formula targetX 1.464*B.bpm4b.x-0.464*B.bpm4a.x"<<endl;
     case kH1:
       cerr << "For 1D histograms, the syntax is: "<<endl;
       cerr << "  TH1F name  'title' variable nbin xlo xhi [cut-expr]"<<endl;          cerr << "Example: "<<endl;
       cerr << "  TH1F  tgtx 'target X' targetX 100 -2 2"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       cerr << "optionally can impose THaCut expression 'cut-expr'"<<endl;
       break;
     case kH2:
       cerr << "For 2D histograms, the syntax is: "<<endl;
       cerr << "  TH2F  name  'title'  var1  var2";
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
  // Printout the definitions
  typedef vector<THaString>::const_iterator Iter;
  cout << "\n=== Number of variables "<<fVarnames.size()<<endl;
  for (Iter is = fVarnames.begin();
    is != fVarnames.end(); is++) cout << " Variable = "<<*is<<endl;
  cout << "\n=== Number of formulas "<<fFormnames.size()<<endl;
  for (Iter is = fFormnames.begin();
    is != fFormnames.end(); is++) cout << "Formula name = "<<*is<<endl;
  cout << "\n=== Formula definitions "<<fFormdef.size()<<endl;
  for (Iter is = fFormdef.begin();
    is != fFormdef.end(); is++) cout << "Formula definition = "<<*is<<endl;
  cout << "\n=== Number of 1d histograms "<<fH1dname.size()<<endl;
  for (Vsiz_t i = 0; i < fH1dname.size(); i++) {
    cout << "1d histo "<<i<<"   name "<<fH1dname[i];
    cout << "  title =  "<<fH1dtit[i]<<endl;
    cout << "   var = "<<fH1plot[i];
    cout << "   binning "<<fH1dbin[i]<<"   "<<fH1dxlo[i];
    cout << "   "<<fH1dxhi[i]<<endl;
    if (fH1cutid[i] != fgNocut) cout << "Cut condition " << fH1cut[i] << endl;
  }
  cout << "\n=== Number of 2d histograms "<<fH2dname.size()<<endl;
  for (Vsiz_t i = 0; i < fH2dname.size(); i++) {
    cout << "2d histo "<<i<<"   name "<<fH2dname[i]<<endl;
    cout << "  title =  "<<fH2dtit[i];
    cout << "  x var = "<<fH2plotx[i];
    cout << "  y var = "<<fH2ploty[i]<<endl;
    cout << "  x binning "<<fH2dbinx[i]<<"   "<<fH2dxlo[i];
    cout << "   "<<fH2dxhi[i]<<endl;
    cout << "  y binning "<<fH2dbiny[i]<<"   "<<fH2dylo[i];
    cout << "   "<<fH2dyhi[i]<<endl;
    if (fH2cutid[i] != fgNocut) cout << "Cut condition " << fH2cut[i] << endl;
  }
  cout << endl << endl;
}

//_____________________________________________________________________________
Int_t THaOutput::ParseTitle(const THaString& sline)
{
  // Parse the string that defines the histogram.  The title must be
  // enclosed in single quotes (e.g. 'my title').  Ret value 'result'
  // means:  -1 == error,  1 == ok 1D histogram,  2 == ok 2D histogram
  Int_t result = -1;
  stitle = "";   sfvar1 = "";  sfvar2  = "";
  iscut = fgNocut;  scut = "";
  THaString::size_type pos1 = sline.find_first_of("'");
  THaString::size_type pos2 = sline.find_last_of("'");
  if (pos1 != THaString::npos && pos2 > pos1) {
    stitle = sline.substr(pos1+1,pos2-pos1-1);
  }
  THaString ctemp = sline.substr(pos2+1,sline.size()-pos2);
  vector<THaString> stemp = ctemp.Split();
  if (stemp.size() > 1) {
     sfvar1 = stemp[0];
     Int_t ssize = stemp.size();
     if (ssize == 4 || ssize == 5) {
       sscanf(stemp[1].c_str(),"%d",&n1);
       sscanf(stemp[2].c_str(),"%f",&xl1);
       sscanf(stemp[3].c_str(),"%f",&xh1);
       if (ssize == 5) {
         iscut = 1; scut = stemp[4];
       }
       result = 1;
     }
     if (ssize == 8 || ssize == 9) {
       sfvar2 = stemp[1];
       sscanf(stemp[2].c_str(),"%d",&n1);
       sscanf(stemp[3].c_str(),"%f",&xl1);
       sscanf(stemp[4].c_str(),"%f",&xh1);
       sscanf(stemp[5].c_str(),"%d",&n2);
       sscanf(stemp[6].c_str(),"%f",&xl2);
       sscanf(stemp[7].c_str(),"%f",&xh2);
       if (ssize == 9) {
         iscut = 1; scut = stemp[8]; 
       }
       result = 2;
     }
  }
  return result;
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
ClassImp(THaOdata)
ClassImp(THaOutput)
