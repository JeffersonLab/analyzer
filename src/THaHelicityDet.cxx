//*-- Author :    Vincet Sulkosky and R. Feuerbach,  Jan 2006

////////////////////////////////////////////////////////////////////////
//
// THaHelicityDet
//
// Derived class from THaDetector.  Provides an interface between
// the THaHelicity class and the Analyzer's interface for the 
// decode/reconstruct cycle and the helicity variables on an event
// by event basis.  The ring buffer data is also made available
// from this class for both G0 delayed and in-time modes.
//
// The user can use public method SetState() to adjust the state
// of the THaHelicity class.  A reasonable default exists when
// the THaHelicity constructor is called, however the user should
// confirm that the helicity mode is set correctly for their 
// experimental run.
//
// The state is defined by
//
//   fgG0mode == G0 delayed mode (1) or in-time mode (0)
//   fgG0delay = number of windows helicity is delayed
//   fSign = Overall sign (as determined by Moller)
//   fWhichSpec = fgLarm/fgRarm = (0/1) Which spectrometer to believe.
//   fCheckRedund = Do we check redundancy (yes=1, no=0)
//
////////////////////////////////////////////////////////////////////////


#include "THaHelicityDet.h"
#include "VarDef.h"
#include "THaGlobals.h"
#include <cstdio>
#include <iostream>

using namespace std;

//_____________________________________________________________________________

THaHelicityDet::THaHelicityDet( const char* name, const char* description ,
				THaApparatus* apparatus )
  : THaDetector( name, description, apparatus ) 
{
  // Constructor
}

//_____________________________________________________________________________
THaHelicityDet::~THaHelicityDet()
{
  // Destructor
  DefineVariables( kDelete );
}

//_____________________________________________________________________________
Int_t THaHelicityDet::Decode( const THaEvData& ev ) {

  if(fHelicity.Decode(ev) != 1) return NotOK;
  return 0;
}

//_____________________________________________________________________________
Int_t THaHelicityDet::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  const char* var_prefix = "fHelicity.";

  const RVarDef var1[] = {
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
    { 0 }
  };
  DefineVarsFromList( var1, mode, var_prefix );
  
  const RVarDef var2[] = {
    { "mode",    "Helicity mode",               "fMode" },
    { "delay",   "Delay of helicity",           "fDelay" },
    { "sign",    "Sign determined by Moller",   "fSign" },
    { "spec",    "Spectrometer",                "fSpec" },
    { "redund",  "Redundancy flag",             "fCheck" },
    { 0 }
  };
  DefineVarsFromList( var2, mode );
  return 0;
}

//____________________________________________________________________
void THaHelicityDet::SetROC (int arm, int roc, 
			     int helheader, int helindex,
			     int timeheader, int timeindex)
{
  fHelicity.SetROC (arm, roc, helheader, helindex, timeheader, timeindex);
  fRoc[arm] = roc;                  // ROC 
  fHelHeader[arm] = helheader;      // Header for helicity bit
  fHelIndex[arm] = helindex;        // Index from header
  fTimeHeader[arm] = timeheader;    // Header for timestamp
  fTimeIndex[arm] = timeindex;      // Index from header
}

//____________________________________________________________________
void THaHelicityDet::SetState(int mode, int delay, 
			 int sign, int spec, int redund)
{
  fHelicity.SetState(mode,delay,sign,spec,redund);
  fMode = mode;    // G0 mode (1) or in-time helicity (0)
  fDelay = delay;  // delay of helicity (# windows)
  fSign = sign;    // Overall sign (as determined by Moller)
  fSpec = spec;    // Which spectrometer do we believe ?
  fCheck = redund; // Do we check redundancy (yes=1, no=0)
}

//____________________________________________________________________
void THaHelicityDet::Print(Option_t *option) const
{
  cout << "\n\n====================== \n";
  cout << "Print of Helicity Mode : "<<endl; 
  cout << "\nG0 mode (1) or in-time helicity (0) = "<<fMode<<endl;
  cout << "delay of helicity (# windows) = "<<fDelay<<endl;
  cout << "Overall sign (as determined by Moller) = "<<fSign<<endl;
  cout << "Which spectrometer = "<<fSpec<<endl;
  cout << "Do we check redundancy (yes=1, no=0) = "<<fCheck<<endl;
}

ClassImp(THaHelicityDet)
