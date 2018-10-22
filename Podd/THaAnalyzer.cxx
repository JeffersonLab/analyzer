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

#include <iostream>
#include <fstream>
#include "THaAnalyzer.h"
#include "THaRunBase.h"
#include "THaEvent.h"
#include "THaOutput.h"
#include "THaEvData.h"
#include "THaGlobals.h"
#include "THaSpectrometer.h"
#include "THaNamedList.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "THaPhysicsModule.h"
#include "THaPostProcess.h"
#include "THaBenchmark.h"
#include "THaEvtTypeHandler.h"
#include "THaEpicsEvtHandler.h"
#include "TList.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"
#include "TDatime.h"
#include "TClass.h"
#include "TError.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TMath.h"
#include "TDirectory.h"
#include "THaCrateMap.h"

#include <fstream>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <exception>
#include <stdexcept>

using namespace std;
using namespace Decoder;

const char* const THaAnalyzer::kMasterCutName = "master";
const char* const THaAnalyzer::kDefaultOdefFile = "output.def";

const int MAXSTAGE = 100;   // Sanity limit on number of stages
const int MAXCOUNTER = 200; // Sanity limit on number of counters

// Pointer to single instance of this object
THaAnalyzer* THaAnalyzer::fgAnalyzer = 0;

//FIXME:
// do we need to "close" scalers/EPICS analysis if we reach the event limit?

//_____________________________________________________________________________
THaAnalyzer::THaAnalyzer() :
  fFile(NULL), fOutput(NULL), fEpicsHandler(NULL),
  fOdefFileName(kDefaultOdefFile), fEvent(NULL), fNStages(0), fNCounters(0),
  fWantCodaVers(-1),
  fStages(NULL), fCounters(NULL), fNev(0), fMarkInterval(1000), fCompress(1),
  fVerbose(2), fCountMode(kCountRaw), fBench(NULL), fPrevEvent(NULL),
  fRun(NULL), fEvData(NULL), fApps(NULL), fPhysics(NULL),
  fPostProcess(NULL), fEvtHandlers(NULL),
  fIsInit(kFALSE), fAnalysisStarted(kFALSE), fLocalEvent(kFALSE),
  fUpdateRun(kTRUE), fOverwrite(kTRUE), fDoBench(kFALSE),
  fDoHelicity(kFALSE), fDoPhysics(kTRUE), fDoOtherEvents(kTRUE),
  fDoSlowControl(kTRUE), fFirstPhysics(true), fExtra(0)

{
  // Default constructor.

  // Allow only one analyzer object (because we use various global lists)
  if( fgAnalyzer ) {
    Error("THaAnalyzer", "only one instance of THaAnalyzer allowed.");
    MakeZombie();
    return;
  }
  fgAnalyzer = this;

  // Use the global lists of analysis objects.
  fApps    = gHaApps;
  fPhysics = gHaPhysics;
  fEvtHandlers = gHaEvtHandlers;

  // EPICs data
  fEpicsHandler = new THaEpicsEvtHandler("epics","EPICS event type");
  //  fEpicsHandler->SetDebugFile("epicsdat.txt");
  fEvtHandlers->Add(fEpicsHandler);


  // Timers
  fBench = new THaBenchmark;
}

//_____________________________________________________________________________
THaAnalyzer::~THaAnalyzer()
{
  // Destructor.

  Close();
  delete fExtra; fExtra = 0;
  delete fPostProcess;  //deletes PostProcess objects
  delete fBench;
  delete [] fStages;
  delete [] fCounters;
  if( fgAnalyzer == this )
    fgAnalyzer = NULL;
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

  // If list of modules does not yet exist, create it.
  // Destructor will clean up.
  if( !fPostProcess )
    fPostProcess = new TList;

  fPostProcess->Add(module);
  return 0;
}

//_____________________________________________________________________________
void THaAnalyzer::ClearCounters()
{
  // Clear statistics counters
  for( int i=0; i<fNCounters; i++ ) {
    fCounters[i].count = 0;
  }
}

//_____________________________________________________________________________
void THaAnalyzer::Close()
{
  // Close output files and delete fOutput, fFile, and fRun objects.
  // Also delete fEvent if it was allocated automatically by us.

  // Close all Post-process objects, but do not delete them
  // (destructor does that)
  TIter nextp(fPostProcess);
  TObject *obj;
  while((obj=nextp())) {
    (static_cast<THaPostProcess*>(obj))->Close();
  }

  if( gHaRun && *gHaRun == *fRun )
    gHaRun = NULL;

  delete fEvData; fEvData = NULL;
  delete fOutput; fOutput = NULL;
  if( TROOT::Initialized() )
    delete fFile;
  fFile = NULL;
  delete fRun; fRun = NULL;
  if( fLocalEvent ) {
    delete fEvent; fEvent = fPrevEvent = NULL;
  }
  fIsInit = fAnalysisStarted = kFALSE;
}


//_____________________________________________________________________________
THaAnalyzer::Stage_t* THaAnalyzer::DefineStage( const Stage_t* item )
{
  if( !item || item->key < 0 )
    return NULL;
  if( item->key >= MAXSTAGE ) {
    Error("DefineStage", "Too many analysis stages.");
    return NULL;
  }
  if( item->key >= fNStages ) {
    Int_t newmax = item->key+1;
    Stage_t* tmp = new Stage_t[newmax];
    memcpy(tmp,fStages,fNStages*sizeof(Stage_t));
    memset(tmp+fNStages,0,(newmax-fNStages)*sizeof(Stage_t));
    delete [] fStages;
    fStages = tmp;
    fNStages = newmax;
  }
  return static_cast<Stage_t*>
    ( memcpy(fStages+item->key,item,sizeof(Stage_t)) );
}

//_____________________________________________________________________________
THaAnalyzer::Counter_t* THaAnalyzer::DefineCounter( const Counter_t* item )
{
  if( !item || item->key < 0 )
    return NULL;
  if( item->key >= MAXCOUNTER ) {
    Error("DefineCounters", "Too many statistics counters.");
    return NULL;
  }
  if( item->key >= fNCounters ) {
    Int_t newmax = item->key+1;
    Counter_t* tmp = new Counter_t[newmax];
    memcpy(tmp,fCounters,fNCounters*sizeof(Counter_t));
    memset(tmp+fNCounters,0,(newmax-fNCounters)*sizeof(Counter_t));
    delete [] fCounters;
    fCounters = tmp;
    fNCounters = newmax;
  }
  return static_cast<Counter_t*>
    ( memcpy(fCounters+item->key,item,sizeof(Counter_t)) );
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
void THaAnalyzer::EnableScalers( Bool_t )
{
  cout << "Warning:: Scalers are handled by event handlers now"<<endl;
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

  const Stage_t* theStage = fStages+n;

  //FIXME: support stage-wise blocks of histograms
  //  if( theStage->hist_list ) {
    // Fill histograms
  //  }

  bool ret = true;
  if( theStage->cut_list ) {
    gHaCuts->EvalBlock( theStage->cut_list );
    if( theStage->master_cut &&
	!theStage->master_cut->GetResult() ) {
      if( theStage->countkey >= 0 ) // stage may not have a counter
	Incr(theStage->countkey);
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
    Warning( "GetDecoder", "Decoder not yet set up. You need to intialize "
	     "the analyzer with Init(run) first." );
  }
  return fEvData;
}

//_____________________________________________________________________________
void THaAnalyzer::InitStages()
{
  // Define analysis stages. Called from Init(). Does nothing if called again.
  // Derived classes can override this method to define their own
  // stages or additional stages. This should be done with caution.

  if( fNStages>0 )  return;
  const Stage_t stagedef[] = {
    { kRawDecode,   kRawDecodeTest,   "RawDecode" },
    { kDecode,      kDecodeTest,      "Decode" },
    { kCoarseTrack, kCoarseTrackTest, "CoarseTracking" },
    { kCoarseRecon, kCoarseReconTest, "CoarseReconstruct" },
    { kTracking,    kTrackTest,       "Tracking" },
    { kReconstruct, kReconstructTest, "Reconstruct" },
    { kPhysics,     kPhysicsTest,     "Physics" },
    { -1 }
  };
  const Stage_t* idef = stagedef;
  while( DefineStage(idef++) ) {}
}

//_____________________________________________________________________________
void THaAnalyzer::InitCounters()
{
  // Allocate statistics counters.
  // See notes for InitStages() for additional information.

  if( fNCounters>0 )  return;
  const Counter_t counterdef[] = {
    { kNevRead,            "events read" },
    { kNevGood,            "events decoded" },
    { kNevPhysics,         "physics events" },
    { kNevEpics,           "slow control events" },
    { kNevOther,           "other event types" },
    { kNevPostProcess,     "events post-processed" },
    { kNevAnalyzed,        "physics events analyzed" },
    { kNevAccepted,        "events accepted" },
    { kDecodeErr,          "decoding error" },
    { kCodaErr,            "CODA errors" },
    { kRawDecodeTest,      "skipped after raw decoding" },
    { kDecodeTest,         "skipped after Decode" },
    { kCoarseTrackTest,    "skipped after Coarse Tracking" },
    { kCoarseReconTest,    "skipped after Coarse Reconstruct" },
    { kTrackTest,          "skipped after Tracking" },
    { kReconstructTest,    "skipped after Reconstruct" },
    { kPhysicsTest,        "skipped after Physics" },
    { -1 }
  };
  const Counter_t* jdef = counterdef;
  while( DefineCounter(jdef++) ) {}
}

//_____________________________________________________________________________
void THaAnalyzer::InitCuts()
{
  // Internal function that sets up structures to handle cuts more efficiently:
  //
  // - Find pointers to the THaNamedList lists that hold the cut blocks.
  // - find pointer to each block's master cut

  for( int i=0; i<fNStages; i++ ) {
    Stage_t* theStage = fStages+i;
    // If block not found, this will return NULL and work just fine later.
    theStage->cut_list = gHaCuts->FindBlock( theStage->name );

    if( theStage->cut_list ) {
      TString master_cut( theStage->name );
      master_cut.Append( '_' );
      master_cut.Append( kMasterCutName );
      theStage->master_cut = gHaCuts->FindCut( master_cut );
    } else
      theStage->master_cut = NULL;
  }
}

//_____________________________________________________________________________
Int_t THaAnalyzer::InitModules( TList* module_list, TDatime& run_time,
				Int_t erroff, const char* baseclass )
{
  // Initialize a list of THaAnalysisObjects for time 'run_time'.
  // If 'baseclass' given, ensure that each object in the list inherits
  // from 'baseclass'.

  static const char* const here = "InitModules()";

  if( !module_list || !baseclass || !*baseclass )
    return -3-erroff;

  TIter next( module_list );
  Int_t retval = 0;
  TObject* obj;
  while( (obj = next()) ) {
    if( !obj->IsA()->InheritsFrom( baseclass )) {
      Error( here, "Object %s (%s) is not a %s. Analyzer initialization "
	     "failed.", obj->GetName(), obj->GetTitle(), baseclass );
      retval = -2;
      break;
    }
    THaAnalysisObject* theModule = dynamic_cast<THaAnalysisObject*>( obj );
    if( !theModule ) {
      Error( here, "Object %s (%s) is not a THaAnalysisObject. Analyzer "
	     "initialization failed.", obj->GetName(), obj->GetTitle() );
      retval = -2;
      break;
    } else if( theModule->IsZombie() ) {
      Warning( here, "Removing zombie module %s (%s) from list of %s objects",
	       obj->GetName(), obj->GetTitle(), baseclass );
      module_list->Remove( theModule );
      delete theModule;
      continue;
    }
    try {
      retval = theModule->Init( run_time );
    }
    catch( exception& e ) {
      Error( here, "Exception %s caught during initialization of module "
	     "%s (%s). Analyzer initialization failed.",
	     e.what(), obj->GetName(), obj->GetTitle() );
      retval = -1;
      goto errexit;
    }
    if( retval != kOK || !theModule->IsOK() ) {
      Error( here, "Error %d initializing module %s (%s). Analyzer initial"
	     "ization failed.", retval, obj->GetName(), obj->GetTitle() );
      if( retval == kOK )
	retval = -1;
      break;
    }
  }
 errexit:
  if( retval != 0 ) retval -= erroff;
  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::Init( THaRunBase* run )
{
  // Initialize the analyzer.

  // This is a wrapper so we can conveniently control the benchmark counter
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

  static const char* const here = "Init";
  Int_t retval = 0;

  if (fWantCodaVers > 0) run->SetCodaVersion(fWantCodaVers);

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
    if( gSystem->AccessPathName(fOutFileName) == kFALSE ) { //sic
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
    return -10;
  }
  fFile->SetCompressionLevel(fCompress);

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
      return 254;
    } else if( fIsInit ) {
      // If previously initialized, we might have to clean up first.
      // If we had automatically allocated an event before, and now the
      // user specifies his/her own (fEvent != 0), then get rid of the
      // previous event. Otherwise keep the old event since it would just be
      // re-created anyway.
      if( !fLocalEvent || fEvent )
	new_event = true;
      if( fLocalEvent && fEvent ) {
	delete fPrevEvent; fPrevEvent = NULL;
	fLocalEvent = kFALSE;
      }
    }
  }
  if( !fEvent )  {
    fEvent = new THaEvent;
    fLocalEvent = kTRUE;
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
    delete fEvData; fEvData = NULL;
    if( gHaDecoder )
      fEvData = static_cast<THaEvData*>(gHaDecoder->New());
    if( !fEvData ) {
      Error( here, "Failed to create decoder object. "
	     "Something is very wrong..." );
      return 241;
    }
    new_decoder = true;
  }

  if (fWantCodaVers > 0) fEvData->SetCodaVersion(fWantCodaVers);

  // Make sure the run is initialized.
  bool run_init = false;
  if( !run->IsInit()) {
    run_init = true;
    retval = run->Init();
    if( retval )
      return retval;  //Error message printed by run class
  }

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
	return 240;
    }
  }
#endif

  // Make sure we save a copy of the run in fRun.
  // Note that the run may be derived from THaRunBase, so this is a bit tricky.
  // Check if run changed using Compare to be sure we note change of split runs
  if( new_run || fRun->Compare(run) ) {
    delete fRun;
    fRun = static_cast<THaRunBase*>(run->IsA()->New());
    if( !fRun )
      return 252; // urgh
    *fRun = *run;  // Copy the run via its virtual operator=
  }

  // Make the current run available globally - the run parameters are
  // needed by some modules
  gHaRun = fRun;

  // Print run info
  if( fVerbose>0 ) {
    fRun->Print("STARTINFO");
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

  // Initialize all apparatuses, scalers, and physics modules.
  // Quit if any errors.
  if( !((retval = InitModules( fApps,    run_time, 20, "THaApparatus")) ||
	(retval = InitModules( fPhysics, run_time, 40, "THaPhysicsModule")) ||
	(retval = InitModules( fEvtHandlers, run_time, 50, "THaEvtTypeHandler"))
	)) {

    // Set up cuts here, now that all global variables are available
    if( fCutFileName.IsNull() ) {
      // No test definitions -> make sure list is clear
      gHaCuts->Clear();
      fLoadedCutFileName = "";
    } else {
      if( fCutFileName != fLoadedCutFileName ) {
	// New test definitions -> load them
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
    TIter nextp(fPostProcess);
    TObject *obj;
    while ( !retval && (obj=nextp()) ) {
      retval = (static_cast<THaPostProcess*>(obj))->Init(run_time);
    }
  }

  if ( retval == 0 ) {
    if ( ! fOutput ) {
      Error( here, "Error initializing THaOutput for objects(again!)" );
      retval = -5;
    } else {
      // call the apparatuses again, to permit them to write more
      // complex output directly to the TTree
      (retval = InitOutput( fApps, 20, "THaApparatus")) ||
	(retval = InitOutput( fPhysics, 40, "THaPhysicsModule"));
    }
  }

  // If initialization succeeded, set status flags accordingly
  if( retval == 0 ) {
    fIsInit = kTRUE;
  }
  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::InitOutput( const TList* module_list,
			       Int_t erroff, const char* baseclass )
{
  // Initialize a list of THaAnalysisObject's for output
  // If 'baseclass' given, ensure that each object in the list inherits
  // from 'baseclass'.
  static const char* const here = "InitOutput()";

  if( !module_list )
    return -3-erroff;
  TIter next( module_list );
  bool fail = false;
  Int_t retval = 0;
  TObject* obj;
  while( !fail && (obj = next())) {
    if( baseclass && !obj->IsA()->InheritsFrom( baseclass )) {
      Error( here, "Object %s (%s) is not a %s. Analyzer initialization "
	     "failed.", obj->GetName(), obj->GetTitle(), baseclass );
      retval = -2;
      fail = true;
    } else {
      THaAnalysisObject* theModule = dynamic_cast<THaAnalysisObject*>( obj );
      theModule->InitOutput( fOutput );
      if( !theModule->IsOKOut() ) {
	Error( here, "Error initializing output for  %s (%s). "
	       "Analyzer initialization failed.",
	       obj->GetName(), obj->GetTitle() );
	retval = -1;
	fail = true;
      }
    }
  }
  return (retval == 0) ? 0 : retval - erroff;
}


//_____________________________________________________________________________
Int_t THaAnalyzer::ReadOneEvent()
{
  // Read one event from current run (fRun) and raw-decode it using the
  // current decoder (fEvData)

  if( fDoBench ) fBench->Begin("RawDecode");

  bool to_read_file = false;
  if( !fEvData->IsMultiBlockMode() ||
      (fEvData->IsMultiBlockMode() && fEvData->BlockIsDone()) )
    to_read_file = true;

  // Find next event buffer in CODA file. Quit if error.
  Int_t status = THaRunBase::READ_OK;
  if (to_read_file)
    status = fRun->ReadEvent();

  // there may be a better place to do this, but this works
  if (fWantCodaVers > 0) {
    fEvData->SetCodaVersion(fWantCodaVers);
  } else {
    fEvData->SetCodaVersion(fRun->GetCodaVersion());
  }

  switch( status ) {
  case THaRunBase::READ_OK:
    // Decode the event
    if (to_read_file) {
      status = fEvData->LoadEvent( fRun->GetEvBuffer() );
    } else {
      status = fEvData->LoadFromMultiBlock( );  // load next event in block
    }
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
};

//_____________________________________________________________________________
void THaAnalyzer::AddEpicsEvtType(Int_t itype)
{
    if (fEpicsHandler) fEpicsHandler->AddEvtType(itype);
}

//_____________________________________________________________________________
Int_t THaAnalyzer::SetCountMode( Int_t mode )
{
  // Set event counting mode. The default mode is kCountPhysics.
  // If mode >= 0 and mode is one of kCountAll, kCountPhysics, or kCountRaw,
  // then set the mode. If mode >= 0 but unknown, return -mode.
  // If mode < 0, don't change the mode but return the current count mode.
  //
  // Changing the counting mode should only be necessary in special cases.

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
  for (int i = 0; i < fNCounters; i++) {
    if( GetCount(i) > w )
      w = GetCount(i);
  }
  w = IntDigits(w);

  bool first = true;
  for (int i = 0; i < fNCounters; i++) {
    const char* text = fCounters[i].description;
    if( GetCount(i) != 0 && text && *text ) {
      if( first ) {
	cout << "Counter summary:" << endl;
	first = false;
      }
      cout << setw(w) << GetCount(i) << "  " << text << endl;
    }
  }
}

//_____________________________________________________________________________
void THaAnalyzer::PrintScalers() const
{
  // may want to loop over scaler event handlers and use their print methods.
  // but that can be done with the End method of the event handler
  // Print scaler statistics
  bool first = true;
    if( first ) {
      cout << "Scalers are event handlers now and can be summarized by "<<endl;
      cout << "those objects"<<endl;
      first = false;
    }
    cout << endl;
    //OLD WAY    theScaler->PrintSummary();
   if( !first ) cout << endl;
}

//_____________________________________________________________________________
void THaAnalyzer::PrintCutSummary() const
{
  // Print summary of cuts etc.
  // Only print to screen if fVerbose>1, but always print to
  // the summary file if a summary file is requested.

  if( gHaCuts->GetSize() > 0 ) {
    cout << "Cut summary:" << endl;
    if( fVerbose>1 )
      gHaCuts->Print("STATS");
    if( fSummaryFileName.Length() > 0 ) {
      ofstream ostr(fSummaryFileName);
      if( ostr ) {
	// Write to file via cout
	streambuf* cout_buf = cout.rdbuf();
	cout.rdbuf(ostr.rdbuf());
	TDatime now;
	cout << "Cut Summary for run " << fRun->GetNumber()
	     << " completed " << now.AsString()
	     << endl << endl;
	gHaCuts->Print("STATS");
	cout.rdbuf(cout_buf);
	ostr.close();
      }
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

  fFirstPhysics = kTRUE;

  TIter nexta(fApps);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nexta()) ) {
    obj->Begin( fRun );
  }
  TIter nextp(fPhysics);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nextp()) ) {
    obj->Begin( fRun );
  }
  TIter nexte(fEvtHandlers);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nexte()) ) {
    obj->Begin( fRun );
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::EndAnalysis()
{
  // Execute End() for all Apparatus and Physics modules. Internal function
  // called right after event loop is finished for each run.

  TIter nexta(fApps);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nexta()) ) {
    obj->End( fRun );
  }
  TIter nextp(fPhysics);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nextp()) ) {
    obj->End( fRun );
  }
  TIter nexte(fEvtHandlers);
  while( THaAnalysisObject* obj = static_cast<THaAnalysisObject*>(nexte()) ) {
    obj->End( fRun );
  }
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

  TObject* obj = 0;
  TString stage = "Decode";
  if( fDoBench ) fBench->Begin(stage);
  TIter next(fApps);
  try {
    while( (obj = next()) ) {
      THaApparatus* theApparatus = static_cast<THaApparatus*>(obj);
      theApparatus->Clear();
      theApparatus->Decode( *fEvData );
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kDecode) )  return kSkip;

    //--- Main physics analysis. Calls the following for each defined apparatus
    //    THaSpectrometer::CoarseTrack  (only for spectrometers)
    //    THaApparatus::CoarseReconstruct
    //    THaSpectrometer::Track        (only for spectrometers)
    //    THaApparatus::Reconstruct
    //
    // Test blocks are evaulated after each of these stages

    //-- Coarse processing

    stage = "CoarseTracking";
    if( fDoBench ) fBench->Begin(stage);
    next.Reset();
    while( (obj = next()) ) {
      THaSpectrometer* theSpectro = dynamic_cast<THaSpectrometer*>(obj);
      if( theSpectro )
	theSpectro->CoarseTrack();
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kCoarseTrack) )  return kSkip;


    stage = "CoarseReconstruct";
    if( fDoBench ) fBench->Begin(stage);
    next.Reset();
    while( (obj = next()) ) {
      THaApparatus* theApparatus = static_cast<THaApparatus*>(obj);
      theApparatus->CoarseReconstruct();
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kCoarseRecon) )  return kSkip;

    //-- Fine (Full) Reconstruct().

    stage = "Tracking";
    if( fDoBench ) fBench->Begin(stage);
    next.Reset();
    while( (obj = next()) ) {
      THaSpectrometer* theSpectro = dynamic_cast<THaSpectrometer*>(obj);
      if( theSpectro )
	theSpectro->Track();
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kTracking) )  return kSkip;


    stage = "Reconstruct";
    if( fDoBench ) fBench->Begin(stage);
    next.Reset();
    while( (obj = next()) ) {
      THaApparatus* theApparatus = static_cast<THaApparatus*>(obj);
      theApparatus->Reconstruct();
    }
    if( fDoBench ) fBench->Stop(stage);
    if( !EvalStage(kReconstruct) )  return kSkip;

    //--- Process the list of physics modules

    stage = "Physics";
    if( fDoBench ) fBench->Begin(stage);
    TIter next_physics(fPhysics);
    while( (obj = next_physics()) ) {
      THaPhysicsModule* theModule = static_cast<THaPhysicsModule*>(obj);
      theModule->Clear();
      Int_t err = theModule->Process( *fEvData );
      if( err == THaPhysicsModule::kTerminate )
	code = kTerminate;
      else if( err == THaPhysicsModule::kFatal ) {
	code = kFatal;
	break;
      }
    }
    if( fDoBench ) fBench->Stop(stage);
    if( code == kFatal ) return kFatal;

    //--- Evaluate "Physics" test block

    if( !EvalStage(kPhysics) )
      // any status code form the physics analysis overrides skip code
      return (code == kOK) ? kSkip : code;

  } // end try
  catch( exception& e ) {
    TString module_name = (obj != 0) ? obj->GetName() : "unknown";
    TString module_desc = (obj != 0) ? obj->GetTitle() : "unknown";
    Error( here, "Caught exception %s in module %s (%s) during %s analysis "
	   "stage. Terminating analysis.", e.what(), module_name.Data(),
	   module_desc.Data(), stage.Data() );
    if( fDoBench ) fBench->Stop(stage);
    code = kFatal;
    goto errexit;
  }

  //---  Process output
  if( fDoBench ) fBench->Begin("Output");
  try {
    //--- If Event defined, fill it.
    if( fEvent ) {
      fEvent->GetHeader()->Set( static_cast<UInt_t>(fEvData->GetEvNum()),
				fEvData->GetEvType(),
				fEvData->GetEvLength(),
				fEvData->GetEvTime(),
				fEvData->GetHelicity(),
				fRun->GetNumber()
				);
      fEvent->Fill();
    }
    // Write to output file
    if( fOutput ) fOutput->Process();
  }
  catch( exception& e ) {
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
  // Analyze slow control (EPICS) data and write then to output.
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
  TIter next(fPostProcess);
  while( THaPostProcess* obj = static_cast<THaPostProcess*>(next())) {
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

  Int_t retval = kOK;

  Incr(kNevGood);

  //--- Evaluate test block "RawDecode"
  bool rawfail = false;
  if( !EvalStage(kRawDecode) ) {
    retval = kSkip;
    rawfail = true;
  }

  TIter nextp(fEvtHandlers);
  while( THaEvtTypeHandler* obj = static_cast<THaEvtTypeHandler*>(nextp()) ) {
    obj->Analyze(fEvData);
  }

  bool evdone = false;
  //=== Physics triggers ===
  if( fEvData->IsPhysicsTrigger() && fDoPhysics ) {
    Incr(kNevPhysics);
    retval = PhysicsAnalysis(retval);
    evdone = true;
  }

  //=== EPICS data ===
  fEvData->SetEpicsEvtType(fEpicsHandler->GetEvtType());
  if( fDoSlowControl && fEpicsHandler->IsMyEvent(fEvData->GetEvType()) ) {
    Incr(kNevEpics);
    retval = SlowControlAnalysis(retval);
    evdone = true;
  }

  //=== Other events ===
  if( !evdone && fDoOtherEvents ) {
    Incr(kNevOther);
    retval = OtherAnalysis(retval);

  } // End trigger type test


  //=== Post-processing (e.g. event filtering) ===
  if( fPostProcess ) {
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
  if( fVerbose>2 && fRun->GetFirstEvent()>1 )
    cout << "Skipping " << fRun->GetFirstEvent() << " events" << endl;

  //--- The main event loop.

  fNev = 0;
  bool terminate = false, fatal = false;
  UInt_t nlast = fRun->GetLastEvent();
  fAnalysisStarted = kTRUE;
  BeginAnalysis();
  if( fFile ) {
    fFile->cd();
    fRun->Write("Run_Data");  // Save run data to first ROOT file
  }

  while ( !terminate && fNev < nlast &&
	  (status = ReadOneEvent()) != THaRunBase::READ_EOF ) {

    //--- Skip events with errors, unless fatal
    if( status == THaRunBase::READ_FATAL )
      break;
    if( status != THaRunBase::READ_OK )
      continue;

    UInt_t evnum = fEvData->GetEvNum();

    // Count events according to the requested mode
    // Whether or not to ignore events prior to fRun->GetFirstEvent()
    // is up to the analysis routines.
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
    if( fVerbose>1 && evnum > 0 && (evnum % fMarkInterval == 0))
      cout << dec << evnum << endl;

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

  //--- Report statistics
  if( fVerbose>0 ) {
    cout << dec;
    if( status == THaRunBase::READ_EOF )
      cout << "End of file";
    else if ( fNev == nlast )
      cout << "Event limit reached.";
    else if ( fatal )
      cout << "Fatal processing error.";
    else if ( terminate )
      cout << "Terminated during processing.";
    cout << endl;

    if( !fatal ) {
      PrintCounters();

      if( fVerbose>1 )
	PrintScalers();
    }
  }

  // Print cut summary (also to file if one given)
  if( !fatal )
    PrintCutSummary();

  // Print timing statistics, if benchmarking enabled
  if( fDoBench && !fatal ) {
    cout << "Timing summary:" << endl;
    fBench->Print("Init");
    fBench->Print("RawDecode");
    fBench->Print("Decode");
    fBench->Print("CoarseTracking");
    fBench->Print("CoarseReconstruct");
    fBench->Print("Tracking");
    fBench->Print("Reconstruct");
    fBench->Print("Physics");
    fBench->Print("Output");
    fBench->Print("Cuts");
  }
  if( (fVerbose>1 || fDoBench) && !fatal )
    fBench->Print("Total");

  //keep the last run available
  //  gHaRun = NULL;
  return fNev;
}

//_____________________________________________________________________________
void THaAnalyzer::SetCodaVersion(Int_t vers)
{
  if (vers != 2 && vers != 3) {
    cout << "ERROR:THaRunBase:SetCodaVersion versions must be 2 or 3"<<endl;
    cout << "Setting version = 2 by default "<<endl;
    fWantCodaVers = 2;
    return;
  }
  fWantCodaVers = vers;
}


//_____________________________________________________________________________

ClassImp(THaAnalyzer)
