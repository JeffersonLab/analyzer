#ifndef ROOT_THaOutput
#define ROOT_THaOutput

//////////////////////////////////////////////////////////////////////////
//
// THaOutput
//
//////////////////////////////////////////////////////////////////////////

#define THAOMAX 100
#include "TObject.h"
#include "THaString.h"
#include "TTree.h"
#include <vector>
#include <map>
#include <iterator>
#include <iostream>

class THaFormula;
class THaVar;
class TH1F;
class TH2F;
class THaCut;
class THaVform;
class THaVhist;
class THaScalerGroup;
class THaEvData;

using namespace std;

class THaOdata {
// Utility class used by THaOutput to store arrays 
// up to size 'nsize' for tree output.
public:
  THaOdata(int n=THAOMAX) : nsize(n) { data = new Double_t[n]; Clear(); }
  THaOdata(const THaOdata& other) : nsize(other.nsize) {
    data = new Double_t[nsize]; ndata = other.ndata;
    memcpy( data, other.data, nsize*sizeof(Double_t));
  }
  THaOdata& operator=(const THaOdata& rhs) { 
    if( this != &rhs ) {
      if( nsize < rhs.nsize ) {
	nsize = rhs.nsize; delete [] data; data = new Double_t[nsize];
      }
      ndata = rhs.ndata; memcpy( data, rhs.data, nsize*sizeof(Double_t));
    }
    return *this;   
  }
  virtual ~THaOdata() { delete [] data; };
  void AddBranches(TTree *T, std::string name) {
     std::string sname = "Ndata." + name;
     std::string leaf = sname;
     T->Branch(sname.c_str(),&ndata,(leaf+"/I").c_str());
     leaf = "data["+leaf+"]/D";
     T->Branch(name.c_str(),data,leaf.c_str());
  }
  void Clear( Option_t* opt="" ) {
    ndata = 0; memset(data, 0, nsize*sizeof(Double_t)); 
  }
  Int_t Fill(Double_t dat) { return Fill(0, dat); };
  Int_t Fill(Int_t i, Double_t dat) {
    if (i >= 0 && i < nsize-1) {
      if (i >= ndata) ndata = i+1;
      data[i] = dat;
      return 1;
    }
    return 0;
  }
  Int_t Fill(Int_t n, const Double_t* array) {
    if( n<0 || n>nsize ) return 0;
    memcpy( data, array, n*sizeof(Double_t));
    ndata = n;
    return 1;
  }
  Double_t Get(Int_t index=0) {
    if( index<0 || index>nsize ) return 0;
    return data[index];
  }
  Int_t       ndata;   // Number of array elements
  Int_t       nsize;   // Maximum number of elements
  Double_t*   data;    // [ndata] Array data

private:

  ClassDef(THaOdata,3)  // Variable sized array
};


class THaScalerKey {
// Utility class used by THaOutput to store a list
// of 'keys' to access scaler data for output.
public:
  THaScalerKey(THaString nm, THaString bk, Int_t rc,
          Int_t hel, Int_t sl, Int_t ch) : 
    fName(nm), fBank(bk), fRC(rc), fHelicity(hel), fSlot(sl), fChan(ch) { }
  THaScalerKey(THaString nm, THaString bk) : 
    fName(nm), fBank(bk), fRC(0), fHelicity(0), fSlot(-1), fChan(-1) { }
  virtual ~THaScalerKey() { }
  void AddBranches(TTree *T, std::string spref="") {
    std::string name = "";
    if (spref != "noprefix") name = fBank + "_";  
    if (GetHelicity() > 0) name += "P_"; 
    if (GetHelicity() < 0) name += "M_";
    name += fName;
    name = DashToUbar(name); // Can't have "-", replace with "_".
    std::string tinfo = name + "/D";
    T->Branch(name.c_str(), &fData, tinfo.c_str(), 4000);
  }
  void Fill(Double_t dat) { fData = dat; };
  void Print() {
    cout << "Scaler key name = "<<fName<<"   bank = "<<fBank;
    cout << "  sl = "<<fSlot;
    cout << "  ch = "<<fChan<<"  RC = "<<fRC<<"  hel = "<<fHelicity;
    cout << "  data = "<<fData<<endl;
  }
  Double_t GetData() { return fData; };
  THaString GetChanName() { return fName; };
  THaString GetBank() { return fBank; };
  Bool_t SlotDefined() { return (fSlot >= 0); };
  Bool_t IsRate() { return (fRC == 0); };
  Bool_t IsCounts() { return !IsRate(); };
  Int_t GetHelicity() { return fHelicity; }; // 0 = none, +1,-1 = helicity
  Int_t GetSlot()  { return fSlot; };
  Int_t GetChan() { return fChan; };

private:

  THaScalerKey(const THaScalerKey& key) {}
  THaScalerKey& operator=(const THaScalerKey& key) { return *this; }
  string DashToUbar(string& var) {
    // Replace "-" with "_" because "-" does not work in
    // a tree variable name: Draw() thinks it's minus.
    int pos,pos0=0;
    string output = var;
    while (1) {
      pos = output.find("-");
      if (pos > pos0) {
        output.replace(pos, 1, "_"); pos0 = pos;
      } else { break; }
    }
    return output;
  }
  Double_t fData;
  std::string fName, fBank;
  Int_t fRC, fHelicity, fSlot, fChan;
  ClassDef(THaScalerKey,3)  // Scaler data for output
};



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

protected:

  virtual Int_t LoadFile( const char* filename );
  virtual Int_t Attach();
  virtual Int_t FindKey(const THaString& key) const;
  virtual void  ErrFile(Int_t iden, const THaString& sline) const;
  virtual Int_t ChkHistTitle(Int_t key, const THaString& sline);
  virtual Int_t BuildBlock(const THaString& blockn);
  virtual THaString StripBracket(THaString& var) const; 
  void BuildList(std::vector<THaString > vdata);
  void AddScaler(THaString name, THaString bank, 
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
  std::vector<THaString>  fEpicsKey;
  std::vector<THaScalerKey*> fScalerKey;
  TTree *fTree, *fEpicsTree; 
  std::map<string, TTree*> fScalTree;
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

  ClassDef(THaOutput,0)  
};

#endif








