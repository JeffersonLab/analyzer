#ifndef ROOT_THaAnalyzer
#define ROOT_THaAnalyzer

//////////////////////////////////////////////////////////////////////////
//
// THaAnalyzer
// 
//////////////////////////////////////////////////////////////////////////

#include "TString.h"

class THaEvent;
class THaRun;
class THaOutput;
class TList;
class TIter;
class TFile;
class TDatime;
class THaCut;
class THaBenchmark;
class THaEvData;

class THaAnalyzer {

public:
  THaAnalyzer();
  virtual ~THaAnalyzer();

  const char*    GetOutFileName()    const   { return fOutFileName.Data(); }
  const char*    GetCutFileName()    const   { return fCutFileName.Data(); }
  const char*    GetOdefFileName()   const   { return fOdefFileName.Data(); }
  const char*    GetSummaryFileName() const  { return fSummaryFileName.Data(); }
  const TFile*   GetOutFile()        const   { return fFile; }
  Int_t          GetCompressionLevel() const { return fCompress; }
  void           SetEvent( THaEvent* event )     { fEvent = event; }
  void           SetOutFile( const char* name )  { fOutFileName = name; }
  void           SetCutFile( const char* name )  { fCutFileName = name; }
  void           SetOdefFile( const char* name ) { fOdefFileName = name; }
  void           SetSummaryFile( const char* name ) { fSummaryFileName = name; }
  void           SetCompressionLevel( Int_t level ) { fCompress = level; }
  void           SetMarkInterval( UInt_t interval ) { fMarkInterval = interval; }
  void           SetVerbosity( Int_t level )        { fVerbose = level; }
  void           EnableBenchmarks( Bool_t b = kTRUE ) { fDoBench = b; }

  virtual void   Close();
  virtual Int_t  Init( THaRun& run );
  virtual Int_t  Process( THaRun& run );
          Int_t  Process( THaRun* run ) { 
	    if(!run) return -1; return Process(*run);
	  }
  
protected:
  static const char* const kMasterCutName;

  enum EStage      { kRawDecode, kDecode, kCoarseRecon, kReconstruct, kPhysics,
		     kMaxStage };
  enum ESkipReason { kEvFileTrunc, kCodaErr, kRawDecodeTest, kDecodeTest, 
		     kCoarseReconTest, kReconstructTest, kPhysicsTest, 
		     kMaxSkip };
  // Statistics counters and message texts
  struct Skip_t {
    const char* reason;
    Int_t       count;
  };
  // Test and histogram blocks
  struct Stage_t;
  friend struct Stage_t;
  struct Stage_t {
    const char*   name;
    ESkipReason   skipkey;
    TList*        cut_list;
    TList*        hist_list;
    THaCut*       master_cut;
  };
    
  TFile*         fFile;            //The ROOT output file.
  THaOutput*     fOutput;          //Flexible ROOT output (tree, histograms)
  TString        fOutFileName;     //Name of output ROOT file.
  TString        fCutFileName;     //Name of cut definition file to load
  TString        fLoadedCutFileName;//Name of last loaded cut definition file
  TString        fOdefFileName;    //Name of output definition file
  TString        fSummaryFileName; //Name of file to write analysis summary to
  THaEvent*      fEvent;           //The event structure to be written to file.
  Stage_t*       fStages;          //Cuts/histograms for each analysis stage [kMaxStage]
  Skip_t*        fSkipCnt;         //Counters for reasons to skip events [kMaxSkip]
  UInt_t         fNev;             //Current event number
  UInt_t         fMarkInterval;    //Interval for printing event numbers
  Int_t          fCompress;        //Compression level for ROOT output file
  Bool_t         fDoBench;         //Collect detailed timing statistics
  THaBenchmark*  fBench;           //Counters for timing statistics
  Int_t          fVerbose;         //Write verbose messages
  Bool_t         fLocalEvent;      //True if fEvent allocated by this object
  Bool_t         fIsInit;          //True if Init() called successfully
  Bool_t         fAnalysisStarted; //True if Process() run and output file still open
  THaEvent*      fPrevEvent;       //Event structure found during Init()
  THaRun*        fRun;             //Current run

  virtual Int_t  DoInit( THaRun& run );
  virtual bool   EvalStage( EStage n );
  virtual void   InitCuts();
  virtual Int_t  InitModules( TIter& iter, TDatime& time, Int_t erroff,
			      const char* baseclass = NULL );
  virtual Int_t  ReadOneEvent( THaRun& run, THaEvData& evdata );

  ClassDef(THaAnalyzer,0)  //Hall A Analyzer Standard Event Loop
};


#endif






