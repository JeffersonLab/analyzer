//*-- Author :    Ole Hansen   12/05/2000

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
#include "THaRun.h"
#include "THaEvent.h"
#include "THaOutput.h"
#include "THaEvData.h"
#include "THaGlobals.h"
#include "THaApparatus.h"
#include "THaNamedList.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "THaScalerGroup.h"
#include "THaPhysicsModule.h"
#include "THaCodaData.h"
#include "THaPostProcess.h"
#include "THaBenchmark.h"
#include "TList.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"
#include "TDatime.h"
#include "TClass.h"
#include "TError.h"
#include "TSystem.h"

#include <fstream>
#include <algorithm>

using namespace std;

const char* const THaAnalyzer::kMasterCutName = "master";

// Pointer to single instance of this object
THaAnalyzer* THaAnalyzer::fgAnalyzer = NULL;

//_____________________________________________________________________________
THaAnalyzer::THaAnalyzer() :
  fMaxStage(0), fMaxSkip(0),
  fFile(NULL), fOutput(NULL), fOdefFileName("output.def"), fEvent(NULL),
  fStages(NULL), fSkipCnt(NULL), fNev(0), fMarkInterval(1000), fCompress(1), 
  fDoBench(kFALSE), fBench(NULL), fVerbose(2), 
  fLocalEvent(kFALSE), fIsInit(kFALSE), fAnalysisStarted(kFALSE),
  fUpdateRun(kTRUE), fOverwrite(kTRUE), fPrevEvent(NULL), fRun(NULL),
  fEvData(NULL)
{
  // Default constructor.

  if( fgAnalyzer ) {
    Error("THaAnalyzer", "only one instance of THaAnalyzer allowed.");
    MakeZombie();
    return;
  }
  fgAnalyzer = this;

  InitStages(); // define the cut/accounting stages for this class
                // over-ride this to add more stages

  fBench = new THaBenchmark;
}

//_____________________________________________________________________________
void THaAnalyzer::InitStages()
{
  // Perform the initialization/definition of the stages and cuts
  fMaxStage = kPhysics+1;
  // Initialize descriptions of analysis stages
  fStages = new Stage_t[ fMaxStage ];
  const struct StageDef_t {
    int       key;
    int       skipkey;
    const char* name;
  } stagedef[] = {
    { kPreDecode,   kPreDecodeTest,   "PreDecode" },
    { kRawDecode,   kRawDecodeTest,   "RawDecode" },
    { kDecode,      kDecodeTest,      "Decode" },
    { kCoarseRecon, kCoarseReconTest, "CoarseReconstruct" },
    { kReconstruct, kReconstructTest, "Reconstruct" },
    { kPhysics,     kPhysicsTest,     "Physics" },
    { fMaxStage }
  };
  const StageDef_t* idef = stagedef;
  while( idef->key != fMaxStage ) {
    fStages[ idef->key ].name    = idef->name;
    fStages[ idef->key ].skipkey = idef->skipkey;
    idef++;
  }

  // Initialize event skip statistics counters
  fMaxSkip = kPhysicsTest+1;
  fSkipCnt = new Skip_t[ fMaxSkip ];
  const struct SkipDef_t {
    int         key;
    const char* text;
  } skipdef[] = {
    { kEvFileTrunc,            "truncated event file" },
    { kCodaErr,                "CODA error" },
    { kPreDecodeTest,          "failed master cut at pre-code stage" },
    { kRawDecodeTest,          "failed master cut after raw decoding" },
    { kDecodeTest,             "failed master cut after Decode" },
    { kCoarseReconTest,        "failed master cut after Coarse Reconstruct" },
    { kReconstructTest,        "failed master cut after Reconstruct" },
    { kPhysicsTest,            "failed master cut after Physics" },
    { fMaxSkip }
  };
  const SkipDef_t* jdef = skipdef;
  while( jdef->key != fMaxSkip ) {
    fSkipCnt[ jdef->key ].reason = jdef->text;
    jdef++;
  }
}

//_____________________________________________________________________________
THaAnalyzer::~THaAnalyzer()
{
  // Destructor. 

  Close();
  delete fBench;
  delete [] fStages;
  delete [] fSkipCnt;
  if( fgAnalyzer == this )
    fgAnalyzer = NULL;
}

//_____________________________________________________________________________
void THaAnalyzer::Close()
{
  // Close output files and delete fOutput, fFile, and fRun objects.
  // Also delete fEvent if it was allocated automatically by us.
  
  // Clean-up all Post-process objects
  if ( gHaPostProcess ) {
    TIter nextp(gHaPostProcess);
    TObject *obj;
    while ( (obj=nextp()) ) {
      (static_cast<THaPostProcess*>(obj))->CleanUp();
    }
    delete gHaPostProcess; gHaPostProcess=0;
  }

  if( gHaRun && *gHaRun == *fRun ) 
    gHaRun = NULL;

  delete fEvData; fEvData = NULL;
  delete fOutput; fOutput = NULL;
  delete fFile;   fFile   = NULL;
  delete fRun;    fRun    = NULL;
  if( fLocalEvent ) {
    delete fEvent; fEvent = fPrevEvent = NULL; 
  }
  fNev = 0;
  fIsInit = kFALSE;
  fAnalysisStarted = kFALSE;
}


//_____________________________________________________________________________
void THaAnalyzer::EnableHelicity( Bool_t enable ) 
{
  // Enable/disable helicity decoding
  if( enable )
    SetBit(kHelicityEnabled);
  else
    ResetBit(kHelicityEnabled);
}

//_____________________________________________________________________________
Bool_t THaAnalyzer::HelicityEnabled() const
{
  // Test if helicity decoding enabled
  return TestBit(kHelicityEnabled);
}

//_____________________________________________________________________________
void THaAnalyzer::Print( Option_t* opt ) const
{
  // Report status of the analyzer.

  if( fRun )
    fRun->Print();
}

//_____________________________________________________________________________
void THaAnalyzer::PrintSummary( const THaRun* run ) const
{
  // Print summary of cuts etc.
  // Only print to screen if fVerbose>1, but always print to
  // the summary file if a summary file is requested.

  if( gHaCuts->GetSize() > 0 ) {
    if( fVerbose>1 )
      gHaCuts->Print("STATS");
    if( fSummaryFileName.Length() > 0 ) {
      TString filename(fSummaryFileName);
      Ssiz_t pos, dot=-1;
      while(( pos = filename.Index(".",dot+1)) != kNPOS ) dot=pos;
      const char* tag = Form("_%d",run->GetNumber());
      if( dot != -1 ) 
	filename.Insert(dot,tag);
      else
	filename.Append(tag);
      ofstream ostr(filename.Data());
      if( ostr ) {
	// Write to file via cout
	streambuf* cout_buf = cout.rdbuf();
	cout.rdbuf(ostr.rdbuf());
	TDatime now;
	cout << "Cut Summary for run " << run->GetNumber() 
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
      fSkipCnt[ theStage->skipkey ].count++;
      ret = false;
    }      
  }
  if( fDoBench ) fBench->Stop("Cuts");
  return ret;
}

//_____________________________________________________________________________
void THaAnalyzer::InitCuts()
{
  // Internal function that sets up structures to handle cuts more efficiently:
  //
  // - Find pointers to the THaNamedList lists that hold the cut blocks.
  // - find pointer to each block's master cut

  for( int i=0; i<fMaxStage; i++ ) {
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
Int_t THaAnalyzer::InitModules( const TList* module_list, TDatime& run_time, 
				Int_t erroff, const char* baseclass )
{
  // Initialize a list of THaAnalysisObjects for time 'run_time'.
  // If 'baseclass' given, ensure that each object in the list inherits 
  // from 'baseclass'.

  static const char* const here = "InitModules()";

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
      THaAnalysisObject* theModule = static_cast<THaAnalysisObject*>( obj );
      theModule->Init( run_time );
      if( !theModule->IsOK() ) {
	Error( here, "Error initializing module %s (%s). "
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
Int_t THaAnalyzer::Init( THaRun* run )
{
  // Initialize the analyzer.

  // This is a wrapper so we can conveniently control the benchmark counter
  if( !run ) return -1;

  if( fDoBench ) fBench->Begin("Init");
  Int_t retval = DoInit( run );
  if( fDoBench ) fBench->Stop("Init");
  return retval;
}
  
//_____________________________________________________________________________
Int_t THaAnalyzer::DoInit( THaRun* run )
{
  // Internal function called by Init(). This is where the actual work is done.

  static const char* const here = "Init";
  Int_t retval = 0;

  //--- Open the output file if necessary so that Trees and Histograms
  //    are created on disk.
 
  // File previously created and name changed?
  if( fFile && fOutFileName != fFile->GetName()) {
    if( fAnalysisStarted ) {
      Error( here, "Cannot change output file name after analysis has been "
	     "started. Close() first, then Init() or Process() again." );
      return -11;
    }
    Close();
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
      Error( here, "Failed to create decoder object. Something is very wrong." );
      return 241;
    }
    new_decoder = true;
  }

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

  // Warn user if trying to analyze the same run twice with overlapping
  // event ranges
  // FIXME: generalize event range business
  // FIXME: skip this in batch mode
  if( fAnalysisStarted && !new_run && 
      ((fRun->GetLastEvent()  >= run->GetFirstEvent() &&
        fRun->GetFirstEvent() <  run->GetLastEvent()) ||
       (fRun->GetFirstEvent() <= run->GetLastEvent() &&
	fRun->GetLastEvent()  >  run->GetFirstEvent()) )) {
    Warning( here, "You are analyzing the same run twice with "
	     "overlapping event ranges! prev: %d-%d, now: %d-%d",
	     fRun->GetFirstEvent(), fRun->GetLastEvent(),
	     run->GetFirstEvent(), run->GetLastEvent() );
    cout << "Are you sure (y/n)?" << endl;
    char c = 0;
    while( c != 'y' && c != 'n' && c != EOF ) {
      cin >> c;
    }
    if( c != 'y' && c != 'Y' ) 
      return 240;
  }

  // Make sure we save a copy of the run in fRun.
  // Note that the run may be derived from THaRun, so this is a bit tricky.
  if( new_run ) {
    delete fRun;
    fRun = static_cast<THaRun*>(run->IsA()->New());
    if( !fRun )
      return 252; // urgh
    *fRun = *run;  // Copy the run via its virtual operator=
  }

  // Print run info
  if( fVerbose>0 ) {
    run->Print("STARTINFO");
  }

  // Clear counters unless we are continuing an analysis
  if( !fAnalysisStarted ) {
    for( int i=0; i<fMaxSkip; i++ )
      fSkipCnt[i].count = 0;
  }

  //---- If previously initialized and nothing changed, we are done----
  if( !need_init ) 
    return 0;

  // Obtain time of the run, the one parameter we really need 
  // for initializing the modules
  TDatime run_time = run->GetDate();

  // Tell the decoder the run time. This will trigger decoder
  // initialization (reading of crate map data etc.)
  fEvData->SetRunTime( run_time.Convert());

  // Initialize all apparatuses, scalers, and physics modules.
  // Quit if any errors.
  if( !((retval = InitModules( gHaApps,    run_time, 20, "THaApparatus")) ||
	(retval = InitModules( gHaScalers, run_time, 30, "THaScalerGroup")) ||
	(retval = InitModules( gHaPhysics, run_time, 40, "THaPhysicsModule"))
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

    // Post-process has to be initialized after all cuts are known
    if ( gHaPostProcess ) {
      TIter nextp(gHaPostProcess);
      TObject *obj;
      while ( !retval && (obj=nextp()) ) {
	retval = (static_cast<THaPostProcess*>(obj))->Init(run_time);
      }
    }
  }

  // If initialization succeeded, set status flags accordingly
  if( retval == 0 ) {
    fIsInit = kTRUE;
  }
  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::ReadOneEvent( THaRun* run, THaEvData* evdata )
{
  // Read one event from 'run' into 'evdata'.

  if( fDoBench ) fBench->Begin("RawDecode");

  // Find next event buffer in CODA file. Quit if error.
  Int_t status = run->ReadEvent();
  if (status != S_SUCCESS) {
    if (status == S_EVFILE_TRUNC) 
      fSkipCnt[kEvFileTrunc].count++;
    else if (status == CODA_ERROR) 
      fSkipCnt[kCodaErr].count++;
  } else {

    // Decode the event
    // FIXME: return code mixup with above
    status = evdata->LoadEvent( run->GetEvBuffer() );

    // Count good events
    fNev++;
  }

  if( fDoBench ) fBench->Stop("RawDecode");
  return status;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::Process( THaRun* run )
{
  // Process the given run. Loop over all events in the event range and
  // analyze all apparatuses defined in the global apparatus list.
  // Fill Event structure if it is defined.
  // If Event and Filename are defined, then fill the output tree with Event
  // and write the file.

  static const char* const here = "Process()";

  if( !run ) {
    if( fRun )
      run = fRun;
    else
      return -1;
  }
  fBench->Reset();
  fBench->Begin("Total");

  //--- Initialization. Creates fFile, fOutput, and fEvent if necessary.
  Int_t status = Init( run );
  if( status != 0 ) {
    fBench->Stop("Total");
    return status;
  }
  
  //--- Re-open the CODA file. Should succeed since this was tested in Init().
  if( (status = run->OpenFile()) ) {
    Error( here, "Failed to re-open the input file. "
	   "Make sure the file still exists.");
    fBench->Stop("Total");
    return -4;
  }

  // Make the current run available globally - the run parameters are
  // needed by some modules
  gHaRun = run;

  // Enable/disable helicity decoding as requested
  fEvData->EnableHelicity( HelicityEnabled() );
  // Decode scalers only if gHaScalers is not empty
  fEvData->EnableScalers( (gHaScalers->GetSize()>0) );

  // Informational messages
  if( fVerbose>1 ) {
    cout << "Decoder: helicity " 
	 << (fEvData->HelicityEnabled() ? "enabled" : "disabled")
	 << endl;
    cout << "Decoder: scalers " 
	 << (fEvData->ScalersEnabled() ? "enabled" : "disabled")
	 << endl;
    cout << endl << "Starting analysis" << endl;
  }
  if( fVerbose>2 && run->GetFirstEvent()>1 )
    cout << "Skipping " << run->GetFirstEvent() << " events" << endl;

  //--- The main event loop.
  TIter next( gHaApps );
  TIter next_scaler( gHaScalers );
  TIter next_physics( gHaPhysics );

  fNev = 0;
  UInt_t nev_physics = 0, nev_analyzed = 0;
  UInt_t nlast = run->GetLastEvent();
  bool terminate = false;
  bool fatal = false;
  bool first = true;
  fAnalysisStarted = kTRUE;
  while ( !terminate && (status = ReadOneEvent( run, fEvData )) != EOF && 
	  nev_physics < nlast ) {

    //--- Skip bad events.
    if( status )
      continue;

    UInt_t evnum = fEvData->GetEvNum();

    //--- Print marks periodically
    if( fVerbose>1 && evnum > 0 && (evnum % fMarkInterval == 0))  
      cout << dec << evnum << endl;

    //--- Update run parameters
    if( fUpdateRun )
      run->Update( fEvData );

    //--- Evaluate test block "PreDecode" (THaEvData variables available)
    if( !EvalStage(kPreDecode) ) continue;

    //=== Physics triggers ===
    if( fEvData->IsPhysicsTrigger()) {
      nev_physics++;

      // Skip physics events until we reach the first requested event
      if( nev_physics < run->GetFirstEvent())
	continue;
      if( first ) {
	first = false;
	if( fVerbose>2 )
	  cout << "Starting physics analysis at event " 
	       << nev_physics << endl;
      }
      nev_analyzed++;

      // Update counters in the run object
      run->IncrNumAnalyzed();

      //--- Evaluate test block "RawDecode"

      if( !EvalStage(kRawDecode) ) continue;

      //--- Process all apparatuses that are defined in the global list gHaApps
      //    First Decode() everything, then Reconstruct()

      if( fDoBench ) fBench->Begin("Decode");
      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->Clear();
	theApparatus->Decode( *fEvData );
      }
      if( fDoBench ) fBench->Stop("Decode");
      if( !EvalStage(kDecode) )  continue;

      //--- 2nd step: Coarse Reconstruct(). This is mostly VDC tracking.

      if( fDoBench ) fBench->Begin("Reconstruct");
      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->CoarseReconstruct();
      }
      if( fDoBench ) fBench->Stop("Reconstruct");
      if( !EvalStage(kCoarseRecon) )  continue;

      //-- 3rd step: Fine (Full) Reconstruct().
      if( fDoBench ) fBench->Begin("Reconstruct");
      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->Reconstruct();
      }
      if( fDoBench ) fBench->Stop("Reconstruct");
      if( !EvalStage(kReconstruct) )  continue;

      //--- Process the list of physics modules

      if( fDoBench ) fBench->Begin("Physics");
      next_physics.Reset();
      while( THaPhysicsModule* theModule =
	     static_cast<THaPhysicsModule*>( next_physics() )) {
	theModule->Clear();
	Int_t err = theModule->Process( *fEvData );
	if( err == THaPhysicsModule::kTerminate )
	  terminate = true;
	else if ( err == THaPhysicsModule::kFatal ) {
	  terminate = fatal = true;
	  break;
	}
      }
      if( fDoBench ) fBench->Stop("Physics");
      if( fatal ) continue;

      //--- Evaluate "Physics" test block

      if( !EvalStage(kPhysics) )  continue;

      //--- If Event defined, fill it.
      if( fDoBench ) fBench->Begin("Output");
      if( fEvent ) {
	fEvent->GetHeader()->Set( (UInt_t)fEvData->GetEvNum(), 
				  fEvData->GetEvType(),
				  fEvData->GetEvLength(),
				  fEvData->GetEvTime(),
				  fEvData->GetHelicity(),
				  run->GetNumber()
				  );
	fEvent->Fill();
      }

      //---  Process output
      if( fOutput ) fOutput->Process();
      if( fDoBench ) fBench->Stop("Output");

      //--- Post-process for physics events
      if (gHaPostProcess) {
	TIter nextp(gHaPostProcess);
	TObject *obj;
	while ( (obj=nextp()) ) {
	  (static_cast<THaPostProcess*>(obj))->Process(run);
	}
      }
    }

    //=== EPICS data ===
    else if( fEvData->IsEpicsEvent()) {

      if ( fOutput ) fOutput->ProcEpics(fEvData);

    }

    //=== Scaler triggers ===
    else if( fEvData->IsScalerEvent()) {
      nev_analyzed++;

      //--- Loop over all defined scalers and execute LoadData()

      if( fDoBench ) fBench->Begin("Scaler");
      next_scaler.Reset();
      while( THaScalerGroup* theScaler =
	     static_cast<THaScalerGroup*>( next_scaler() )) {
	theScaler->LoadData( *fEvData );
	if ( fOutput ) fOutput->ProcScaler(theScaler);
      }
      if( fDoBench ) fBench->Begin("Scaler");

    } // End trigger type test

    // for all non-physics trigger-types, force processing by PostProcess
    if ( ! fEvData->IsPhysicsTrigger() && gHaPostProcess ) {
      TIter nextp(gHaPostProcess);
      TObject *obj;
      while ( (obj=nextp()) ) {
	(static_cast<THaPostProcess*>(obj))->Process(run,1);
      }
    }

  }  // End of event loop
  
  // Save final run parameters locally
  *fRun = *run;

  fBench->Stop("Total");

  //--- Report statistics

  if( fVerbose>0 ) {
    cout << dec;
    if( status == EOF )
      cout << "End of file";
    else if ( nev_physics == nlast )
      cout << "Event limit reached.";
    else if ( fatal )
      cout << "Fatal processing error.";
    else if ( terminate )
      cout << "Terminated during processing.";
    cout << endl;

    cout << "Processed " << fNev << " events, " 
	 << nev_analyzed << " data events, " 
	 << nev_physics << " physics events. " << endl;

    if( fVerbose>1 ) {
      for (int i = 0; i < fMaxSkip; i++) {
	if (fSkipCnt[i].count != 0) 
	  cout << "Skipped " << fSkipCnt[i].count
	       << " events due to " << fSkipCnt[i].reason << endl;
      }

      // Print scaler statistics

      first = true;
      next_scaler.Reset();
      while( THaScalerGroup* theScaler =
	     static_cast<THaScalerGroup*>( next_scaler() )) {
	if( !first ) 
	  cout << endl;
	else
	  first = false;
	theScaler->PrintSummary();
      }
      if( !first ) cout << endl;
    }
  }

  // Print timing statistics, if benchmarking enabled

  if( fDoBench ) {
    fBench->Print("Prestart");
    fBench->Print("Init");
    fBench->Print("RawDecode");
    fBench->Print("Decode");
    fBench->Print("Reconstruct");
    fBench->Print("Physics");
    fBench->Print("Output");
    fBench->Print("Cuts");
    fBench->Print("Scaler");
  }
  if( fVerbose>1 || fDoBench )
    fBench->Print("Total");
  
  //--- Close the input file
  run->CloseFile();

  // Write the output file and clean up.
  // This writes the Tree as well as any objects (histograms etc.)
  // that are defined in the current directory.

  if( fOutput ) fOutput->End();
  if( fFile ) {
    run->Write("Run_Data");  // Save run data to ROOT file
    //    fFile->Write();//already done by fOutput->End()
    fFile->Purge();         // get rid of excess object "cycles"
  }

  // Print cut summary (also to file if one given)
  PrintSummary(run);

  //keep the last run available
  //  gHaRun = NULL;
  return nev_physics;
}
//_____________________________________________________________________________

ClassImp(THaAnalyzer)

