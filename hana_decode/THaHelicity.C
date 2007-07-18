//*-- Author :    Robert Michaels    Sept 2002

////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// Helicity of the beam.  
//    +1 = plus,  -1 = minus,  0 = unknown
// 
// For data prior to G0 mode, the helicity is determined
// by an ADC value (high value = plus, pedestal value = minus).  
// If the helicity flag is in delayed mode, then we must use the 
// G0 helicity prediction algorithm and verify from the timestamp
// whether the helicity window has changed.
//
// This class is used by THaEvData.  The helicity and timestamp
// data are registered global variables.
//
// The user can use public method SetState() to adjust the state
// of this class.  However, a reasonable default exists.
// The state is defined by
//
//   fgG0mode == G0 delayed mode (1) or in-time mode (0)
//   fgG0delay = number of windows helicity is delayed
//   fSign = Overall sign (as determined by Moller)
//   fWhichSpec = fgLarm/fgRarm = Which spectrometer to believe.
//   fCheckRedund = Do we check redundancy (yes=1, no=0)
//
////////////////////////////////////////////////////////////////////////


#include "THaHelicity.h"
#include "THaEvData.h"
#include "TMath.h"
using namespace std;

ClassImp(THaHelicity)

const Double_t THaHelicity::fgTdiff = 14050;

//____________________________________________________________________
THaHelicity::THaHelicity( ) :
  fTdavg(NULL), fTdiff(NULL), fT0(NULL), fT9(NULL), fTlastquad(NULL),
  fTtol(NULL), fQrt(NULL), fGate(NULL), fFirstquad(NULL), fEvtype(NULL),
  fTimestamp(NULL), validTime(NULL), validHel(NULL), t9count(NULL),
  present_reading(NULL), predicted_reading(NULL), q1_reading(NULL),
  present_helicity(NULL), saved_helicity(NULL), q1_present_helicity(NULL),
  quad_calibrated(NULL), hbits(NULL), fNqrt(NULL)
{
  InitMemory();
  SetState(0, 0, -1, fgLarm, 0);  // reasonable default state.
}

//____________________________________________________________________
THaHelicity::~THaHelicity( ) 
{
  if (fQrt) delete [] fQrt;
  if (fGate) delete [] fGate;
  if (fFirstquad) delete [] fFirstquad;
  if (fEvtype) delete [] fEvtype;
  if (fTimestamp) delete [] fTimestamp;
  if (fTdavg) delete [] fTdavg;
  if (fTdiff) delete [] fTdiff;
  if (fT0) delete [] fT0;
  if (fT9) delete [] fT9;
  if (fTlastquad) delete [] fTlastquad;
  if (fTtol) delete [] fTtol;
  if (t9count) delete [] t9count;
  if (fNqrt) delete [] fNqrt;
  if (present_reading) delete [] present_reading;
  if (q1_reading) delete [] q1_reading;
  if (predicted_reading) delete [] predicted_reading;
  if (present_helicity) delete [] present_helicity;
  if (saved_helicity) delete [] saved_helicity;
  if (q1_present_helicity) delete [] q1_present_helicity;
  if (quad_calibrated) delete [] quad_calibrated;
  if (hbits) delete [] hbits;
  if (validTime) delete [] validTime;
  if (validHel) delete [] validHel;
}

//____________________________________________________________________
int THaHelicity::GetHelicity() const 
{
// Get the helicity data for this event (1-DAQ mode).
// By convention the return codes:   -1 = minus, +1 = plus, 0 = unknown
     int lhel,rhel;
     lhel = -2;
     rhel = -2;
     if (validHel[fgRarm]) rhel = GetHelicity("R");
     if (validHel[fgLarm]) lhel = GetHelicity("L");
     if (lhel > -2 && rhel > -2) {
       if (lhel != rhel) {
	 cout << "THaHelicity::GetHelicity: Inconsistent helicity";
         cout << " between spectrometers."<<endl;
       }
     }
     if (lhel > -2) return lhel;
     if (rhel > -2) return rhel;
     return 0;
}

//____________________________________________________________________
int THaHelicity::GetHelicity(const TString& spec) const {
// Get the helicity data for this event. 
// spec should be "L"(left) or "R"(right) corresponding to 
// which spectrometer read the helicity info.
// Otherwise you get Hel which is from one spectrometer 
// internally chosen.
// By convention the return codes:  
//      -1 = minus,  +1 = plus,  0 = unknown

      if ((spec == "left") || (spec == "L")) return Lhel;
      if ((spec == "right") || (spec == "R")) return Rhel;
      return Hel;
}

//____________________________________________________________________
void THaHelicity::SetState(int mode, int delay, 
                      int sign, int spec, int redund) 
{
// Set the state of this object 
  fgG0mode = mode;    // G0 mode (1) or in-time helicity (0)
  fgG0delay = delay;  // delay of helicity (# windows)
  fSign = sign;       // Overall sign (as determined by Moller)
  fWhichSpec = spec;  // Which spectrometer do we believe ?
  fCheckRedund = redund; // Do we check redundancy (yes=1, no=0)
  InitG0();           // This is only relevant for G0 mode,
                      // but it doesn't hurt.
}

//____________________________________________________________________
void THaHelicity::InitMemory() {
  validTime           = new Int_t[2];
  validHel            = new Int_t[2];
  fQrt                = new Int_t[2];
  fGate               = new Int_t[2];
  fFirstquad          = new Int_t[2];
  fEvtype             = new Int_t[2];
  fTimestamp          = new Double_t[2];
  fTdavg              = new Double_t[2];
  fTdiff              = new Double_t[2];
  fTtol               = new Double_t[2];
  fT0                 = new Double_t[2];
  fT9                 = new Double_t[2];
  fTlastquad          = new Double_t[2];
  t9count             = new Int_t[2];
  fNqrt               = new UInt_t[2];
  present_reading     = new Int_t[2];   
  q1_reading          = new Int_t[2];   
  predicted_reading   = new Int_t[2];
  present_helicity    = new Int_t[2];
  saved_helicity      = new Int_t[2];
  quad_calibrated     = new Int_t[2]; 
  q1_present_helicity = new Int_t[2];
  hbits               = new Int_t[fgNbits];
}

//____________________________________________________________________
void THaHelicity::InitG0() {
// Initialize some things relevant to G0 mode.
// This is irrelevant for in-time mode, but its good to zero things.
  fTdavg[0]  = fgTdiff;  // Avg time diff between qrt's
  fTdavg[1]  = fgTdiff;    
  fTtol[0]   = 20;   
  fTtol[1]   = 20;       
  fT0[0]     = 0;
  fT0[1]     = 0;
  fTlastquad[0] = 0;
  fTlastquad[1] = 0;
  fFirstquad[0] = 1;
  fFirstquad[1] = 1;
  memset(hbits, 0, fgNbits*sizeof(Int_t));
  memset(t9count, 0, 2*sizeof(Int_t));
  memset(fNqrt, 0, 2*sizeof(UInt_t));
  memset(q1_reading, 0, 2*sizeof(Int_t));
  memset(predicted_reading, Unknown, 2*sizeof(Int_t));
  memset(saved_helicity, Unknown, 2*sizeof(Int_t));
  memset(quad_calibrated, 0, 2*sizeof(Int_t));
  recovery_flag = 1;
  iseed = 0;
  iseed_earlier = 0;
  inquad = 0;
}

//____________________________________________________________________
Int_t THaHelicity::Decode( const THaEvData& evdata ) {
// Decode Helicity data.

// In ancient history there was no TIR data for helicity,
// only ADC data; and the timestamp came from ROC14; ask 
// Bob if you care about year <= 2002

 ClearEvent();  

 if (fgG0mode == 0) {  // Non-G0 (i.e. in-time) helicity mode

   if (fCheckRedund) {

// Redundant ADC helicity data. (bit = ADC high/low)
// Obnoxious complaints when this disagrees with TIR bits.
// As of 2003 the data are in
//   (1,25,14,15) / (3,25,14,15) = (roc,slot,chanlo,chanhi)
// In previous times the data were (even more redundantly) in 
//   (1,25,60,61) / (2,25,14,15) / (2,25,46,47)

    int roc, slot, chan, chanlo, chanhi, adc_data;
    roc = 0; slot = 0; chanlo = 0; chanhi = 0;

 // This code won't find data if pedestal suppression is on 
 // and it should be off for helicity ADC channels, but in
 // case not, we set the bits to Minus.    
    Ladc_hbit = Minus;
    Radc_hbit = Minus;

    for (int i = 0; i < 2; i++) {
      if (i == 0) {roc=1; slot=25; chanlo=14; chanhi=15;}
      if (i == 1) {roc=3; slot=25; chanlo=14; chanhi=15;}
      for (chan = chanlo; chan <= chanhi; chan++) {
        if ( !evdata.GetNumHits(roc,slot,chan ))continue;
        adc_data = evdata.GetData(roc,slot,chan,0);

        if (HELDEBUG >= 1) {
	  cout << "crate "<<roc<<"   slot "<<slot<<"   chan "<<chan<<"   data "<<adc_data<<"   ";
          if (adc_data > HCUT) cout << "  above cut !";
          cout << endl;
        }

// Assign gate and helicity bit data.

        if (roc==1) {   // Right HRS
          if (chan==15) {
	    if (adc_data > HCUT) {
	       Radc_gate = Plus;
	    } else {
               Radc_gate = Minus;
	    }
	  }
	  if (chan==14) {
	    if (adc_data > HCUT) {
	       Radc_hbit = Plus;
	    } else {
               Radc_hbit = Minus;
	    }
	  }
	}

        if (roc==3) {
// As I write this (9/04), there is no gate on L-arm, only helicity bit.
// This would lead to complaints below  if fCheckRedund==1.
	  if (chan==15) {
	    if (adc_data > HCUT) {
	       Ladc_hbit = Plus;
	    } else {
               Ladc_hbit = Minus;
	    }
	  }
	}

      }
    }

// Logic: if gate==0, helicity remains unknown.  If gate==1 helicity
// is determined by helicity bit.

    if (Ladc_gate == Plus) Ladc_helicity = Ladc_hbit;
    if (Radc_gate == Plus) Radc_helicity = Radc_hbit;
    
    if (HELDEBUG) {
      cout << "ADC helicity info "<<endl;
      cout << "L-arm gate "<<Ladc_gate<<"  helic. bit "<<Ladc_hbit;
      cout << "    resulting helicity "<<Ladc_helicity<<endl;
      cout << "R-arm gate "<<Radc_gate<<"  helic. bit "<<Radc_hbit;
      cout << "    resulting helicity "<<Radc_helicity<<endl;
    }

   }

// Here is the new (ca 2002) way to get helicity and timestamp

   Int_t myroc, data, qrt, gate, helicity_bit;
   myroc=0; data=0; qrt=0; gate=0; helicity_bit=0;

   for (myroc = 10; myroc <= 11; myroc++) {

     if(evdata.GetRocLength(myroc) <= 4) continue;
     data = evdata.GetRawData(myroc,3);   
     helicity_bit = (data & 0x10) >> 4;
     qrt = (data & 0x20) >> 5;
     gate = (data & 0x40) >> 6;

     if (gate != 0) {
        if (helicity_bit) {
	   if (myroc == 11) {  // roc11 = LHRS
              Lhel = Plus;
           } else {
              Rhel = Plus;
           }
        } else {
           if (myroc == 11) {
              Lhel = Minus;
           } else {
              Rhel = Minus;
           }
        }
     }
   }

   if (fWhichSpec == fgLarm) { // pick a spectrometer
       myroc = 11;
       Hel = Lhel;  
   } else {
       myroc = 10;
       Hel = Rhel;  
   }

   validHel[fWhichSpec] = 1;
   timestamp = evdata.GetRawData(myroc,4);
   validTime[fWhichSpec] = 1;
   if (HELDEBUG) { 
     cout << "In-time helicity check for roc "<<myroc<<endl;
     cout << "Raw word 0x"<<hex<<data<<dec;
     cout << "  Gate "<<gate<<"  qrt "<<qrt;
     cout << "  helicity bit "<<helicity_bit;
     cout << "  Resulting helicity "<<Hel<<endl;
     cout << "  timestamp "<<timestamp<<endl;
   }

   if (fCheckRedund) {
   
// Here we check redunandcy and complain if something disagrees.
// This might be flakey due to timing delays, miscabling, things
// being turned off, etc, but at least it shows the idea.

//     if (Lhel != Rhel || Lhel != Ladc_helicity || 
//         Lhel != Radc_helicity) {
     if (Lhel != Radc_helicity) { // for DVCS...
          cout << "THaHelicity::WARNING:: ";
          cout << " Inconsistent helicity data !!"<<endl;
     }
   }

 } else {   // G0 helicity mode (data are delayed).

   for (Int_t spec = fgLarm; spec <= fgRarm; spec++) {
      fArm = spec;  // fArm is global
      ReadData ( evdata );    
      QuadCalib ();      
      LoadHelicity ();
   }

   // Sign determined by Moller measurement
   Lhel = fSign * present_helicity[fgLarm];
   Rhel = fSign * present_helicity[fgRarm];
   if (fWhichSpec == fgLarm) {
     Hel = Lhel;  
   } else {
     Hel = Rhel;  
   }
   if (HELDEBUG >= 1) cout << "G0 helicitites "<<Lhel<<"  "<<Rhel<<endl;

 }

 return OK;
}
 
//____________________________________________________________________
Double_t THaHelicity::GetTime() const {
// Get the timestamp, a ~105 kHz clock.
  if (fgG0mode == 0) return timestamp;
  if (validTime[fgLarm]) return fTimestamp[fgLarm];
  if (validTime[fgRarm]) return fTimestamp[fgRarm];
  return 0;
}

//____________________________________________________________________
void THaHelicity::ClearEvent() {
// Clear the event data

  memset(validTime, 0, 2*sizeof(Int_t));
  memset(validHel, 0, 2*sizeof(Int_t));
  timestamp = Unknown;
  Lhel = Unknown;
  Rhel = Unknown;
  Hel  = Unknown;
  Ladc_helicity = Unknown; 
  Ladc_gate = Unknown;
  Ladc_hbit = Unknown;
  Radc_helicity = Unknown; 
  Radc_gate = Unknown;
  Radc_hbit = Unknown;
  if (fgG0mode) {
    memset(fQrt, 0, 2*sizeof(Int_t));
    memset(fGate, 0, 2*sizeof(Int_t));
    memset(fTimestamp, 0, 2*sizeof(Double_t));
    memset(present_reading, Unknown, 2*sizeof(Int_t));
    memset(present_helicity, Unknown, 2*sizeof(Int_t));
    memset(fEvtype, 0, 2*sizeof(Int_t));
  }

}

//____________________________________________________________________
void THaHelicity::ReadData( const THaEvData& evdata ) {
// Obtain the present data from the event for G0 helicity mode.

   if (fgG0mode != 1) return;

   int myroc, len, data, nscaler, header;
   int numread, badread, found, index;
   int ring_clock, ring_qrt, ring_helicity;
   int ring_trig, ring_bcm, ring_l1a, ring_v2fh; 

   if (fArm == fgLarm )
     myroc = 11;
   else if (fArm == fgRarm) 
     myroc = 10;
   else
     return;

   len = evdata.GetRocLength(myroc);
   if (len <= 4) return;
   data = evdata.GetRawData(myroc,3);   
   present_reading[fArm] = (data & 0x10) >> 4;
   fQrt[fArm] = (data & 0x20) >> 5;
   fGate[fArm] = (data & 0x40) >> 6;
   fTimestamp[fArm] = evdata.GetRawData(myroc,4);
   validTime[fArm] = 1;
   found = 0;
   index = 5;
   while ( !found ) {
     data = evdata.GetRawData(myroc,index++);
     if ( (data & 0xffff0000) == 0xfb0b0000) found = 1;
     if (index >= len) break;
   }
   if ( found ) {
     nscaler = data & 0x7;
   } else {
     if (HELDEBUG >= 1) cout << "WARNING: Cannot find scalers."<<endl;
     nscaler = 0;
   }
   fEvtype[fArm] = evdata.GetEvType();
   if (HELDEBUG >= 2) {
     cout << dec << "--> Data for spectrometer " << fArm << endl;
     cout << "  evtype " << fEvtype[fArm];
     cout << "  helicity " << present_reading[fArm];
     cout << "  qrt " << fQrt[fArm];
     cout << " gate " << fGate[fArm];
     cout << "   time stamp " << fTimestamp[fArm] << endl;
   }
   if (nscaler <= 0) return;
   if (nscaler > 2) nscaler = 2;  // shouldn't be necessary
// 32 channels of scaler data for two helicities.
   if (HELDEBUG >= 3) cout << "Synch event ----> " << endl;
   for (int ihel = 0; ihel < nscaler; ihel++) { 
       header = evdata.GetRawData(myroc,index++);
       if (HELDEBUG >= 3) {
         cout << "Scaler for helicity = " << dec << ihel;
         cout << "  unique header = " << hex << header << endl;
       }
       for (int ichan = 0; ichan < 32; ichan++) {
	 data = evdata.GetRawData(myroc,index++);
         if (HELDEBUG >= 3) {
           cout << "channel # " << dec << ichan+1;
           cout << "  (hex) data = " << hex << data << endl;
	 }
       }
   }
   numread = evdata.GetRawData(myroc,index++);
   badread = evdata.GetRawData(myroc,index++);
   if (HELDEBUG >= 3) 
      cout << "FIFO num of last good read " << dec << numread << endl;
// We hope badread=0 forever.  If not, two possibilities :
//   a) Skip that run, or
//   b) Cut out bad events and use ring buffer to recover.
// See R. Michaels if these messages ever appear !!
   if (badread != 0) {
      cout << "NOTE: There are bad scaler readings !! " << endl;
      cout << "FIFO num of last bad read " << endl;
      cout << badread << endl;
   }
// Subset of scaler channels stored in a 30 Hz ring buffer.
   int nring = 0;
   while (index < len && nring == 0) {
     header = evdata.GetRawData(myroc,index++);
     if ((header & 0xffff0000) == 0xfb1b0000) {
         nring = header & 0x3ff;
     }
   }
   if (HELDEBUG >= 3) 
     cout << "Num in ring buffer = " << dec << nring << endl;
   for (int iring = 0; iring < nring; iring++) {
     ring_clock = evdata.GetRawData(myroc,index++);
     data = evdata.GetRawData(myroc,index++);
     ring_qrt = (data & 0x10) >> 4;
     ring_helicity = (data & 0x1);
     ring_trig = evdata.GetRawData(myroc,index++);
     ring_bcm = evdata.GetRawData(myroc,index++);
     ring_l1a = evdata.GetRawData(myroc,index++);
     ring_v2fh = evdata.GetRawData(myroc,index++);
     if (HELDEBUG >= 3) {
        cout << "buff [" << dec << iring << "] ";
        cout << "  clck " << ring_clock << "  qrt " << ring_qrt;
        cout << "  hel " << ring_helicity;
        cout << "  trig " << ring_trig << "  bcm " << ring_bcm;
        cout << "  L1a "<<ring_l1a<<"  v2fh "<<ring_v2fh<<endl;
     }
   }
   return;
}

//____________________________________________________________________
void THaHelicity::QuadCalib() {
// Calibrate the helicity predictor shift register.

  // Is the Qrt signal delayed such that the evt9 at the beginning of
  // a Qrt does NOT contain the qrt flag?
   int delayed_qrt = 1; 

   fTdiff[fArm] = fTimestamp[fArm] - fT0[fArm];
   if (HELDEBUG >= 2) {
     cout << "QuadCalib "<<fTimestamp[fArm]<<"  "<<fT0[fArm];
     cout << "  "<<fTdiff[fArm]<<"  "<<fQrt[fArm]<<endl;
   }
   if (fFirstquad[fArm] == 0 &&
       fTdiff[fArm] > (1.25*fTdavg[fArm] + fTtol[fArm])) {
	// Try a recovery.  Use time to flip helicity by the number of
	// missed quads, unless this are more than 30 (~1 sec).  
        Int_t nqmiss = (Int_t)(fTdiff[fArm]/fTdavg[fArm]);
        if (HELDEBUG >= 2)
          cout << "Recovering large DT, nqmiss = "<<nqmiss<<endl;
        if (nqmiss < 30) {
	  for (Int_t i = 0; i < nqmiss; i++) {
	    fNqrt[fArm]++;
	    QuadHelicity(1);
	    fT0[fArm] += fTdavg[fArm];
	    q1_present_helicity[fArm] = present_helicity[fArm];
	    if (HELDEBUG>=2) {
	      printf("QuadCalibQrt %5d  M  M %1d %2d  %10.0f  %10.0f  %10.0f Missing\n",
		     fNqrt[fArm],q1_reading[fArm],q1_present_helicity[fArm],
		     fTimestamp[fArm],fT0[fArm],fTdiff[fArm]);
	    }

	  }
          fTdiff[fArm] = fTimestamp[fArm] - fT0[fArm];
        } else { 
          cout << "WARN: THaHelicity: Skipped QRT.  Low rate ? " << endl;
          recovery_flag = 1;    // clear & recalibrate the predictor
          quad_calibrated[fArm] = 0;
          fFirstquad[fArm] = 1;
        }
   }
   if (fQrt[fArm] == 1  && fFirstquad[fArm] == 1) {
       fT0[fArm] = fTimestamp[fArm];
       fFirstquad[fArm] = 0;
   }
   if (fEvtype[fArm] == 9) {
       t9count[fArm] += 1;
       if ( delayed_qrt && fQrt[fArm] ) {
	 // don't record this time     
       } else {
	 fT9[fArm] = fTimestamp[fArm]; // this sets the timing reference.
       }
   }
   if (fQrt[fArm] == 1) t9count[fArm] = 0;

// The most solid predictor of a new helicity window:
   // (RJF) Simply look for sufficient time to have passed with a fQrt signal.
   // Do not worry about  evt9's for now, since that transition is redundant
   // when a new Qrt is found. And frequently the evt9 came immediately
   // BEFORE the Qrt.
   if ( ( fTdiff[fArm] > 0.5*fTdavg[fArm] )  && ( fQrt[fArm] == 1 ) ) {
// ||
// On rare occassions QRT bit might be missing.  Then look for
// evtype 9 w/ gate = 0 within the first 0.5 msec after 4th window.
// However this doesn't work well because of time fluctuations,
// so I leave it out for now.  Missing QRT is hopefully rare.
//        (( fTdiff[fArm] > 1.003*fTdavg[fArm] )  &&
//     ( fEvtype[fArm] == 9 && fGate[fArm] == 0 && t9count[fArm] > 3 )) ) {
       if (HELDEBUG >= 2) cout << "found qrt "<<endl;

// Update fT0 to match the nearest/closest last evt9 (extrapolated from the
//  last observed type 9 event) at the beginning of the Qrt.
       fT0[fArm] = TMath::Floor((fTimestamp[fArm]-fT9[fArm])/(.25*fTdavg[fArm]))
	 *(.25*fTdavg[fArm]) + fT9[fArm];
       q1_reading[fArm] = present_reading[fArm];
       QuadHelicity();
       q1_present_helicity[fArm] = present_helicity[fArm];
       fNqrt[fArm]++;
       if (quad_calibrated[fArm] && !CompHel() ) {
// This is ok if it doesn't happen too often.  You lose these events.
          cout << "WARNING: THaHelicity: QuadCalib";
          cout << " G0 prediction failed."<<endl;
          if (HELDEBUG >= 3) {
            cout << "Qrt " << fQrt[fArm] << "  Tdiff "<<fTdiff[fArm];
            cout<<"  gate "<<fGate[fArm]<<" evtype "<<fEvtype[fArm]<<endl;
	  }
          recovery_flag = 1;    // clear & recalibrate the predictor
          quad_calibrated[fArm] = 0;
          fFirstquad[fArm] = 1;
          present_helicity[fArm] = Unknown;
       }
       if (HELDEBUG>=2) {
	 printf("QuadCalibQrt %5d  %1d  %1d %1d %2d  %10.0f  %10.0f  %10.0f\n",
		fNqrt[fArm],fEvtype[fArm],fQrt[fArm],q1_reading[fArm],
		q1_present_helicity[fArm],fTimestamp[fArm],fT0[fArm],
		fTdiff[fArm]);
       }
   }
   fTdiff[fArm] = fTimestamp[fArm] - fT0[fArm];
   return;
}


//____________________________________________________________________
void THaHelicity::LoadHelicity() {
// Load the helicity in G0 mode.  
// If fGate == 0, helicity remains 'Unknown'.

 if ( quad_calibrated[fArm] ) validHel[fArm] = 1;
 if ( !quad_calibrated[fArm]  || fGate[fArm] == 0 ) {
       present_helicity[fArm] = Unknown;
       return;
 }
 if (HELDEBUG >= 2) cout << "Loading helicity ##########"<<endl;
// Look for pattern (+ - - +)  or (- + + -) to decide where in quad
 if (fTdiff[fArm] < 0.4*fTdavg[fArm] && fQrt[fArm] == 1) {
          inquad = 1;  
 }
 if (fTdiff[fArm] < 0.9*fTdavg[fArm] && fQrt[fArm] == 0 &&
     present_reading[fArm] != q1_reading[fArm]) {
          inquad = 2;  // same is inquad = 3
 }
 if (fTdiff[fArm] < 1.1*fTdavg[fArm] && fQrt[fArm] == 0 &&
     present_reading[fArm] == q1_reading[fArm]) {
          inquad = 4;  
 }
 if (inquad == 1 || inquad >= 4) {
     present_helicity[fArm] = q1_present_helicity[fArm];
 } else {
     if (q1_present_helicity[fArm] == Plus) 
            present_helicity[fArm] = Minus;
     if (q1_present_helicity[fArm] == Minus) 
            present_helicity[fArm] = Plus;
 }
 if (HELDEBUG >= 2) {
    cout << dec << "Quad   "<<inquad;
    cout << "   Time diff "<<fTdiff[fArm];
    cout << "   Qrt  "<<fQrt[fArm];
    cout << "   Helicity "<<present_helicity[fArm]<<endl;
 }
 return;
}

//____________________________________________________________________
void THaHelicity::QuadHelicity(Int_t cond) {
// Load the helicity from the present reading for the
// start of a quad.
  int i, dummy;
  if (recovery_flag) nb = 0;
  recovery_flag = 0;
  if (nb < fgNbits) {
      hbits[nb] = present_reading[fArm];
      nb++;
      quad_calibrated[fArm] = 0;
  } else if (nb == fgNbits) {   // Have finished loading
      iseed_earlier = GetSeed();
      iseed = iseed_earlier;
      for (i = 0; i < fgNbits+1; i++) {
          predicted_reading[fArm] = RanBit(0);
          dummy = RanBit(1);
      }
      present_helicity[fArm] = predicted_reading[fArm];
// Delay by fgG0delay windows which is fgG0delay/4 quads
      for (i = 0; i < fgG0delay/4; i++)
          present_helicity[fArm] = RanBit(1);
      nb++;
      saved_helicity[fArm] = present_helicity[fArm];
      quad_calibrated[fArm] = 1;
  } else {      
      present_helicity[fArm] = saved_helicity[fArm];
      if (cond == 0) {
        if (fTimestamp[fArm] - fTlastquad[fArm] < 
	   0.3 * fTdavg[fArm]) return;  // Don't calibrate twice same qrt.
      }
      predicted_reading[fArm] = RanBit(0);
      present_helicity[fArm] = RanBit(1);
      saved_helicity[fArm] = present_helicity[fArm];
      fTlastquad[fArm] = fTimestamp[fArm];
      if (HELDEBUG >= 1) {
        cout << "quad helicities  " << dec;
        cout << predicted_reading[fArm];           // -1, +1
        cout << " ?=? " << present_reading[fArm];  //  0,  1
        cout << "   pred-> " << present_helicity[fArm]<<endl;
        if (CompHel()) cout << "HELICITY OK "<<endl;
      }
      quad_calibrated[fArm] = 1;

  }
  return;
}

//____________________________________________________________________
Int_t THaHelicity::RanBit(int which) {
// This is the random bit generator according to the G0
// algorithm described in "G0 Helicity Digital Controls" by 
// E. Stangland, R. Flood, H. Dong, July 2002.
// Argument:                                                    
//        which = 0 or 1                                        
//     if 0 then iseed_earlier is modified                      
//     if 1 then iseed is modified                              
// Return value:                                                
//        helicity (-1 or +1).

  static int IB1 = 1;           // Bit 1
  static int IB3 = 4;           // Bit 3
  static int IB4 = 8;           // Bit 4
  static int IB24 = 8388608;    // Bit 24 
  static int MASK = IB1+IB3+IB4+IB24;

  if (which == 0) {
     if(iseed_earlier & IB24) {    
         iseed_earlier = ((iseed_earlier^MASK)<<1) | IB1;
         return Plus;
     } else  { 
         iseed_earlier <<= 1;
         return Minus;
     }
  } else {
     if(iseed & IB24) {    
         iseed = ((iseed^MASK)<<1) | IB1;
         return Plus;
     } else  { 
         iseed <<= 1;
         return Minus;
     }
  }

};


//____________________________________________________________________
UInt_t THaHelicity::GetSeed() {
  int seedbits[fgNbits];
  UInt_t ranseed = 0;
  for (int i = 0; i < 20; i++) seedbits[23-i] = hbits[i];
  seedbits[3] = hbits[20]^seedbits[23];
  seedbits[2] = hbits[21]^seedbits[22]^seedbits[23];
  seedbits[1] = hbits[22]^seedbits[21]^seedbits[22];
  seedbits[0] = hbits[23]^seedbits[20]^seedbits[21]^seedbits[23];
  for (int i=fgNbits-1; i >= 0; i--) ranseed = ranseed<<1|(seedbits[i]&1);
  ranseed = ranseed&0xFFFFFF;
  return ranseed;
}

//____________________________________________________________________
Bool_t THaHelicity::CompHel() {
// Compare the present reading to the predicted reading.
// The raw data are 0's and 1's which compare to predictions of
// -1 and +1.  A prediction of 0 occurs when there is no gate.
  if (present_reading[fArm] == 0 && 
      predicted_reading[fArm] == -1) return kTRUE;
  if (present_reading[fArm] == 1 && 
      predicted_reading[fArm] == 1) return kTRUE;
  return kFALSE;
}









