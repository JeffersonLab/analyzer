//*-- Author :    Robert Michaels   05/08/2002

#define CHECKOUT 1

#include "THaOutput.h"
#include "TObject.h"
#include "THaFormula.h"
#include "THaVarList.h"
#include "THaVar.h"
#include "THaGlobals.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"

ClassImp(THaOdata)
ClassImp(THaOutput)

const Int_t THaOutput::fgVariden;
const Int_t THaOutput::fgFormiden;
const Int_t THaOutput::fgTh1fiden;
const Int_t THaOutput::fgTh2fiden;

THaOutput::THaOutput() {
  fTree = 0;
}

THaOutput::~THaOutput() {
  delete fTree;
  delete [] fForm;
  delete [] fVar;
  delete [] fH1vtype;
  delete [] fH1form;
  delete [] fH2vtypex;
  delete [] fH2formx;
  delete [] fH2vtypey;
  delete [] fH2formy;
}

Int_t THaOutput::Init( ) {

  Int_t iform,ivar,ihist;
  char tinfo[20], cname[100];
  fNbout = 4000;  
  fKeyint.clear();
  fKeyint.insert( make_pair (THaString("variable"), fgVariden));
  fKeyint.insert( make_pair (THaString("formula"), fgFormiden));
  fKeyint.insert( make_pair (THaString("th1f"), fgTh1fiden));
  fKeyint.insert( make_pair (THaString("th2f"), fgTh2fiden));

  if (fTree == 0) fTree = new TTree("T","Hall A Analyzer Output DST");

  if ( !gHaVars ) return -2;
  if ( LoadFile() != 0) return -3;

  fNvar = fVarnames.size();  // this gets reassigned below
  fNform = fFormnames.size();
  fN1d = fH1dname.size();
  fN2d = fH2dname.size();
  fForm = new Double_t[fNform];

  THaVar *pvar;
  vector<string> stemp1,stemp2;
  for (ivar = 0; ivar < fNvar; ivar++) {
    pvar = gHaVars->Find(fVarnames[ivar].c_str());
    if (pvar) {
      strcpy(tinfo,fVarnames[ivar].c_str());  strcat(tinfo,"/D");
      if (pvar->IsArray()) {
        fArrays.push_back( pvar );
        fOdata.push_back(new THaOdata());
        stemp1.push_back(fVarnames[ivar]);
      } else {
        fVariables.push_back( pvar );
        stemp2.push_back(fVarnames[ivar]);
      }
    } else {
      cout << "\nTHaOutput::Init: WARNING: Global variable ";
      cout << fVarnames[ivar] << " does not exist. "<< endl;
      cout << "There is probably a typo error... "<<endl;
    }
  }
  for (Int_t k = 0; (unsigned long)k < fOdata.size(); k++) {
     fTree->Branch(stemp1[k].c_str(), fOdata[k]->IsA()->GetName(),
         &fOdata[k]);
  } 
  fNvar = stemp2.size();
  fVar = new Double_t[fNvar];
  for (Int_t k = 0; k < fNvar; k++) {
     fTree->Branch(stemp2[k].c_str(), &fVar[k], tinfo, fNbout);
  }
  for (iform = 0; iform < fNform; iform++) {
    sprintf(cname,"f%d",iform);
    fFormulas.push_back(new THaFormula(cname,fFormdef[iform].c_str()));
    if (fFormulas[iform]->IsError()) {
      cout << "THaOutput::Init: WARNING: Error in formula ";
      cout << fFormnames[iform] << endl;
      cout << "There is probably a typo error... " << endl;
    }
  }
  for (iform = 0; iform < fNform; iform++) {
    strcpy(tinfo,fFormnames[iform].c_str());  strcat(tinfo,"/D");
    fTree->Branch(fFormnames[iform].c_str(), &fForm[iform], tinfo, fNbout);   
  }
  fH1vtype = new Int_t[fN1d];
  fH1form  = new Int_t[fN1d];
  memset(fH1vtype, -1, fN1d*sizeof(Int_t));
  memset(fH1form, -1, fN1d*sizeof(Int_t));
  for (ihist = 0; ihist < fN1d; ihist++) {
    for (iform = 0; iform < fNform; iform++) {
      if (fH1plot[ihist] == fFormnames[iform]) {
        fH1vtype[ihist] = fgFormiden;
	fH1form[ihist] = iform;  
      }
    } 
    pvar = gHaVars->Find(fH1plot[ihist].c_str());
    if (pvar != 0 && fH1vtype[ihist] != fgFormiden) 
        fH1vtype[ihist] = fgVariden;
    fH1var.push_back(pvar);
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
  for (ihist = 0; ihist < fN2d; ihist++) {
    for (iform = 0; iform < fNform; iform++) {
      if (fH2plotx[ihist] == fFormnames[iform]) {
        fH2vtypex[ihist] = fgFormiden;
	fH2formx[ihist] = iform;  
      }
      if (fH2ploty[ihist] == fFormnames[iform]) {
        fH2vtypey[ihist] = fgFormiden;
	fH2formy[ihist] = iform;  
      }
    } 
    pvar = gHaVars->Find(fH2plotx[ihist].c_str());
    if (pvar != 0 && fH2vtypex[ihist] != fgFormiden) 
        fH2vtypex[ihist] = fgVariden;
    fH2varx.push_back(pvar);
    pvar = gHaVars->Find(fH2ploty[ihist].c_str());
    if (pvar != 0 && fH2vtypey[ihist] != fgFormiden) 
        fH2vtypey[ihist] = fgVariden;
    fH2vary.push_back(pvar);
    fH2d.push_back( new TH2F (fH2dname[ihist].c_str(), 
         fH2dtit[ihist].c_str(), 
	 fH2dbinx[ihist], fH2dxlo[ihist], fH2dxhi[ihist],
	 fH2dbiny[ihist], fH2dylo[ihist], fH2dyhi[ihist]));
  }
  return 0;
}
 
#ifdef IFCANWORK
// Preferred method, but doesn't work, hence turned off with ifdef
Int_t THaOutput::AddToTree(char *name, TObject *tobj) {
// Add a TObject to the tree
// I would have preferred to use this method, but it did not work !!
// Instead, it works to add a TObject like this for THaOutput *fOut :
// if (fOut->TreeDefined()) fOut->GetTree()->Branch(...etc)
  if (fTree != 0 && tobj != 0) {
    fTree->Branch(name,tobj->IsA()->GetName(),&tobj);
    return 0;
  }
  return -1;
}
#endif

Int_t THaOutput::Process() {
  static int iev = 0;
  iev++;
  for (Int_t iform = 0; iform < fNform; iform++) {
    if (fFormulas[iform])
      if ( !fFormulas[iform]->IsError() ) {
        fForm[iform] = fFormulas[iform]->Eval();
      }
  }
  THaVar *pvar;
  unsigned long k;
  for (Int_t ivar = 0; ivar < fNvar; ivar++) {
    pvar = fVariables[ivar];
    if (pvar) fVar[ivar] = pvar->GetValue();
  }
  for (k = 0; k < fOdata.size(); k++) { 
    fOdata[k]->Clear();
    pvar = fArrays[k];
    if ( pvar == 0) continue;
    for (Int_t i = 0; i < pvar->GetLen(); i++) {
       if (fOdata[k]->Fill(i,pvar->GetValue(i)) != 1) 
	 cout << "THaOutput::ERROR: storing too much variable sized data: " << pvar->GetName() <<endl;
    }
  }
  for (Int_t ihist = 0; ihist < fN1d; ihist++) {
    if (fH1vtype[ihist] == fgFormiden) {
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
    Float_t x = 0;  Float_t y = 0;
    if (fH2vtypex[ihist] == fgFormiden) {
      if (fH2formx[ihist] >= 0) {
        x = fForm[fH2formx[ihist]];
      }
    } else {
      pvar = fH2varx[ihist];    
      if (pvar) {
        if (pvar->IsArray()) {
          cout << "WARNING: 2d histos w/ arrays not supported"<<endl;
          x = 0; // cannot handle arrays yet
        } else {
          x = pvar->GetValue();
        }
      }
    }
    if (fH2vtypey[ihist] == fgFormiden) {
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

Int_t THaOutput::End() {
  if (fTree != 0) fTree->Write();
  Int_t ihist;
  for (ihist = 0; ihist < fN1d; ihist++) fH1d[ihist]->Write();
  for (ihist = 0; ihist < fN2d; ihist++) fH2d[ihist]->Write();
  return 0;
}

Int_t THaOutput::LoadFile() {
// Process the file that defines the output
  ifstream *odef;
  THaString loadfile = "output.def";
  odef = new ifstream(loadfile.c_str());
  if ( ! (*odef) ) {
    ErrFile(-1, loadfile);
    return -1;
  }
  THaString comment = "#";
  vector<THaString> strvect;
  THaString sinput,sline;
  while (getline(*odef,sinput)) {
    strvect.clear();   strvect = sinput.Split();
    sline = "";
    for (vector<THaString>::iterator str = strvect.begin();
      str != strvect.end(); str++) {
      if ( *str == comment ) break;
      sline += *str;   sline += " ";
    }
    if (sline == "") continue;
    strvect = sline.Split();
    if (strvect.size() < 2) {
      ErrFile(0, sline);
      continue;
    }
    Int_t status;
    Int_t ikey = FindKey(strvect[0]);
    switch (ikey) {
      case fgVariden:
  	  fVarnames.push_back(strvect[1]);
          break;
      case fgFormiden:
          if (strvect.size() < 3) {
	    ErrFile(fgFormiden, sline);
            continue;
	  }
          fFormnames.push_back(strvect[1]);
          fFormdef.push_back(strvect[2]);
          break;
      case fgTh1fiden:
  	  status = ParseTitle(sline);
          if (status != 1) {
	    ErrFile(fgTh1fiden, sline);
            continue;
	  }
	  fH1dname.push_back(strvect[1]);
          fH1dtit.push_back(stitle);
          fH1plot.push_back(sfvar1); 
          fH1dbin.push_back(n1);
	  fH1dxlo.push_back(xl1);
	  fH1dxhi.push_back(xh1);
          break;    
      case fgTh2fiden:
  	  status = ParseTitle(sline);
          if (status != 2) {
	    ErrFile(fgTh2fiden, sline);
            continue;
	  }
	  fH2dname.push_back(strvect[1]);
          fH2dtit.push_back(stitle);  
          fH2plotx.push_back(sfvar1);  
          fH2ploty.push_back(sfvar2);  
          fH2dbinx.push_back(n1);
	  fH2dxlo.push_back(xl1);
	  fH2dxhi.push_back(xh1);
          fH2dbiny.push_back(n2);
	  fH2dylo.push_back(xl2);
	  fH2dyhi.push_back(xh2);
          break;
      default:
        cout << "Warning: keyword "<<strvect[0]<<" undefined "<<endl;
    }
  }
  delete odef;
#ifdef CHECKOUT
  Print();
#endif
  return 0;
}

Int_t THaOutput::FindKey(THaString key) {
// Return integer flag corresponding to
// case-insensitive keyword "key" if it exists
    map<THaString, Int_t >::iterator dmap = 
        fKeyint.find(THaString(key).ToLower());
    if (dmap != fKeyint.end()) return dmap->second;
    return -1;
};

void THaOutput::ErrFile(Int_t iden, THaString sline) {
// Print error messages about the output definition file.
  if (iden == -1) {
    cerr << "THaOutput::LoadFile ERROR: file " << sline;
    cerr << " does not exist." << endl;
    cerr << "You must have an output definition file ";
    cerr << "to use class THaOutput."<<endl;
    cerr << "(see ./examples/output.def)"<<endl;
    return;
  }
  cerr << "THaOutput::ERROR: Syntax error in output definition file."<<endl;
  cerr << "The offending line is :\n"<<sline<<endl<<endl;
  switch (iden) {
     case 0:
       cerr << "See the documentation or ask Bob Michaels"<<endl;
       break;
     case fgVariden:
       cerr << "For variables, the syntax is: "<<endl;
       cerr << "    variable  variable-name"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    variable   R.vdc.v2.nclust"<<endl;;
     case fgFormiden:
       cerr << "For formulas, the syntax is: "<<endl;
       cerr << "    formula  formula-name  formula-expression"<<endl;
       cerr << "Example: "<<endl;
       cerr << "    formula targetX 1.464*B.bpm4b.x-0.464*B.bpm4a.x"<<endl;
     case fgTh1fiden:
       cerr << "For 1D histograms, the syntax is: "<<endl;
       cerr << "  TH1F name  'title'  variable nbin xlo xhi"<<endl;
       cerr << "Example: "<<endl;
       cerr << "  TH1F  tgtx 'target X' targetX 100 -2 2"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       break;
     case fgTh2fiden:
       cerr << "For 2D histograms, the syntax is: "<<endl;
       cerr << "  TH2F  name  'title'  var1  var2";
       cerr << "  nbinx xlo xhi  nbiny ylo yhi"<<endl;
       cerr << "Example: "<<endl;
       cerr << "  TH2F t12 't1 vs t2' D.timeroc1  D.timeroc2";
       cerr << "  100 0 20000 100 0 35000"<<endl;
       cerr << "(Title in single quotes.  Variable can be a formula)"<<endl;
       break;
     default:
       break;
  }
};

void THaOutput::Print() {
// Printout the definitions
  cout << "\n=== Number of variables "<<fVarnames.size()<<endl;
  for (vector<THaString>::iterator is = fVarnames.begin();
    is != fVarnames.end(); is++) cout << " Variable = "<<*is<<endl;
  cout << "\n=== Number of formulas "<<fFormnames.size()<<endl;
  for (vector<THaString>::iterator is = fFormnames.begin();
    is != fFormnames.end(); is++) cout << "Formula name = "<<*is<<endl;
  cout << "\n=== Formula definitions "<<fFormdef.size()<<endl;
  for (vector<THaString>::iterator is = fFormdef.begin();
    is != fFormdef.end(); is++) cout << "Formula definition = "<<*is<<endl;
  cout << "\n=== Number of 1d histograms "<<fH1dname.size()<<endl;
  unsigned long i;
  for (i = 0; i < fH1dname.size(); i++) {
    cout << "1d histo "<<i<<"   name "<<fH1dname[i];
    cout << "  title =  "<<fH1dtit[i]<<endl;
    cout << "   var = "<<fH1plot[i];
    cout << "   binning "<<fH1dbin[i]<<"   "<<fH1dxlo[i];
    cout << "   "<<fH1dxhi[i]<<endl;
  }
  cout << "\n=== Number of 2d histograms "<<fH2dname.size()<<endl;
  for (i = 0; i < fH2dname.size(); i++) {
    cout << "2d histo "<<i<<"   name "<<fH2dname[i]<<endl;
    cout << "  title =  "<<fH2dtit[i];
    cout << "  x var = "<<fH2plotx[i];
    cout << "  y var = "<<fH2ploty[i]<<endl;
    cout << "  x binning "<<fH2dbinx[i]<<"   "<<fH2dxlo[i];
    cout << "   "<<fH2dxhi[i]<<endl;
    cout << "  y binning "<<fH2dbiny[i]<<"   "<<fH2dylo[i];
    cout << "   "<<fH2dyhi[i]<<endl;
  }
  cout << endl << endl;
};

Int_t THaOutput::ParseTitle(THaString sline) {
// Parse the string that defines the histogram.  The title must be
// enclosed in single quotes (e.g. 'my title').  Ret value 'result'
// means:  -1 == error,  1 == ok 1D histogram,  2 == ok 2D histogram
  Int_t result = -1;
  stitle = "";   sfvar1 = "";  sfvar2  = "";
  Int_t pos1, pos2;
  pos1 = 0;  pos2 = 0;
  THaString ctemp;
  pos1 = sline.find_first_of("'");
  pos2 = sline.find_last_of("'");
  if (pos1 >= 0 && pos2 > pos1) {
    stitle = sline.substr(pos1+1,pos2-pos1-1);
  }
  ctemp = sline.substr(pos2+1,sline.size()-pos2);
  vector<THaString> stemp = ctemp.Split();
  if ((long)stemp.size() > 1) {
     sfvar1 = stemp[0];
     if ((long)stemp.size() == 4) {
       sscanf(stemp[1].c_str(),"%d",&n1);
       sscanf(stemp[2].c_str(),"%f",&xl1);
       sscanf(stemp[3].c_str(),"%f",&xh1);
       result = 1;
     }
     if ((long)stemp.size() == 8) {
       sfvar2 = stemp[1];
       sscanf(stemp[2].c_str(),"%d",&n1);
       sscanf(stemp[3].c_str(),"%f",&xl1);
       sscanf(stemp[4].c_str(),"%f",&xh1);
       sscanf(stemp[5].c_str(),"%d",&n2);
       sscanf(stemp[6].c_str(),"%f",&xl2);
       sscanf(stemp[7].c_str(),"%f",&xh2);
       result = 2;
     }
  }
  return result;
};



