#ifndef ROOT_THaOutput
#define ROOT_THaOutput

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

#define THAOMAX 50
#include "TObject.h"
#include "THaGlobals.h"
#include "THaVar.h"
#include "THaString.h"
#include "TTree.h"
#include "TNamed.h"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <iterator>

class THaFormula;
class TH1F;
class TH2F;

class THaOdata : public TObject {
// Utility class used by THaOutput to store arrays 
// up to size 'nsize' for tree output.
public:
  THaOdata() { nsize = THAOMAX; data = new Double_t[THAOMAX]; Clear(); }
  THaOdata(int n) : nsize(n) { data = new Double_t[n]; Clear(); }
  ~THaOdata() { delete [] data; };
  void Clear() { ndata = 0; memset(data, 0, nsize*sizeof(Double_t)); };
  Int_t Fill(Int_t i, Double_t dat) {
     if (i >= 0 && i < nsize-1) {
        if (i > ndata) ndata = i+1;
        data[i] = dat;
        return 1;
     }
    return 0;
  };
  Int_t       ndata,nsize;     
  Double_t    *data;    // [ndata] 
  ClassDef(THaOdata,2)  // Variable sized array
private:
  THaOdata(const THaOdata& odata);
  THaOdata& operator=(const THaOdata& odata);
};

class THaOutput {
  
public:

   THaOutput( );
   virtual ~THaOutput(); 

   virtual Int_t Init( );
   virtual Int_t Process( );
   virtual Int_t End();
   virtual Bool_t TreeDefined() { return fTree != 0; };
   virtual TTree* GetTree() { return fTree; };
#ifdef IFCANWORK
// Preferred method, but doesn't work, hence turned off with ifdef
   virtual Int_t AddToTree(char *name, TObject *tobj); 
#endif

protected:

   virtual Int_t LoadFile();
   virtual Int_t FindKey(THaString key);
   virtual void ErrFile(Int_t iden, THaString sline);
   virtual Int_t ParseTitle(THaString sline);
   void Print();

   // Variables, Formulas, Histograms

   vector<THaString> fVarnames;
   vector<THaString> fFormnames, fFormdef;
   vector<THaString> fH1dname, fH1dtit, fH2dname, fH2dtit;
   vector<Int_t> fH1dbin, fH2dbinx, fH2dbiny;
   vector<Double_t> fH1dxlo, fH1dxhi;
   vector<Double_t> fH2dxlo, fH2dxhi, fH2dylo, fH2dyhi;
   Int_t fNform, fNvar, fNbout, fN1d, fN2d;
   vector<THaFormula* > fFormulas;
   vector<THaVar* > fVariables, fArrays;
   Double_t *fForm, *fVar;
   vector<TH1F* > fH1d;
   vector<TH2F* > fH2d;
   vector<THaString> fH1plot, fH2plotx, fH2ploty;
   vector<THaVar* > fH1var, fH2varx, fH2vary;
   Int_t *fH1vtype, *fH1form;
   Int_t *fH2vtypex, *fH2formx;
   Int_t *fH2vtypey, *fH2formy;
   vector<THaOdata* > fOdata;
   TTree *fTree; 

   static const Int_t fgVariden  = 1;
   static const Int_t fgFormiden = 2;
   static const Int_t fgTh1fiden = 3;
   static const Int_t fgTh2fiden = 4;

   map<THaString, Int_t> fKeyint;

private:

   THaOutput(const THaOutput& output);
   THaOutput& operator=(const THaOutput& output);

   THaString stitle, sfvar1, sfvar2;
   Int_t n1,n2;
   float xl1,xl2,xh1,xh2;

   ClassDef(THaOutput,0)  
};

#endif








