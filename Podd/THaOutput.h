#ifndef Podd_THaOutput_h_
#define Podd_THaOutput_h_

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"
#include <vector>
#include <map>
#include <string> 
#include <cstring>

class THaVar;
class TH1F;
class TH2F;
class THaVform;
class THaVhist;
class THaEvData;
class TTree;
class THaEvtTypeHandler;

class THaOdata {
// Utility class used by THaOutput to store arrays 
// up to size 'nsize' for tree output.
public:
  THaOdata(int n=1) : tree(NULL), ndata(0), nsize(n)
  { data = new Double_t[n]; }
  THaOdata(const THaOdata& other);
  THaOdata& operator=(const THaOdata& rhs);
  virtual ~THaOdata() { delete [] data; };
  void AddBranches(TTree* T, std::string name);
  void Clear( Option_t* ="" ) { ndata = 0; }  
  Bool_t Resize(Int_t i);
  Int_t Fill(Int_t i, Double_t dat) {
    if( i<0 || (i>=nsize && Resize(i)) ) return 0;
    if( i>=ndata ) ndata = i+1;
    data[i] = dat;
    return 1;
  }
  Int_t Fill(Int_t n, const Double_t* array) {
    if( n<=0 || (n>nsize && Resize(n-1)) ) return 0;
    memcpy( data, array, n*sizeof(Double_t));
    ndata = n;
    return 1;
  }
  Int_t Fill(Double_t dat) { return Fill(0, dat); };
  Double_t Get(Int_t index=0) {
    if( index<0 || index>=ndata ) return 0;
    return data[index];
  }
    
  TTree*      tree;    // Tree that we belong to
  std::string name;    // Name of the tree branch for the data
  Int_t       ndata;   // Number of array elements
  Int_t       nsize;   // Maximum number of elements
  Double_t*   data;    // [ndata] Array data

private:

  //  ClassDef(THaOdata,3)  // Variable sized array
};


class THaEpicsKey;
class THaEpicsEvtHandler;

class THaOutput {
  
public:

  THaOutput();
  virtual ~THaOutput(); 

  virtual Int_t Init( const char* filename="output.def" );
  virtual Int_t Process();
  virtual Int_t ProcEpics(THaEvData *ev, THaEpicsEvtHandler *han);
  virtual Int_t End();
  virtual Bool_t TreeDefined() const { return fTree != 0; };
  virtual TTree* GetTree() const { return fTree; };

  static void SetVerbosity( Int_t level );
  
protected:

  virtual Int_t LoadFile( const char* filename );
  virtual Int_t Attach();
  virtual Int_t FindKey(const std::string& key) const;
  virtual void  ErrFile(Int_t iden, const std::string& sline) const;
  virtual Int_t ChkHistTitle(Int_t key, const std::string& sline);
  virtual Int_t BuildBlock(const std::string& blockn);
  virtual std::string StripBracket(const std::string& var) const; 
  std::string svPrefix(std::string& histype);
  std::vector<std::string> reQuote(const std::vector<std::string>& input) const;
  std::string CleanEpicsName(const std::string& var) const;
  void BuildList(const std::vector<std::string>& vdata);
  void Print() const;
  // Variables, Formulas, Cuts, Histograms
  Int_t fNvar;
  Double_t *fVar, *fEpicsVar;
  std::vector<std::string> fVarnames, 
                           fFormnames, fFormdef,
                           fCutnames, fCutdef,
                           fArrayNames, fVNames; 
  std::vector<THaVar* >  fVariables, fArrays;
  std::vector<THaVform* > fFormulas, fCuts;
  std::vector<THaVhist* > fHistos;
  std::vector<THaOdata* > fOdata;
  std::vector<THaEpicsKey*>  fEpicsKey;
  TTree *fTree, *fEpicsTree; 
  bool fInit;
  
  enum EId {kVar = 1, kForm, kCut, kH1f, kH1d, kH2f, kH2d, kBlock,
            kBegin, kEnd, kRate, kCount };
  static const Int_t kNbout = 4000;
  static const Int_t fgNocut = -1;

  static Int_t fgVerbose;
  TObject*  fExtra;     // Additional member data (for binary compat.)

private:

  THaOutput(const THaOutput&);
  THaOutput& operator=(const THaOutput& );

  std::string stitle, sfvarx, sfvary, scut;

  THaEvtTypeHandler *fEpicsHandler;

  Int_t nx,ny,iscut;
  Float_t xlo,xhi,ylo,yhi;
  Bool_t fOpenEpics,fFirstEpics,fIsScalar;

  ClassDef(THaOutput,0)  
};

#endif
