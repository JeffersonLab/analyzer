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
class THaRunBase;
class THaOutput;
class TList;
class TIter;
class TFile;
class TDatime;
class THaCut;
class THaBenchmark;
class THaEvData;
class THaPostProcess;
class THaCrateMap;

class THaAnalyzer : public TObject {

public:
  THaAnalyzer();
  virtual ~THaAnalyzer();

  virtual Int_t  AddPostProcess( THaPostProcess* module );
  virtual void   Close();
  virtual Int_t  Init( THaRunBase* run );
          Int_t  Init( THaRunBase& run )    { return Init( &run ); }
  virtual Int_t  Process( THaRunBase* run=NULL );
          Int_t  Process( THaRunBase& run ) { return Process(&run); }
  virtual void   Print( Option_t* opt="" ) const;

  void           EnableBenchmarks( Bool_t b = kTRUE );
  void           EnableHelicity( Bool_t b = kTRUE );
  void           EnableOtherEvents( Bool_t b = kTRUE );
  void           EnableOverwrite( Bool_t b = kTRUE );
  void           EnablePhysicsEvents( Bool_t b = kTRUE );
  void           EnableRunUpdate( Bool_t b = kTRUE );
  void           EnableScalers( Bool_t b = kTRUE );
  void           EnableSlowControl( Bool_t b = kTRUE );
  const char*    GetOutFileName()      const  { return fOutFileName.Data(); }
  const char*    GetCutFileName()      const  { return fCutFileName.Data(); }
  const char*    GetOdefFileName()     const  { return fOdefFileName.Data(); }
  const char*    GetSummaryFileName()  const  { return fSummaryFileName.Data(); }
  TFile*         GetOutFile()          const  { return fFile; }
  Int_t          GetCompressionLevel() const  { return fCompress; }
  THaEvent*      GetEvent()            const  { return fEvent; }
  THaEvData*     GetDecoder()          const  { return fEvData; }
  TList*         GetApps()             const  { return fApps; }
  TList*         GetPhysics()          const  { return fPhysics; }
  TList*         GetScalers()          const  { return fScalers; }
  TList*         GetPostProcess()      const  { return fPostProcess; }
  Bool_t         HasStarted()          const  { return fAnalysisStarted; }
  Bool_t         HelicityEnabled()     const  { return fDoHelicity; }
  Bool_t         PhysicsEnabled()      const  { return fDoPhysics; }
  Bool_t         OtherEventsEnabled()  const  { return fDoOtherEvents; }
  Bool_t         ScalersEnabled()      const  { return fDoScalers; }
  Bool_t         SlowControlEnabled()  const  { return fDoSlowControl; }
  virtual Int_t  SetCountMode( Int_t mode );
  void           SetCrateMapFileName( const char* name );
  void           SetEvent( THaEvent* event )     { fEvent = event; }
  void           SetOutFile( const char* name )  { fOutFileName = name; }
  void           SetCutFile( const char* name )  { fCutFileName = name; }
  void           SetOdefFile( const char* name ) { fOdefFileName = name; }
  void           SetSummaryFile( const char* name ) { fSummaryFileName = name; }
  void           SetCompressionLevel( Int_t level ) { fCompress = level; }
  void           SetMarkInterval( UInt_t interval ) { fMarkInterval = interval; }
  void           SetVerbosity( Int_t level )        { fVerbose = level; }

  static THaAnalyzer* GetInstance() { return fgAnalyzer; }

  // Return codes for analysis routines inside event loop
  enum ERetVal { kOK, kSkip, kTerminate, kFatal };

protected:
  // Test and histogram blocks
  enum {
    kRawDecode = 0, kDecode, kCoarseTrack, kCoarseRecon,
    kTracking, kReconstruct, kPhysics
  };
  struct Stage_t {
    Int_t         key;
    Int_t         countkey;
    const char*   name;
    TList*        cut_list;
    TList*        hist_list;
    THaCut*       master_cut;
  };
  // Statistics counters and message texts
  enum {
    kNevRead = 0, kNevGood, kNevPhysics, kNevScaler, kNevEpics, kNevOther,
    kNevPostProcess, kNevAnalyzed, kNevAccepted,
    kDecodeErr, kCodaErr, kRawDecodeTest, kDecodeTest, kCoarseTrackTest,
    kCoarseReconTest, kTrackTest, kReconstructTest, kPhysicsTest
  };
  struct Counter_t {
    Int_t       key;
    const char* description;
    UInt_t      count;
  };

  enum ECountMode { kCountPhysics, kCountAll, kCountRaw };

  TFile*         fFile;            //The ROOT output file.
  THaOutput*     fOutput;          //Flexible ROOT output (tree, histograms)
  TString        fOutFileName;     //Name of output ROOT file.
  TString        fCutFileName;     //Name of cut definition file to load
  TString        fLoadedCutFileName;//Name of last loaded cut definition file
  TString        fOdefFileName;    //Name of output definition file
  TString        fSummaryFileName; //Name of test/cut statistics output file
  THaEvent*      fEvent;           //The event structure to be written to file.
  Int_t          fNStages;         //Number of analysis stages
  Int_t          fNCounters;       //Number of counters
  Stage_t*       fStages;          //[fNStages] Parameters for analysis stages
  Counter_t*     fCounters;        //[fNCounters] Statistics counters
  UInt_t         fNev;             //Number of events read during most recent replay
  UInt_t         fMarkInterval;    //Interval for printing event numbers
  Int_t          fCompress;        //Compression level for ROOT output file
  Int_t          fVerbose;         //Verbosity level
  Int_t          fCountMode;       //Event counting mode (see ECountMode)
  THaBenchmark*  fBench;           //Counters for timing statistics
  THaEvent*      fPrevEvent;       //Event structure from last Init()
  THaRunBase*    fRun;             //Pointer to current run
  THaEvData*     fEvData;          //Instance of decoder used by us
  TList*         fApps;            //List of apparatuses
  TList*         fPhysics;         //List of physics modules
  TList*         fScalers;         //List of scaler groups
  TList*         fPostProcess;     //List of post-processing modules
  TList*         fEvtHandlers;     //List of Event Type Handlers

  // Status and control flags
  Bool_t         fIsInit;          // Init() called successfully
  Bool_t         fAnalysisStarted; // Process() run and output file open
  Bool_t         fLocalEvent;      // fEvent allocated by this object
  Bool_t         fUpdateRun;       // Update run parameters during replay
  Bool_t         fOverwrite;       // Overwrite existing output files
  Bool_t         fDoBench;         // Collect detailed timing statistics
  Bool_t         fDoHelicity;      // Enable helicity decoding
  Bool_t         fDoPhysics;       // Enable physics event processing
  Bool_t         fDoOtherEvents;   // Enable other event processing
  Bool_t         fDoScalers;       // Enable scaler processing
  Bool_t         fDoSlowControl;   // Enable slow control processing

  // Variables used by analysis functions
  Bool_t         fFirstPhysics;    // Status flag for physics analysis

  // Main analysis functions
  virtual Int_t  BeginAnalysis();
  virtual Int_t  DoInit( THaRunBase* run );
  virtual Int_t  EndAnalysis();
  virtual Int_t  MainAnalysis();
  virtual Int_t  PhysicsAnalysis( Int_t code );
  virtual Int_t  ScalerAnalysis( Int_t code );
  virtual Int_t  SlowControlAnalysis( Int_t code );
  virtual Int_t  OtherAnalysis( Int_t code );
  virtual Int_t  PostProcess( Int_t code );
  virtual Int_t  ReadOneEvent();

  // Support methods
  void           ClearCounters();
  Stage_t*       DefineStage( const Stage_t* stage );
  Counter_t*     DefineCounter( const Counter_t* counter );
  UInt_t         GetCount( Int_t which ) const;
  UInt_t         Incr( Int_t which );
  virtual bool   EvalStage( int n );
  virtual void   InitCounters();
  virtual void   InitCuts();
  virtual void   InitStages();
  virtual Int_t  InitModules( TList* module_list, TDatime& time,
			      Int_t erroff, const char* baseclass = NULL );
  virtual Int_t  InitOutput( const TList* module_list, Int_t erroff,
			     const char* baseclass = NULL );
  virtual void   PrintCounters() const;
  virtual void   PrintScalers() const;
  virtual void   PrintCutSummary() const;

  static THaAnalyzer* fgAnalyzer;  //Pointer to instance of this class

  // In-class constants
  static const char* const kMasterCutName;
  static const char* const kDefaultOdefFile;

private:
  THaAnalyzer( const THaAnalyzer& );
  THaAnalyzer& operator=( const THaAnalyzer& );

  ClassDef(THaAnalyzer,0)  //Hall A Analyzer Standard Event Loop

};

//---------------- inlines ----------------------------------------------------
inline UInt_t THaAnalyzer::GetCount( Int_t i ) const
{
  return fCounters[i].count;
}

//_____________________________________________________________________________
inline UInt_t THaAnalyzer::Incr( Int_t i )
{
  return ++(fCounters[i].count);
}

#endif
