#ifndef Podd_THaAnalyzer_h_
#define Podd_THaAnalyzer_h_

//////////////////////////////////////////////////////////////////////////
//
// THaAnalyzer
//
//////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TString.h"
#include <vector>

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
class THaEvtTypeHandler;
class THaEpicsEvtHandler;
class THaApparatus;
class THaSpectrometer;
class THaPhysicsModule;
class THaAnalysisObject;
namespace Podd {
  class InterStageModule;
}

class THaAnalyzer : public TObject {

public:
  THaAnalyzer();
  virtual ~THaAnalyzer();

  virtual Int_t  AddInterStage( Podd::InterStageModule* module );
  virtual Int_t  AddPostProcess( THaPostProcess* module );
  virtual void   Close();
  virtual Int_t  Init( THaRunBase* run );
          Int_t  Init( THaRunBase& run )    { return Init( &run ); }
  virtual Int_t  Process( THaRunBase* run=nullptr );
          Int_t  Process( THaRunBase& run ) { return Process(&run); }
  virtual void   Print( Option_t* opt="" ) const;

  void           EnableBenchmarks( Bool_t b = true );
  void           EnableHelicity( Bool_t b = true );
  void           EnableOtherEvents( Bool_t b = true );
  void           EnableOverwrite( Bool_t b = true );
  void           EnablePhysicsEvents( Bool_t b = true );
  void           EnableRunUpdate( Bool_t b = true );
  void           EnableSlowControl( Bool_t b = true );
  const char*    GetOutFileName()      const  { return fOutFileName.Data(); }
  const char*    GetCutFileName()      const  { return fCutFileName.Data(); }
  const char*    GetOdefFileName()     const  { return fOdefFileName.Data(); }
  const char*    GetSummaryFileName()  const  { return fSummaryFileName.Data(); }
  TFile*         GetOutFile()          const  { return fFile; }
  Int_t          GetCompressionLevel() const  { return fCompress; }
  THaEvent*      GetEvent()            const  { return fEvent; }
  THaEvData*     GetDecoder()          const;
  const std::vector<THaApparatus*>&
                 GetApps()             const  { return fApps; }
  const std::vector<THaPhysicsModule*>&
                 GetPhysics()          const  { return fPhysics; }
  THaEpicsEvtHandler*
                 GetEpicsEvtHandler()  const  { return fEpicsHandler; }
  const std::vector<THaEvtTypeHandler*>&
                 GetEvtHandlers()      const  { return fEvtHandlers; }
  const std::vector<THaPostProcess*>&
                 GetPostProcess()      const  { return fPostProcess; }
  Bool_t         HasStarted()          const  { return fAnalysisStarted; }
  Bool_t         HelicityEnabled()     const  { return fDoHelicity; }
  Bool_t         PhysicsEnabled()      const  { return fDoPhysics; }
  Bool_t         OtherEventsEnabled()  const  { return fDoOtherEvents; }
  Bool_t         SlowControlEnabled()  const  { return fDoSlowControl; }
  virtual Int_t  SetCountMode( Int_t mode );
  void           SetCrateMapFileName( const char* name );
  void           SetEvent( THaEvent* event )        { fEvent = event; }
  void           SetOutFile( const char* name )     { fOutFileName = name; }
  void           SetCutFile( const char* name )     { fCutFileName = name; }
  void           SetOdefFile( const char* name )    { fOdefFileName = name; }
  void           SetSummaryFile( const char* name ) { fSummaryFileName = name; }
  void           SetCompressionLevel( Int_t level ) { fCompress = level; }
  void           SetMarkInterval( UInt_t interval ) { fMarkInterval = interval; }
  void           SetVerbosity( Int_t level )        { fVerbose = level; }
  void           SetCodaVersion(Int_t vers);

  // Set the EPICS event type
  void           SetEpicsEvtType(Int_t itype);
  void           AddEpicsEvtType(Int_t itype);

  static THaAnalyzer* GetInstance() { return fgAnalyzer; }

  // Return codes for analysis routines inside event loop
  // These should be ordered by severity
  enum ERetVal { kOK, kSkip, kTerminate, kFatal };

  enum {
    kRawDecode = 0, kDecode, kCoarseTrack, kCoarseRecon,
    kTracking, kReconstruct, kPhysics
  };

  // For SetCountMode
  enum ECountMode { kCountPhysics, kCountAll, kCountRaw };

protected:
  // Test and histogram blocks
  class Stage_t {
  public:
    Stage_t( Int_t _key, Int_t _countkey, const char* _name )
      : key(_key), countkey(_countkey), name(_name), cut_list(nullptr),
        hist_list(nullptr), master_cut(nullptr) {}
    Int_t         key;
    Int_t         countkey;
    const char*   name;
    TList*        cut_list;
    TList*        hist_list;
    THaCut*       master_cut;
  };
  // Statistics counters and message texts
  enum {
    kNevRead = 0, kNevGood, kNevPhysics, kNevEpics, kNevOther,
    kNevPostProcess, kNevAnalyzed, kNevAccepted,
    kDecodeErr, kCodaErr, kRawDecodeTest, kDecodeTest, kCoarseTrackTest,
    kCoarseReconTest, kTrackTest, kReconstructTest, kPhysicsTest
  };
  class Counter_t {
  public:
    Counter_t( Int_t _key, const char* _description )
      : key(_key), count(0), description(_description) {}
    Int_t       key;
    UInt_t      count;
    const char* description;
  };

  TFile*         fFile;            //The ROOT output file.
  THaOutput*     fOutput;          //Flexible ROOT output (tree, histograms)
  THaEpicsEvtHandler* fEpicsHandler; // EPICS event handler used by THaOutput
  TString        fOutFileName;     //Name of output ROOT file.
  TString        fCutFileName;     //Name of cut definition file to load
  TString        fLoadedCutFileName;//Name of last loaded cut definition file
  TString        fOdefFileName;    //Name of output definition file
  TString        fSummaryFileName; //Name of test/cut statistics output file
  THaEvent*      fEvent;           //The event structure to be written to file.
  Int_t          fWantCodaVers;    //Version of CODA assumed for file
  std::vector<Stage_t>   fStages;  //Parameters for analysis stages
  std::vector<Counter_t> fCounters;//Statistics counters
  UInt_t         fNev;             //Number of events read during most recent replay //FIXME: make ULong64_t
  UInt_t         fMarkInterval;    //Interval for printing event numbers
  Int_t          fCompress;        //Compression level for ROOT output file
  Int_t          fVerbose;         //Verbosity level
  Int_t          fCountMode;       //Event counting mode (see ECountMode)
  THaBenchmark*  fBench;           //Counters for timing statistics
  THaEvent*      fPrevEvent;       //Event structure from last Init()
  THaRunBase*    fRun;             //Pointer to current run
  THaEvData*     fEvData;          //Instance of decoder used by us

  // Lists of processing modules defined for current analysis
  std::vector<THaApparatus*>           fApps;            // Apparatuses
  std::vector<THaSpectrometer*>        fSpectrometers;   // Spectrometers
  std::vector<THaPhysicsModule*>       fPhysics;         // Physics modules
  // Modules in the following lists are owned by this THaAnalyzer
  std::vector<Podd::InterStageModule*> fInterStage;      // Inter-stage modules
  std::vector<THaEvtTypeHandler*>      fEvtHandlers;     // Event type handlers
  std::vector<THaPostProcess*>         fPostProcess;     // Post-processing mods
  // Combined list of fApps, fInterStage and fPhysics for PhysicsAnalysis.
  // Does not include fPostProcess and fEvtHandlers.
  std::vector<THaAnalysisObject*>      fAnalysisModules; // Analysis modules

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
  Bool_t         fDoSlowControl;   // Enable slow control processing

  // Variables used by analysis functions
  Bool_t         fFirstPhysics;    // Status flag for physics analysis

  // Main analysis functions
  virtual Int_t  BeginAnalysis();
  virtual Int_t  DoInit( THaRunBase* run );
  virtual Int_t  EndAnalysis();
  virtual Int_t  MainAnalysis();
  virtual Int_t  PhysicsAnalysis( Int_t code );
  virtual Int_t  SlowControlAnalysis( Int_t code );
  virtual Int_t  OtherAnalysis( Int_t code );
  virtual Int_t  PostProcess( Int_t code );
  virtual Int_t  ReadOneEvent();

  // Support methods & data
  void           ClearCounters();
  UInt_t         GetCount( Int_t which ) const;
  UInt_t         Incr( Int_t which );
  virtual bool   EvalStage( int n );
  virtual void   InitCounters();
  virtual void   InitCuts();
  virtual void   InitStages();
  virtual Int_t  InitModules( const std::vector<THaAnalysisObject*>& module_list,
                              TDatime& run_time );
  virtual Int_t  InitOutput( const std::vector<THaAnalysisObject*>& module_list );

  enum class EExitStatus { kUnknown = -1, kEOF, kEvLimit, kFatal, kTerminated };
  virtual void   PrepareModuleList();
  virtual void   PrintCounters() const;
  virtual void   PrintExitStatus( EExitStatus status ) const;
  virtual void   PrintRunSummary() const;
  virtual void   PrintCutSummary() const;
  virtual void   PrintTimingSummary() const;
  virtual void   PrintSummary( EExitStatus exit_status ) const;

  static THaAnalyzer* fgAnalyzer;  //Pointer to instance of this class

  TObject*        fExtra;   // Additional member data (for binary compat.)

  // In-class constants
  static const char* const kMasterCutName;
  static const char* const kDefaultOdefFile;

private:
  THaAnalyzer( const THaAnalyzer& );
  THaAnalyzer& operator=( const THaAnalyzer& );

  ClassDef(THaAnalyzer,0)  //Hall A Analyzer Standard Event Loop

};

//---------------- inlines ----------------------------------------------------
inline UInt_t THaAnalyzer::GetCount( Int_t which ) const
{
  return fCounters[which].count;
}

//_____________________________________________________________________________
inline UInt_t THaAnalyzer::Incr( Int_t which )
{
  return ++(fCounters[which].count);
}

#endif
