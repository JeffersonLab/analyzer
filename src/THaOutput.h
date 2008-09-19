#ifndef ROOT_THaOutput
#define ROOT_THaOutput

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "THaString.h"
#include <vector>
#include <map>
#include <string> 
#include <cstring>

class THaVar;
class TH1F;
class TH2F;
class THaVform;
class THaVhist;
class THaScalerGroup;
class THaEvData;
class TTree;

class THaOdata {
// Utility class used by THaOutput to store arrays 
// up to size 'nsize' for tree output.
public:
  THaOdata(int n=1) : tree(NULL), ndata(0), nsize(n) { data = new Double_t[n]; }
  THaOdata(const THaOdata& other);
  THaOdata& operator=(const THaOdata& rhs);
  virtual ~THaOdata() { delete [] data; };
  void AddBranches(TTree* T, std::string name);
  void Clear( Option_t* opt="" ) { ndata = 0; }  
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
class THaScalerKey;


class THaOutput {
  
public:

  THaOutput();
  virtual ~THaOutput(); 

  virtual Int_t Init( const char* filename="output.def" );
  virtual Int_t Process();
  virtual Int_t ProcScaler(THaScalerGroup *sca);
  virtual Int_t ProcEpics(THaEvData *ev);
  virtual Int_t End();
  virtual Bool_t TreeDefined() const { return fTree != 0; };
  virtual TTree* GetTree() const { return fTree; };

  static void SetVerbosity( Int_t level );
  
protected:

  virtual Int_t LoadFile( const char* filename );
  virtual Int_t Attach();
  virtual Int_t FindKey(const THaString& key) const;
  virtual void  ErrFile(Int_t iden, const THaString& sline) const;
  virtual Int_t ChkHistTitle(Int_t key, const THaString& sline);
  virtual Int_t BuildBlock(const THaString& blockn);
  virtual THaString StripBracket(THaString& var) const; 
  std::vector<THaString> reQuote(std::vector<THaString> input) const;
  THaString CleanEpicsName(THaString var) const;
  void BuildList(const std::vector<THaString>& vdata);
  void AddScaler(const THaString& name, const THaString& bank, 
         Int_t helicity = 0, Int_t slot=-1, Int_t chan=-1); 
  void DefScaler(Int_t hel = 0);
  void Print() const;

// Variables, Formulas, Cuts, Histograms

  Int_t fNvar;
  Double_t *fVar, *fEpicsVar;
  std::vector<THaString> fVarnames, 
                         fFormnames, fFormdef,
                         fCutnames, fCutdef,
                         fArrayNames, fVNames; 
  std::vector<THaVar* >  fVariables, fArrays;
  std::vector<THaVform* > fFormulas, fCuts;
  std::vector<THaVhist* > fHistos;
  std::vector<THaOdata* > fOdata;
  std::vector<THaEpicsKey*>  fEpicsKey;
  std::vector<THaScalerKey*> fScalerKey;
  TTree *fTree, *fEpicsTree; 
  std::map<std::string, TTree*> fScalTree;
  bool fInit;

  enum EId {kVar = 1, kForm, kCut, kH1f, kH1d, kH2f, kH2d, kBlock,
            kBegin, kEnd, kRate, kCount };
  static const Int_t kNbout = 4000;
  static const Int_t fgNocut = -1;

private:

  THaOutput(const THaOutput& output) {}
  THaOutput& operator=(const THaOutput& output) { return *this; }

  THaString stitle, sfvarx, sfvary, scut, fScalBank;

  Int_t nx,ny,iscut, fScalRC;
  Float_t xlo,xhi,ylo,yhi;
  Bool_t fOpenEpics,fOpenScal,fFirstEpics,fFirstScal;

  static Int_t fgVerbose;

  ClassDef(THaOutput,0)  
};

#endif








