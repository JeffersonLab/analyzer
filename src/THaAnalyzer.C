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
#include "TList.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"
#include "THaBenchmark.h"
#include "TDatime.h"
#include "TClass.h"
#include "TError.h"

#include <fstream>
#include <algorithm>

// FIXME: Debug
#include "THaVarList.h"

using namespace std;

const char* const THaAnalyzer::kMasterCutName = "master";

//_____________________________________________________________________________
THaAnalyzer::THaAnalyzer() :
  fFile(NULL), fOutput(NULL), fOdefFileName("output.def"), fEvent(0), fNev(0),
  fMarkInterval(1000), fCompress(1), fDoBench(kFALSE), fVerbose(1), 
  fLocalEvent(kFALSE), fIsInit(kFALSE), fPrevEvent(NULL), fRun(NULL)
{
  // Default constructor.

  // FIXME: make sure this object is a singleton since it uses/modifies
  //  global objects (gHaVars, gHaRun etc.)

  // Initialize descriptions of analysis stages

  fStages = new Stage_t[ kMaxStage ];
  const struct StageDef_t {
    EStage      key;
    ESkipReason skipkey;
    const char* name;
  } stagedef[] = {
    { kRawDecode,   kRawDecodeTest,   "RawDecode" },
    { kDecode,      kDecodeTest,      "Decode" },
    { kCoarseRecon, kCoarseReconTest, "CoarseReconstruct" },
    { kReconstruct, kReconstructTest, "Reconstruct" },
    { kPhysics,     kPhysicsTest,     "Physics" },
    { kMaxStage }
  };
  const StageDef_t* idef = stagedef;
  while( idef->key != kMaxStage ) {
    fStages[ idef->key ].name    = idef->name;
    fStages[ idef->key ].skipkey = idef->skipkey;
    idef++;
  }

  // Initialize event skip statistics counters
  fSkipCnt = new Skip_t[ kMaxSkip ];
  const struct SkipDef_t {
    ESkipReason key;
    const char* text;
  } skipdef[] = {
    { kEvFileTrunc,            "truncated event file" },
    { kCodaErr,                "CODA error" },
    { kRawDecodeTest,          "failed master cut after raw decoding" },
    { kDecodeTest,             "failed master cut after Decode" },
    { kCoarseReconTest,        "failed master cut after Coarse Reconstruct" },
    { kReconstructTest,        "failed master cut after Reconstruct" },
    { kPhysicsTest,            "failed master cut after Physics" },
    { kMaxSkip }
  };
  const SkipDef_t* jdef = skipdef;
  while( jdef->key != kMaxSkip ) {
    fSkipCnt[ jdef->key ].reason = jdef->text;
    jdef++;
  }

  // Global variables
  if( gHaVars ) {
    gHaVars->Define("nev", "Event number", fNev );
  }

  fBench = new THaBenchmark;
}

//_____________________________________________________________________________
THaAnalyzer::~THaAnalyzer()
{
  // Destructor. 

  Close();
  delete fBench;
  delete [] fStages;
  delete [] fSkipCnt;

  if( gHaVars )
    gHaVars->RemoveName("nev");
}

//_____________________________________________________________________________
void THaAnalyzer::Close()
{
  // Close output files and delete fOutput, fFile, and fRun objects.
  // Also delete fEvent if it was allocated automatically by us.
  
  delete fOutput; fOutput = NULL;
  delete fFile;   fFile   = NULL;
  delete fRun;    fRun    = NULL;
  if( fLocalEvent ) {
    delete fEvent; fEvent = fPrevEvent = NULL; 
  }
  gHaRun = NULL;
  fNev = 0;
  fIsInit = kFALSE;
  fAnalysisStarted = kFALSE;
}


//_____________________________________________________________________________
bool THaAnalyzer::EvalStage( EStage n )
{
  // Fill histogram block for analysis stage 'n', then evaluate cut block.
  // Return 'false' if master cut is not true, 'true' otherwise.
  // The result can be used to skip further processing of the current event.
  // If event is skipped, increment associated statistics counter.
  // Call InitCuts() before using!  This is an internal function.

  if( fDoBench ) fBench->Begin("Cuts");

  const Stage_t* theStage = fStages+n;

  //  if( theStage->hist_list ) {
    // Fill histograms
  //  }

  if( theStage->cut_list ) {
    gHaCuts->EvalBlock( theStage->cut_list );
    if( theStage->master_cut && 
	!theStage->master_cut->GetResult() ) {
      fSkipCnt[ theStage->skipkey ].count++;
      if( fDoBench ) fBench->Stop("Cuts");
      return false;
    }      
  }
  if( fDoBench ) fBench->Stop("Cuts");
  return true;
}

//_____________________________________________________________________________
void THaAnalyzer::InitCuts()
{
  // Internal function that sets up structures to handle cuts more efficiently:
  //
  // - Find pointers to the THaNamedList lists that hold the cut blocks.
  // - find pointer to each block's master cut

  for( int i=0; i<kMaxStage; i++ ) {
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
Int_t THaAnalyzer::InitModules( TIter& next, TDatime& run_time, Int_t erroff,
				const char* baseclass )
{
  // Initialize a list of THaAnalysisObjects for time 'run_time'.
  // If 'baseclass' given, ensure that each object in the list inherits 
  // from 'baseclass'.

  static const char* const here = "InitModules()";

  bool fail = false;
  Int_t retval = 0;
  TObject* obj;
  while( !fail && (obj = next())) {
    if( baseclass && !obj->IsA()->InheritsFrom( baseclass )) {
      Error( here, "Object %s (%s) is not a %s. Analyzer initialization failed.", 
	     obj->GetName(), obj->GetTitle(), baseclass );
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
Int_t THaAnalyzer::Init( THaRun& run )
{
  // Initialize the analyzer.

  // This is a wrapper so we can conveniently control the benchmark counter
  if( fDoBench ) fBench->Begin("Init");
  Int_t retval = DoInit( run );
  if( fDoBench ) fBench->Stop("Init");
  return retval;
}
  
//_____________________________________________________________________________
Int_t THaAnalyzer::DoInit( THaRun& run )
{
  // Internal function called by Init(). This is where the actual work is done.

  static const char* const here = "Init()";
  Int_t retval = 0;

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

  // Make sure the run is initizlized. 
  if( !run.IsInit()) {
    retval = run.Init();
    if( retval )
      return retval;  //Error message printed by run class
  } 

  // Deal with the run. Possible scenarios:
  // - First time init or re-init after Close():
  //        Init the run, extract time, full init of modules & output.
  // - Re-init without analysis (e.g. user changed his mind which run to
  //   analyze):
  //        Different run (or run modified):
  //             Init run, extract time, if changed, re-init everything
  //        else
  //             no re-init
  // - Re-init with prior analysis (continuing):
  //        Same run:
  //             no re-init 
  //             warn if user tries to analyze same data twice
  //        Different run:
  //             Init run, extract time
  //             Special values of time:
  //               -"continuation" (e.g. 2nd segment) - do not re-init
  //               (-"now" - use current time )
  //

  bool new_run   = ( !fRun || *fRun != run );
  bool need_init = ( !fIsInit || new_event || new_output || new_run );

  // Warn user if trying to analyze the same run twice with overlapping
  // event ranges
  if( fAnalysisStarted && !new_run && 
      (fRun->GetLastEvent() >= run.GetFirstEvent() ||
       run.GetLastEvent() >= fRun->GetFirstEvent() )) {
    Warning( here, "You are analyzing the same run twice with ",
	     "overlapping event ranges!\n"
	     "prev: %d-%d, now: %d-%d",
	     fRun->GetFirstEvent(), fRun->GetLastEvent(),
	     run.GetFirstEvent(), run.GetLastEvent() );
    cout << "Are you sure (y/n)?" << endl;
    char c = 0;
    while( c != 'y' && c != 'n' && c != EOF ) {
      cin >> c;
    }
    if( c != 'y' ) 
      return 240;
  }

  // Make sure we save a copy of the run in fRun.
  // Note that run may derive from THaRun, so this is a bit tricky.
  if( new_run ) {
    delete fRun;
    fRun = (THaRun*)run.IsA()->New();
    if( !fRun )
      return 252; // urgh
    *fRun = run;  // Copy the run via its virtual operator=
  }

  // Print run info
  if( fVerbose ) {
    run.Print("STARTINFO");
  }

  // Clear counters unless we are continuing an analysis
  if( !fAnalysisStarted ) {
    for( int i=0; i<kMaxSkip; i++ )
      fSkipCnt[i].count = 0;
  }

  //---- If previously initialized and nothing changed, we are done----
  if( !need_init ) 
    return 0;

  // Obtain time of the run, the one parameter we really need 
  // for initializing the modules
  TDatime run_time = run.GetDate();

  // Initialize all apparatuses, scalers, and physics modules.
  // Quit if any errors.
  TIter next( gHaApps );
  TIter next_scaler( gHaScalers );
  TIter next_physics( gHaPhysics );
  if( !((retval = InitModules( next,         run_time, 20, "THaApparatus")) ||
	(retval = InitModules( next_scaler,  run_time, 30, "THaScalerGroup")) ||
	(retval = InitModules( next_physics, run_time, 40, "THaPhysicsModule"))
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
  }

  // If initialization succeeded, set status flags accordingly
  if( retval == 0 ) {
    fIsInit = kTRUE;
  }
  return retval;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::ReadOneEvent( THaRun& run, THaEvData& evdata )
{
  // Read one event from 'run' into 'evdata'.

  if( fDoBench ) fBench->Begin("RawDecode");

  // Find next event buffer in CODA file. Quit if error.
  Int_t status = run.ReadEvent();
  if (status != 0) {
    if (status == S_EVFILE_TRUNC) 
      fSkipCnt[kEvFileTrunc].count++;
    else if (status == CODA_ERROR) 
      fSkipCnt[kCodaErr].count++;
    return status;
  } 

  // Decode the event
  evdata.LoadEvent( run.GetEvBuffer() );

  // Count good events
  fNev++;    

  if( fDoBench ) fBench->Stop("RawDecode");
  return 0;
}

//_____________________________________________________________________________
Int_t THaAnalyzer::Process( THaRun& run )
{
  // Process the given run. Loop over all events in the event range and
  // analyze all apparatuses defined in the global apparatus list.
  // Fill Event structure if it is defined.
  // If Event and Filename are defined, then fill the output tree with Event
  // and write the file.

  //  static const char* const here = "Process()";

  fBench->Reset();
  fBench->Begin("Total");

  //--- Initialization.
  Int_t status = Init( run );
  if( status != 0 ) {
    fBench->Stop("Total");
    return status;
  }
  
  //--- If output file is defined, open it.
 
  if( !fFile && !fOutFileName.IsNull() ) {
    cout << "Creating new file: " << fOutFileName << endl;
    fFile = new TFile( fOutFileName.Data(), "RECREATE" );
  } 
  // filename changed and file open? -> Close file and create a new one
  else if ( fFile && fOutFileName != fFile->GetName()) {
    Close();
    cout << "Creating new file: " << fOutFileName << endl;
    fFile = new TFile( fOutFileName.Data(), "RECREATE" );
  }
  // FIXME: case of no file and no file name?
  // At least print some error
    
  if( !fFile || fFile->IsZombie() ) {
    Close();
    fBench->Stop("Total");
    return -10;
  }
  fFile->SetCompressionLevel(fCompress);

  UInt_t nev_physics = 0;
  UInt_t nlim = run.GetLastEvent();

  THaEvData evdata;
  TIter next( gHaApps );
  TIter next_scaler( gHaScalers );
  TIter next_physics( gHaPhysics );

  // Re-open the CODA file. Should succeed since this was tested in Init().
  if( (status = run.OpenFile()) )
    return -11;

  // Make the current run available globally - used by some modules
  gHaRun = &run;

  //--- The main event loop.
  bool terminate = false;
  bool fatal = false;
  while ( !terminate && 
	  (status != EOF && status != CODA_ERROR) && nev_physics < nlim ) {

    //--- Read one event from "run" into "evdata". Skip the event if error.
    if( ReadOneEvent( run, evdata )) 
      continue;
    
    //--- Print marks periodically
    if( fVerbose && (fNev % fMarkInterval == 0))  
      cout << dec << fNev << endl;

    //=== Physics triggers ===
    if( evdata.IsPhysicsTrigger()) {

      nev_physics++;
      if( nev_physics < run.GetFirstEvent() ) continue;

      //--- Evaluate test block 0 "RawDecode"

      if( !EvalStage(kRawDecode) ) continue;

      //--- Process all apparatuses that are defined in the global list gHaApps
      //    First Decode() everything, then Reconstruct()

      if( fDoBench ) fBench->Begin("Decode");
      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->Clear();
	theApparatus->Decode( evdata );
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
	Int_t err = theModule->Process( evdata );
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
	fEvent->GetHeader()->Set( (UInt_t)evdata.GetEvNum(), 
				  evdata.GetEvType(),
				  evdata.GetEvLength(),
				  evdata.GetEvTime(),
				  evdata.GetHelicity(),
				  evdata.GetRunNum()
				  );
	fEvent->Fill();
      }

      //---  Process output
      if( fOutput ) fOutput->Process();
      if( fDoBench ) fBench->Stop("Output");

    }

    //=== Scaler triggers ===
    else if( evdata.IsScalerEvent()) {

      //--- Loop over all defined scalers and execute LoadData()

      if( fDoBench ) fBench->Begin("Scaler");
      next_scaler.Reset();
      while( THaScalerGroup* theScaler =
	     static_cast<THaScalerGroup*>( next_scaler() )) {
	theScaler->LoadData( evdata );
      }
      if( fDoBench ) fBench->Begin("Scaler");

    } // End trigger type test
  }  // End of event loop
  
  fBench->Stop("Total");

  //--- Report statistics

  cout << dec;
  if( status == EOF )
    cout << "End of file";
  else if ( nev_physics == nlim )
    cout << "Event limit reached.";
  else if ( fatal )
    cout << "Fatal processing error.";
  else if ( terminate )
    cout << "Terminated during processing.";
  cout << endl;

  cout << "Processed " << fNev << " events, " 
       << nev_physics << " physics events.\n";

  for (int i = 0; i < kMaxSkip; i++) {
    if (fSkipCnt[i].count != 0) 
      cout << "Skipped " << fSkipCnt[i].count
	   << " events due to " << fSkipCnt[i].reason << endl;
  }

  // Print scaler statistics

  bool first = true;
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

  // Print timing statistics

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
  fBench->Print("Total");

  //--- Close the input file

  run.CloseFile();

  // Write the output file and clean up.
  // This writes the Tree as well as any objects (histograms etc.)
  // that are defined in the current directory.

  if( fOutput ) fOutput->End();
  if( fFile ) {
    run.Write("Run_Data");  // Save run data to ROOT file
    fFile->Write();
    fFile->Purge();         // get rid of excess object "cycles"
  }

  // Print cut summary (also to file if one given)
  if( gHaCuts->GetSize() > 0 ) {
    gHaCuts->Print("STATS");
    if( fSummaryFileName.Length() > 0 ) {
      TString filename(fSummaryFileName);
      Ssiz_t pos, dot=-1;
      while(( pos = filename.Index(".",dot+1)) != kNPOS ) dot=pos;
      const char* tag = Form("_%d",run.GetNumber());
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
	cout << "Cut Summary for run " << run.GetNumber() 
	     << " completed " << now.AsString() 
	     << endl << endl;
	gHaCuts->Print("STATS");
	cout.rdbuf(cout_buf);
	ostr.close();
      }
    }
  }

  gHaRun = NULL;
  return nev_physics;
}
//_____________________________________________________________________________

ClassImp(THaAnalyzer)

