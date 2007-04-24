//*-- Author :    Robert Michaels    Sept 2002
// Updated by     Vincent Sulkosky   Jan  2006
// More updates by Richard Holmes,   Feb 2006
// Changed into an implementation of THaHelicityDet, Ole Hansen, Aug 2006
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
// The user can use public method SetState() to adjust the state
// of this class.  However, a reasonable default exists.
// The state is defined by
//
//   fG0mode == G0 delayed mode (true) or in-time mode (false)
//   fG0delay = number of windows helicity is delayed
//   fSign = Overall sign (as determined by Moller)
//   fWhichSpec = kLeft/kRight = Which spectrometer to believe.
//   fCheckRedund = Do we check redundancy (true/false)
//
////////////////////////////////////////////////////////////////////////

#include "THaHelicity.h"
#include "THaEvData.h"
#include "VarDef.h"
#include "TH1F.h"
#include <iostream>
#include <cstring>

using namespace std;

//FIXME: read database

//____________________________________________________________________
THaHelicity::THaHelicity( const char* name, const char* description,
			  THaApparatus* app ) : 
  THaHelicityDet( name, description, app ),
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
}

//____________________________________________________________________
THaHelicity::THaHelicity()
{
  // Default constructor - for ROOT I/O only
}

//____________________________________________________________________
THaHelicity::~THaHelicity() 
{
  DefineVariables( kDelete );

  delete hahel1;
}

//_____________________________________________________________________________
Int_t THaHelicity::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  const RVarDef var[] = {
    { "helicity","Beam helicity",               "GetHelicity()" },
    { "qrt",     "qrt from ROC",                "GetQrt()" },
    { "quad",    "quad (1, 2, or 4)",           "GetQuad()" },
    { "tdiff",   "time since quad start",       "GetTdiff()" },
    { "gate",    "Helicity Gate from ROC",      "GetGate()" },
    { "pread",   "Present helicity reading",    "GetReading()" },
    { "timestamp","Timestamp from ROC",         "GetTime()" },
    { "validtime","validtime flag",             "GetValidTime()" },
    { "validHel","validHel flag",               "GetValidHelicity()" },
    { "numread", "Latest valid reading",        "GetNumRead()" },
    { "badread", "Latest problematic reading",  "GetBadRead()" },
    { "ringclk", "Ring buffer clock (1024 Hz)", "GetRingClk()" },
    { "ringqrt", "Ring buffer qrt",             "GetRingQrt()" },
    { "ringhel", "Ring buffer helicity",        "GetRingHelicity()" },
    { "ringtrig","Ring buffer trigger",         "GetRingTrig()" },
    { "ringbcm", "Ring buffer BCM",             "GetRingBCM()" },
    { "ringl1a","Ring buffer accepted trigger", "GetRingl1a()" },
    { "ringv2fh","Ring buffer Helicity V-to-F", "GetRingV2fh()" },
    { "mode",    "Helicity mode",               "fG0mode" },
    { "delay",   "Delay of helicity",           "fG0delay" },
    { "sign",    "Sign determined by Moller",   "fSign" },
    { "spec",    "Spectrometer",                "fWhichSpec" },
    { "redund",  "Redundancy flag",             "fCheckRedund" },
    { 0 }
  };
  return DefineVarsFromList( var, mode );
}


//____________________________________________________________________
Int_t THaHelicity::ReadDatabase( const TDatime& date )
{



//Bob's old hardcoded defaults - use if no db file available


// These parameter must be tuned once in life.
// Here's how:  First make them big, like 20000 (if you
// make them too small you'll get lost because fT0 gets
// messed up).  Then run with HELDEBUG=-1.  Look at the
// printouts, some fTdiff params show up most often, take
// the lesser of the ones that show up very often.
// Also have a histo hahel1, hahel2 if HELDEBUG=-1.
  //fgTdiff_LeftHRS = 14000;
  //fgTdiff_RightHRS = 14000;

  // reasonable default state for Ledex
  SetROC (kLeft, 11, 0, 3, 0, 4); // Left arm
  SetState (1, 8, -1, kLeft, 0);  // G0 mode; 8 window delay; sign -1;
                                  // left arm; no redund


  return kOK;
}

//____________________________________________________________________
void THaHelicity::SetState(int mode, int delay, 
                      int sign, int spec, int redund) 
{
  // Set the state of this object 
  fSign = sign;       // Overall sign (as determined by Moller)
}

//____________________________________________________________________
void THaHelicity::SetROC (int arm, int roc,
			  int helheader, int helindex,
			  int timeheader, int timeindex)
{
// Set parameters for ROC readout.  Note, if a header is zero the
// index is taken to be from the start of the ROC (0 = first word of ROC), 
// otherwise it's from the header (0 = first word after header).

  fROC = roc;                  // ROC 
  fHelHeader = helheader;      // Header for helicity bit
  fHelIndex = helindex;        // Index from header
  fTimeHeader = timeheader;    // Header for timestamp
  fTimeIndex = timeindex;      // Index from header
}

//____________________________________________________________________
void THaHelicity::SetRTimeROC (int arm, 
			       int roct2, int t2header, int t2index, 
			       int roct3, int t3header, int t3index)
{
  // Set parameters for reading redundant time info.

  fRTimeROC2 = roct2;                  // ROC 
  fRTimeHeader2 = t2header;    // Header for timestamp
  fRTimeIndex2 = t2index;      // Index from header
  fRTimeROC3 = roct3;                  // ROC 
  fRTimeHeader3 = t3header;    // Header for timestamp
  fRTimeIndex3 = t3index;      // Index from header
}
  

//____________________________________________________________________
void THaHelicity::Clear( Option_t* opt ) 
{
  // Clear the event data

  validTime = 0;
  validHel = 0;
  timestamp = Unknown;
  //FIXME: move to base class
  fHelicity  = Unknown;
  //FIXME: move to ADC helicity
  Ladc_helicity = Unknown; 
  Ladc_gate = Unknown;
  Ladc_hbit = Unknown;
  Radc_helicity = Unknown; 
  Radc_gate = Unknown;
  Radc_hbit = Unknown;
  // FIXME: move to G0 helicity
  fQrt = 0;
  fGate = 0;
  fLastTimestamp = fTimestamp;
  fTimestamp = 0;
  present_reading = Unknown;
  present_helicity = Unknown;
  fEvtype = 0;
  fQuad = 0;
  
}

//____________________________________________________________________
Int_t THaHelicity::Decode( const THaEvData& evdata )
{
  // Decode Helicity data.

  // In ancient history there was no TIR data for helicity,
  // only ADC data; and the timestamp came from ROC14; ask 
  // Bob if you care about year <= 2002

  Clear();  

  if (!fG0mode) {  // Non-G0 (i.e. in-time) helicity mode

    if (fCheckRedund) {

      // Redundant ADC helicity data. (bit = ADC high/low)
      // Obnoxious complaints when this disagrees with TIR bits.
      // As of 2003 the data are in
      //   (1,25,14,15) / (3,25,14,15) = (roc,slot,chanlo,chanhi)
      // In previous times the data were (even more redundantly) in 
      //   (1,25,60,61) / (2,25,14,15) / (2,25,46,47)

      int roc = 0, slot = 0, chan = 0, chanlo = 0, chanhi = 0, adc_data;

      // This code won't find data if pedestal suppression is on 
      // and it should be off for helicity ADC channels, but in
      // case not, we set the bits to Minus.    
      Ladc_hbit = Minus;
      Radc_hbit = Minus;

      //FIXME: URGHHHH - hardcoded detmap info!!!
      for (int i = 0; i < 2; i++) {
	if (i == 0) {roc=1; slot=25; chanlo=14; chanhi=15;}
	if (i == 1) {roc=3; slot=25; chanlo=14; chanhi=15;}
	for (chan = chanlo; chan <= chanhi; chan++) {
	  if ( !evdata.GetNumHits(roc,slot,chan ))
	    continue;
	  adc_data = evdata.GetData(roc,slot,chan,0);

	  if (fDebug >= 3) {
	    cout << "crate "<<roc<<"   slot "<<slot<<"   chan "
		 <<chan<<"   data "<<adc_data<<"   ";
	    if (adc_data > HCUT) 
	      cout << "  above cut !";
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
    
    if (fDebug >= 3) {
      cout << "ADC helicity info "<<endl;
      cout << "L-arm gate "<<Ladc_gate<<"  helic. bit "<<Ladc_hbit;
      cout << "    resulting helicity "<<Ladc_helicity<<endl;
      cout << "R-arm gate "<<Radc_gate<<"  helic. bit "<<Radc_hbit;
      cout << "    resulting helicity "<<Radc_helicity<<endl;
    }

   }

// Here is the new (ca 2002) way to get helicity and timestamp

   Int_t myroc = 0;
 
// Modified 01/23/2006 by V. Sulkosky
// This section modified to get ring buffer data for in-time 
// helicity mode.

     ReadData ( evdata );

     if (fGate != 0) {
        if (present_reading)
	  fHelicity = Plus;
        else
	  fHelicity = Minus;
     }
 
     fHelicity *= fSign;  // Sign determined by Moller measurement
     validHel = 1;
     validTime = 1;
     if (fDebug >= 3) { 
       cout << "In-time helicity check for roc "<<fROC<<endl;
       //cout << "Raw word 0x"<<hex<<data<<dec;
       cout << "  Gate "<<fGate<<"  qrt "<<fQrt;
       cout << "  helicity bit "<<present_reading;
       cout << "  Resulting helicity "<<fHelicity<<endl;
       cout << "  timestamp "<<fTimestamp<<endl;
     }

     if (fCheckRedund) {
   
       // Here we check redunandcy and complain if something disagrees.
       // This might be flakey due to timing delays, miscabling, things
       // being turned off, etc, but at least it shows the idea.

       //     if (Lhel != Rhel || Lhel != Ladc_helicity || 
       //         Lhel != Radc_helicity) {
//        if (Lhel != Radc_helicity) { // for DVCS...
// 	 cout << "THaHelicity::WARNING:: ";
// 	 cout << " Inconsistent helicity data !!"<<endl;
     }

     //FIXME: move to G0Helicity
  } else {   // G0 helicity mode (data are delayed).

    ReadData ( evdata );    
    TimingEvent ();
    QuadCalib ();      
    LoadHelicity ();

    // Sign determined by Moller measurement
   fHelicity = fSign * present_helicity;
   if (fDebug >= 3) 
     cout << "G0 helicity "<<fHelicity<<endl;

  }

  return OK;
}
 

ClassImp(THaHelicity)

