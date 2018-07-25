//*-- Author :    Robert Michaels    Sept 2002
// Changed into a separate class, Ole Hansen, Aug 2006
////////////////////////////////////////////////////////////////////////
//
// THaG0Helicity
//
// Helicity of the beam from G0 electronics in delayed mode
//    +1 = plus,  -1 = minus,  0 = unknown
//
// Also supports in-time mode with delay = 0
// 
////////////////////////////////////////////////////////////////////////

#include "THaG0Helicity.h"
#include "THaEvData.h"
#include "TH1F.h"
#include "TMath.h"
#include <iostream>
#include <cmath>

using namespace std;

// Default parameters
static const Double_t kDefaultTdavg = 14050.;
static const Double_t kDefaultTtol  = 40.;
static const Int_t    kDefaultDelay = 8;
static const Int_t    kDefaultMissQ = 30;

//_____________________________________________________________________________
THaG0Helicity::THaG0Helicity( const char* name, const char* description,
			      THaApparatus* app ) : 
  THaHelicityDet( name, description, app ),
  fG0delay(kDefaultDelay), fEvtype(0), fTdavg(kDefaultTdavg), fTdiff(0.),
  fT0(0.), fT9(0.), fT0T9(kFALSE), fQuad_calibrated(kFALSE),
  fValidHel(kFALSE), fRecovery_flag(kTRUE),
  fTlastquad(0), fTtol(kDefaultTtol), fQuad(0), 
  fFirstquad(0), fLastTimestamp(0.0), fTimeLastQ1(0.0),
  fT9count(0), fPredicted_reading(0), fQ1_reading(0),
  fPresent_helicity(kUnknown), fSaved_helicity(kUnknown),
  fQ1_present_helicity(kUnknown), fMaxMissed(kDefaultMissQ),
  fNqrt(0), fHisto(NULL), fNB(0), fIseed(0), fIseed_earlier(0),
  fInquad(0), fTET9Index(0), fTELastEvtQrt(-1), fTELastEvtTime(-1.),
  fTELastTime(-1.), fTEPresentReadingQ1(-1), fTEStartup(3), fTETime(-1.),
  fTEType9(kFALSE), fManuallySet(0)
{
  memset(fHbits, 0, sizeof(fHbits));
}

//_____________________________________________________________________________
THaG0Helicity::THaG0Helicity() : fHisto(NULL)
{
  // Default constructor for ROOT I/O
}

//_____________________________________________________________________________
THaG0Helicity::~THaG0Helicity() 
{
  DefineVariables( kDelete );

  delete fHisto;
}

//_____________________________________________________________________________
Int_t THaG0Helicity::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;

  // Define standard variables from base class
  THaHelicityDet::DefineVariables( mode );

  const RVarDef var[] = {
    { "qrt",       "qrt from ROC",                 "fQrt" },
    { "nqrt",      "number of qrts seen",          "fNqrt" },
    { "quad",      "quad (1, 2, or 4)",            "fQuad" },
    { "tdiff",     "time since quad start",        "fTdiff" },
    { "gate",      "Helicity Gate from ROC",       "fGate" },
    { "pread",     "Present helicity reading",     "fPresentReading" },
    { "timestamp", "Timestamp from ROC",           "fTimestamp" },
    { "validtime", "validtime flag",               "fValidTime" },
    { "validHel",  "validHel flag",                "fValidHel" },
    { 0 }
  };
  return DefineVarsFromList( var, mode );
}


//_____________________________________________________________________________
Int_t THaG0Helicity::ReadDatabase( const TDatime& date )
{
  // Read parameters from database:  ROC info (detector map), G0 delay, etc.

  // Read general HelicityDet database values (e.g. fSign)
  Int_t st = THaHelicityDet::ReadDatabase( date );
  if( st != kOK )
    return st;

  // Read G0 readout parameters (ROC addresses etc.)
  st = THaG0HelicityReader::ReadDatabase( GetDBFileName(), GetPrefix(),
					  date, fG0Debug );
  if( st != kOK )
    return st;

  // Read parameters for the delayed-mode algorithm
  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  Int_t    delay   = kDefaultDelay;
  Double_t tdavg   = kDefaultTdavg;
  Double_t ttol    = kDefaultTtol;
  Int_t    missqrt = kDefaultMissQ;

  DBRequest req[] = {
    { "delay",    &delay,      kInt,    0, 1, -2 },
    { "tdavg",    &tdavg,      kDouble, 0, 1, -2 },
    { "ttol",     &ttol,       kDouble, 0, 1, -2 },
    { "missqrt",  &missqrt,    kInt,    0, 1, -2 },
    { 0 }
  };
  st = LoadDB( file, date, req );
  fclose(file);
  if( st )
    return kInitError;

  // Manually set parameters take precedence over the database values.
  // This is meant for quick-and-dirty debugging.
  if( !TESTBIT(fManuallySet,0) )
    fG0delay = delay;
  if( !TESTBIT(fManuallySet,1) )
    fTdavg = tdavg;
  if( !TESTBIT(fManuallySet,2) )
    fTtol = ttol;
  if( !TESTBIT(fManuallySet,3) )
    fMaxMissed = missqrt;

  return kOK;
}

//_____________________________________________________________________________
Int_t THaG0Helicity::Begin( THaRunBase* )
{
  // Book a histogram, if requested
  if (fDebug == -1) {
    if( !fHisto )
      fHisto = new TH1F("hahel1","Time diff for QRT",1000,11000,22000);
    else
      //TODO: only clear if run number changes?
      fHisto->Clear();
  }
  return 0;
}

//_____________________________________________________________________________
void THaG0Helicity::Clear( Option_t* opt ) 
{
  // Clear event-by-event data

  THaHelicityDet::Clear(opt);
  fLastTimestamp = fTimestamp;
  THaG0HelicityReader::Clear(opt);
  fEvtype = fQuad = 0;
  fValidHel = kFALSE;
  fPresent_helicity = kUnknown;
}

//_____________________________________________________________________________
Int_t THaG0Helicity::Decode( const THaEvData& evdata )
{
  // Decode Helicity data.
  // Return 1 if helicity was assigned, 0 if not, <0 if error.

  Int_t err = ReadData( evdata );
  if( err ) {
    Error( Here("Decode"), "Error decoding helicity data." );
    return err;
  }

  fEvtype = evdata.GetEvType();
  if (fDebug >= 3) {
    cout << dec << "--> Data for spectrometer " << GetName() << endl;
    cout << "  evtype " << fEvtype;
    cout << "  helicity reading " << fPresentReading;
    cout << "  qrt " << fQrt;
    cout << " gate " << fGate;
    cout << "   time stamp " << fTimestamp << endl;
  }

  if( fG0delay > 0 ) {
    TimingEvent();
    QuadCalib();      
    LoadHelicity();
  } 
  else if( fGate ) {
    fPresent_helicity = ( fPresentReading ) ? kPlus : kMinus;
  }

  if( fSign >= 0 )
    fHelicity = fPresent_helicity;
  else
    fHelicity = ( fPresent_helicity == kPlus ) ? kMinus : kPlus;

  if (fDebug >= 3) 
    cout << "G0 helicity "<< GetName() << " =  " << fHelicity <<endl;

  return HelicityValid() ? 1 : 0;
}

//_____________________________________________________________________________
Int_t THaG0Helicity::End( THaRunBase* )
{
  // End of run processing. Write histograms.

  if( fHisto )
    fHisto->Write();

  return 0;
}

//_____________________________________________________________________________
void THaG0Helicity::SetDebug( Int_t level )
{
  // Set debug level of this detector as well as the THaG0HelicityReader 
  // helper class.

  THaHelicityDet::SetDebug( level );
  fG0Debug = level;
}

//_____________________________________________________________________________
void THaG0Helicity::SetG0Delay( Int_t value )
{
  // Set delay of helicity data (# windows). 
  // This value will override the value from the database.

  fG0delay = value;
  fManuallySet |= BIT(0);
}

//_____________________________________________________________________________
void THaG0Helicity::SetTdavg( Double_t value )
{
  // Set average time delay (# channels)
  // This value will override the value from the database.

  fTdavg = value;
  fManuallySet |= BIT(1);
}

//_____________________________________________________________________________
void THaG0Helicity::SetTtol( Double_t value )
{
  // Set the tolerance in timing when looking for missing quads
  // (# channels)
  // This value will override the value from the database.

  fTtol = value;
  fManuallySet |= BIT(2);
}

//_____________________________________________________________________________
void THaG0Helicity::SetMaxMsQrt( Int_t value )
{
  // Set the maximum number of missing quads permitted before
  // forcing the helicity calibration to be re-performed.
  // (# quads)
  // This value will override the value from the database.

  fMaxMissed = value;
  fManuallySet |= BIT(3);
}

//_____________________________________________________________________________
void THaG0Helicity::TimingEvent() 
{
  // Check for and process timing events
  // A timing event is a T9, *or* if enough time elapses without a
  // T9, it's any event with QRT==1 following an event with QRT==0.
  //
  //  NOTE: This presently has NO EFFECT on the helicity processing,
  //        just provides warning messages if something suspicious
  //        happens.

  static const char* const here = "TimingEvent";

  if (fEvtype == 9) {
    fTEType9 = kTRUE;
    fTETime = fTimestamp;
  }
  else if (fQrt && !fTELastEvtQrt
	   && fTimestamp - fTELastTime > 8 * fTdavg) {
    // After a while we give up on finding T9s and instead take
    // either the average timestamp of the first QRT==1 we find
    // and the previous event (if that's not to far in the past)
    // or just the timestamp of the QRT==1 event (if it is).
    // This is lousy accuracy but better than nothing.
    fTEType9 = kFALSE;
    if (fTELastEvtTime>0.
	&& (fTimestamp - fTELastEvtTime) < 0.1 * fTdavg) {
      fTETime = (fTimestamp + fTELastEvtTime) * 0.5;
    } else {
      // Either we have not seen a previous event or it was too far away,
      // work with the present reading
      fTETime = fTimestamp;
    }
  }
  else {
    fTELastEvtQrt = fQrt;
    fTELastEvtTime = fTimestamp;
    return;
  }

  // Now check for anomalies.

  if (fTELastTime > 0)
    {
      // Look for missed timing events
      Double_t t9diff = 0.25 * fTdavg;
      Double_t tdiff = fTETime - fTELastTime;
      Int_t nt9miss = (Int_t) (tdiff / t9diff - 0.5);
      fTET9Index += nt9miss;
      if (fTET9Index > 3) {
	fTEPresentReadingQ1 = -1;
	fTET9Index = fTET9Index % 4;
      }
      if (fTEType9 &&
	  TMath::Abs(tdiff - (nt9miss+1) * t9diff) > 10*(nt9miss + 1) )
	Warning( Here(here), "Weird time difference between timing events: "
		 "%f at %f.", tdiff, fTETime);
    }
  ++fTET9Index;
  if (fQrt) {
    if (fTET9Index != 4 && !fTEStartup)
      Warning("THaG0Helicity", "QRT wrong spacing: %d at %f.", 
	      fTET9Index, fTETime);
    fTET9Index = 0;
    fTEPresentReadingQ1 = fPresentReading;
    fTEStartup = 0;
  }
  else {
    if (fTEStartup) 
      --fTEStartup;
    if (fTEPresentReadingQ1 > -1)
      if ((fPresentReading == fTEPresentReadingQ1 && 
	   (fTET9Index == 1 || fTET9Index == 2))
	  || (fPresentReading != fTEPresentReadingQ1 && 
	      fTET9Index == 3))
	Warning( Here(here), "Wrong helicity pattern: "
		 "%d in window 1, %d in window %d at timestamp %f.",
	 fTEPresentReadingQ1, fPresentReading, (fTET9Index+1), fTETime);
  }
  // Now push back information
  fTELastTime = fTETime;
  fTELastEvtQrt = fQrt;
  fTELastEvtTime = fTimestamp;

  return;
}

//_____________________________________________________________________________
void THaG0Helicity::QuadCalib()
{
  // Calibrate the helicity predictor shift register.

  const char* const here = "QuadCalib";

  // Is the Qrt signal delayed such that the evt9 at the beginning of
  // a Qrt does NOT contain the qrt flag?
  const int delayed_qrt = 1; 

  if (fEvtype == 9) {
    fT9count += 1;
    if ( delayed_qrt && fQrt ) {
      // don't record this time     
    } else {
      fT9 = fTimestamp; // this sets the timing reference.
    }
  }
   
  if (fEvtype != 9 && !fGate) return;

  fTdiff = fTimestamp - fT0;
  if (fDebug >= 3) {
    cout << here << " " << fTimestamp<<"  "<<fT0;
    cout << "  "<<fTdiff
	 << " " << fEvtype
	 <<"  "<<fQrt<<endl;
  }
  if (fFirstquad == 0 &&
      fTdiff > (1.25*fTdavg + fTtol)) {
    // Try a recovery.  Use time to flip helicity by the number of
    // missed quads, unless this are more than 30 (~1 sec).  
    Int_t nqmiss = (Int_t)(fTdiff/fTdavg);
    if (fDebug >= 1)
      Info( Here(here), "Recovering large DT, nqmiss = %d "
	    "at timestamp %f, tdiff %f", nqmiss, fTimestamp, fTdiff );
    if (fQuad_calibrated && nqmiss < fMaxMissed) {
      for (Int_t i = 0; i < nqmiss; i++) {
	if (fQrt && fT9 > 0 && fTimestamp - fT9 < 8*fTdavg) {
	  fT0 = TMath::Floor((fTimestamp-fT9)/(.25*fTdavg))
	    *(.25*fTdavg) + fT9;
	  fT0T9 = kTRUE;
	  }
	else {
	  fT0 += fTdavg;
	  fT0T9 = kFALSE;
	}
	QuadHelicity(1);
	fNqrt++;

	fQ1_reading = (fPredicted_reading == -1) ? 0 : 1;

	fQ1_present_helicity = fPresent_helicity;
	if (fDebug>=1) {
	  Info(Here(here)," %5d  M  M %1d %2d  %10.0f  %10.0f  %10.0f Missing",
	       fNqrt,fQ1_reading,fQ1_present_helicity,fTimestamp,fT0,fTdiff);
	}
      }
      fTdiff = fTimestamp - fT0;
    } else { 
      Warning( Here(here), "Cannot recover: Too many skipped QRT (Low rate ?) "
	       "or uninitialized" );
      fNqrt += nqmiss; // advance counter for later check
      fT0 += nqmiss*fTdavg;
      fRecovery_flag = kTRUE;    // clear & recalibrate the predictor
      fQuad_calibrated = kFALSE;
      fFirstquad = 1;
    }
  }
  if (fQrt && fFirstquad == 1) {
    fT0 = fTimestamp;
    fT0T9 = kFALSE;
    fFirstquad = 0;
  }

  // Check for QRT at anomalous separations

  if (fEvtype == 9 && fQrt) {
    if (fTimeLastQ1 > 0) {
      Double_t q1tdiff = fmod(fTimestamp - fTimeLastQ1,fTdavg);
      if (q1tdiff > .1*fTdavg)
	Warning( Here(here), "QRT==1 timing error --  Last time = %f "
	     "This time = %f\nHELICITY SIGNALS MAY BE CORRUPT", 
		 fTimeLastQ1, fTimestamp );
    }
    fTimeLastQ1 = fTimestamp;
  }

  if (fQrt) 
    fT9count = 0;

  // The most solid predictor of a new helicity window:
  // (RJF) Simply look for sufficient time to have passed with a fQrt signal.
  // Do not worry about  evt9's for now, since that transition is redundant
  // when a new Qrt is found. And frequently the evt9 came immediately
  // BEFORE the Qrt.
  // NOTE: missing QRT's are already handled by the 'nmissq' section above
  if ( ( fTdiff > 0.5*fTdavg ) && fQrt ) {
    if (fDebug >= 3) 
      Info(Here(here),"found qrt ");

    // If we have T9 within recent time: Update fT0 to match the
    // nearest/closest last evt9 (extrapolated from the last
    // observed type 9 event) at the beginning of the Qrt.
    // Otherwise: if there was a previous event very recently, take
    // average of its and this timestamp.  And if not, just take
    // this timestamp.  This will usually be sufficiently accurate
    // for our purposes, though having T9 would be nicer.
    if (fT9 > 0 && fTimestamp - fT9 < 8*fTdavg) {
      fT0 = TMath::Floor((fTimestamp-fT9)/(.25*fTdavg))
	*(.25*fTdavg) + fT9;
      fT0T9 = kTRUE;
    }
    else {
      fT0 = (fLastTimestamp == 0 
	     || fTimestamp - fLastTimestamp > 0.1*fTdavg)
	? fTimestamp : 0.5 * (fTimestamp + fLastTimestamp);
      fT0T9 = kFALSE;
    }
    fQ1_reading = fPresentReading;
    QuadHelicity();
    fNqrt++;
    fQ1_present_helicity = fPresent_helicity;
    if (fQuad_calibrated && fDebug >= 3) 
      cout << "------------  quad calibrated --------------------------"<<endl;
    if (fQuad_calibrated && !CompHel() ) {
      // This is ok if it doesn't happen too often.  You lose these events.
      Warning( Here(here), "QuadCalib G0 prediction failed at timestamp %f.\n"
	       "HELICITY ASSIGNMENT IN PREVIOUS %d WINDOWS MAY BE INCORRECT",
	       fTimestamp, fG0delay);
      if (fDebug >= 3) {
	cout << "Qrt " << fQrt << "  Tdiff "<<fTdiff;
	cout<<"  gate "<<fGate<<" evtype "<<fEvtype<<endl;
      }
      fRecovery_flag = kTRUE;    // clear & recalibrate the predictor
      fQuad_calibrated = kFALSE;
      fFirstquad = 1;
      fPresent_helicity = kUnknown;
    }
    if (fDebug>=3) {
      Info(Here(here), "%5d  %1d  %1d %1d %2d  %10.0f  %10.0f  %10.0f",fNqrt,
	 fEvtype,fQrt,fQ1_reading,fQ1_present_helicity,fTimestamp,fT0,fTdiff);
    }
    if (fDebug==-1) { // Only used during an initial calibration.
      cout << fTdiff<<endl;
      if (fHisto) {
	fHisto->Fill(fTdiff);
      }
    }
  }
  fTdiff = fTimestamp - fT0;
  return;
}


//_____________________________________________________________________________
void THaG0Helicity::LoadHelicity()
{
  // Load the helicity in G0 mode.  
  // If fGate == 0, helicity remains 'Unknown'.

  static const char* const here = "LoadHelicity";

  if ( fQuad_calibrated ) 
    fValidHel = kTRUE;
  if ( !fQuad_calibrated  || !fGate ) {
    fPresent_helicity = kUnknown;
    return;
  }
  if (fDebug >= 2) 
    cout << "Loading helicity ##########" << endl;
  // Look for pattern (+ - - +)  or (- + + -) to decide where in quad
  // Ignore timing for assignment, but later we'll check it.
  fInquad = 0;
  if (fQrt) {
    fInquad = 1;  
    if (fPresentReading != fQ1_reading) {
      Warning( Here(here), "Invalid bit pattern !! "
	       "fPresentReading != fQ1_reading while QRT == 1 at timestamp "
	       "%f.\nHELICITY ASSIGNMENT MAY BE INCORRECT", fTimestamp);
    }
  }
  if (!fQrt && fPresentReading != fQ1_reading) {
    fInquad = 2;  // same is inquad = 3
  }
  if (!fQrt && fPresentReading == fQ1_reading) {
    fInquad = 4;  
  }
  if (fInquad == 0) {
    fPresent_helicity = kUnknown;
    Error( Here(here), "Quad assignment impossible !! "
	   " qrt = %d  present read = %d  Q1 read = %d  at timestamp %f.\n"
	   "HELICITY SET TO UNKNOWN", 
	   fQrt, fPresentReading, fQ1_reading, fTimestamp );
  }
  else if (fInquad == 1 || fInquad >= 4) {
    fPresent_helicity = fQ1_present_helicity;
  } else {
    if (fQ1_present_helicity == kPlus) 
      fPresent_helicity = kMinus;
    if (fQ1_present_helicity == kMinus) 
      fPresent_helicity = kPlus;
  }

  // Here we check timing, with stringency depending on whether fTdiff
  // was set using T9 or not
  if (
      (!fT0T9 &&
       ((fInquad == 1 && fTdiff > 0.5 * fTdavg) ||
	(fInquad == 4 && fTdiff < 0.5 * fTdavg)))
      ||
      (fT0T9 &&
       ((fInquad == 1 && fTdiff > 0.3 * fTdavg) ||
	(fInquad == 2 && (fTdiff < 0.2 * fTdavg ||
			 fTdiff > 0.8 * fTdavg)) ||
	(fInquad == 4 && fTdiff < 0.7 * fTdavg))))
    {
      Warning( Here(here), "Quad assignment inconsistent with timing !! "
	       "quad = %d  tdiff = %f  at timestamp %f. "
	       "HELICITY ASSIGNMENT MAY BE INCORRECT",
	       fInquad, fTdiff, fTimestamp );
    }
   

  if (fDebug >= 2) {
    cout << dec << "Quad   "<<fInquad;
    cout << "   Time diff "<<fTdiff;
    cout << "   Qrt  "<<fQrt;
    cout << "   Helicity "<<fPresent_helicity<<endl;
  }
  fQuad = fInquad;
  return;
}

//_____________________________________________________________________________
void THaG0Helicity::QuadHelicity( Int_t cond )
{
  // Load the helicity from the present reading for the
  // start of a quad.
  //  Requires:  cond!=0 in order to force the quad to advance
  //             fT0 to be updated and set for THIS quad
  static const char* const here = "QuadHelicity";
  
  if (fRecovery_flag) {
    fNB = 0;
    fSaved_helicity = kUnknown;
  }
  fRecovery_flag = kFALSE;

  fPresent_helicity = fSaved_helicity;
  // Make certain a given qrt is used only once UNLESS
  // a reset of the Quad calibration has happened
  if (cond == 0 && fNB>0
      && fTlastquad > 0
      && fT0 - fTlastquad < 0.3 * fTdavg) {
    if (fDebug >= 2) Warning(Here(here),"SKIP this quad, QRT's too close");
    return;
  }
  fTlastquad = fT0;
  
  if (fNB < kNbits) {
    fHbits[fNB] = fPresentReading;
    fNB++;
    fQuad_calibrated = kFALSE;
  } else if (fNB == kNbits) {   // Have finished loading
    fIseed_earlier = GetSeed();
    fIseed = fIseed_earlier;
    for( int i = 0; i < kNbits+1; i++) {
      fPredicted_reading = RanBit(0);
      RanBit(1);
    }

    if( fPredicted_reading > 0 )
      fPresent_helicity = kPlus;
    else if( fPredicted_reading < 0 )
      fPresent_helicity = kMinus;
    else
      fPresent_helicity = kUnknown;

    // Delay by fG0delay windows which is fG0delay/4 quads
    for( int i = 0; i < fG0delay/4; i++)
      fPresent_helicity = RanBit(1);
    fNB++;
    fSaved_helicity = fPresent_helicity;
    fQuad_calibrated = kTRUE;
  } else {
    fPredicted_reading = RanBit(0);
    fSaved_helicity = fPresent_helicity = RanBit(1);
    if (fDebug >= 3) {
      cout << "quad helicities  " << dec;
      cout << fPredicted_reading;           // -1, +1
      cout << " ?=? " << fPresentReading;  //  0,  1
      cout << "   pred-> " << fPresent_helicity
	   << " time " << fTimestamp
	   << "  <<<<<<<<<<<<<================================="
	   <<endl;
      if (CompHel()) cout << "HELICITY OK "<<endl;
    }
    fQuad_calibrated = kTRUE;

  }
  return;
}

//_____________________________________________________________________________
THaHelicityDet::EHelicity THaG0Helicity::RanBit( int which )
{
  // This is the random bit generator according to the G0
  // algorithm described in "G0 Helicity Digital Controls" by 
  // E. Stangland, R. Flood, H. Dong, July 2002.
  // Argument:                                                    
  //        which = 0 or 1                                        
  //     if 0 then fIseed_earlier is modified                      
  //     if 1 then fIseed is modified                              
  // Return value:                                                
  //        helicity (-1 or +1).

  const int MASK = BIT(0)+BIT(2)+BIT(3)+BIT(23);

  if (which == 0) {
     if( TESTBIT(fIseed_earlier,23) ) {    
         fIseed_earlier = ((fIseed_earlier^MASK)<<1) | BIT(0);
         return kPlus;
     } else  { 
         fIseed_earlier <<= 1;
         return kMinus;
     }
  } else {
     if( TESTBIT(fIseed,23) ) {    
         fIseed = ((fIseed^MASK)<<1) | BIT(0);
         return kPlus;
     } else  { 
         fIseed <<= 1;
         return kMinus;
     }
  }
}


//_____________________________________________________________________________
UInt_t THaG0Helicity::GetSeed()
{
  int seedbits[kNbits];
  UInt_t ranseed = 0;
  for (int i = 0; i < 20; i++)
    seedbits[23-i] = fHbits[i];
  seedbits[3] = fHbits[20]^seedbits[23];
  seedbits[2] = fHbits[21]^seedbits[22]^seedbits[23];
  seedbits[1] = fHbits[22]^seedbits[21]^seedbits[22];
  seedbits[0] = fHbits[23]^seedbits[20]^seedbits[21]^seedbits[23];
  for (int i=kNbits-1; i >= 0; i--) 
    ranseed = ranseed<<1|(seedbits[i]&1);
  ranseed &= 0xFFFFFF;
  return ranseed;
}

//_____________________________________________________________________________
Bool_t THaG0Helicity::CompHel()
{
  // Compare the present reading to the predicted reading.
  // The raw data are 0's and 1's which compare to predictions of
  // -1 and +1.  A prediction of 0 occurs when there is no gate.
  if (fPresentReading == 0 && 
      fPredicted_reading == kMinus) return kTRUE;
  if (fPresentReading == 1 && 
      fPredicted_reading == kPlus) return kTRUE;
  return kFALSE;
}

ClassImp(THaG0Helicity)
