//*-- Author :    Ole Hansen   12/05/2000

//////////////////////////////////////////////////////////////////////////
//
// THaAnalyzer
//
// Base class for a "Hall A analyzer" class and default "analyzer".
// Performs standard analysis: Decode, Reconstruct, Process.
// If user has defined an Event class, executes its Fill() method
// and writes it to the output file.
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
#include "evio.h"
#include "THaCodaData.h"
#include "TList.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"
#include "TBenchmark.h"
#include "TDatime.h"
#include "TClass.h"
#include "TError.h"

#include <iostream>
#include <algorithm>

// FIXME: Debug
#include "THaVarList.h"

ClassImp(THaAnalyzer)

const char* const THaAnalyzer::kMasterCutName = "master";

//_____________________________________________________________________________
THaAnalyzer::THaAnalyzer() :
  fFile(NULL), fOutput(NULL), fOdefFileName("output.def"), fEvent(0), fNev(0)
{
  // Default constructor.

  // Initialize descriptions of analysis stages

  fStages = new Stage_t[ kMaxStage ];
  struct StageDef_t {
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
  StageDef_t* idef = stagedef;
  while( idef->key != kMaxStage ) {
    fStages[ idef->key ].name    = idef->name;
    fStages[ idef->key ].skipkey = idef->skipkey;
    idef++;
  }

  // Initialize event skip statistics counters
  fSkipCnt = new Skip_t[ kMaxSkip ];
  struct SkipDef_t {
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
  SkipDef_t* jdef = skipdef;
  while( jdef->key != kMaxSkip ) {
    fSkipCnt[ jdef->key ].reason = jdef->text;
    jdef++;
  }

  // Global variables
  if( gHaVars ) {
    gHaVars->Define("nev", "Event number", fNev );
  }
}

//_____________________________________________________________________________
THaAnalyzer::~THaAnalyzer()
{
  // Destructor. Does not delete the fEvent object since
  // they are defined by the caller.

  Close();
  delete [] fStages;
  delete [] fSkipCnt;

  if( gHaVars )
    gHaVars->RemoveName("nev");
}

//_____________________________________________________________________________
void THaAnalyzer::Close()
{
  // Close output file and delete ROOT tree if they were created.
  
  delete fOutput;
  delete fFile;  
  fOutput = 0;
  fFile = 0;
}


//_____________________________________________________________________________
inline
bool THaAnalyzer::EvalStage( EStage n )
{
  // Fill histogram block for analysis stage 'n', then evaluate cut block.
  // Return 'false' if master cut is not true, 'true' otherwise.
  // The result can be used to skip further processing of the current event.
  // If event is skipped, increment associated statistics counter.
  // Call InitCuts() before using!  This is an internal function.

  Stage_t* theStage = fStages+n;

  //  if( theStage->hist_list ) {
    // Fill histograms
  //  }

  if( theStage->cut_list ) {
    gHaCuts->EvalBlock( theStage->cut_list );
    if( theStage->master_cut && 
	!theStage->master_cut->GetResult() ) {
      fSkipCnt[ theStage->skipkey ].count++;
      return false;
    }      
  }
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
Int_t THaAnalyzer::Process( THaRun& run )
{
  // Process the given run. Loop over all events in the event range and
  // analyze all apparatuses defined in the global apparatus list.
  // Fill Event structure if it is defined.
  // If Event and Filename are defined, then fill the output tree with Event
  // and write the file.

  static const char* const here = "Process()";

  Int_t status;
  status = run.OpenFile();
  if( status < 0 ) return status;

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
    
  if( fFile && fFile->IsZombie() ) {
    delete fFile;
    run.CloseFile();
    return -10;
  }
  //  fFile->SetCompressionLevel(0);

  //--- Output tree and histograms

  TTree* outputTree = NULL;
  if( fEvent) fEvent->Reset();
  if( !fOutput ) fOutput = new THaOutput();

  //--- Initialize counters

  fNev = 0;
  for( int i=0; i<kMaxSkip; i++ )
    fSkipCnt[i].count = 0;
  UInt_t nev_physics = 0;
  UInt_t nlim = run.GetLastEvent();
  bool verbose = true, first = true;

  THaEvData evdata;
  TIter next( gHaApps );
  TIter next_scaler( gHaScalers );
  TIter next_physics( gHaPhysics );
  TDatime run_time;

  //--- The main event loop.

  TBenchmark bench;
  bench.Start("Statistics");

  status = 0;
  bool terminate = false;
  bool fatal = false;
  while ( !terminate && 
	  (status != EOF && status != CODA_ERROR) && nev_physics < nlim ) {

    status = run.ReadEvent();

    if (status != 0) {
      if (status == S_EVFILE_TRUNC) 
          fSkipCnt[kEvFileTrunc].count++;
      else if (status == CODA_ERROR) 
          fSkipCnt[kCodaErr].count++;
      continue;  // skip event
    } 

    fNev++;
    evdata.LoadEvent( run.GetEvBuffer() );

    //--- Initialization

    if ( first ) {
      // Must get a prestart event before we can initialize
      // because the prestart event contains the run time.
      // FIXME: Isn't this is overly restrictive?
      if( !evdata.IsPrestartEvent() ) continue;

      run_time.Set( evdata.GetRunTime() );
      run.SetDate( run_time );
      run.SetNumber( evdata.GetRunNum() );
      run.ReadDatabase();
      gHaRun = &run;

      if( verbose ) {
	cout << "Run Number: " << run.GetNumber() << endl;
	cout << "Run Time  : " << run_time.AsString() << endl;
	//	cout << "Run Type  : " << evdata.GetRunType() << endl;
      }
      first = false;

      // save run data to ROOT file
      // FIXME: give this different name?
      run.Write("Run_Data");

      // Initialize all apparatuses, scalers, and physics modules.
      // Quit if any errors.
      Int_t retval = 0;
      if( !((retval = InitModules( next,         run_time, 20, "THaApparatus")) ||
	    (retval = InitModules( next_scaler,  run_time, 30, "THaScalerGroup")) ||
	    (retval = InitModules( next_physics, run_time, 40, "THaPhysicsModule"))
	    )) {
	
	// Set up cuts here, now that all global variables are available
	
	if( !fCutFileName.IsNull() ) 
	  gHaCuts->Load( fCutFileName );
	InitCuts();

	// fOutput must be initialized after all apparatuses are
	// initialized and before adding anything to its tree.

	if( (retval = fOutput->Init( fOdefFileName )) < 0 ) {
	  Error( here, "Error initializing THaOutput." );
	} else if( retval == 1 ) 
	  retval = 0;  // Ignore re-initialization attempt
	else {
	  outputTree = fOutput->GetTree();
	  if( fEvent && outputTree ) 
	    outputTree->Branch( "Event_Branch", fEvent->IsA()->GetName(), 
				&fEvent, 16000, 99 );
	}
      }
      if( retval ) {
        delete fOutput;
        delete fFile;
	run.CloseFile();
	return retval;
      }

    } //if(first)

    // Print marks every 1000 events

    if( verbose && (fNev%1000 == 0))  
      cout << dec << fNev << endl;

    //=== Physics triggers ===

    if( evdata.IsPhysicsTrigger()) {

      nev_physics++;
      if( nev_physics < run.GetFirstEvent() ) continue;

      //--- Evaluate test block 0 "RawDecode"

      if( !EvalStage(kRawDecode) ) continue;

      //--- Process all apparatuses that are defined in the global list gHaApps
      //    First Decode() everything, then Reconstruct()

      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->Clear();
	theApparatus->Decode( evdata );
      }
      if( !EvalStage(kDecode) )  continue;

      //--- 2nd step: Coarse Reconstruct(). This is mostly VDC tracking.

      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->CoarseReconstruct();
      }
      if( !EvalStage(kCoarseRecon) )  continue;

      //-- 3rd step: Fine (Full) Reconstruct().
      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->Reconstruct();
      }
      if( !EvalStage(kReconstruct) )  continue;

      //--- Process the list of physics modules

      next_physics.Reset();
      while( THaPhysicsModule* theModule =
	     static_cast<THaPhysicsModule*>( next_physics() )) {
	theModule->Clear();
	Int_t err = theModule->Process();
	if( err == THaPhysicsModule::kTerminate )
	  terminate = true;
	else if ( err == THaPhysicsModule::kFatal ) {
	  terminate = fatal = true;
	  break;
	}
      }
      if( fatal ) continue;

      //--- Evaluate test block 3 "Physics"

      if( !EvalStage(kPhysics) )  continue;

      //--- If Event defined, fill it.
      if( fEvent ) {
	fEvent->GetHeader()->Set( evdata.GetEvNum(), 
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

    }

    //=== Scaler triggers ===
    else if( evdata.IsScalerEvent()) {

      //--- Loop over all defined scalers and execute LoadData()

      next_scaler.Reset();
      while( THaScalerGroup* theScaler =
	     static_cast<THaScalerGroup*>( next_scaler() )) {
	theScaler->LoadData( evdata );
      }

    } // End trigger type test
  }  // End of event loop
  
  bench.Stop("Statistics");

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

  // Print cut summary

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

  // Print timing statistics

  bench.Print("Statistics");

  //--- Close the input file

  run.CloseFile();

  // Write the output file and clean up.
  // This writes the Tree as well as any objects (histograms etc.)
  // that are defined in the current directory.

  if( fOutput ) fOutput->End();
  if( fFile ) fFile->Write();

  gHaRun = NULL;

  return nev_physics;
}
