#ifndef Podd_THaVhist_h_
#define Podd_THaVhist_h_

//////////////////////////////////////////////////////////////////////////
//
// THaVhist
//
//////////////////////////////////////////////////////////////////////////

#include "THaVform.h"
#include "TTree.h"
#include <vector>
#include <string>

class THaVar;
class TH1F;
class TH2F;
class THaCut;

using std::string;

class THaVhist {
  
public:

   THaVhist(const string& type, const string& name, const string& title);
   virtual ~THaVhist(); 

// Set up the axis and cuts, as applicable.
   void  SetX(Int_t nbinx, Double_t xlo, Double_t xhi, const string& varx);
   void  SetX(THaVform *varx);
   void  SetY(Int_t nbiny, Double_t ylo, Double_t yhi, const string& vary);
   void  SetY(THaVform *vary);
   void  SetCut(const string& cut);
   void  SetCut(THaVform *cut);
   void  SetScalarTrue() { fScalar = 1; };
   const string& GetVarX() const { return fVarX; };
   const string& GetVarY() const { return fVarY; };
   const string& GetCutStr() const  { return fScut; };
   Int_t CheckCut(Int_t index=0);
   Bool_t HasCut() const { return !fScut.empty(); };
   Bool_t IsValid() const { return fProc; };
// Must Init() once at beginning after various Set()'s
   Int_t Init();
// Must ReAttach() if pointers to global variables reset
   void ReAttach();
// Must Process() each event. 
   Int_t Process();
// Must End() to write histogram to output at end of analysis.
   Int_t End();
// Self-explanatory printouts.
   void  Print() const;
   void  ErrPrint() const;
// CheckValid() checks if this histogram is valid.
// If invalid, you get no output.
   void CheckValidity();
// IsScalar() is kTRUE if histogram is a scalar.
   Bool_t IsScalar() { return (fScalar==1); };
   Int_t GetSize() { return fSize; };

protected:

   Int_t ParseVar();
   Int_t BookHisto(Int_t hfirst, Int_t hlast);
   Int_t FindVarSize();
   Bool_t FindEye(const string& var);
   Bool_t FindEyeOffset(const string& var);
   Int_t GetCut(Int_t index=0); 

   enum FEr { kOK = 0, kNoBinX, kIllFox, kIllFoy, kIllCut,
              kNoX, kAxiSiz, kCutSix, kCutSiy,
              kUnk };

   static const int fgVERBOSE = 1;
   static const int fgVHIST_HUGE = 10000;

   string fType, fName, fTitle, fVarX, fVarY, fScut;
   Int_t fNbinX, fNbinY, fSize, fInitStat, fScalar, fEye, fEyeOffset;
   Double_t fXlo, fXhi, fYlo, fYhi;
   Bool_t fFirst, fProc;

   std::vector<TH1* > fH1;
   THaVform *fFormX, *fFormY, *fCut;
   Bool_t fMyFormX, fMyFormY, fMyCut;

private:

  THaVhist(const THaVhist& vhist);
  THaVhist& operator=(const THaVhist& vhist);

  ClassDef(THaVhist,0)  // Vector of histograms.

};

inline 
void THaVhist::SetX(Int_t nbinx, Double_t xlo, Double_t xhi,
		    const string& varx)
{
  fNbinX = nbinx;  fXlo = xlo;  fXhi = xhi;  fVarX = varx;
};


inline 
void THaVhist::SetX(THaVform *varx)
{
  if(fMyFormX) delete fFormX;
  fFormX = varx;
  fMyFormX = false;
};


inline 
void THaVhist::SetY(Int_t nbiny, Double_t ylo, Double_t yhi,
		    const string& vary)
{
  fNbinY = nbiny;  fYlo = ylo;  fYhi = yhi;  fVarY = vary;
};


inline 
void THaVhist::SetY(THaVform *vary)
{
  if(fMyFormY) delete fFormY;
  fFormY = vary;
  fMyFormY = false;
};


inline 
void THaVhist::SetCut(const string& cut)
{
  fScut = cut;
};


inline 
void THaVhist::SetCut(THaVform *cut)
{
  if(fMyCut) delete fCut;
  fCut = cut;
  fMyCut = false;
};

inline
Int_t THaVhist::CheckCut(Int_t index) { 
  // Check the cut.  Returns !=0 if cut condition passed.
  return (fCut) ? Int_t(fCut->GetData(index)) : 1;
}

#endif






