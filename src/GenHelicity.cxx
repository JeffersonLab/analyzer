//*-- Author :    Robert Michaels    Sept 2002
// Updated by     Vincent Sulkosky   Jan  2006
// More updates by Richard Holmes,   Feb 2006
////////////////////////////////////////////////////////////////////////
//
// GenHelicity
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


#include "GenHelicity.h"
#include "THaEvData.h"
#include <string.h>
using namespace std;

ClassImp(GenHelicity)

const Double_t GenHelicity::fgTdiff = 14050;

//____________________________________________________________________
GenHelicity::GenHelicity( ) :
  fTdavg(NULL), fTdiff(NULL), fT0(NULL), fT9(NULL), 
  fT0T9(NULL), fTlastquad(NULL),
  fTtol(NULL), fQrt(NULL), fGate(NULL), fFirstquad(NULL), fEvtype(NULL),
  fQuad(NULL), 
  fTimestamp(NULL), fLastTimestamp(NULL), fTimeLastQ1(NULL),
  validTime(NULL), validHel(NULL), t9count(NULL),
  present_reading(NULL), predicted_reading(NULL), q1_reading(NULL),
  present_helicity(NULL), saved_helicity(NULL), q1_present_helicity(NULL),
  quad_calibrated(NULL), hbits(NULL), fNqrt(NULL),
  fTET9Index(NULL),
  fTELastEvtQrt(NULL),
  fTELastEvtTime(NULL),
  fTELastTime(NULL),
  fTEPresentReadingQ1(NULL),
  fTEStartup(NULL),
  fTETime(NULL),
  fTEType9(NULL)
{
  InitMemory();
  // reasonable default state for GEN.
  SetROC (fgRarm, 11, 0, 3, 0, 4); // likewise
  SetState (1, 8, -1, fgRarm, 0);  // G0 mode; 8 window delay; sign -1;
                                  // right arm; no redund
  SetROC (fgRarm, 28, 0xfadcb0b9, 2, 0xfabc0007, 6); // "Right arm" is ROC
                                   //            28; headers and indices
   // Redundant clocks: 2 words after std clock, and same location i
   // as std clock but in ROC 23:
  SetRTimeROC (1, 28, 0xfabc0007, 8, 23, 0xfabc0007, 6);
}

//____________________________________________________________________
GenHelicity::~GenHelicity( ) 
{
  if (fQrt) delete [] fQrt;
  if (fGate) delete [] fGate;
  if (fFirstquad) delete [] fFirstquad;
  if (fEvtype) delete [] fEvtype;
  if (fQuad) delete [] fQuad;
  if (fTimestamp) delete [] fTimestamp;
  if (fLastTimestamp) delete [] fLastTimestamp;
  if (fTimeLastQ1) delete [] fTimeLastQ1;
  if (fTdavg) delete [] fTdavg;
  if (fTdiff) delete [] fTdiff;
  if (fT0) delete [] fT0;
  if (fT9) delete [] fT9;
  if (fT0T9) delete [] fT0T9;
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
  if (fTET9Index) delete [] fTET9Index;
  if (fTELastEvtQrt) delete [] fTELastEvtQrt;
  if (fTELastEvtTime) delete [] fTELastEvtTime;
  if (fTELastTime) delete [] fTELastTime;
  if (fTEPresentReadingQ1) delete [] fTEPresentReadingQ1;
  if (fTEStartup) delete [] fTEStartup;
  if (fTETime) delete [] fTETime;
  if (fTEType9) delete [] fTEType9;
}

//____________________________________________________________________
int GenHelicity::GetHelicity() const 
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
	 cout << "GenHelicity::GetHelicity: Inconsistent helicity";
         cout << " between spectrometers."<<endl;
       }
     }
     if (lhel > -2) return lhel;
     if (rhel > -2) return rhel;
     return 0;
}

//____________________________________________________________________
int GenHelicity::GetHelicity(const TString& spec) const {
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
void GenHelicity::SetState(int mode, int delay, 
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
void GenHelicity::SetROC (int arm, int roc,
			  int helheader, int helindex,
			  int timeheader, int timeindex)
{
// Set parameters for ROC readout.  Note, if a header is zero the
// index is taken to be from the start of the ROC (0 = first word of ROC), 
// otherwise it's from the header (0 = first word after header).

  fROC[arm] = roc;                  // ROC 
  fHelHeader[arm] = helheader;      // Header for helicity bit
  fHelIndex[arm] = helindex;        // Index from header
  fTimeHeader[arm] = timeheader;    // Header for timestamp
  fTimeIndex[arm] = timeindex;      // Index from header
}

//____________________________________________________________________
void GenHelicity::SetRTimeROC (int arm, 
			       int roct2, int t2header, int t2index, 
			       int roct3, int t3header, int t3index)
{
  // Set parameters for reading redundant time info.

  fRTimeROC2[arm] = roct2;                  // ROC 
  fRTimeHeader2[arm] = t2header;    // Header for timestamp
  fRTimeIndex2[arm] = t2index;      // Index from header
  fRTimeROC3[arm] = roct3;                  // ROC 
  fRTimeHeader3[arm] = t3header;    // Header for timestamp
  fRTimeIndex3[arm] = t3index;      // Index from header
}
  

//____________________________________________________________________
void GenHelicity::InitMemory() {
  validTime           = new Int_t[2];
  validHel            = new Int_t[2];
  fQrt                = new Int_t[2];
  fGate               = new Int_t[2];
  fFirstquad          = new Int_t[2];
  fEvtype             = new Int_t[2];
  fQuad               = new Int_t[2];
  fTimestamp          = new Double_t[2];
  fLastTimestamp      = new Double_t[2];
  fTimeLastQ1         = new Double_t[2];
  fTdavg              = new Double_t[2];
  fTdiff              = new Double_t[2];
  fTtol               = new Double_t[2];
  fT0                 = new Double_t[2];
  fT9                 = new Double_t[2];
  fT0T9               = new Bool_t[2];
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
  fTET9Index       = new   Int_t[2];
  fTELastEvtQrt       = new   Int_t[2];
  fTELastEvtTime      = new   Double_t[2];
  fTELastTime         = new   Double_t[2];
  fTEPresentReadingQ1 = new   Int_t[2];
  fTEStartup          = new   Int_t[2];
  fTETime             = new   Double_t[2];
  fTEType9            = new   Bool_t[2];
  for (int i=0; i<2; i++) {
    validTime[i] = validHel[i] = fQrt[i] = fGate[i] = fFirstquad[i] = fEvtype[i] = fQuad[i] = 0;
    fTimestamp[i] = fLastTimestamp[i] = fTimeLastQ1[i] = fTdavg[i] = fTdiff[i] = fTtol[i] = fT0[i] = fT9[i] = 0.;
    fT0T9[i] = fTEType9[i] = false;
    fTlastquad[i] = fTELastEvtTime[i] = fTELastTime[i] = fTETime[i] = 0.;
    t9count[i] = fNqrt[i] = present_reading[i] = q1_reading[i] = predicted_reading[i] = present_helicity[i] = 0;
    saved_helicity[i] = quad_calibrated[i] = q1_present_helicity[i] = fTET9Index[i] = fTELastEvtQrt[i] = 0;
    fTEPresentReadingQ1[i] = fTEStartup[i] = 0;
  }
  for (int i=0; i<fgNbits; hbits[i++]=0);
}

//____________________________________________________________________
void GenHelicity::InitG0() {
// Initialize some things relevant to G0 mode.
// This is irrelevant for in-time mode, but its good to zero things.
  fTdavg[0]  = fgTdiff;  // Avg time diff between qrt's
  fTdavg[1]  = fgTdiff;    
  fTtol[0]   = 20;   
  fTtol[1]   = 20;       
  fT0[0]     = 0;
  fT0[1]     = 0;
  fT9[0]     = 0;
  fT9[1]     = 0;
  fTimestamp[0] = 0;
  fTimestamp[1] = 0;
  fLastTimestamp[0] = 0;
  fLastTimestamp[1] = 0;
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
  memset (fTET9Index, 0, 2 * sizeof (Int_t));
  memset (fTELastEvtQrt, -1, 2 * sizeof (Int_t));
  memset (fTELastEvtTime, -1, 2 * sizeof (Double_t));
  memset (fTELastTime, -1, 2 * sizeof (Double_t));
  memset (fTEPresentReadingQ1, -1, 2 * sizeof (Int_t));
  memset (fTEStartup, 3, 2 * sizeof (Int_t));
  memset (fTETime, -1, 2 * sizeof (Double_t));
  memset (fTEType9, 0, 2 * sizeof (Bool_t));
}

//____________________________________________________________________
Int_t GenHelicity::Decode( const THaEvData& evdata ) {
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

        if (HELDEBUG >= 3) {
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
    
    if (HELDEBUG >= 3) {
      cout << "ADC helicity info "<<endl;
      cout << "L-arm gate "<<Ladc_gate<<"  helic. bit "<<Ladc_hbit;
      cout << "    resulting helicity "<<Ladc_helicity<<endl;
      cout << "R-arm gate "<<Radc_gate<<"  helic. bit "<<Radc_hbit;
      cout << "    resulting helicity "<<Radc_helicity<<endl;
    }

   }

// Here is the new (ca 2002) way to get helicity and timestamp

   Int_t myroc;
   myroc=0;
 
// Modified 01/23/2006 by V. Sulkosky
// This section modified to get ring buffer data for in-time 
// helicity mode.

   for (Int_t spec = fgLarm; spec <= fgRarm; spec++) {

     fArm = spec;  // fArm is global
     ReadData ( evdata );

     if (fGate[fArm] != 0) {
        if (present_reading[fArm]) {
	   if (fArm == fgLarm) {  // roc11 = LHRS
              Lhel = Plus;
           } else {
              Rhel = Plus;
           }
        } else {
           if (fArm == fgLarm) {
              Lhel = Minus;
           } else {
              Rhel = Minus;
           }
        }
     }
   }

   if (fWhichSpec == fgLarm) { // pick a spectrometer
       myroc = fROC[fgLarm];
       Hel = fSign * Lhel;  // Sign determined by Moller measurement
   } else {
       myroc = fROC[fgRarm];
       Hel = fSign * Rhel;  // Sign determined by Moller measurement
   }

   validHel[fWhichSpec] = 1;
   validTime[fWhichSpec] = 1;
   if (HELDEBUG >= 3) { 
     cout << "In-time helicity check for roc "<<myroc<<endl;
     //cout << "Raw word 0x"<<hex<<data<<dec;
     cout << "  Gate "<<fGate[fWhichSpec]<<"  qrt "<<fQrt[fWhichSpec];
     cout << "  helicity bit "<<present_reading[fArm];
     cout << "  Resulting helicity "<<Hel<<endl;
     cout << "  timestamp "<<fTimestamp[fWhichSpec]<<endl;
   }

   if (fCheckRedund) {
   
// Here we check redunandcy and complain if something disagrees.
// This might be flakey due to timing delays, miscabling, things
// being turned off, etc, but at least it shows the idea.

//     if (Lhel != Rhel || Lhel != Ladc_helicity || 
//         Lhel != Radc_helicity) {
     if (Lhel != Radc_helicity) { // for DVCS...
          cout << "GenHelicity::WARNING:: ";
          cout << " Inconsistent helicity data !!"<<endl;
     }
   }

 } else {   // G0 helicity mode (data are delayed).

   for (Int_t spec = fgLarm; spec <= fgRarm; spec++) {
      fArm = spec;  // fArm is global
      ReadData ( evdata );    
      TimingEvent ();
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
   if (HELDEBUG >= 3) cout << "G0 helicities "<<Lhel<<"  "<<Rhel<<endl;

 }

 return OK;
}
 
//____________________________________________________________________
Double_t GenHelicity::GetTime() const {
// Get the timestamp, a ~105 kHz clock.
  if (validTime[fgLarm]) return fTimestamp[fgLarm];
  if (validTime[fgRarm]) return fTimestamp[fgRarm];
  return 0;
}

//____________________________________________________________________
void GenHelicity::ClearEvent() {
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
  memset(fQrt, 0, 2*sizeof(Int_t));
  memset(fGate, 0, 2*sizeof(Int_t));
  fLastTimestamp[0] = fTimestamp[0];
  fLastTimestamp[1] = fTimestamp[1];
  memset(fTimestamp, 0, 2*sizeof(Double_t));
  memset(present_reading, Unknown, 2*sizeof(Int_t));
  memset(present_helicity, Unknown, 2*sizeof(Int_t));
  memset(fEvtype, 0, 2*sizeof(Int_t));
  memset(fQuad, 0, 2*sizeof(Int_t));
  
}

//____________________________________________________________________
void GenHelicity::ReadData( const THaEvData& evdata ) {
// Obtain the present data from the event for G0 helicity mode.

   int myroc, len, data, nscaler, header;
   int found, index;

   if (fArm == fgLarm || fArm == fgRarm) 
     myroc = fROC[fArm];
   else
     return;

   len = evdata.GetRocLength(myroc);
   if (len <= 4) return;

   Int_t ihel;
   Int_t itime;
   ihel = FindWord (evdata, myroc, fHelHeader[fArm], fHelIndex[fArm]);
   if (ihel < 0)
     {
       cout << "GenHelicity WARNING: Cannot find helicity" << endl;
       return;
     }
   itime = FindWord (evdata, myroc, fTimeHeader[fArm], fTimeIndex[fArm]);
   if (itime < 0)
     {
       cout << "GenHelicity WARNING: Cannot find timestamp" << endl;
       return;
     }

   data = evdata.GetRawData(myroc,ihel);   
   present_reading[fArm] = (data & 0x10) >> 4;
   fQrt[fArm] = (data & 0x20) >> 5;
   fGate[fArm] = (data & 0x40) >> 6;
   fTimestamp[fArm] = evdata.GetRawData(myroc,itime);

   // Look for redundant clock info and patch up time if it appears wrong
   static Double_t oldt1 = -1;
   static Double_t oldt2 = -1;
   static Double_t oldt3 = -1;
   if (fRTimeROC2[fArm] > 0 && fRTimeROC3[fArm] > 0)
     {
       Int_t itime2 = FindWord (evdata, fRTimeROC2[fArm], fRTimeHeader2[fArm], fRTimeIndex2[fArm]);
       Int_t itime3 = FindWord (evdata, fRTimeROC3[fArm], fRTimeHeader3[fArm], fRTimeIndex3[fArm]);
       if (itime2 >= 0 && itime3 >= 0)
	 {
	   Double_t t1 = fTimestamp[fArm];
	   Double_t t2 = evdata.GetRawData (fRTimeROC2[fArm], itime2);
	   Double_t t3 = evdata.GetRawData (fRTimeROC3[fArm], itime3);
	   Double_t t2t1 = oldt1 + t2 - oldt2;
	   Double_t t3t1 = oldt1 + t3 - oldt3;
	   // We believe t1 unless it disagrees with t2 and t2 agrees with t3
	   if (oldt1 >= 0 && TMath::Abs (t1-t2t1) > 3 && TMath::Abs (t2t1-t3t1) <= 3)
	     {
	       if (HELDEBUG >= 1)
		 cout << "WARNING GenHelicity: Clock 1 disagrees with 2, 3; using 2: " << t1 << " " << t2t1 << " " << t3t1 << endl;
	       fTimestamp[fArm] = t2t1;
	     }
	   if (HELDEBUG >= 3)
	     {
	       cout << "Clocks: " << t1 
		    << " " << t2 << "->" << t2t1 
		    << " " << t3 << "->" << t3t1 
		    << endl;
	     }
	   oldt1 = fTimestamp[fArm];
	   oldt2 = t2;
	   oldt3 = t3;
	 }
     }
       
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
     if (HELDEBUG >= 2) cout << "WARNING: Cannot find scalers."<<endl;
     nscaler = 0;
   }
   fEvtype[fArm] = evdata.GetEvType();
   if (HELDEBUG >= 3) {
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
   if (HELDEBUG >= 2) cout << "Synch event ----> " << endl;
   for (int ihel = 0; ihel < nscaler; ihel++) { 
       header = evdata.GetRawData(myroc,index++);
       if (HELDEBUG >= 2) {
         cout << "Scaler for helicity = " << dec << ihel;
         cout << "  unique header = " << hex << header << endl;
       }
       for (int ichan = 0; ichan < 32; ichan++) {
	 data = evdata.GetRawData(myroc,index++);
         if (HELDEBUG >= 2) {
           cout << "channel # " << dec << ichan+1;
           cout << "  (hex) data = " << hex << data << endl;
	 }
       }
   }
   fNumread = evdata.GetRawData(myroc,index++);
   fBadread = evdata.GetRawData(myroc,index++);
   if (HELDEBUG >= 2) 
      cout << "FIFO num of last good read " << dec << fNumread << endl;
// We hope fBadread=0 forever.  If not, two possibilities :
//   a) Skip that run, or
//   b) Cut out bad events and use ring buffer to recover.
// See R. Michaels if these messages ever appear !!
   if (fBadread != 0) {
      cout << "NOTE: There are bad scaler readings !! " << endl;
      cout << "FIFO num of last bad read " << endl;
      cout << fBadread << endl;
   }
// Subset of scaler channels stored in a 30 Hz ring buffer.
   int nring = 0;
   while (index < len && nring == 0) {
     header = evdata.GetRawData(myroc,index++);
     if ((header & 0xffff0000) == 0xfb1b0000) {
         nring = header & 0x3ff;
     }
   }
   if (HELDEBUG >= 2) 
     cout << "Num in ring buffer = " << dec << nring << endl;
   for (int iring = 0; iring < nring; iring++) {
     fRing_clock = evdata.GetRawData(myroc,index++);
     data = evdata.GetRawData(myroc,index++);
     fRing_qrt = (data & 0x10) >> 4;
     fRing_helicity = (data & 0x1);
     fRing_trig = evdata.GetRawData(myroc,index++);
     fRing_bcm = evdata.GetRawData(myroc,index++);
     fRing_l1a = evdata.GetRawData(myroc,index++);
     fRing_v2fh = evdata.GetRawData(myroc,index++);
     if (HELDEBUG >= 2) {
        cout << "buff [" << dec << iring << "] ";
        cout << "  clck " << fRing_clock << "  qrt " << fRing_qrt;
        cout << "  hel " << fRing_helicity;
        cout << "  trig " << fRing_trig << "  bcm " << fRing_bcm;
        cout << "  L1a "<<fRing_l1a<<"  v2fh "<<fRing_v2fh<<endl;
     }
   }
   return;
}

//____________________________________________________________________
void GenHelicity::TimingEvent() 
{
  // Check for and process timing events
  // A timing event is a T9, *or* if enough time elapses without a
  // T9, it's any event with QRT==1 following an event with QRT==0.

  if (fEvtype[fArm] == 9)
    {
      fTEType9[fArm] = true;
      fTETime[fArm] = fTimestamp[fArm];
    }
  else if (fQrt[fArm] && !fTELastEvtQrt[fArm]
	   && fTimestamp[fArm] - fTELastTime[fArm] > 8 * fTdavg[fArm])
    {
      // After a while we give up on finding T9s and instead take
      // either the average timestamp of the first QRT==1 we find
      // and the previous event (if that's not to far in the past)
      // or just the timestamp of the QRT==1 event (if it is).
      // This is lousy accuracy but better than nothing.
      fTEType9[fArm] = false;
      fTETime[fArm] = (fTimestamp[fArm] - fTELastEvtTime[fArm]) < 0.5 * fTdavg[fArm] ? 
	(fTimestamp[fArm] + fTELastEvtTime[fArm]) * 0.5 :
	fTimestamp[fArm];
    }
  else
    {
      fTELastEvtQrt[fArm] = fQrt[fArm];
      fTELastEvtTime[fArm] = fTimestamp[fArm];
      return;
    }

  // Now check for anomalies.

  if (fTELastTime[fArm] > 0)
    {
      // Look for missed timing events
      Double_t tdiff = fTETime[fArm] - fTELastTime[fArm];
      Int_t nt9miss = (Int_t) (tdiff / 3508 - 0.5);
      fTET9Index[fArm] += nt9miss;
      if (fTET9Index[fArm] > 3)
	{
	  fTEPresentReadingQ1[fArm] = -1;
	  fTET9Index[fArm] = fTET9Index[fArm] % 4;
	}
      if (fTEType9[fArm] &&
	  TMath::Abs (tdiff - (nt9miss + 1) * 3508) > 3 * (nt9miss + 1))
	cout << "WARNING GenHelicity: Weird time difference between timing events: " << tdiff 
	     << " at " << fTETime[fArm] << endl;
    }
  ++fTET9Index[fArm];
  if (fQrt[fArm] == 1)
    {
      if (fTET9Index[fArm] != 4 && !fTEStartup[fArm])
	cout << "WARNING GenHelicity: QRT wrong spacing: " << fTET9Index[fArm] 
	     << " at " << fTETime[fArm] << endl;
      fTET9Index[fArm] = 0;
      fTEPresentReadingQ1[fArm] = present_reading[fArm];
      fTEStartup[fArm] = 0;
    }
  else
    {
      if (fTEStartup[fArm]) --fTEStartup[fArm];
      if (fTEPresentReadingQ1[fArm] > -1)
	if ((present_reading[fArm] == fTEPresentReadingQ1[fArm] && 
	     (fTET9Index[fArm] == 1 || fTET9Index[fArm] == 2))
	    || (present_reading[fArm] != fTEPresentReadingQ1[fArm] && 
		fTET9Index[fArm] == 3))
	  cout << "WARNING GenHelicity: Wrong helicity pattern: " << fTEPresentReadingQ1[fArm] 
	       << " in window 1, " << present_reading[fArm] 
	       << " in window " << (fTET9Index[fArm]+1) 
	       << " at " << fTETime[fArm] << endl;
    }
  // Now push back information
  fTELastTime[fArm] = fTETime[fArm];
  fTELastEvtQrt[fArm] = fQrt[fArm];
  fTELastEvtTime[fArm] = fTimestamp[fArm];
}

//____________________________________________________________________
void GenHelicity::QuadCalib() {
// Calibrate the helicity predictor shift register.

  // Is the Qrt signal delayed such that the evt9 at the beginning of
  // a Qrt does NOT contain the qrt flag?
   int delayed_qrt = 1; 

   if (fEvtype[fArm] == 9) {
       t9count[fArm] += 1;
       if ( delayed_qrt && fQrt[fArm] ) {
	 // don't record this time     
       } else {
	 fT9[fArm] = fTimestamp[fArm]; // this sets the timing reference.
       }
   }
   
   if (fEvtype[fArm] != 9 && fGate[fArm] == 0) return;

   fTdiff[fArm] = fTimestamp[fArm] - fT0[fArm];
   if (HELDEBUG >= 3) {
     cout << "QuadCalib "<<fTimestamp[fArm]<<"  "<<fT0[fArm];
     cout << "  "<<fTdiff[fArm]
	  << " " << fEvtype[fArm]
	  <<"  "<<fQrt[fArm]<<endl;
   }
   if (fFirstquad[fArm] == 0 &&
       fTdiff[fArm] > (1.25*fTdavg[fArm] + fTtol[fArm])) {
	// Try a recovery.  Use time to flip helicity by the number of
	// missed quads, unless this are more than 30 (~1 sec).  
        Int_t nqmiss = (Int_t)(fTdiff[fArm]/fTdavg[fArm]);
        if (HELDEBUG >= 1)
          cout << "Recovering large DT, nqmiss = "<<nqmiss
	       << " at timestamp " << fTimestamp[fArm]
	       << " tdiff " << fTdiff[fArm]
	       <<endl;
        if (nqmiss < 30) {
	  for (Int_t i = 0; i < nqmiss; i++) {
	    fNqrt[fArm]++;
	    QuadHelicity(1);

 	    q1_reading[fArm] = predicted_reading[fArm] == -1 ? 0 : 1;

	    if (fQrt[fArm] == 1 && fT9[fArm] > 0 
		&& fTimestamp[fArm] - fT9[fArm] < 8*fTdavg[fArm])
	      {
		fT0[fArm] = TMath::Floor((fTimestamp[fArm]-fT9[fArm])/(.25*fTdavg[fArm]))
		  *(.25*fTdavg[fArm]) + fT9[fArm];
		fT0T9[fArm] = true;
	      }
	    else
	      {
		fT0[fArm] += fTdavg[fArm];
		fT0T9[fArm] = false;
	      }
	    q1_present_helicity[fArm] = present_helicity[fArm];
	    if (HELDEBUG>=1) {
	      printf("QuadCalibQrt %5d  M  M %1d %2d  %10.0f  %10.0f  %10.0f Missing\n",
		     fNqrt[fArm],q1_reading[fArm],q1_present_helicity[fArm],
		     fTimestamp[fArm],fT0[fArm],fTdiff[fArm]);
	    }

	  }
          fTdiff[fArm] = fTimestamp[fArm] - fT0[fArm];
        } else { 
          cout << "WARN: GenHelicity: Skipped QRT.  Low rate ? " << endl;
          recovery_flag = 1;    // clear & recalibrate the predictor
          quad_calibrated[fArm] = 0;
          fFirstquad[fArm] = 1;
        }
   }
   if (fQrt[fArm] == 1  && fFirstquad[fArm] == 1) {
       fT0[fArm] = fTimestamp[fArm];
       fT0T9[fArm] = false;
       fFirstquad[fArm] = 0;
   }

   // Check for QRT at anomalous separations

   if (fEvtype[fArm] == 9 && fQrt[fArm] == 1)
     {
       if (fTimeLastQ1[fArm] > 0)
	 {
	   Double_t q1tdiff = fTimestamp[fArm] - fTimeLastQ1[fArm];
	   if (q1tdiff < .9 * fTdavg[fArm] ||
	       (q1tdiff > 1.1 * fTdavg[fArm] && q1tdiff < 1.9 * fTdavg[fArm]))
	     cout << "WARNING: GenHelicity: QRT==1 timing error --"
		  << " Last time = " << fTimeLastQ1[fArm]
		  << " This time = " << fTimestamp[fArm] << endl
		  << "HELICITY SIGNALS MAY BE CORRUPT" << endl;
	 }
       fTimeLastQ1[fArm] = fTimestamp[fArm];
     }

   if (fQrt[fArm] == 1) t9count[fArm] = 0;

// The most solid predictor of a new helicity window:
   // (RJF) Simply look for sufficient time to have passed with a fQrt signal.
   // Do not worry about  evt9's for now, since that transition is redundant
   // when a new Qrt is found. And frequently the evt9 came immediately
   // BEFORE the Qrt.
   if ( ( fTdiff[fArm] > 0.5*fTdavg[fArm] ) && ( fQrt[fArm] == 1 ) ) {
// ||
// On rare occassions QRT bit might be missing.  Then look for
// evtype 9 w/ gate = 0 within the first 0.5 msec after 4th window.
// However this doesn't work well because of time fluctuations,
// so I leave it out for now.  Missing QRT is hopefully rare.
//        (( fTdiff[fArm] > 1.003*fTdavg[fArm] )  &&
//     ( fEvtype[fArm] == 9 && fGate[fArm] == 0 && t9count[fArm] > 3 )) ) {
       if (HELDEBUG >= 3) cout << "found qrt "<<endl;

       // If we have T9 within recent time: Update fT0 to match the
       // nearest/closest last evt9 (extrapolated from the last
       // observed type 9 event) at the beginning of the Qrt.
       // Otherwise: if there was a previous event very recently, take
       // average of its and this timestamp.  And if not, just take
       // this timestamp.  This will usually be sufficiently accurate
       // for our purposes, though having T9 would be nicer.
       if (fT9[fArm] > 0 && fTimestamp[fArm] - fT9[fArm] < 8*fTdavg[fArm])
	 {
	   fT0[fArm] = TMath::Floor((fTimestamp[fArm]-fT9[fArm])/(.25*fTdavg[fArm]))
	     *(.25*fTdavg[fArm]) + fT9[fArm];
	   fT0T9[fArm] = true;
	 }
       else
	 {
	   fT0[fArm] = (fLastTimestamp == 0 
			|| fTimestamp[fArm] - fLastTimestamp[fArm] > 0.5*fTdavg[fArm])
	     ? fTimestamp[fArm] : 0.5 * (fTimestamp[fArm] + fLastTimestamp[fArm]);
	   fT0T9[fArm] = false;
	 }
       q1_reading[fArm] = present_reading[fArm];
       QuadHelicity();
       q1_present_helicity[fArm] = present_helicity[fArm];
       fNqrt[fArm]++;
       if (quad_calibrated[fArm] && !CompHel() ) {
// This is ok if it doesn't happen too often.  You lose these events.
          cout << "WARNING: GenHelicity: QuadCalib";
          cout << " G0 prediction failed at timestamp " << fTimestamp[fArm] 
	       <<endl;
	  cout << "GenHelicity::HELICITY ASSIGNMENT IN PREVIOUS " 
	       << fgG0delay
	       << " WINDOWS MAY BE INCORRECT" << endl;
          if (HELDEBUG >= 3) {
            cout << "Qrt " << fQrt[fArm] << "  Tdiff "<<fTdiff[fArm];
            cout<<"  gate "<<fGate[fArm]<<" evtype "<<fEvtype[fArm]<<endl;
	  }
          recovery_flag = 1;    // clear & recalibrate the predictor
          quad_calibrated[fArm] = 0;
          fFirstquad[fArm] = 1;
          present_helicity[fArm] = Unknown;
       }
       if (HELDEBUG>=3) {
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
void GenHelicity::LoadHelicity() {
// Load the helicity in G0 mode.  
// If fGate == 0, helicity remains 'Unknown'.

 if ( quad_calibrated[fArm] ) validHel[fArm] = 1;
 if ( !quad_calibrated[fArm]  || fGate[fArm] == 0 ) {
       present_helicity[fArm] = Unknown;
       return;
 }
 if (HELDEBUG >= 2) cout << "Loading helicity ##########"<<endl;
// Look for pattern (+ - - +)  or (- + + -) to decide where in quad
// Ignore timing for assignment, but later we'll check it.
 inquad = 0;
 if (fQrt[fArm] == 1) {
   inquad = 1;  
   if (present_reading[fArm] != q1_reading[fArm])
     {
       cout << "GenHelicity::WARNING::  Invalid bit pattern !! "
	    << "present_reading != q1_reading while QRT == 1 at timestamp " 
	    << fTimestamp[fArm]<< endl;
       cout << "GenHelicity::HELICITY ASSIGNMENT MAY BE INCORRECT" << endl;
     }
 }
 if (fQrt[fArm] == 0 &&
     present_reading[fArm] != q1_reading[fArm]) {
          inquad = 2;  // same is inquad = 3
 }
 if (fQrt[fArm] == 0 &&
     present_reading[fArm] == q1_reading[fArm]) {
          inquad = 4;  
 }
 if (inquad == 0)
   {
     present_helicity[fArm] = Unknown;
     cout << "GenHelicity::WARNING::  Quad assignment impossible !! "
	  << " qrt = " << fQrt[fArm]
	  << " present read = " << present_reading[fArm]
	  << " Q1 read = " << q1_reading[fArm] 
	  << " at timestamp " << fTimestamp[fArm]<< endl;
     cout << "GenHelicity::HELICITY SET TO UNKNOWN" << endl;
   }
 else if (inquad == 1 || inquad >= 4) {
     present_helicity[fArm] = q1_present_helicity[fArm];
 } else {
     if (q1_present_helicity[fArm] == Plus) 
            present_helicity[fArm] = Minus;
     if (q1_present_helicity[fArm] == Minus) 
            present_helicity[fArm] = Plus;
 }

 // Here we check timing, with stringency depending on whether fTdiff
 // was set using T9 or not
 if (
     (!fT0T9[fArm] &&
      ((inquad == 1 && fTdiff[fArm] > 0.5 * fTdavg[fArm]) ||
       (inquad == 4 && fTdiff[fArm] < 0.5 * fTdavg[fArm])))
   ||
     (fT0T9[fArm] &&
      ((inquad == 1 && fTdiff[fArm] > 0.3 * fTdavg[fArm]) ||
       (inquad == 2 && (fTdiff[fArm] < 0.2 * fTdavg[fArm] ||
			fTdiff[fArm] > 0.8 * fTdavg[fArm])) ||
       (inquad == 4 && fTdiff[fArm] < 0.7 * fTdavg[fArm]))))
   {
     cout << "GenHelicity::WARNING::  Quad assignment inconsistent with timing !! "
	  << "quad = " << inquad << " tdiff = " << fTdiff[fArm] 
	  << " at timestamp " << fTimestamp[fArm]<< endl;
     cout << "GenHelicity::HELICITY ASSIGNMENT MAY BE INCORRECT" << endl;
   }
   

 if (HELDEBUG >= 2) {
    cout << dec << "Quad   "<<inquad;
    cout << "   Time diff "<<fTdiff[fArm];
    cout << "   Qrt  "<<fQrt[fArm];
    cout << "   Helicity "<<present_helicity[fArm]<<endl;
 }
 fQuad[fArm] = inquad;
 return;
}

//____________________________________________________________________
void GenHelicity::QuadHelicity(Int_t cond) {
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
      if (HELDEBUG >= 3) {
        cout << "quad helicities  " << dec;
        cout << predicted_reading[fArm];           // -1, +1
        cout << " ?=? " << present_reading[fArm];  //  0,  1
        cout << "   pred-> " << present_helicity[fArm]
	     << " time " << fTimestamp[fArm]
	     <<endl;
        if (CompHel()) cout << "HELICITY OK "<<endl;
      }
      quad_calibrated[fArm] = 1;

  }
  return;
}

//____________________________________________________________________
Int_t GenHelicity::RanBit(int which) {
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
UInt_t GenHelicity::GetSeed() {
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
Bool_t GenHelicity::CompHel() {
// Compare the present reading to the predicted reading.
// The raw data are 0's and 1's which compare to predictions of
// -1 and +1.  A prediction of 0 occurs when there is no gate.
  if (present_reading[fArm] == 0 && 
      predicted_reading[fArm] == -1) return kTRUE;
  if (present_reading[fArm] == 1 && 
      predicted_reading[fArm] == 1) return kTRUE;
  return kFALSE;
}

//____________________________________________________________________
Int_t GenHelicity::FindWord (const THaEvData& evdata, const Int_t roc, const Int_t header, const Int_t index) const
{
  Int_t len = evdata.GetRocLength (roc);
  if (len <= 4) return -1;

  Int_t i;
  if (header == 0)
    i = index;
  else
    {
      for (i = 0; 
	   i < len && evdata.GetRawData (roc, i) != header;
	   ++i)
	{}
      i += index;
    }
  if (i >= len) return -1;
  return i;
}
