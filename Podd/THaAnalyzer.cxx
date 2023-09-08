//*-- Author :    Ole Hansen   12-May-2000

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

#include "THaAnalyzer.h"
#include "THaRunBase.h"
#include "THaEvent.h"
#include "THaOutput.h"
#include "THaEvData.h"
#include "THaGlobals.h"
#include "THaSpectrometer.h"
#include "THaCutList.h"
#include "THaPhysicsModule.h"
#include "InterStageModule.h"
#include "THaPostProcess.h"
#include "THaBenchmark.h"
#include "THaEvtTypeHandler.h"
#include "THaEpicsEvtHandler.h"
#include "TList.h"
#include "TTree.h"
#include "TFile.h"
#include "TDatime.h"
#include "TError.h"
#include "TClass.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TDirectory.h"
#include "THaCrateMap.h"
#include "Helper.h"

#include <iostream>
#include <iomanip>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <vector>

using namespace std;
using namespace Decoder;
using namespace Podd;

const char* const THaAnalyzer::kMasterCutName = "master";
const char* const THaAnalyzer::kDefaultOdefFile = "output.def";

// Pointer to single instance of this object
THaAnalyzer* THaAnalyzer::fgAnalyzer = nullptr;

//FIXME:
// do we need to "close" scalers/EPICS analysis if we reach the event limit?

//_____________________________________________________________________________
// Convert from type-unsafe ROOT containers to STL vectors
template<typename T>
inline size_t ListToVector( TList* lst, vector<T*>& vec )
{
  if( lst ) {
    vec.reserve(std::max(lst->GetSize(), 0));
    TIter next_item(lst);
    while( TObject* obj = next_item() ) {
      if( obj->IsZombie() )
        // Skip objects whose constructor failed
        continue;
      auto* item = dynamic_cast<T*>(obj);
      if( item ) {
        // Skip duplicates
        auto it = std::find(ALL(vec), item);
        if( it == vec.end() )
          vec.push_back(item);
      }
    }
  }
  return vec.size();
}

//_____________________________________________________________________________
THaAnalyzer::THaAnalyzer()
  : fFile(nullptr)
  , fOutput(nullptr)
  , fEpicsHandler(nullptr)
  , fOdefFileName(kDefaultOdefFile)
  , fEvent(nullptr)
  , fWantCodaVers(-1)
  , fNev(0)
  , fMarkInterval(1000)
  , fCompress(1)
  , fVerbose(2)
  , fCountMode(kCountRaw)
  , fBench(nullptr)
  , fPrevEvent(nullptr)
  , fRun(nullptr)
  , fEvData(nullptr)
  , fIsInit(false)
  , fAnalysisStarted(false)
  , fLocalEvent(false)
  , fUpdateRun(true)
  , fOverwrite(true)
  , fDoBench(false)
  , fDoHelicity(false)
  , fDoPhysics(true)
  , fDoOtherEvents(true)
  , fDoSlowControl(true)
  , fFirstPhysics(true)
  , fExtra(nullptr)
{
  // Default constructor.

  //FIXME: relax this
  // Allow only one analyzer object (because we use various global lists)
  if( fgAnalyzer ) {
    Error("THaAnalyzer", "only one instance of THaAnalyzer allowed.");
    MakeZombie();
    return;
  }
  fgAnalyzer = this;

  // EPICS data
  fEpicsHandler = new THaEpicsEvtHandler("epics","EPICS event type");
  //  fEpicsHandler->SetDebugFile("epicsdat.txt");
  fEvtHandlers.push_back(fEpicsHandler);

  // Timers
  fBench = new THaBenchmark;
}

//_____________________________________________________________________________
THaAnalyzer::~THaAnalyzer()
{
  // Destructor.

  THaAnalyzer::Close();
  DeleteContainer(fPostProcess);
  DeleteContainer(fEvtHandlers);
  DeleteContainer(fInterStage);
  delete fExtra; fExtra = nullptr;
  delete fBench;
  if( fgAnalyzer == this )
    fgAnalyzer = nullptr;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::AddInterStage( Podd::InterStageModule* module )
{
  // Add 'module' to the list of inter-stage modules. See AddPostProcess()
  // for additional comments

  //FIXME: code duplication
  const char* const here = "AddInterStage";

  // No module, nothing to do
  if( !module )
    return 0;

  // Can't add modules in the middle of things
  if( fAnalysisStarted ) {
    Error( Here(here), "Cannot add analysis modules while analysis "
                             "is in progress. Close() this analysis first." );
    return 237;
  }

  // Init this module if Analyzer already initialized. Otherwise, the
  // module will be initialized in Init() later.
  if( fIsInit ) {
    // FIXME: debug
    if( !fRun || !fRun->IsInit()) {
      Error(Here(here),"fIsInit, but bad fRun?!?");
      return 236;
    }
    TDatime run_time = fRun->GetDate();
    Int_t retval = module->Init(run_time);
    if( retval )
      return retval;
  }

  fInterStage.push_back(module);
  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::AddPostProcess( THaPostProcess* module )
{
  // Add 'module' to the list of post-processing modules. This can only
  // be done if no analysis is in progress. If the Analyzer has been
  // initialized, the module will be initialized immediately for the current
  // run time.

  // No module, nothing to do
  if( !module )
    return 0;

  // Can't add modules in the middle of things
  if( fAnalysisStarted ) {
    Error( "AddPostProcess", "Cannot add analysis modules while analysis "
	   "is in progress. Close() this analysis first." );
    return 237;
  }

  // Init this module if Analyzer already initialized. Otherwise, the
  // module will be initialized in Init() later.
  if( fIsInit ) {
    // FIXME: debug
    if( !fRun || !fRun->IsInit()) {
      Error("AddPostProcess","fIsInit, but bad fRun?!?");
      return 236;
    }
    TDatime run_time = fRun->GetDate();
    Int_t retval = module->Init(run_time);
    if( retval )
      return retval;
  }

  fPostProcess.push_back(module);
  return 0;
}

//_____________________________________________________________________________
void THaAnalyzer::ClearCounters()
{
  // Clear statistics counters
  for( auto& theCounter : fCounters ) {
    theCounter.count = 0;
  }
}

//_____________________________________________________________________________
void THaAnalyzer::Close()
{
  // Close output files and delete fOutput, fFile, and fRun objects.
  // Also delete fEvent if it was allocated automatically by us.

  // Close all Post-process objects, but do not delete them
  // (destructor does that)
  for( auto* postProc : fPostProcess)
    postProc->Close();

  // After Close(), will need to Init() again, where these lists will be filled
  fApps.clear();
  fSpectrometers.clear();
  fPhysics.clear();
  fEvtHandlers.clear();

  if( gHaRun && *gHaRun == *fRun )
    gHaRun = nullptr;

  delete fEvData; fEvData = nullptr;
  delete fOutput; fOutput = nullptr;
  if( TROOT::Initialized() )
    delete fFile;
  fFile = nullptr;
  delete fRun; fRun = nullptr;
  if( fLocalEvent ) {
    delete fEvent; fEvent = fPrevEvent = nullptr;
  }
  fIsInit = fAnalysisStarted = false;
}


//_____________________________________________________________________________
void THaAnalyzer::EnableBenchmarks( Bool_t b )
{
  fDoBench = b;
}

//_____________________________________________________________________________
void THaAnalyzer::EnableHelicity( Bool_t b )
{
  fDoHelicity = b;
}

//_____________________________________________________________________________
void THaAnalyzer::EnableRunUpdate( Bool_t b )
{
  fUpdateRun = b;
}

//_____________________________________________________________________________
void THaAnalyzer::EnableOtherEvents( Bool_t b )
{
  fDoOtherEvents = b;
}

//_____________________________________________________________________________
void THaAnalyzer::EnableOverwrite( Bool_t b )
{
  fOverwrite = b;
}

//_____________________________________________________________________________
void THaAnalyzer::EnablePhysicsEvents( Bool_t b )
{
  fDoPhysics = b;
}

//_____________________________________________________________________________
void THaAnalyzer::EnableSlowControl( Bool_t b )
{
  fDoSlowControl = b;
}

//_____________________________________________________________________________
bool THaAnalyzer::EvalStage( int n )
{
  // Fill histogram block for analysis stage 'n', then evaluate cut block.
  // Return 'false' if master cut is not true, 'true' otherwise.
  // The result can be used to skip further processing of the current event.
  // If event is skipped, increment associated statistics counter.
  // Call InitCuts() before using!  This is an internal function.

  if( fDoBench ) fBench->Begin("Cuts");

  const Stage_t& theStage = fStages[n];

  //FIXME: support stage-wise blocks of histograms
  //  if( theStage.hist_list ) {
    // Fill histograms
  //  }

  bool ret = true;
  if( theStage.cut_list ) {
    THaCutList::EvalBlock( theStage.cut_list );
    if( theStage.master_cut &&
	!theStage.master_cut->GetResult() ) {
      if( theStage.countkey >= 0 ) // stage may not have a counter
	Incr(theStage.countkey);
      ret = false;
    }
  }
  if( fDoBench ) fBench->Stop("Cuts");
  return ret;
}

//_____________________________________________________________________________
THaEvData* THaAnalyzer::GetDecoder() const
{
  // Return the decoder object that this analyzer uses to process the input

  if( !fEvData ) {
    Warning( "GetDecoder", "Decoder not yet set up. You need to "
             "initialize the analyzer with Init(run) first." );
  }
  return fEvData;
}

//_____________________________________________________________________________
void THaAnalyzer::InitStages()
{
  // Define analysis stages. Called from Init(). Does nothing if called again.
  // Derived classes can override this method to define their own
  // stages or additional stages. This should be done with caution.

  if( !fStages.empty() )  return;
  fStages.reserve(kPhysics - kRawDecode + 1);
  fStages = {
    {kRawDecode,   kRawDecodeTest,   "RawDecode"},
    {kDecode,      kDecodeTest,      "Decode"},
    {kCoarseTrack, kCoarseTrackTest, "CoarseTracking"},
    {kCoarseRecon, kCoarseReconTest, "CoarseReconstruct"},
    {kTracking,    kTrackTest,       "Tracking"},
    {kReconstruct, kReconstructTest, "Reconstruct"},
    {kPhysics,     kPhysicsTest,     "Physics"}
  };
}

//_____________________________________________________________________________
void THaAnalyzer::InitCounters() {
  // Allocate statistics counters.
  // See notes for InitStages() for additional information.

  if( !fCounters.empty() ) return;
  fCounters.reserve(kPhysicsTest - kNevRead + 1);
  fCounters = {
    {kNevRead,         "events read"},
    {kNevGood,         "events decoded"},
    {kNevPhysics,      "physics events"},
    {kNevEpics,        "slow control events"},
    {kNevOther,        "other event types"},
    {kNevPostProcess,  "events post-processed"},
    {kNevAnalyzed,     "physics events analyzed"},
    {kNevAccepted,     "events accepted"},
    {kDecodeErr,       "decoding error"},
    {kCodaErr,         "CODA errors"},
    {kRawDecodeTest,   "skipped after raw decoding"},
    {kDecodeTest,      "skipped after Decode"},
    {kCoarseTrackTest, "skipped after Coarse Tracking"},
    {kCoarseReconTest, "skipped after Coarse Reconstruct"},
    {kTrackTest,       "skipped after Tracking"},
    {kReconstructTest, "skipped after Reconstruct"},
    {kPhysicsTest,     "skipped after Physics"}
  };
}

//_____________________________________________________________________________
void THaAnalyzer::InitCuts()
{
  // Internal function that sets up structures to handle cuts more efficiently:
  //
  // - Find pointers to the THaNamedList lists that hold the cut blocks.
  // - find pointer to each block's master cut

  for( auto& theStage : fStages ) {
    // If block not found, this will return nullptr and work just fine later.
    theStage.cut_list = gHaCuts->FindBlock( theStage.name );

    if( theStage.cut_list ) {
      TString master_cut( theStage.name );
      master_cut.Append( '_' );
      master_cut.Append( kMasterCutName );
      theStage.master_cut = gHaCuts->FindCut( master_cut );
    } else
      theStage.master_cut = nullptr;
  }
}

//_____________________________________________________________________________
Int_t THaAnalyzer::InitModules(
  const std::vector<THaAnalysisObject*>& module_list, TDatime& run_time )
{
  // Initialize a list of THaAnalysisObjects for time 'run_time'.
  // If 'baseclass' given, ensure that each object in the list inherits
  // from 'baseclass'.

  static const char* const here = "InitModules()";

  Int_t retval = 0;
  for( auto it = module_list.begin(); it != module_list.end(); ) {
    auto* theModule = *it;
    if( fVerbose > 1 )
      cout << "Initializing " << theModule->GetName() << endl;
    try {
      retval = theModule->Init( run_time );
    }
    catch( const exception& e ) {
      Error(here, "Exception %s caught during initialization of module "
	     "%s (%s). Analyzer initialization failed.",
            e.what(), theModule->GetName(), theModule->GetTitle() );
      retval = -1;
      goto errexit;
    }
    if( retval != kOK || !theModule->IsOK() ) {
      Error( here, "Error %d initializing module %s (%s). "
             "Analyzer initialization failed.",
            retval, theModule->GetName(), theModule->GetTitle() );
      if( retval == kOK )
	retval = -1;
      break;
    }
    ++it;
  }
 errexit:
  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::Init( THaRunBase* run )
{
  // Initialize the analyzer.

  // This is a wrapper, so we can conveniently control the benchmark counter
  if( !run ) return -1;

  if( !fIsInit ) fBench->Reset();
  fBench->Begin("Total");

  if( fDoBench ) fBench->Begin("Init");
  Int_t retval = DoInit( run );
  if( fDoBench ) fBench->Stop("Init");

  // Stop "Total" counter since Init() may be called separately from Process()
  fBench->Stop("Total");
  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::DoInit( THaRunBase* run )
{
  // Internal function called by Init(). This is where the actual work is done.

  //FIXME:should define return codes in header
  static const char* const here = "Init";
  Int_t retval = 0;

  //--- Initialize the run object, if necessary
  if( !run ) {
    Error(here, "run is null");
    return -243;
  }
  // If we've been told to use a specific CODA version, tell the run
  // object about it. (FIXME: This may not work with non-CODA runs.)
  if( fWantCodaVers > 0 && run->SetDataVersion(fWantCodaVers) < 0 ) {
    Error( here, "Failed to set CODA version %d for run. Call expert.",
           fWantCodaVers );
    return -242;
  }
  // Make sure the run is initialized.
  bool run_init = false;
  if( !run->IsInit()) {
    cout << "Initializing run object" << endl;
    run_init = true;
    retval = run->Init();
    if( retval )
      return retval;  //Error message printed by run class
  }

  //--- Open the output file if necessary so that Trees and Histograms
  //    are created on disk.

  // File previously created and name changed?
  if( fFile && fOutFileName != fFile->GetName()) {
    // But -- we are analyzing split runs (so multiple initializations)
    // and the tree's file has split too (so fFile has changed automatically)
#if 0
    if( fAnalysisStarted ) {
      Error( here, "Cannot change output file name after analysis has been "
	     "started. Close() first, then Init() or Process() again." );
      return -11;
    }
    Close();
#endif
  }
  if( !fFile ) {
    if( fOutFileName.IsNull() ) {
      Error( here, "Must specify an output file. Set it with SetOutFile()." );
      return -12;
    }
    // File exists?
    if( !gSystem->AccessPathName(fOutFileName) ) { //sic
      if( !fOverwrite ) {
	Error( here, "Output file %s already exists. Choose a different "
	       "file name or enable overwriting with EnableOverwrite().",
	       fOutFileName.Data() );
	return -13;
      }
      cout << "Overwriting existing";
    } else
      cout << "Creating new";
    cout << " output file: " << fOutFileName << endl;
    fFile = new TFile( fOutFileName.Data(), "RECREATE" );
  }
  if( !fFile || fFile->IsZombie() ) {
    Error( here, "failed to create output file %s. Check file/directory "
	   "permissions.", fOutFileName.Data() );
    Close();
    return -14;
  }
  fFile->SetCompressionLevel(fCompress);

  //--- Set up the summary output file, if any
  if( !fAnalysisStarted && !fSummaryFileName.IsNull() ) {
    ofstream ofs;
    auto openmode = fOverwrite ? ios::out : ios::app;
    ofs.open(fSummaryFileName, openmode);
    if( !ofs ) {
      Error(here, "Failed to open summary file %s. Check file/directory "
                  "permissions", fSummaryFileName.Data());
      return -15;
    }
    ofs << "==== " << TDatime().AsString();
    ofs.close();
  }

  // Set up the analysis stages and allocate counters.
  if( !fIsInit ) {
    InitStages();
    InitCounters();
  }

  // Allocate the event structure.
  // Use the default (containing basic event header) unless the user has
  // specified a different one.
  bool new_event = false;
  // Event changed since a previous initialization?
  if( fEvent != fPrevEvent ) {
    if( fAnalysisStarted ) {
      // Don't allow a new event in the middle of the analysis!
      Error( here, "Cannot change event structure for continuing analysis. "
	     "Close() this analysis, then Init() again." );
      return -254;
    } else if( fIsInit ) {
      // If previously initialized, we might have to clean up first.
      // If we had automatically allocated an event before, and now the
      // user specifies his/her own (fEvent != 0), then get rid of the
      // previous event. Otherwise, keep the old event since it would just be
      // re-created anyway.
      if( !fLocalEvent || fEvent )
	new_event = true;
      if( fLocalEvent && fEvent ) {
	delete fPrevEvent; fPrevEvent = nullptr;
	fLocalEvent = false;
      }
    }
  }
  if( !fEvent )  {
    fEvent = new THaEvent;
    fLocalEvent = true;
    new_event = true;
  }
  fPrevEvent = fEvent;
  fEvent->Reset();

  // Allocate the output module. Recreate if necessary.  At this time, this is
  // always THaOutput, and the user cannot specify his/her own.
  // A new event requires recreation of the output module because the event
  // occupies a dedicated branch in the output tree.
  bool new_output = false;
  if( !fOutput || new_event ) {
    delete fOutput;
    fOutput = new THaOutput;
    new_output = true;
  }

  //--- Create our decoder from the TClass specified by the user.
  bool new_decoder = false;
  if( !fEvData || fEvData->IsA() != gHaDecoder ) {
    delete fEvData; fEvData = nullptr;
    if( gHaDecoder )
      fEvData = static_cast<THaEvData*>(gHaDecoder->New());
    if( !fEvData ) {
      Error( here, "Failed to create decoder object. "
	     "Something is very wrong..." );
      return -241;
    }
    new_decoder = true;
  }
  // Set run-level info that was retrieved when initializing the run.
  // In case we analyze a continuation segment, fEvtData may not see
  // the event(s) where these data come from (usually Prestart).
  fEvData->SetRunInfo(run->GetNumber(),
                      run->GetType(),
                      run->GetDate().Convert());

  // Deal with the run.
  bool new_run   = ( !fRun || *fRun != *run );
  bool need_init = ( !fIsInit || new_event || new_output || new_run ||
		     new_decoder || run_init );

#if 0
  // Warn user if trying to analyze the same run twice with overlapping
  // event ranges.  Broken for split files now, so disable warning.
  // FIXME: generalize event range business?
  if( fAnalysisStarted && !new_run && fCountMode!=kCountRaw &&
      ((fRun->GetLastEvent()  >= run->GetFirstEvent() &&
	fRun->GetFirstEvent() <  run->GetLastEvent()) ||
       (fRun->GetFirstEvent() <= run->GetLastEvent() &&
	fRun->GetLastEvent()  >  run->GetFirstEvent()) )) {
    Warning( here, "You are analyzing the same run twice with "
	     "overlapping event ranges! prev: %d-%d, now: %d-%d",
	     fRun->GetFirstEvent(), fRun->GetLastEvent(),
	     run->GetFirstEvent(), run->GetLastEvent() );
    if( !gROOT->IsBatch() ) {
      cout << "Are you sure (y/n)?" << endl;
      char c = 0;
      while( c != 'y' && c != 'n' && c != EOF ) {
	cin >> c;
      }
      if( c != 'y' && c != 'Y' )
	return -240;
    }
  }
#endif

  // Make sure we save a copy of the run in fRun.
  // Note that the run may be derived from THaRunBase, so this is a bit tricky.
  // Check if run changed using Compare to be sure we note change of split runs
  if( new_run || fRun->Compare(run) ) {
    delete fRun;
    fRun = static_cast<THaRunBase*>(run->IsA()->New());
    if( !fRun ) {
      Error(here, "Failed to create copy of run object. "
                  "Something is very wrong...");
      return -252;
    }
    *fRun = *run;  // Copy the run via its virtual operator=
  }

  // Make the current run available globally - the run parameters are
  // needed by some modules
  gHaRun = fRun;

  // Print run info
  if( fVerbose>0 ) {
    fRun->Print("STARTINFO");
  }

  // Write startup info to summary file
  if( !fSummaryFileName.IsNull() ) {
    ofstream ofs(fSummaryFileName.Data(), ios::app);
    if( !ofs ) {
      // This would be odd, since it just worked above
      Error(here, "Failed to open summary file %s (after previous success). "
                  "Check file/directory permissions", fSummaryFileName.Data());
      return -15;
    }
    ofs << " " << (fAnalysisStarted ? "Continuing" : "Started") << " analysis"
        << ", run " << run->GetNumber() << endl;
    ofs << "Reading from ";
    // Redirect cout to ofs
    auto* cout_buf = cout.rdbuf();
    cout.rdbuf(ofs.rdbuf());
    fRun->Print("NAMEDESC");
    cout.rdbuf(cout_buf);
    ofs << endl;
    ofs.close();
  }

  // Clear counters unless we are continuing an analysis
  if( !fAnalysisStarted )
    ClearCounters();

  //---- If previously initialized and nothing changed, we are done----
  if( !need_init )
    return 0;

  // Obtain time of the run, the one parameter we really need
  // for initializing the modules
  TDatime run_time = fRun->GetDate();

  // Tell the decoder the run time. This will trigger decoder
  // initialization (reading of crate map data etc.)
  fEvData->SetRunTime( run_time.Convert());

  // Tell the decoder about the run's CODA version
  fEvData->SetDataVersion( run->GetDataVersion() );

  // Use the global lists of analysis objects.
  if( !fAnalysisStarted ) {
    ListToVector(gHaApps, fApps);
    ListToVector(gHaApps, fSpectrometers);
    ListToVector(gHaPhysics, fPhysics);
    ListToVector(gHaEvtHandlers, fEvtHandlers);
  }

  // Initialize all apparatuses, physics modules, event type handlers
  // and inter-stage modules in that order. Quit on any errors.
  cout << "Initializing analysis objects" << endl;
  vector<THaAnalysisObject*> modulesToInit;
  modulesToInit.reserve(fApps.size() + fPhysics.size() +
                        fEvtHandlers.size() + fInterStage.size());
  modulesToInit.insert(modulesToInit.end(), ALL(fApps));
  modulesToInit.insert(modulesToInit.end(), ALL(fPhysics));
  modulesToInit.insert(modulesToInit.end(), ALL(fEvtHandlers));
  modulesToInit.insert(modulesToInit.end(), ALL(fInterStage));
  retval = InitModules(modulesToInit, run_time);
  if( retval == 0 ) {

    // Set up cuts here, now that all global variables are available
    if( fCutFileName.IsNull() ) {
      // No test definitions -> make sure list is clear
      gHaCuts->Clear();
      fLoadedCutFileName = "";
    } else {
      if( fCutFileName != fLoadedCutFileName ) {
	// New test definitions -> load them
	cout << "Loading cuts from " << fCutFileName << endl;
	gHaCuts->Load( fCutFileName );
	fLoadedCutFileName = fCutFileName;
      }
      // Ensure all tests are up-to-date. Global variables may have changed.
      gHaCuts->Compile();
    }
    // Initialize local pointers to test blocks and master cuts
    InitCuts();

    // fOutput must be initialized after all apparatuses are
    // initialized and before adding anything to its tree.

    // first, make sure we are in the output file, but remember the previous
    // state. This makes a difference if another ROOT-file is opened to read
    // in simulated or old data
    TDirectory *olddir = gDirectory;
    fFile->cd();

    cout << "Initializing output" << endl;
    if( (retval = fOutput->Init( fOdefFileName )) < 0 ) {
      Error( here, "Error initializing THaOutput." );
    } else if( retval == 1 )
      retval = 0;  // Reinitialization ok, not an error
    else {
      // If initialized ok, but not re-initialized, make a branch for
      // the event structure in the output tree
      TTree* outputTree = fOutput->GetTree();
      if( outputTree )
	outputTree->Branch( "Event_Branch", fEvent->IsA()->GetName(),
			    &fEvent, 16000, 99 );
    }
    olddir->cd();

    // Post-process has to be initialized after all cuts are known
    if( !fPostProcess.empty() )
      cout << "Initializing post processors" << endl;
    for( auto* obj : fPostProcess) {
      retval = obj->Init(run_time);
      if( retval != 0 )
        break;
    }
  }

  if ( retval == 0 ) {
    if ( ! fOutput ) {
      Error( here, "Error initializing THaOutput for objects(again!)" );
      retval = -5;
    } else {
      // call the apparatuses again, to permit them to write more
      // complex output directly to the TTree
      retval = InitOutput(modulesToInit);
    }
  }

  // If initialization succeeded, set status flags accordingly
  if( retval == 0 ) {
    fIsInit = true;
  }
  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::InitOutput( const std::vector<THaAnalysisObject*>& module_list )
{
  // Initialize a list of THaAnalysisObject's for output
  // If 'baseclass' given, ensure that each object in the list inherits
  // from 'baseclass'.
  static const char* const here = "InitOutput()";

  Int_t retval = 0;
  for( auto* theModule : module_list ) {
    theModule->InitOutput( fOutput );
    if( !theModule->IsOKOut() ) {
      Error( here, "Error initializing output for  %s (%s). "
          "Analyzer initialization failed.",
          theModule->GetName(), theModule->GetTitle() );
      retval = -1;
      break;
    }
  }
  return retval;
}


//_____________________________________________________________________________
Int_t THaAnalyzer::ReadOneEvent()
{
  // Read one event from current run (fRun) and raw-decode it using the
  // current decoder (fEvData)

  if( fDoBench ) fBench->Begin("RawDecode");

  // Find next event buffer in CODA file. Quit if error.
  Int_t status = THaRunBase::READ_OK;
  if( !fEvData->DataCached() )
    status = fRun->ReadEvent();

  switch( status ) {
  case THaRunBase::READ_OK:
    // Decode the event
    status = fEvData->LoadEvent( fRun->GetEvBuffer() );
    switch( status ) {
    case THaEvData::HED_OK:     // fall through
    case THaEvData::HED_WARN:
      status = THaRunBase::READ_OK;
      Incr(kNevRead);
      break;
    case THaEvData::HED_ERR:
      // Decoding error
      status = THaRunBase::READ_ERROR;
      Incr(kDecodeErr);
      break;
    case THaEvData::HED_FATAL:
      status = THaRunBase::READ_FATAL;
      break;
    default:
      Error("THaAnalyzer::ReadEvent",
            "Unsupported decoder return code %d. Call expert.", status);
      status = THaRunBase::READ_FATAL;
      break;
    }
    break;

  case THaRunBase::READ_EOF:    // fall through
  case THaRunBase::READ_FATAL:
    // Just exit on EOF - don't count it
    break;
  default:
    Incr(kCodaErr);
    break;
  }

  if( fDoBench ) fBench->Stop("RawDecode");
  return status;
}

//_____________________________________________________________________________
void THaAnalyzer::SetEpicsEvtType(Int_t itype)
{
    if (fEpicsHandler) fEpicsHandler->SetEvtType(itype);
    if (fEvData) fEvData->SetEpicsEvtType(itype);
}

//_____________________________________________________________________________
void THaAnalyzer::AddEpicsEvtType(Int_t itype)
{
    if (fEpicsHandler) fEpicsHandler->AddEvtType(itype);
}

//_____________________________________________________________________________
Int_t THaAnalyzer::SetCountMode( Int_t mode )
{
  // Set the way the internal event counter is defined.
  // If mode >= 0 and mode is one of kCountAll, kCountPhysics, or kCountRaw,
  // then set the mode. If mode >= 0 but unknown, return -mode.
  // If mode < 0, don't change the mode but return the current count mode.

  if( mode < 0 )
    return fCountMode;

  if( mode != kCountAll && mode != kCountPhysics && mode != kCountRaw )
    return -mode;

  fCountMode = mode;
  return mode;
}

//_____________________________________________________________________________
void THaAnalyzer::SetCrateMapFileName( const char* name )
{
  // Set name of file from which to read the crate map.
  // For simplicity, a simple string like "mymap" is automatically
  // converted to "db_mymap.dat". See THaAnalysisObject::GetDBFileList
  // Must be called before initialization.

  THaEvData::SetDefaultCrateMapName( name );
}

//_____________________________________________________________________________
void THaAnalyzer::Print( Option_t* ) const
{
  // Report status of the analyzer.

  //FIXME: preliminary
  if( fRun )
    fRun->Print();
}

//_____________________________________________________________________________
void THaAnalyzer::PrintCounters() const
{
  // Print statistics counters
  UInt_t w = 0;
  auto ncounters = static_cast<Int_t>(fCounters.size());
  for( Int_t i = 0; i < ncounters; i++ ) {
    if( GetCount(i) > w )
      w = GetCount(i);
  }
  w = IntDigits(SINT(w));

  bool first = true;
  for( Int_t i = 0; i < ncounters; i++ ) {
    const char* text = fCounters[i].description;
    if( GetCount(i) != 0 && text && *text ) {
      if( first ) {
	cout << "Counter summary:" << endl;
	first = false;
      }
      cout << setw(w) << GetCount(i) << "  " << text << endl;
    }
  }
  cout << endl;
}

//_____________________________________________________________________________
void THaAnalyzer::PrintExitStatus(EExitStatus status) const
{
  // Print analyzer exit status

  cout << dec;
  switch( status ) {
    case EExitStatus::kUnknown:
      cout << "Analysis ended.";
      break;
    case EExitStatus::kEOF:
      cout << "End of file.";
      break;
    case EExitStatus::kEvLimit:
      cout << "Event limit reached.";
      break;
    case EExitStatus::kFatal:
      cout << "Fatal processing error.";
      break;
    case EExitStatus::kTerminated:
      cout << "Terminated during processing.";
      break;
  }
  cout << endl;
}

//_____________________________________________________________________________
void THaAnalyzer::PrintRunSummary() const
{
  // Print general summary of run

  cout << "==== " << TDatime().AsString()
       << " Summary for run " << fRun->GetNumber()
       << endl;
}

//_____________________________________________________________________________
void THaAnalyzer::PrintCutSummary() const
{
  // Print summary of cuts

  if( gHaCuts->GetSize() > 0 ) {
    cout << "Cut summary:" << endl;
    gHaCuts->Print("STATS");
  }
}

//_____________________________________________________________________________
void THaAnalyzer::PrintTimingSummary() const
{
  // Print timing statistics, if benchmarking enabled
  if( (fVerbose > 1 || fDoBench) ) {
    vector<TString> names;
    if( fDoBench ) {
      cout << "Timing summary:" << endl;
      names = {
        "Init", "Begin", "RawDecode", "Decode", "CoarseTracking",
        "CoarseReconstruct", "Tracking", "Reconstruct", "Physics", "End",
        "Output", "Cuts"
      };
    }
    names.emplace_back("Total");
    fBench->PrintByName(names);
  }
}

//_____________________________________________________________________________
void THaAnalyzer::PrintSummary( EExitStatus exit_status ) const
{
  // Print summary (cuts etc.)
  // Only print to screen if fVerbose>1.
  // Always write to the summary file if a summary file is requested.

  if( fVerbose > 0 ) {
    PrintExitStatus(exit_status);
    PrintRunSummary();
    if( exit_status != EExitStatus::kFatal ) {
      PrintCounters();
      if( fVerbose > 1 )
        PrintCutSummary();
    }
  }
  PrintTimingSummary();

  if( !fSummaryFileName.IsNull() ) {
    // Append to the summary file. If fOverwrite is set, it was already
    // recreated earlier
    ofstream ostr(fSummaryFileName, ios::app );
    if( ostr ) {
      // Write to file via cout
      auto* cout_buf = cout.rdbuf();
      cout.rdbuf(ostr.rdbuf());

      PrintExitStatus(exit_status);
      PrintRunSummary();
      if( exit_status != EExitStatus::kFatal ) {
        PrintCounters();
        PrintCutSummary();
      }
      PrintTimingSummary();

      cout.rdbuf(cout_buf);
      ostr.close();
    }
  }

}

//_____________________________________________________________________________
Int_t THaAnalyzer::BeginAnalysis()
{
  // Internal function called right before the start of the event loop
  // for each run.
  // Initializes subroutine-specific variables.
  // Executes Begin() for all Apparatus and Physics modules.

  fFirstPhysics = true;

  if( fDoBench ) fBench->Begin("Begin");

  for( auto* theModule : fAnalysisModules ) {
    theModule->Begin( fRun );
  }

  for( auto* obj : fEvtHandlers ) {
    obj->Begin(fRun);
  }

  if( fDoBench ) fBench->Stop("Begin");

  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::EndAnalysis()
{
  // Execute End() for all Apparatus and Physics modules. Internal function
  // called right after event loop is finished for each run.

  if( fDoBench ) fBench->Begin("End");

  for( auto* theModule : fAnalysisModules ) {
    theModule->End( fRun );
  }

  for( auto* obj : fEvtHandlers ) {
    obj->End(fRun);
  }

  if( fDoBench ) fBench->Stop("End");

  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::PhysicsAnalysis( Int_t code )
{
  // Analysis of physics events

  static const char* const here = "PhysicsAnalysis";

  // Don't analyze events that did not pass RawDecode cuts
  if( code != kOK )
    return code;

  //--- Skip physics events until we reach the first requested event
  if( fNev < fRun->GetFirstEvent() )
    return kSkip;

  if( fFirstPhysics ) {
    fFirstPhysics = false;
    if( fVerbose>2 )
      cout << "Starting physics analysis at event " << GetCount(kNevPhysics)
	   << endl;
  }
  // Update counters
  fRun->IncrNumAnalyzed();
  Incr(kNevAnalyzed);

  //--- Process all apparatuses that are defined in fApps
  //    First Decode(), then Reconstruct()

  const char* stage = "";
  THaAnalysisObject* obj = nullptr;  // current module, for exception error message

  try {
    stage = "Decode";
    if( fDoBench ) fBench->Begin(stage);
    for( auto* mod : fAnalysisModules ) {
      obj = mod;
      mod->Clear();
    }
    for( auto* app : fApps ) {
      obj = app;
      app->Decode(*fEvData);
    }
    for( auto* mod : fInterStage ) {
      if( mod->GetStage() == kDecode ) {
        obj = mod;
        mod->Process(*fEvData);
      }
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kDecode) ) return kSkip;

    //--- Main physics analysis. Calls the following for each defined apparatus
    //    THaSpectrometer::CoarseTrack  (only for spectrometers)
    //    THaApparatus::CoarseReconstruct
    //    THaSpectrometer::Track        (only for spectrometers)
    //    THaApparatus::Reconstruct
    //
    // Test blocks are evaluated after each of these stages

    //-- Coarse processing

    stage = "CoarseTracking";
    if( fDoBench ) fBench->Begin(stage);
    for( auto* spectro : fSpectrometers ) {
      obj = spectro;
      spectro->CoarseTrack();
    }
    for( auto* mod : fInterStage ) {
      if( mod->GetStage() == kCoarseTrack ) {
        obj = mod;
        mod->Process(*fEvData);
      }
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kCoarseTrack) )  return kSkip;


    stage = "CoarseReconstruct";
    if( fDoBench ) fBench->Begin(stage);
    for( auto* app : fApps ) {
      obj = app;
      app->CoarseReconstruct();
    }
    for( auto* mod : fInterStage ) {
      if( mod->GetStage() == kCoarseRecon ) {
        obj = mod;
        mod->Process(*fEvData);
      }
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kCoarseRecon) )  return kSkip;

    //-- Fine (Full) Reconstruct().

    stage = "Tracking";
    if( fDoBench ) fBench->Begin(stage);
    for( auto* spectro : fSpectrometers ) {
      obj = spectro;
      spectro->Track();
    }
    for( auto* mod : fInterStage ) {
      if( mod->GetStage() == kTracking ) {
        obj = mod;
        mod->Process(*fEvData);
      }
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kTracking) )  return kSkip;


    stage = "Reconstruct";
    if( fDoBench ) fBench->Begin(stage);
    for( auto* app : fApps ) {
      obj = app;
      app->Reconstruct();
    }
    for( auto* mod : fInterStage ) {
      if( mod->GetStage() == kReconstruct ) {
        obj = mod;
        mod->Process(*fEvData);
      }
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kReconstruct) )  return kSkip;

    //--- Process the list of physics modules

    stage = "Physics";
    if( fDoBench ) fBench->Begin(stage);
    for( auto* physmod : fPhysics ) {
      obj = physmod;
      Int_t err = physmod->Process( *fEvData );
      if( err == THaPhysicsModule::kTerminate )
        code = kTerminate;
      else if( err == THaPhysicsModule::kFatal ) {
        code = kFatal;
        break;
      }
    }
    for( auto* mod : fInterStage ) {
      if( mod->GetStage() == kPhysics ) {
        obj = mod;
        mod->Process(*fEvData);
      }
    }
    if( fDoBench ) fBench->Stop(stage);
    if( code == kFatal ) return kFatal;

    //--- Evaluate "Physics" test block

    if( !EvalStage(kPhysics) )
      // any status code form the physics analysis overrides skip code
      return (code == kOK) ? kSkip : code;

  } // end try
  catch( const exception& e ) {
    TString module_name = (obj != nullptr) ? obj->GetName() : "unknown";
    TString module_desc = (obj != nullptr) ? obj->GetTitle() : "unknown";
    Error( here, "Caught exception %s in module %s (%s) during %s analysis "
	   "stage. Terminating analysis.", e.what(), module_name.Data(),
	   module_desc.Data(), stage );
    if( fDoBench ) fBench->Stop(stage);
    code = kFatal;
    goto errexit;
  }

  //---  Process output
  if( fDoBench ) fBench->Begin("Output");
  try {
    //--- If Event defined, fill it.
    if( fEvent ) {
      fEvent->GetHeader()->Set( fEvData->GetEvNum(),
				fEvData->GetEvType(),
				fEvData->GetEvLength(),
				fEvData->GetEvTime(),
				fEvData->GetHelicity(),
				fEvData->GetTrigBits(),
				fRun->GetNumber()
				);
      fEvent->Fill();
    }
    // Write to output file
    if( fOutput ) fOutput->Process();
  }
  catch( const exception& e ) {
    Error( here, "Caught exception %s during output of event %u. "
	   "Terminating analysis.", e.what(), fNev );
    code = kFatal;
  }
  if( fDoBench ) fBench->Stop("Output");

 errexit:
  return code;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::SlowControlAnalysis( Int_t code )
{
  // Analyze slow control (EPICS) data and write them to output.
  // Ignores RawDecode results and requested event range, so EPICS
  // data are always analyzed continuously from the beginning of the run.

  if( code == kFatal )
    return code;
  if ( !fEpicsHandler ) return kOK;
  if( fDoBench ) fBench->Begin("Output");
  if( fOutput ) fOutput->ProcEpics(fEvData, fEpicsHandler);
  if( fDoBench ) fBench->Stop("Output");
  if( code == kTerminate )
    return code;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::OtherAnalysis( Int_t code )
{
  // Analysis of other events (i.e. events that are not physics, scaler, or
  // slow control events).
  //
  // This default version does nothing. Returns 'code'.

  return code;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::PostProcess( Int_t code )
{
  // Post-processing via generic PostProcess modules
  // May be used for event filtering etc.
  // 'code' (status of the preceding analysis) is passed to the
  // THaPostProcess::Process() function for optional evaluation,
  // e.g. skipping events that fail analysis stage cuts.

  if( fDoBench ) fBench->Begin("PostProcess");

  if( code == kFatal )
    return code;
  for( auto* obj : fPostProcess ) {
    Int_t ret = obj->Process(fEvData,fRun,code);
    if( obj->TestBits(THaPostProcess::kUseReturnCode) &&
	ret > code )
      code = ret;
  }
  if( fDoBench ) fBench->Stop("PostProcess");
  return code;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::MainAnalysis()
{
  // Main analysis carried out for each event

  static const char* const here = "MainAnalysis";

  Int_t retval = kOK;

  Incr(kNevGood);

  //--- Evaluate test block "RawDecode"
  bool rawfail = false;
  if( !EvalStage(kRawDecode) ) {
    retval = kSkip;
    rawfail = true;
  }

  //FIXME Move to "OtherAnalysis"?
  for( auto* obj : fEvtHandlers ) {
    try {
      obj->Analyze(fEvData);
    }
    catch( const exception& e) {
      // Generic exceptions are not fatal. Print message and continue.
      Error( here, "%s", e.what() );
    }
  }

  bool evdone = false;
  //=== Physics triggers ===
  if( fEvData->IsPhysicsTrigger() && fDoPhysics ) {
    Incr(kNevPhysics);
    retval = PhysicsAnalysis(retval);
    evdone = true;
  }

  //=== EPICS data ===
  if( fEpicsHandler->GetNumTypes() > 0 ) {
    fEvData->SetEpicsEvtType(fEpicsHandler->GetEvtType());
    if( fDoSlowControl && fEpicsHandler->IsMyEvent(fEvData->GetEvType()) ) {
      Incr(kNevEpics);
      retval = SlowControlAnalysis(retval);
      evdone = true;
    }
  }

  //=== Other events ===
  if( !evdone && fDoOtherEvents ) {
    Incr(kNevOther);
    retval = OtherAnalysis(retval);

  } // End trigger type test


  //=== Post-processing (e.g. event filtering) ===
  if( !fPostProcess.empty() ) {
    Incr(kNevPostProcess);
    retval = PostProcess(retval);
  }

  // Take care of the RawDecode skip counter
  if( rawfail && retval != kSkip && GetCount(kRawDecodeTest)>0 )
    fCounters[kRawDecodeTest].count--;

  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::Process( THaRunBase* run )
{
  // Process the given run. Loop over all events in the event range and
  // analyze all apparatuses defined in the global apparatus list.
  // Fill Event structure if it is defined.
  // If Event and Filename are defined, then fill the output tree with Event
  // and write the file.

  static const char* const here = "Process";

  if( !run ) {
    if( fRun )
      run = fRun;
    else
      return -1;
  }

  //--- Initialization. Creates fFile, fOutput, and fEvent if necessary.
  //    Also copies run to fRun if run is different from fRun
  Int_t status = Init( run );
  if( status != 0 ) {
    return status;
  }

  // Restart "Total" since it is stopped in Init()
  fBench->Begin("Total");

  //--- Re-open the data source. Should succeed since this was tested in Init().
  if( (status = fRun->Open()) != THaRunBase::READ_OK ) {
    Error( here, "Failed to re-open the input file. "
	   "Make sure the file still exists.");
    fBench->Stop("Total");
    return -4;
  }

  // Make the current run available globally - the run parameters are
  // needed by some modules
  gHaRun = fRun;

  // Enable/disable helicity decoding as requested
  fEvData->EnableHelicity( HelicityEnabled() );
  // Set decoder reporting level. FIXME: update when THaEvData is updated
  fEvData->SetVerbose( (fVerbose>2) );
  fEvData->SetDebug( (fVerbose>3) );

  // Informational messages
  if( fVerbose>1 ) {
    cout << "Decoder: helicity "
	 << (fEvData->HelicityEnabled() ? "enabled" : "disabled")
	 << endl;
    cout << endl << "Starting analysis" << endl;
  }
  // Events prior to fRun->GetFirstEvent() are skipped in MainAnalysis()
  if( fVerbose>2 && fRun->GetFirstEvent()>1 )
    cout << "Skipping " << fRun->GetFirstEvent() << " events" << endl;

  //--- The main event loop.

  if( fDoBench ) fBench->Begin("Init");
  fNev = 0;
  bool terminate = false, fatal = false;
  UInt_t nlast = fRun->GetLastEvent();
  fAnalysisStarted = true;
  PrepareModuleList();
  if( fDoBench ) fBench->Stop("Init");
  BeginAnalysis();
  if( fFile ) {
    if( fDoBench ) fBench->Begin("Output");
    fFile->cd();
    fRun->Write("Run_Data");  // Save run data to first ROOT file
    if( fDoBench ) fBench->Stop("Output");
  }

  while ( !terminate && fNev < nlast &&
	  (status = ReadOneEvent()) != THaRunBase::READ_EOF ) {

    //--- Skip events with errors, unless fatal
    if( status == THaRunBase::READ_FATAL ) {
      terminate = fatal = true;
      continue;
    }
    if( status != THaRunBase::READ_OK )
      continue;

    UInt_t evnum = fEvData->GetEvNum();

    // Set the event counter according to the requested mode
    switch(fCountMode) {
    case kCountPhysics:
      if( fEvData->IsPhysicsTrigger() )
	fNev++;
      break;
    case kCountAll:
      fNev++;
      break;
    case kCountRaw:
      fNev = evnum;
      break;
    default:
      break;
    }

    //--- Print marks periodically
    if( fVerbose > 1 && fNev > 0 && (fNev % fMarkInterval == 0) &&
        // Avoid duplicates that may occur if a physics event is followed by
        // non-physics events. Only physics events update the event number.
        (fCountMode == kCountAll || fEvData->IsPhysicsTrigger()) )
      cout << dec << fNev << endl;

    //--- Update run parameters with current event
    if( fUpdateRun )
      fRun->Update( fEvData );

    //--- Clear all tests/cuts
    if( fDoBench ) fBench->Begin("Cuts");
    gHaCuts->ClearAll();
    if( fDoBench ) fBench->Stop("Cuts");

    //--- Perform the analysis
    Int_t err = MainAnalysis();
    switch( err ) {
    case kOK:
      break;
    case kSkip:
      continue;
    case kFatal:
      fatal = terminate = true;
      continue;
    case kTerminate:
      terminate = true;
      break;
    default:
      Error( here, "Unknown return code from MainAnalysis(): %d", err );
      terminate = fatal = true;
      continue;
    }

    Incr(kNevAccepted);

  }  // End of event loop

  EndAnalysis();

  //--- Close the input file
  fRun->Close();

  // Save final run parameters in run object of caller, if any
  *run = *fRun;

  // Write the output file and clean up.
  // This writes the Tree as well as any objects (histograms etc.)
  // that are defined in the current directory.

  if( fDoBench ) fBench->Begin("Output");
  // Ensure that we are in the output file's current directory
  // ... someone might have pulled the rug from under our feet

  // get the CURRENT file, since splitting might have occurred
  if( fOutput && fOutput->GetTree() )
    fFile = fOutput->GetTree()->GetCurrentFile();
  if( fFile )   fFile->cd();
  if( fOutput ) fOutput->End();
  if( fFile ) {
    fRun->Write("Run_Data");  // Save run data to ROOT file
    //    fFile->Write();//already done by fOutput->End()
    fFile->Purge();         // get rid of excess object "cycles"
  }
  if( fDoBench ) fBench->Stop("Output");

  fBench->Stop("Total");

  //--- Report statistics and summary information (also to file if one given)
  EExitStatus exit_status = EExitStatus::kUnknown;
  if( status == THaRunBase::READ_EOF )
    exit_status = EExitStatus::kEOF;
  else if( fNev == nlast )
    exit_status = EExitStatus::kEvLimit;
  else if( fatal )
    exit_status = EExitStatus::kFatal;
  else if( terminate )
    exit_status = EExitStatus::kTerminated;

  PrintSummary(exit_status);

  //keep the last run available
  //  gHaRun = nullptr;
  return SINT(fNev);
}

//_____________________________________________________________________________
void THaAnalyzer::SetCodaVersion( Int_t vers )
{
  if (vers != 2 && vers != 3) {
    Warning( "THaAnalyzer::SetCodaVersion", "Illegal CODA version = %d. "
        "Must be 2 or 3. Setting version to 2.", vers );
    vers = 2;
  }
  fWantCodaVers = vers;
}

//_____________________________________________________________________________
void THaAnalyzer::PrepareModuleList()
{
  // Fill fAnalysisModules in the order fApps, fInterStage, fPhysics to be
  // used with PhysicsAnalysis().

  fAnalysisModules.clear();
  fAnalysisModules.reserve(fApps.size() + fInterStage.size() +
                           fPhysics.size());
  fAnalysisModules.insert(fAnalysisModules.end(), ALL(fApps));
  fAnalysisModules.insert(fAnalysisModules.end(), ALL(fInterStage));
  fAnalysisModules.insert(fAnalysisModules.end(), ALL(fPhysics));
}

//_____________________________________________________________________________

ClassImp(THaAnalyzer)
