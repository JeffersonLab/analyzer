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
// For data prior to Sept 1, 2002, you can set fgG0mode=0 and
// this algorithm should work.  For data after Sept 1, 2002 we
// use the G0 helicity scheme.  Set fgG0mode=1 and then set 
// fgG0delay=8 or =0 according to the delay of helicity.
//
// This class is used by THaEvData.  The helicity and timestamp
// data are registered global variables.
//
////////////////////////////////////////////////////////////////////////


#include "THaHelicity.h"
#include "THaEvData.h"
#include <string.h>
using namespace std;

ClassImp(THaHelicity)

const Double_t THaHelicity::fgTdiff = 14050;

//____________________________________________________________________
THaHelicity::THaHelicity( ) 
{
  fDetMap = 0;
  Init();
}

//____________________________________________________________________
THaHelicity::~THaHelicity( ) 
{
#ifndef STANDALONE
  if (fDetMap) delete fDetMap;
#endif
  if (fgG0mode) {
    delete [] fQrt;
    delete [] fGate;
    delete [] fFirstquad;
    delete [] fEvtype;
    delete [] fTimestamp;
    delete [] fTdavg;
    delete [] fTdiff;
    delete [] fT0;
    delete [] fTlastquad;
    delete [] fTtol;
    delete [] t9count;
    delete [] present_reading;
    delete [] q1_reading;
    delete [] predicted_reading;
    delete [] present_helicity;
    delete [] saved_helicity;
    delete [] q1_present_helicity;
    delete [] quad_calibrated;
    delete [] hbits;
    delete [] validTime;
    delete [] validHel;
  }
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
void THaHelicity::Init() {
  if (fgG0mode == 0) {
// For non-G0 mode where we used ADC flags
#ifndef STANDALONE
    fDetMap = new THaDetMap;
    fDetMap->AddModule(1,25,60,61);  // Redundant channels
    fDetMap->AddModule(2,25,14,15);    
    fDetMap->AddModule(2,25,46,47);    
    fDetMap->AddModule(14,6,0,1);
#endif
    indices[0][0] = 60;
    indices[0][1] = 61;
    indices[1][0] = 14;
    indices[1][1] = 15;
  } else {
    fQrt                = new Int_t[2];
    fGate               = new Int_t[2];
    fFirstquad          = new Int_t[2];
    fEvtype             = new Int_t[2];
    fTimestamp          = new Double_t[2];
    fTdavg              = new Double_t[2];
    fTdiff              = new Double_t[2];
    fTtol               = new Double_t[2];
    fT0                 = new Double_t[2];
    fTlastquad          = new Double_t[2];
    t9count             = new Int_t[2];
    present_reading     = new Int_t[2];   
    q1_reading          = new Int_t[2];   
    predicted_reading   = new Int_t[2];
    present_helicity    = new Int_t[2];
    saved_helicity      = new Int_t[2];
    quad_calibrated     = new Int_t[2]; 
    q1_present_helicity = new Int_t[2];
    hbits               = new Int_t[fgNbits];
    validTime           = new Int_t[2];
    validHel            = new Int_t[2];
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
    memset(q1_reading, 0, 2*sizeof(Int_t));
    memset(predicted_reading, Unknown, 2*sizeof(Int_t));
    memset(saved_helicity, Unknown, 2*sizeof(Int_t));
    memset(quad_calibrated, 0, 2*sizeof(Int_t));
    ClearEvent();
    recovery_flag = 1;
    iseed = 0;
    iseed_earlier = 0;
    inquad = 0;
  }  
}

//____________________________________________________________________
Int_t THaHelicity::Decode( const THaEvData& evdata ) {
// Decode Helicity  data.

 ClearEvent();  

 if (fgG0mode == 0) {  // Non-G0 helicity mode
   UShort_t k;
   Int_t indx;
#ifndef STANDALONE
   for( UShort_t i = 0; i < fDetMap->GetSize(); i++ ) {
     THaDetMap::Module* d = fDetMap->GetModule( i );   
     for( UShort_t j = 0; j < evdata.GetNumChan( d->crate, d->slot); j++) {
      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      Double_t data = evdata.GetData (d->crate, d->slot, chan, 0);
      if( chan > d->hi || chan < d->lo ) continue;   
       if (d->crate == 1) {
         indx = NCHAN+1;
         for ( k = 0; k < 2; k++) {	
           if (chan == indices[0][k]) indx = k;
         }
         if(indx >=0 && indx < NCHAN) Ladc_helicity[indx] = data;
       }
       if (d->crate == 2) {
         indx = NCHAN+1;
         for (k = 0; k < 2; k++) {
           if (chan == indices[1][k]) indx = k;
	 }
         if(indx >=0 && indx < NCHAN) Radc_helicity[indx] = data;
       }
       if (d->crate == 14) {
          if(chan == 0) timestamp = data;
       }
     }
   }
#else
   cout << "Error: Cannot analyze pre-G0-mode data STANDALONE."<<endl;
//  Must recompile with STANDALONE commented out in Makefile.
//  Then run with rest of analyzer.
//  This is not too bad since the analyzer will mostly live in the G0 era.
#endif
// Use 2 flags to make sure of helicity, for both L and R spectrom.
// helicity[0] is should be opposite of helicity[1].
   Lhel = Minus;
   if (Ladc_helicity[0] > HCUT) {
      Lhel = Plus;
      if (Ladc_helicity[1] > HCUT) Lhel = Unknown;
   } else {
      if (Ladc_helicity[1] < HCUT) Lhel = Unknown;
   } 
   Rhel = Minus;
   if (Radc_helicity[0] > HCUT) {
      Rhel = Plus;
      if (Radc_helicity[1] > HCUT) Rhel = Unknown;
   } else {
      if (Radc_helicity[1] < HCUT) Rhel = Unknown;
   }
   Hel = Rhel;   // R spectrometer flag is used if running 1-spectrom DAQ.
   if (HELDEBUG >= 1) 
      cout << "Helicity L,R "<<Lhel<<" "<<Rhel<<" time "<<timestamp<<endl;
   return OK;

 } else {   // G0 helicity mode:

   for (Int_t spec = fgLarm; spec <= fgRarm; spec++) {
      fArm = spec;  // fArm is global
      ReadData ( evdata );    
      QuadCalib ();      
      LoadHelicity ();
   }
   Lhel = present_helicity[fgLarm];
   Rhel = present_helicity[fgRarm];
   Hel = Lhel;   // default helicity for 1-arm setup
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
// Reset data to prepare for next event.
  
  if (fgG0mode == 0) {
     memset( Ladc_helicity, 0, NCHAN*sizeof(Double_t) );
     memset( Radc_helicity, 0, NCHAN*sizeof(Double_t) );
  } else {
     memset(fQrt, 0, 2*sizeof(Int_t));
     memset(fGate, 0, 2*sizeof(Int_t));
     memset(fTimestamp, 0, 2*sizeof(Double_t));
     memset(present_reading, Unknown, 2*sizeof(Int_t));
     memset(present_helicity, Unknown, 2*sizeof(Int_t));
     memset(validTime, 0, 2*sizeof(Int_t));
     memset(validHel, 0, 2*sizeof(Int_t));
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
   fTdiff[fArm] = fTimestamp[fArm] - fT0[fArm];
   if (HELDEBUG >= 2) {
     cout << "QuadCalib "<<fTimestamp[fArm]<<"  "<<fT0[fArm];
     cout << "  "<<fTdiff[fArm]<<"  "<<fQrt[fArm]<<endl;
   }
   if (fFirstquad[fArm] == 0 &&
       fTdiff[fArm] > (1.25*fTdavg[fArm] + fTtol[fArm])) {
        cout << "WARN: THaHelicity: Skipped QRT.  Low rate ? " << endl;
        if (HELDEBUG >= 2)
          cout << fTdiff[fArm] << "  " << (Int_t)fTimestamp[fArm] << endl;
        recovery_flag = 1;    // clear & recalibrate the predictor
        quad_calibrated[fArm] = 0;
        fFirstquad[fArm] = 1;
   }
   if (fQrt[fArm] == 1  && fFirstquad[fArm] == 1) {
       fT0[fArm] = fTimestamp[fArm];
       fFirstquad[fArm] = 0;
   }
   if (fEvtype[fArm] == 9) t9count[fArm] += 1;
   if (fQrt[fArm] == 1) t9count[fArm] = 0;
   if (
// The most solid predictor of a new helicity window.
      (( fTdiff[fArm] > 0.8*fTdavg[fArm] )  &&
	 ( fQrt[fArm] == 1 ) && fEvtype[fArm]==9) ||
// But sometimes event type 9 may be missed.
        (( fTdiff[fArm] > fTdavg[fArm] )  &&
	 ( fQrt[fArm] == 1 )) ) {
// ||
// On rare occassions QRT bit might be missing.  Then look for
// evtype 9 w/ gate = 0 within the first 0.5 msec after 4th window.
// However this doesn't work well because of time fluctuations,
// so I leave it out for now.  Missing QRT is hopefully rare.
//        (( fTdiff[fArm] > 1.003*fTdavg[fArm] )  &&
//     ( fEvtype[fArm] == 9 && fGate[fArm] == 0 && t9count[fArm] > 3 )) ) {
       if (HELDEBUG >= 2) cout << "found qrt "<<endl;
       fT0[fArm] = fTimestamp[fArm];
       q1_reading[fArm] = present_reading[fArm];
       QuadHelicity();
       q1_present_helicity[fArm] = present_helicity[fArm];
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
void THaHelicity::QuadHelicity() {
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
      if (fTimestamp[fArm] - fTlastquad[fArm] < 
	 0.3 * fTdavg[fArm]) return;  // Don't calibrate twice same qrt.
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









