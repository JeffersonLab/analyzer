//*-- AUTHOR :    Robert Michaels   05/08/2002

//////////////////////////////////////////////////////////////////////////
//
// THaVhist
//
// Vector of histograms; of course can be just 1 histogram.
// This object uses TH1 class of ROOT to form a histogram
// used by THaOutput.
// The X and Y axis, as well as cut conditions, can be
// formulas of global variables.  
// They can also be arrays of variables, or vector formula,
// though certain rules apply about the dimensions; see
// THaOutput documentation for those rules.
//
//
// author:  R. Michaels    May 2003
//
//////////////////////////////////////////////////////////////////////////

#include "THaVhist.h"
#include "THaVform.h"
#include "THaString.h"
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
#include "TROOT.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iostream>

using namespace std;
using namespace THaString;

//_____________________________________________________________________________
THaVhist::THaVhist( const string& type, const string& name, 
		    const string& title ) :
  fType(type), fName(name), fTitle(title), fNbinX(0), fNbinY(0), fSize(0),
  fInitStat(0), fScalar(0), fEye(0), fEyeOffset(0), fXlo(0.), fXhi(0.), fYlo(0.), fYhi(0.),
  fFirst(kTRUE), fProc(kTRUE), fFormX(NULL), fFormY(NULL), fCut(NULL),
  fMyFormX(kFALSE), fMyFormY(kFALSE), fMyCut(kFALSE)
{ 
  fH1.clear();
}

//_____________________________________________________________________________
THaVhist::~THaVhist() 
{
  if (fMyFormX) delete fFormX;
  if (fMyFormY) delete fFormY;
  if (fMyCut) delete fCut;
  if( TROOT::Initialized() ) {
    for (std::vector<TH1*>::iterator ith = fH1.begin();
	 ith != fH1.end(); ++ith) delete *ith;
  }
}

//_____________________________________________________________________________
void THaVhist::CheckValidity( ) 
{
  fProc = kTRUE;
  if (fEye == 0 && fScalar == 0 && fSize != static_cast<Int_t>(fH1.size())) {
     fProc = kFALSE;
     ErrPrint();
     cerr << "THaVhist:ERROR:: Inconsistent sizes."<<endl;
  }
  if (fFormX == 0) {
     fProc = kFALSE;
     ErrPrint();
     cerr << "THaVhist:ERROR:: No X axis defined."<<endl;
  }
  if (fInitStat != 0) {
     fProc = kFALSE;
     ErrPrint();
     cerr << "THaVhist:ERROR:: Improperly initialized."<<endl;
  }
} 

//_____________________________________________________________________________
Int_t THaVhist::Init( ) 
{
  // Must be called once at beginning of execution.
  // Initializes this vector of histograms.
  // Sets up the X, Y, and cuts as THaVforms.
  // Checks the dimensionality of this object and
  // calls BookHisto() to instantiate the histograms.
  // fInitStat is returned.
  // fInitStat ==  0  --> all ok
  // fInitStat !=0 --> various errors, interpreted 
  //                   with ErrPrint();

  const int ldebug = 0;

  for (std::vector<TH1*>::iterator ith = fH1.begin();
       ith != fH1.end(); ++ith) delete *ith;
  fH1.clear();
  fInitStat = 0;
  Int_t status;
  string sname;

  if (ldebug) cout << "THaVhist :: init "<<fName<<endl;

  if (fNbinX == 0) {
     cerr << "THaVhist:ERROR:: Histogram "<<fName<<" has no bins."<<endl;
     fInitStat = kNoBinX;
     return fInitStat;
  }
  if (fFormX == 0) {
     if (fVarX == "") {
       cerr << "THaVhist:WARNING:: Empty X formula."<<endl;
     }
     sname = fName+"X";
     if (ldebug) cout << "THaVhist ::  X var "<<sname<<endl;

     if (FindEye(fVarX)) {
       fFormX = new THaVform("eye",sname.c_str(),"[0]");
       fFormX->SetEyeOffset(fEyeOffset);
     } else {
       fFormX = new THaVform("formula",sname.c_str(),fVarX.c_str());
     }
     fMyFormX = kTRUE;

     status = fFormX->Init();
     if (status != 0) {
       fFormX->ErrPrint(status);
       fInitStat = kIllFox;
       return fInitStat;
     }
  }

  if (ldebug) cout << "THaVhist :: Y bins "<<fNbinY<<endl;

  if (fNbinY != 0 && fFormY == 0) {
     if (fVarY == "") {
       cerr << "THaVhist:WARNING:: Empty Y formula."<<endl;
     }
     sname = fName+"Y";
     if (FindEye(fVarY)) {
       fFormY = new THaVform("eye",sname.c_str(),"[0]");
       fFormY->SetEyeOffset(fEyeOffset);
     } else {
       fFormY = new THaVform("formula",sname.c_str(),fVarY.c_str());
     }
     fMyFormY = kTRUE;

     status = fFormY->Init();
     if (status != 0) {
       fFormY->ErrPrint(status);
       fInitStat = kIllFoy;
       return fInitStat;
     }
  }
  if (fCut == 0 && HasCut()) {
     sname = fName+"Cut";
     fCut = new THaVform("cut",sname.c_str(),fScut.c_str());
     fMyCut = kTRUE;

     status = fCut->Init();
     if (status != 0) {
       fCut->ErrPrint(status);
       fInitStat = kIllCut;
       return fInitStat;
     }
  }


  fSize = FindVarSize();

  if (ldebug) cout << "THaVhist :: fSize =  "<<fSize<<endl;

  if (fSize < 0) {
    switch (fSize) {
      case -1:
        fInitStat = kNoX;
        return fInitStat;

      case -2:
        fInitStat = kAxiSiz;
        return fInitStat;

      case -3:
        fInitStat = kCutSix;
        return fInitStat;

      case -4:
        fInitStat = kCutSiy;
        return fInitStat;

      default:
        fInitStat = kUnk;
        return fInitStat;
    }
  }

  BookHisto(0, fSize);

  return fInitStat;
}

//_____________________________________________________________________________
void THaVhist::ReAttach( ) 
{
  if (fFormX && fMyFormX) fFormX->ReAttach();
  if (fFormY && fMyFormY) fFormY->ReAttach();
  if (fCut && fMyCut) fCut->ReAttach();
  return;
}


//_____________________________________________________________________________
Bool_t THaVhist::FindEye(const string& var) {
// If the variable is "[I]" it is an "Eye" variable.
// This means we will simply use I = 0,1,2, to fill that axis.
  const int debug=0;
  string::size_type pos, pos1,pos2;
  string eye  = "[I]";
  if(debug) cout << "FindEye for var = "<<var<<endl;
  pos1 = var.find(ToUpper(eye),0);
  pos2 = var.find(ToLower(eye),0);
  pos = string::npos;
  if (pos1 != string::npos) pos = pos1;
  if (pos2 != string::npos) pos = pos2;
  if (pos  != string::npos) {
    if (var.length() == eye.length()) {
      if(debug) cout << "Is an Eye var [I]"<<endl;
      return kTRUE;
    } 
  }
// If you did not find [I] there could be an offset like [I+1]. 
  if(debug) cout << "Checking for Eye offset "<<endl;
  return FindEyeOffset(var);
}

//_____________________________________________________________________________
Bool_t THaVhist::FindEyeOffset(const string& var) {
// If the variable is "[I+offset]" it is an "Eye" variable
// with offset fEyeOffset.
// This means we will use I = 0+offset, 1+offset, 2+offset ...
// to fill that axis.
  int debug=0;
  fEyeOffset = 0;
  string::size_type pos, pos1,pos2,pos3;
  string eyestart  = "[I+";
  string eyeend = "]";
  pos1 = var.find(ToUpper(eyestart),0);
  pos2 = var.find(ToLower(eyestart),0);
  pos = string::npos;
  if (pos1 != string::npos) pos = pos1;
  if (pos2 != string::npos) pos = pos2;
  if (pos  != string::npos) {
    pos3 = var.find(eyeend,pos);
    if (pos3 != string::npos) {
      pos3 = var.find(eyeend,pos);
      if (pos3 != string::npos) {
         string cnum = var.substr(pos+3,var.length()-4);
         fEyeOffset = atoi(cnum.c_str());       
         if (debug) cout << "FindEyeOffset: substring with number "<<pos<<"   "<<pos3<<"    "<<cnum<<"   "<<fEyeOffset<<endl;
	 return kTRUE;
      }
    }
  }
  if (debug) cout << "Not an Eye variable! "<<endl;
  return kFALSE;
}

//_____________________________________________________________________________
Int_t THaVhist::FindVarSize()
{
 // Find the size of this object, according to these rules:
 //
 // 1) Scalar: One histogram
 // If this histogram was declared as "sTH1F", "sTH2F" etc in the
 // output definition file (type starts with "s", case insensitive), 
 // it is a scalar, i.e. one histogram.  
 // If X (Y) is a vector and Y (X) is a scalar, then all the vectors 
 // get filled for the given scalar.
 // If the X and Y are both vectors they must have the same 
 // dimensions or there is an error, and the indices track.
 // The cut can be a scalar or a vector; if the cut is a 
 // vector it must have the same dimenions as the X or the Y since
 // the indices track with the variable which is a vector.
 //
 // 2) potentially a vector -- the old (ca 2003) behavior.
 // If the histogram was declared as vTH1F, vTH2F, etc it is 
 // potentially a vector, depending on rules 2a, 2b, 2c below.  
 // Actually this is the case if the first character is not "s"
 // or is absent, e.g. just TH1F, TH2F, etc. (old behavior).
 // Rules to determine size:
 // 2a) If one of the variables is an "eye" ("[I]") then  
 //    the other variable to determines the eye's size.
 //    (Also note, "[I+4]" is an "eye" but would add an offset of 4 to 
 //     the index I.  I suppose the most common use would be [I+1]. )
 // 2b) If the cut is a scalar, and if one of X or Y is 
 //     a scalar, this histo will be a scalar (i.e. one element).
 // 2c) If neither a) nor b), then the size is the size of the 
 //     X component.
 // The sizes of X,Y,cuts must obey the rules encoded below.
 // Return code:
 //      1 = size is 1, i.e. a scalar.
 //    > 1 = size of this vector.
 //    < 0 = Rules were violated, as follows:
 //     -1 = No X defined.  Must have at least an X.
 //     -2 = Inconsistent size of X and Y.
 //     -3 = Cut size inconsistent with X.
 //     -4 = Cut size inconsistent with Y.

  const int ldebug=0;

  if (!fFormX) return -2;  // Error, must have at least an X.
  Int_t sizex = 0, sizey = 0, sizec = 0;
  if (fCut) sizec = fCut->GetSize();
// From output definition file, it's already known to be a scalar
// because of the "s" prefix, e.g. "sth2f"
  if (fScalar) {  
    sizex = fFormX->GetSize();
    if (ldebug) cout << "THaVhist:: is scalar , sizex = "<<sizex<<"  sizec "<<sizec<<"   fFormY "<<fFormY<<endl;
    if (fFormY) {
      sizey = fFormY->GetSize();
      if (ldebug) cout << "THaVhist::  sizey = "<<sizey<<endl;
      if (sizey != sizex) {
	cerr << "Scalar histogram, but inconsistent sizes of X and Y"<<endl;
        return -2;
      } 
    }
    if (sizec > 1) { // if sizec=1 or 0 it's ok.
      if (sizec != sizex) {
	cerr << "Scalar histogram, but cut size inconsistent"<<endl;
        return -3;
      }
    }
    return 1;   // It's a scalar
  }

// Find the size by rules 2a, 2b, 2c (explained above) :  
// This is the old (circa 2003) behavior
  if ( fFormX->IsEye() ) {
    fEye = 1;
  } else {
    sizex = fFormX->GetSize();
  }
  if (fFormY) { 
    sizey = fFormY->GetSize();
    if ( fFormY->IsEye() ) {
      fEye = 1;
      sizey = sizex;
    } else {
      if (fFormX->IsEye()) sizex = sizey;
    }
  }
  if ( (sizex != 0 && sizey != 0) && (sizex != sizey) ) { 
    cerr<< "THaVhist::ERROR: inconsistent axis sizes"<<endl;
    return -2;
  }
  if ( (sizex != 0 && sizec > 1) && (sizex != sizec) ) {
    cerr<< "THaVhist::ERROR: inconsistent cut size"<<endl;
    return -3;
  }
  if ( (sizey != 0 && sizec > 1) && (sizey != sizec) ) {
    cerr<< "THaVhist::ERROR: inconsistent cut size"<<endl;
    return -4;
  }
  if ( (sizec <= 1) && (sizex <=1 || sizey <= 1) ) {
    fScalar = 1;  // This object is a scalar.
    return 1;  
  }            
  fScalar = 0;    // This object is a vector.
  return sizex;
}


//_____________________________________________________________________________
Int_t THaVhist::BookHisto(Int_t hfirst, Int_t hlast) 
{ 
  // Using ROOT Histogram classes, set up the vector
  // of histograms of type fType (th1f, th1d, th2f, th2d) 
  // for vector indices that run from hfirst to hlast.
  const int ldebug=0;
  fSize = hlast;
  if(ldebug) cout << "BookHisto:: "<<hfirst<<"  "<<hlast<<endl;
  if (hfirst > hlast) return -1;
  if (fSize > fgVHIST_HUGE) {
    // fSize cannot be infinite.  
    // The following probably indicates a usage error.
    cerr << "THaVhist::WARNING: Asking for a too-huge";
    cerr << " number of histograms !!" << endl;
    cerr << "Truncating at " << fgVHIST_HUGE << endl;
    fSize = fgVHIST_HUGE;
  }
  // Booking a histogram with zero bins is suspicious.
  if (fNbinX == 0) cerr << "THaVhist:WARNING: fNbinX = 0."<<endl;

  string sname = fName;
  string stitle = fTitle;
  bool doing_array = (fSize>1);
  for (Int_t i = hfirst; i < fSize; ++i) {
    if (ldebug) cout << "BookHisto "<<i<<"   "<<fType<<endl;
    if (fEye == 0 && doing_array) {
       sname = fName + Form("%d",i); 
       stitle = fTitle + Form(" %d",i);
    }
    if (fEye == 1 && i > hfirst) continue;
    if (CmpNoCase(fType,"th1f") == 0) {
       fH1.push_back(new TH1F(sname.c_str(), 
	     stitle.c_str(), fNbinX, fXlo, fXhi));
    }
    if (CmpNoCase(fType,"th1d") == 0) {
       fH1.push_back(new TH1D(sname.c_str(), 
	     stitle.c_str(), fNbinX, fXlo, fXhi));
    }
    if (CmpNoCase(fType,"th2f") == 0) { 
      fH1.push_back(new TH2F(sname.c_str(), stitle.c_str(), 
       	  fNbinX, fXlo, fXhi, fNbinY, fYlo, fYhi));
      if (fNbinY == 0) {
      // Booking a 2D histo with zero Y bins is suspicious.
       cerr << "THaVhist:WARNING:: ";
       cerr << "2D histo with fNbiny = 0 ?"<<endl;
      }
    }
    if (CmpNoCase(fType,"th2d") == 0) {
      fH1.push_back(new TH2D(sname.c_str(), stitle.c_str(), 
	  fNbinX, fXlo, fXhi, fNbinY, fYlo, fYhi));
      if (fNbinY == 0) {
        cerr << "THaVhist:WARNING:: ";
        cerr << "2D histo with fNbiny = 0 ?"<<endl;
      }
    }
  }
  return 0;
}
 
//_____________________________________________________________________________
Int_t THaVhist::Process() 
{
  // Every event must Process() to fill histograms.
  // Two cases: This object is either a scalar or a vector.
  // If it's a scalar, then we fill one histogram, and if one 
  // of the variables is an array then all its elements are put 
  // into that histogram.  
  // If it's a vector, then (as previously verified) 2 or more 
  // from among [x, y, cuts] are vectors of same size.  Hence 
  // fill a vector of histograms with indices running in parallel.
  // Also, a vector of histograms can grow in size (up to a 
  // sensible limit) if the inputs grow in size.

  // Check validity just once for efficiency.  
  const int ldebug=0;
  if (ldebug) cout << "----------------  THaVhist :: Process  "<<fName<<"   //  "<<fTitle<<endl<<flush;
  if (fFirst) {
    fFirst = kFALSE;  
    CheckValidity();
  }
  if ( !IsValid() ) return -1;
  
  if (fMyFormX) fFormX->Process();
  if (fFormY && fMyFormY) fFormY->Process();
  Int_t sizec = 0;
  if (fCut && fMyCut) {
     fCut->Process();
     sizec = fCut->GetSize();
  }

  if ( IsScalar() ) {  
    // The following is my interpretation of the original code with a bugfix
    // for the case where both X and Y are variable-size arrays (23-Jan-17 joh)
    Int_t sizex = fFormX->GetSize();
    if(ldebug) cout << "THaVhist :: Process   sizex  "<<sizex<<endl<<flush;

    if( fFormY ) {  // 2D histo
      Int_t sizey = fFormY->GetSize(); 
      if(ldebug) cout << "THaVhist :: Process   sizey  "<<sizey<<endl<<flush;
     // Vararrys that report zero size are empty; nothing to do
      if( (sizex == 0 && fFormX->IsVarray()) ||
	  (sizey == 0 && fFormY->IsVarray()) )
	return 0;
      // Slightly tricky coding here to avoid redundant testing inside the loop
      Int_t zero = 0, i = 0;
      // If GetSize()==0 for either variable, retrieve index = 0
      Int_t *ix = (sizex == 0) ? &zero : &i;
      Int_t *iy = (sizey == 0) ? &zero : &i;
      // if cut size > 1, let the index track the other variable's index
      Int_t *ic = (sizec <= 1) ? &zero : &i;

      if(ldebug) cout << "THaVhist :: Process   ix, iy, ic  "<<*ix<<"  "<<*iy<<"  "<<*ic<<endl;
      
      // Number of valid elements:
      // If GetSize()==0 for X or Y, it's the size of the other variable
      //  (one or more entries in Y (X) vs one fixed X (Y))
      // If both have size > 0, it's the smaller of the two
      //  (diagonal elements of the XY index matrix)
      Int_t n = (sizex == 0 || sizey == 0) ? max(sizex,sizey) : min(sizex,sizey);
      if(ldebug) cout << "THaVhist :: Process   n  "<<n<<endl;
      for ( ; i < n; ++i) {
	//        cout << "THaVhist :: proc loop: data  "<<i<<"  "<<fFormX->GetData(*ix)<<"   "<<fFormY->GetData(*iy)<<"  *ic "<<*ic<<endl<<flush;
	if ( CheckCut(*ic)==0 ) continue;
	//  cout << "THaVhist :: proc loop:     FILLING HISTO "<<i<<endl;
 	fH1[0]->Fill(fFormX->GetData(*ix), fFormY->GetData(*iy));
      }

    } else {  // 1D histo
      
      for (Int_t i = 0; i < sizex; ++i) {
        if(ldebug) cout << "THaVhist :: 1D histo "<<i<<"  "<<sizec<<endl;
        if (sizec == sizex) {
  	   if ( CheckCut(i)==0 ) continue;
	} else {
 	   if ( CheckCut()==0 ) continue;
	}
	fH1[0]->Fill(fFormX->GetData(i));
      }
    }

  } else { // Vector histogram.  

// Expand if the size has changed.
    Int_t size = FindVarSize();
    if (size < 0) return -1;
    if (size > fSize) {
      BookHisto(fSize, size);
      fSize = size;
    }    

    // Slightly tricky coding here to avoid redundant testing inside the loop
    Int_t zero = 0, i;
    Int_t* idx = (fEye == 1) ? &zero : &i;
    if( fFormY ) {
      for (i = 0; i < fSize; ++i) {
	if ( CheckCut(i)==0 ) continue; 
	fH1[*idx]->Fill(fFormX->GetData(i), fFormY->GetData(i));
      }
    } else {
      for (i = 0; i < fSize; ++i) {
	if ( CheckCut(i)==0 ) continue; 
	fH1[*idx]->Fill(fFormX->GetData(i));
      }
    }
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaVhist::End() 
{
  for (vector<TH1* >::iterator ith = fH1.begin(); 
      ith != fH1.end(); ++ith ) (*ith)->Write();
  return 0;
}


//_____________________________________________________________________________
void THaVhist::ErrPrint() const
{
  cerr << "THaVhist::ERROR:: Invalid histogram."<<endl;
  cerr << "Offending line:"<<endl;
  cerr << fType << "  " << fName << "  '" << fTitle<< "'  ";
  cerr << GetVarX() << "   "  << fNbinX;
  cerr << "  " << fXlo << "  " << fXhi;
  if (fNbinY != 0) {
    cerr << GetVarY() << "  " << fNbinY;
    cerr << "  " << fYlo << "  " << fYhi;
  }
  if (HasCut()) cerr << " "<<fScut;
  cerr << endl;
  switch (fInitStat) {
    case kNoBinX:
      cerr << "Number of bins in X is zero."<<endl;
      cerr << "This must be a typo error."<<endl;
      break;
    case kIllFox:
      cerr << "Illegal formula on X axis."<<endl;
      if (fFormX) fFormX->ErrPrint(fFormX->Init());
      break;
    case kIllFoy:
      cerr << "Illegal formula on Y axis."<<endl;
      if (fFormY) fFormY->ErrPrint(fFormY->Init());
      break;
    case kIllCut:
      cerr << "Illegal formula for Cut."<<endl;
      if (fCut) fCut->ErrPrint(fCut->Init());
      break;
    case kNoX:
      cerr << "No X axis defined.  Must at least have an X."<<endl;
      break;
    case kAxiSiz:
      cerr << "Inconsistent size of X and Y axes."<<endl;
      break;
    case kCutSix:
      cerr << "Size of Cut inconsistent with size of X."<<endl;
      break;
    case kCutSiy:
      cerr << "Size of Cut inconsistent with size of Y."<<endl;
      break;
    case kOK:
    case kUnk:
    default:
      cerr << "Unknown error."<<endl;  
      break;
  }
}

//_____________________________________________________________________________
void THaVhist::Print() const
{
  cout << "  Histo "<<fType<<" : "<<fName;
  cout << "  Size : "<<fSize;
  cout << "  Title : '"<<fTitle<<"' "<<endl;
  cout << "  X : "<<GetVarX()<<"  nbins "<<fNbinX;
  cout << "  xlo "<<fXlo<<"  xhi "<<fXhi<<endl;
  if (fNbinY != 0) {
    cout << "  Y : "<<GetVarY()<<"  nbins "<<fNbinY;
    cout << "  ylo "<<fYlo<<"  yhi "<<fYhi<<endl;
  }
  if (HasCut()) cout << "  Cut : "<<fScut<<endl;
  if (fInitStat != kOK) ErrPrint();
}

//_____________________________________________________________________________
ClassImp(THaVhist)
