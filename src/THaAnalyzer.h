
#ifndef ROOT_THaAnalyzer
#define ROOT_THaAnalyzer

//////////////////////////////////////////////////////////////////////////
//
// THaAnalyzer
// 
// THaAnalyzer is the base class for a "Hall A analyzer" class.
// An analyzer defines the basic actions to perform during analysis.
// THaAnalyzer is the default analyzer that is used if no user class is
// defined.  It performs a standard analysis consisting of
//
//   1. Decoding/Calibrating
//   2. Track Reconstruction
//   3. Physics variable processing
//
// At the end of each step, testing and histogramming are done for
// the appropriate block defined in the global test/histogram lists.
//
//////////////////////////////////////////////////////////////////////////

#include "TString.h"
#include "THaCutList.h"
#include "THaGlobals.h"

class THaPhysics;
class THaEvent;
class THaRun;
class THaOutput;
class THaNamedList;
class TFile;

class THaAnalyzer {

public:
  THaAnalyzer( );
  virtual ~THaAnalyzer();

  virtual const TString* GetCutBlockNames()  const { return fCutBlockNames; }
  virtual const char*    GetFilename()       const { return fOutFile.Data(); }
  virtual Int_t          GetNblocks()        const { return fNblocks; }
  virtual const TFile*   GetOutFile()        const { return fFile; }
  virtual void           SetCutBlocks( Int_t n, const char* name );
  virtual void           SetCutBlocks( const TString* names );
  virtual void           SetCutBlocks( const char** names );
  virtual void           SetEvent( THaEvent* event )    { fEvent = event; }
  virtual void           SetPhysics( THaPhysics* phys ) { fPhysics = phys; }
  virtual void           SetOutFile( const char* name ) { fOutFile = name; }

  virtual void           Close();
  virtual Int_t          Process( THaRun& run );
  
protected:
  static const char* const kMasterCutName;
  static const Int_t fMaxSkip = 25;

  TFile*         fFile;            //The ROOT output file.
  THaOutput*     fOutput;          //Flexible ROOT output (tree, histograms)
  TString        fOutFile;         //Name of output ROOT file.
  THaPhysics*    fPhysics;         //The physics quantities for this analysis.
  THaEvent*      fEvent;           //The event structure to be written to file.
  Int_t          fNblocks;         //Number of analysis stages
  TString*       fCutBlockNames;   //Array of cut block names
  TString*       fHistBlockNames;  //Array of histogram block names
  TString*       fMasterCutNames;  //Names of the "master cuts" for each cut block
  THaNamedList** fCutBlocks;       //Array of pointers to the blocks of cuts
  UInt_t         fNev;             //Current event number
  Int_t          *fSkipCnt;        //Counters for reasons to skip events.

  Int_t          EvalCuts( Int_t n );
  virtual void   SetupCuts();

  ClassDef(THaAnalyzer,0)  //Hall A Analyzer Standard Event Loop
};


#endif






