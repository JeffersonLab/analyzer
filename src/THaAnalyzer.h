#ifndef ROOT_THaAnalyzer
#define ROOT_THaAnalyzer

//////////////////////////////////////////////////////////////////////////
//
// THaAnalyzer
// 
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
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

class THaAnalyzer : public TObject {

public:
  THaAnalyzer();
  virtual ~THaAnalyzer();

  void           EnableBenchmarks( Bool_t b = kTRUE ) { fDoBench = b; }
  void           EnableRunUpdate( Bool_t b = kTRUE )  { fUpdateRun = b; }
  void           EnableOverwrite( Bool_t b = kTRUE )  { fOverwrite = b; }
  const char*    GetOutFileName()    const   { return fOutFileName.Data(); }
  const char*    GetCutFileName()    const   { return fCutFileName.Data(); }
  const char*    GetOdefFileName()   const   { return fOdefFileName.Data(); }
  const char*    GetSummaryFileName() const  { return fSummaryFileName.Data(); }
  const TFile*   GetOutFile()        const   { return fFile; }
  Int_t          GetCompressionLevel() const { return fCompress; }
  Bool_t         HasStarted()          const { return fAnalysisStarted; }
  void           SetEvent( THaEvent* event )     { fEvent = event; }
  void           SetOutFile( const char* name )  { fOutFileName = name; }
  void           SetCutFile( const char* name )  { fCutFileName = name; }
  void           SetOdefFile( const char* name ) { fOdefFileName = name; }
  void           SetSummaryFile( const char* name ) { fSummaryFileName = name; }
  void           SetCompressionLevel( Int_t level ) { fCompress = level; }
  void           SetMarkInterval( UInt_t interval ) { fMarkInterval = interval; }
  void           SetVerbosity( Int_t level )        { fVerbose = level; }

  virtual void   Close();
  virtual Int_t  Init( THaRun* run );
          Int_t  Init( THaRun& run )    { return Init( &run ); }
  virtual Int_t  Process( THaRun* run=NULL );
          Int_t  Process( THaRun& run ) { return Process(&run); }
  virtual void   Print( Option_t* opt="" ) const;

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
  TString        fSummaryFileName; //Name of test/cut statistics output file
  THaEvent*      fEvent;           //The event structure to be written to file.
  Stage_t*       fStages;          //Cuts/histograms for each analysis stage [kMaxStage]
  Skip_t*        fSkipCnt;         //Counters for reasons to skip events [kMaxSkip]
  UInt_t         fNev;             //Number of events read during most recent replay
  UInt_t         fMarkInterval;    //Interval for printing event numbers
  Int_t          fCompress;        //Compression level for ROOT output file
  Bool_t         fDoBench;         //Collect detailed timing statistics
  THaBenchmark*  fBench;           //Counters for timing statistics
  Int_t          fVerbose;         //Write verbose messages
  Bool_t         fLocalEvent;      //True if fEvent allocated by this object
  Bool_t         fIsInit;          //True if Init() called successfully
  Bool_t         fAnalysisStarted; //True if Process() run and output file still open
  Bool_t         fUpdateRun;       //If true, update run parameters during replay
  Bool_t         fOverwrite;       //If true, overwrite any existing output files
  THaEvent*      fPrevEvent;       //Event structure found during Init()
  THaRun*        fRun;             //Current run

  virtual Int_t  DoInit( THaRun* run );
  virtual bool   EvalStage( EStage n );
  virtual void   InitCuts();
  virtual Int_t  InitModules( const TList* module_list, TDatime& time, 
			      Int_t erroff, const char* baseclass = NULL );
  virtual Int_t  ReadOneEvent( THaRun* run, THaEvData* evdata );
  virtual void   PrintSummary( const THaRun* run ) const;

  static THaAnalyzer* fgAnalyzer;  //Pointer to instance of this class

private:
  THaAnalyzer( const THaAnalyzer& );
  THaAnalyzer& operator=( const THaAnalyzer& );
  
  ClassDef(THaAnalyzer,0)  //Hall A Analyzer Standard Event Loop

};


#endif






