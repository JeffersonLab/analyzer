{
//
//  Demo script to analyze scalers within the context of the analyzer.
//  Data from one spectrometer are treated.  Something like this could
//  be implemented inside a standard analysis script.
//  
//  A simple change necessary to analyze the new "EvLeft" synchronous
//  scaler readout is shown below.
//
//  R. Michaels, Jan 2002
//
// Set up the equipment to be analyzed.

   THaApparatus* HRSL = new THaHRS("L","Left arm HRS");
   gHaApps->Add( HRSL );

   THaScaler* scaler;
   scaler = new THaScaler("Left");  // These are "event type 140" which
                                 // are inserted asynchronously.
   gHaScalers->Add( scaler );
   scaler = new THaScaler("EvLeft"); // This is a fairly new event readout
                            // which is synchronous with physics events.
   gHaScalers->Add( scaler );

// Define the run(s) that we want to analyze.
// We just set up one, but this could be many.

   THaRun* run = new THaRun( "run.dat" );
   Int_t status = run->OpenFile();
   if( status < 0 ) {
     cout << "Error opening run file.  Quitting."<<endl;
     exit(0);
   }
   run->SetLastEvent( 1000 );

// Output file
   TFile *fFile = new TFile( "scaler.root" , "RECREATE" );

// Define an ntuple here.  
//                    0  1  2  3   4  5  6   7  8  9  10 11 12  13   
   char rawrates[]="time:u1:u3:u10:d1:d3:d10:t1:t2:t3:t4:t5:clk:tacc:";
//                     14  15  16   17  18   19  20  21  22  23  24  25
   char asymmetries[]="au1:au3:au10:ad1:ad3:ad10:at1:at2:at3:at4:at5:aclk";
   int nlen = strlen(rawrates) + strlen(asymmetries);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,rawrates);
   strcat(string_ntup,asymmetries);
   TNtuple *ntup = new TNtuple("ascal","Scaler Rates",string_ntup);
 
   Float_t* farray_ntup = new Float_t[26];   // dimension of farray must match number of 
                                             // semi-colon separated strings of string_ntup

// Define BCMs here.  Left Arm  u1,u3,u10,d1,d3,d10
   Int_t nbcm = 6;
   TString *bcms = new TString[nbcm];
   bcms[0] = "bcm_u1";
   bcms[1] = "bcm_u3";
   bcms[2] = "bcm_u10";
   bcms[3] = "bcm_d1";
   bcms[4] = "bcm_d3";
   bcms[5] = "bcm_d10";
   Float_t bcmped[nbcm] = { 52.5, 44.1, 110.7, 0., 94.2, 227. };

   UInt_t nev = 0, nev_physics = 0;
   UInt_t nlim = run->GetLastEvent();
   bool verbose = true, first = true;

   THaEvData evdata;

   TIter next( gHaApps );
   TIter next_scaler( gHaScalers );
   TDatime run_time;

// Event loop.  This is fairly similar to the default THaAnalyzer::Process(*run)
// but the emphasis is on scalers.  Also, 'run' is a pointer here.

   while( (status = run->ReadEvent()) == 0 && nev < nlim ) {
      nev++;
      evdata.LoadEvent( run->GetEvBuffer() );

    //--- Initialization

      if ( first ) {
      // Must get a prestart event before we can initialize
      // because the prestart event contains the run time.
      if( !evdata.IsPrestartEvent() ) continue;

      run->SetDate(evdata.GetRunTime());
      run_time = run->GetDate();

      if( verbose ) {
	cout << "Run Number: " << evdata.GetRunNum() << endl;
	cout << "Run Time  : " << run_time.AsString() << endl;
      }
      run->SetNumber( evdata.GetRunNum() );
      first = false;

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
          cout << "Init scaler "<<endl;
	  if( theScaler->Init( run_time ) != 0 ) {
	    retval = -31;
	    fail = true;
            cout << "failure to init scaler !"<<endl;
	  }
	}
      }
      if( fail ) {
	delete fTree;
	delete fFile;
	run->CloseFile();
        cout << "Initialization failure !"<<endl;
      }
    }

    // Print marks every 1000 events

    if( verbose && (nev%1000 == 0))  
      cout << dec << nev << endl;

// Here we loop over scalers.  
// Note, it is not necessary to check if  evdata.IsScalerEvent()  because that is done
// for you and is fast; also because the "EvLeft" are found in physics triggers.  But
// they are also scaler events.

      next_scaler.Reset();
      while( !fail && (obj = next_scaler())) {
	if( obj->IsA()->InheritsFrom("THaScaler")) {
	  THaScaler* theScaler = static_cast<THaScaler*>( obj );
	  theScaler->LoadData( evdata );

// Now fill ntuple, but only if we have renewed the scaler data (not every event
// contains scaler data), and only for the bank we want.

	  if (theScaler->IsRenewed() && 
             strcmp(theScaler->GetName(),"EvLeft") == 0)  { 

// test:          theScaler->Print();  

              farray_ntup[0] = ((Float_t)theScaler->GetPulser("clock"))/1024;
              farray_ntup[1] = (Float_t)theScaler->GetBcmRate("bcm_u1");
              farray_ntup[2] = (Float_t)theScaler->GetBcmRate("bcm_u3");
              farray_ntup[3] = (Float_t)theScaler->GetBcmRate("bcm_u10");
              farray_ntup[4] = (Float_t)theScaler->GetBcmRate("bcm_d1");
              farray_ntup[5] = (Float_t)theScaler->GetBcmRate("bcm_d3");
              farray_ntup[6] = (Float_t)theScaler->GetBcmRate("bcm_d10");
              for (int trig = 1; trig <= 5; trig++) farray_ntup[6+trig] = (Float_t)theScaler->GetTrigRate(trig);
              farray_ntup[12] = (Float_t)theScaler->GetPulserRate("clock");
              farray_ntup[13] = (Float_t)theScaler->GetNormRate(0,"TS-accept");  // for helicity 0
              Float_t sum,asy;
              for (int ibcm = 0; ibcm < 6; ibcm++ ) {
                   sum = (Float_t)theScaler->GetBcmRate(1,bcms[ibcm].Data()) - bcmped[ibcm]
                       + (Float_t)theScaler->GetBcmRate(-1,bcms[ibcm].Data()) - bcmped[ibcm];
                asy = -999;
                if (sum != 0) {  // helicity correlated asymmetry
	           asy = ((Float_t)theScaler->GetBcmRate(1,bcms[ibcm].Data()) - 
                          (Float_t)theScaler->GetBcmRate(-1,bcms[ibcm].Data())) / sum;
                } 
                farray_ntup[14+ibcm] = asy;
	      }
              for (trig = 1; trig <= 5; trig++) {
                asy = -999;
                if ((Float_t)theScaler->GetBcmRate(1,"bcm_u3") > 3000 && 
                    (Float_t)theScaler->GetBcmRate(-1,"bcm_u3") > 3000) {  // require the beam is on.    
                    sum = (Float_t)theScaler->GetTrigRate(1,trig)/(Float_t)theScaler->GetBcmRate(1,"bcm_u3") 
                      +  (Float_t)theScaler->GetTrigRate(-1,trig)/(Float_t)theScaler->GetBcmRate(-1,"bcm_u3");
                    if (sum != 0) {
                      asy = ((Float_t)theScaler->GetTrigRate(1,trig)/(Float_t)theScaler->GetBcmRate(1,"bcm_u3")
                       -  (Float_t)theScaler->GetTrigRate(-1,trig)/(Float_t)theScaler->GetBcmRate(-1,"bcm_u3")) / sum;
	 	    }
		}
                farray_ntup[19+trig] = asy;
	      }
              sum = (Float_t)theScaler->GetPulser(1,"clock") + (Float_t)theScaler->GetPulser(-1,"clock");
              asy = -999;
              if (sum != 0) {
	        asy = ((Float_t)theScaler->GetPulser(1,"clock") - 
                       (Float_t)theScaler->GetPulser(-1,"clock")) / sum;
              }
              farray_ntup[25] = asy; 
              ntup->Fill(farray_ntup);
	  }
	}
      }
   

  }  // End of event loop
  

  // Print scaler statistics

  next_scaler.Reset();
  while( !fail && (obj = next_scaler())) {
     if( obj->IsA()->InheritsFrom("THaScaler")) {
	THaScaler* theScaler = static_cast<THaScaler*>( obj );
	theScaler->PrintSummary();
     }
  }

  //--- Close the input file

  run->CloseFile();

  // Write the output file and clean up.
  // This writes the Tree as well as any objects (histograms etc.)
  // that are defined in the current directory.

  if( fFile ) {
      fFile->Write();
      fFile->Close();
  }

  cout << "\nAll done !   Now look at ntuple in scaler.root file..."<<endl;

}



