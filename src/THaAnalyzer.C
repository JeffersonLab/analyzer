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
//#include "THaPhysics.h"
#include "THaEvent.h"
#include "THaEvData.h"
#include "THaGlobals.h"
#include "THaApparatus.h"
#include "THaNamedList.h"
#include "THaCutList.h"
#include "THaScaler.h"
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
#include <cstring>

// FIXME: Debug
#include "THaVarList.h"

ClassImp(THaAnalyzer)

const char* const THaAnalyzer::kMasterCutName = "master";

//_____________________________________________________________________________
THaAnalyzer::THaAnalyzer()
{
  // Default constructor.

  fEvent = 0;
  fPhysics = 0;
  fFile = 0;
  fTree = 0;
  fNev = 0;

  // Set the default cut block names. These can be redefined via SetCutBlocks()

  fNblocks = 3;
  fCutBlockNames    = new TString[ fNblocks ];
  fCutBlockNames[0] = "Decode";
  fCutBlockNames[1] = "Reconstruct";
  fCutBlockNames[2] = "Physics";

  // FIXME: Set the default histogram block names.

  fHistBlockNames   = new TString[ fNblocks ];

  fMasterCutNames   = new TString[ fNblocks ];
  fCutBlocks        = new THaNamedList*[ fNblocks ];
  fSkipCnt          = new Int_t[fMaxSkip];

  if( gHaVars ) {
    gHaVars->Define("nev", "Event number", fNev );
  }
}

//_____________________________________________________________________________
THaAnalyzer::~THaAnalyzer()
{
  // Destructor. Does not delete the fPhysics and fEvent objects since
  // they are defined by the caller.

  Close();
  delete [] fCutBlocks;
  delete [] fMasterCutNames;
  delete [] fHistBlockNames;
  delete [] fCutBlockNames;
  delete [] fSkipCnt;

  if( gHaVars )
    gHaVars->RemoveName("nev");
}

//_____________________________________________________________________________
void THaAnalyzer::Close()
{
  // Close output file and delete ROOT tree if they were created.

  delete fFile; //closes file
  fFile = 0;
  fTree = 0;  //Tree already deleted when file was deleted (FIXME: check this!)
}

//_____________________________________________________________________________
inline
Int_t THaAnalyzer::EvalCuts( Int_t n )
{
  // Evaluate cut block 'n' and return result of the "master cut".
  // The result can be used to skip further processing of the current event.
  // Call SetupCuts() befure using!  This is an internal function.

  gHaCuts->EvalBlock( fCutBlocks[n] );
  return gHaCuts->Result( fMasterCutNames[n].Data(), THaCutList::kNoWarn );
}

//_____________________________________________________________________________
Int_t THaAnalyzer::Process( THaRun& run )
{
  // Process the given run. Loop over all events in the event range and
  // analyze all apparatuses defined in the global apparatus list.
  // Fill Physics and Event structures if they are defined.
  // If Event and Filename are defined, then fill the output tree with Event
  // and write the file.

  static const char* const here = "Process()";

  Int_t status = run.OpenFile();
  if( status < 0 ) return status;

  //--- If output file is defined, open it.
 
  if( !fFile && !fOutFile.IsNull() ) {
    cout << "Creating new file: " << fOutFile << endl;
    fFile = new TFile( fOutFile.Data(), "RECREATE" );
  } 
  // filename changed and file open? -> Close file and create a new one
  else if ( fFile && strcmp( fFile->GetName(), fOutFile.Data())) {
    Close();
    cout << "Creating new file: " << fOutFile << endl;
    fFile = new TFile( fOutFile.Data(), "RECREATE" );
  }
    
  if( fFile && fFile->IsZombie() ) {
    delete fFile;
    run.CloseFile();
    return -10;
  }
  //  fFile->SetCompressionLevel(0);

  //--- Set up a Tree to hold the output

  if( !fTree ) {
    fTree = new TTree("T", "Output DST");
    if( fEvent ) {
      fTree->Branch( "Event Branch", fEvent->IsA()->GetName(), &fEvent,
		     16000, 99 );
    }
  }

  if( fEvent) fEvent->Reset();

  //--- Initialize counters

  fNev = 0;
  memset(fSkipCnt, 0, fMaxSkip*sizeof(Int_t));
  UInt_t nev_physics = 0;
  UInt_t nlim = run.GetLastEvent();
  bool verbose = true, first = true;

  SetupCuts();

  THaEvData evdata;
  TIter next( gHaApps );
  TIter next_scaler( gHaScalers );
  TDatime run_time;

  //--- The main event loop.

  TBenchmark bench;
  bench.Start("Statistics");

  status = 0;
  while ( (status != EOF && status != CODA_ERROR) && nev_physics < nlim ) {

    status = run.ReadEvent();

    if (status != 0) {
      if (status == S_EVFILE_TRUNC) 
          fSkipCnt[0]++;
      else if (status == CODA_ERROR) 
          fSkipCnt[1]++;
      continue;  // skip event
    } 

    fNev++;
    evdata.LoadEvent( run.GetEvBuffer() );

    //--- Initialization

    if ( first ) {
      // Must get a prestart event before we can initialize
      // because the prestart event contains the run time.
      if( !evdata.IsPrestartEvent() ) continue;

      run.SetDate(evdata.GetRunTime());
      run_time = run.GetDate();

      if( verbose ) {
	cout << "Run Number: " << evdata.GetRunNum() << endl;
	cout << "Run Time  : " << run_time.AsString() << endl;
	//	cout << "Run Type  : " << evdata.GetRunType() << endl;
      }
      run.SetNumber( evdata.GetRunNum() );
      first = false;

      // save run data to ROOT file
      // FIXME: give this different name?
      run.Write("Run Data");
      
      // Initialize apparatuses. Make sure that every object in
      // gHaApps really is an apparatus.  Quit if any errors.
      
      bool fail = false;
      Int_t retval = 0;
      TObject* obj;
      while( !fail && (obj = next())) {
	if( !obj->IsA()->InheritsFrom("THaApparatus")) {
	  Error( here, "Apparatus %s is not a THaApparatus. "
		 "Analyzer initialization failed.", obj->GetName() );
	  retval = -20;
	  fail = true;
	} else {
	  THaApparatus* theApparatus = static_cast<THaApparatus*>( obj );
	  theApparatus->Init( run_time );
	  if( !theApparatus->IsOK() ) {
	    retval = -21;
	    fail = true;
	  }
	}
      }

      // Initialize scalers. Quit if any errors.
      // Similar to apparatuses.

      while( !fail && (obj = next_scaler())) {
	if( !obj->IsA()->InheritsFrom("THaScaler")) {
	  Error( here, "Scaler %s is not a THaScaler. "
		 "Analyzer initialization failed.", obj->GetName() );
	  retval = -30;
	  fail = true;
	} else {
	  THaScaler* theScaler = static_cast<THaScaler*>( obj );
	  if( theScaler->Init( run_time ) != 0 ) {
	    retval = -31;
	    fail = true;
	  }
	}
      }
      if( fail ) {
	delete fTree;
	delete fFile;
	run.CloseFile();
	return retval;
      }
    }

    // Print marks every 1000 events

    if( verbose && (fNev%1000 == 0))  
      cout << dec << fNev << endl;

    //=== Physics triggers ===

    if( evdata.IsPhysicsTrigger()) {

      nev_physics++;
      if( nev_physics < run.GetFirstEvent() ) continue;

      //--- Process all apparatuses that are defined in the global list gHaApps
      //    First Decode() everything, then Reconstruct()

      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->Decode( evdata );
      }

      //--- Evaluate test block 1

      if( EvalCuts(0) == 0 ) {
          fSkipCnt[2]++;
          continue;
      }

      //--- Fill histograms in block 1



      //--- 2nd step: Reconstruct(). This is mostly VDC tracking.

      next.Reset();
      while( THaApparatus* theApparatus =
	     static_cast<THaApparatus*>( next() )) {
	theApparatus->Reconstruct();
      }

      //--- Evaluate test block 2

      if( EvalCuts(1) == 0 ) {
           fSkipCnt[3]++;
           continue;
      }


      //--- Fill histograms in block 2


      //--- If Physics class defined, fill it (i.e. compute physics quantities).

      //    if( fPhysics ) {
      //      fPhysics->Fill();

      //--- Evaluate test block 3

      if( EvalCuts(2) == 0 ) {
           fSkipCnt[4]++;
           continue;
      }

      //--- Fill histograms in block 3

      //    }
      //--- If Event class defined, fill it.

      if( fEvent ) {
	fEvent->SetHeader( evdata.GetEvNum(), evdata.GetRunNum(), 0 );
	fEvent->Fill();
      }

      //---  Fill the output tree if there is one.

      if( fTree ) fTree->Fill();

    } 

    //=== Scaler triggers ===

    if( evdata.IsScalerEvent()) {

      //--- Loop over all defined scalers and execute LoadData()

      next_scaler.Reset();
      while( THaScaler* theScaler =
	     static_cast<THaScaler*>( next_scaler() )) {
	theScaler->LoadData( evdata );
      }
    }
  }  // End of event loop
  
  bench.Stop("Statistics");

  //--- Report statistics

  cout << dec;
  if( status == EOF )
    cout << "End of file";
  else if ( nev_physics == nlim )
    cout << "Event limit reached.";
  cout << endl;

  cout << "Processed " << fNev << " events, " 
       << nev_physics << " physics events.\n";

  for (int i = 0; i < fMaxSkip; i++) {
    if (fSkipCnt[i] != 0) cout << "Skipped " << fSkipCnt[i]
             << " events due to reason # " << i << endl;
  }

  // Print scaler statistics

  first = true;
  next_scaler.Reset();
  while( THaScaler* theScaler =
	 static_cast<THaScaler*>( next_scaler() )) {
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

  if( fFile ) fFile->Write();

  return nev_physics;
}

//_____________________________________________________________________________
void THaAnalyzer::SetCutBlocks( Int_t n, const char* name )
{
  if( n < 0 || n >= fNblocks ) return;
  fCutBlockNames[n] = name;
}

//_____________________________________________________________________________
void THaAnalyzer::SetCutBlocks( const TString* name )
{
  for( int i=0; i<fNblocks; i-- )
    fCutBlockNames[i] = name[i];
}

//_____________________________________________________________________________
void THaAnalyzer::SetCutBlocks( const char** name )
{
  for( int i=0; i<fNblocks; i-- )
    fCutBlockNames[i] = name[i];
}

//_____________________________________________________________________________
void THaAnalyzer::SetupCuts()
{
  // Internal function that sets up structures to handle cuts more efficiently:
  //
  // - Generate the names of the "master cuts" for the current cut block names.
  // If the "master cut" exists and is false, further processing of the 
  // current event is skipped.
  //
  // - Find pointers to the THaNamedList lists that hold the cut blocks.
  //

  for( int i=0; i<fNblocks; i++ ) {
    fMasterCutNames[i] = fCutBlockNames[i] + "_";
    fMasterCutNames[i] += kMasterCutName;

    fCutBlocks[i] = gHaCuts->FindBlock( fCutBlockNames[i] );
  }
}
