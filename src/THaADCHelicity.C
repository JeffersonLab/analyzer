//*-- Author :    Robert Michaels    Sept 2002
// Updated by     Vincent Sulkosky   Jan  2006
// More updates by Richard Holmes,   Feb 2006
// Changed into an implementation of THaADCHelicityDet, Ole Hansen, Aug 2006
////////////////////////////////////////////////////////////////////////
//
// THaADCHelicity
//
// Helicity of the beam - from a single ADC.
//    +1 = plus,  -1 = minus,  0 = unknown
// 
////////////////////////////////////////////////////////////////////////

#include "THaADCHelicity.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include <iostream>

using namespace std;

//FIXME: read database

static const Double_t kDefaultThreshold = 5000.0;

//____________________________________________________________________
THaADCHelicity::THaADCHelicity( const char* name, const char* description,
				THaApparatus* app ) : 
  THaHelicityDet( name, description, app ),
  fADC_hdata(kBig), fADC_Gate(kBig), fADC_Hel(kUnknown), 
  fThreshold(kDefaultThreshold), fIgnoreGate(kFALSE)
{
  // Constructor
}



//____________________________________________________________________
THaADCHelicity::THaADCHelicity()
{
  // Default constructor - for ROOT I/O only
}

//____________________________________________________________________
THaADCHelicity::~THaADCHelicity() 
{
  DefineVariables( kDelete );
}

//_____________________________________________________________________________
Int_t THaADCHelicity::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  const RVarDef var[] = {
//     { "helicity","Beam helicity",               "GetHelicity()" },
//     { "qrt",     "qrt from ROC",                "GetQrt()" },
//     { "quad",    "quad (1, 2, or 4)",           "GetQuad()" },
//     { "tdiff",   "time since quad start",       "GetTdiff()" },
//     { "gate",    "Helicity Gate from ROC",      "GetGate()" },
//     { "pread",   "Present helicity reading",    "GetReading()" },
//     { "timestamp","Timestamp from ROC",         "GetTime()" },
//     { "validtime","validtime flag",             "GetValidTime()" },
//     { "validHel","validHel flag",               "GetValidHelicity()" },
//     { "numread", "Latest valid reading",        "GetNumRead()" },
//     { "badread", "Latest problematic reading",  "GetBadRead()" },
//     { "ringclk", "Ring buffer clock (1024 Hz)", "GetRingClk()" },
//     { "ringqrt", "Ring buffer qrt",             "GetRingQrt()" },
//     { "ringhel", "Ring buffer helicity",        "GetRingHelicity()" },
//     { "ringtrig","Ring buffer trigger",         "GetRingTrig()" },
//     { "ringbcm", "Ring buffer BCM",             "GetRingBCM()" },
//     { "ringl1a","Ring buffer accepted trigger", "GetRingl1a()" },
//     { "ringv2fh","Ring buffer Helicity V-to-F", "GetRingV2fh()" },
//     { "mode",    "Helicity mode",               "fG0mode" },
//     { "delay",   "Delay of helicity",           "fG0delay" },
//     { "sign",    "Sign determined by Moller",   "fSign" },
//     { "spec",    "Spectrometer",                "fWhichSpec" },
//     { "redund",  "Redundancy flag",             "fCheckRedund" },
    { 0 }
  };
  return DefineVarsFromList( var, mode );
}


//____________________________________________________________________
Int_t THaADCHelicity::ReadDatabase( const TDatime& date )
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
//   SetROC (kLeft, 11, 0, 3, 0, 4); // Left arm
//   SetState (1, 8, -1, kLeft, 0);  // G0 mode; 8 window delay; sign -1;
                                  // left arm; no redund

  //	if (i == 0) {roc=1; slot=25; chanlo=14; chanhi=15;}
  //	if (i == 1) {roc=3; slot=25; chanlo=14; chanhi=15;}


  return kOK;
}

// //____________________________________________________________________
// void THaADCHelicity::SetState(int mode, int delay, 
//                       int sign, int spec, int redund) 
// {
//   // Set the state of this object 
//   fSign = sign;       // Overall sign (as determined by Moller)
// }

// //____________________________________________________________________
// void THaADCHelicity::SetROC (int arm, int roc,
// 			  int helheader, int helindex,
// 			  int timeheader, int timeindex)
// {
// // Set parameters for ROC readout.  Note, if a header is zero the
// // index is taken to be from the start of the ROC (0 = first word of ROC), 
// // otherwise it's from the header (0 = first word after header).

//   fROC = roc;                  // ROC 
//   fHelHeader = helheader;      // Header for helicity bit
//   fHelIndex = helindex;        // Index from header
//   fTimeHeader = timeheader;    // Header for timestamp
//   fTimeIndex = timeindex;      // Index from header
// }

//____________________________________________________________________
// void THaADCHelicity::SetRTimeROC (int arm, 
// 			       int roct2, int t2header, int t2index, 
// 			       int roct3, int t3header, int t3index)
// {
//   // Set parameters for reading redundant time info.

//   fRTimeROC2 = roct2;                  // ROC 
//   fRTimeHeader2 = t2header;    // Header for timestamp
//   fRTimeIndex2 = t2index;      // Index from header
//   fRTimeROC3 = roct3;                  // ROC 
//   fRTimeHeader3 = t3header;    // Header for timestamp
//   fRTimeIndex3 = t3index;      // Index from header
// }
  

//____________________________________________________________________
void THaADCHelicity::Clear( Option_t* opt ) 
{
  // Clear the event data

  THaHelicityDet::Clear(opt);
  fADC_hdata = kBig;
  fADC_Gate  = kBig;
  fADC_Hel   = kUnknown;
}

//____________________________________________________________________
Int_t THaADCHelicity::Decode( const THaEvData& evdata )
{
  // Decode Helicity data.
  // Return 1 if helicity was assigned, 0 if not, -1 if error.
  
  Clear();  

  // In ancient history there was no TIR data for helicity,
  // only ADC data; and the timestamp came from ROC14; ask 
  // Bob if you care about year <= 2002

  // Redundant ADC helicity data. (bit = ADC high/low)
  // Obnoxious complaints when this disagrees with TIR bits.
  // As of 2003 the data are in
  //   (1,25,14,15) / (3,25,14,15) = (roc,slot,chanlo,chanhi)
  // In previous times the data were (even more redundantly) in 
  //   (1,25,60,61) / (2,25,14,15) / (2,25,46,47)

  // This code won't find data if pedestal suppression is on 
  // and it should be off for helicity ADC channels, but in
  // case not, we set the bits to Minus.    
  //FIXME: why not Unknown?
  fADC_Hel = kMinus;

  // We can safely assume that the detector map has exactly one entry,
  // and chhi = chlow+1. ReadDatabase ensures this.

  THaDetMap::Module* d = fDetMap->GetModule(0);
  if( !d )
    return -1;
  Int_t roc = d->crate, slot = d->slot, chlow = d->lo, chhi = d->hi;

  Int_t ret = 0;
  bool gate_high = false;

  for( Int_t chan = chlow; chan <= chhi; chan++ ) {
    if ( !evdata.GetNumHits( roc, slot, chan ))
      continue;
    Double_t data = 
      static_cast<Double_t>(evdata.GetData( roc, slot, chan, 0 ));

    if (fDebug >= 3) {
      cout << "crate "<<roc<<"   slot "<<slot<<"   chan "
	   <<chan<<"   data "<<data<<"   ";
      if (data > fThreshold) 
	cout << "  above cut !";
      cout << endl;
    }

    // Assign gate and helicity bit data.
    if( chan == chlow ) {
      fADC_hdata = data; 
    } else if( chan == chhi ) {
      // NB: As of 9/04, there is no gate on L-arm, only helicity bit.
      // For this arm, set fIgnoreGate true.
      fADC_Gate = data;
      gate_high = ( data > fThreshold );
    }

    // Logic: if gate==0 helicity remains unknown. If gate==1 
    // (or we are ingoring the gate) then helicity is determined by 
    // the helicity bit.

    if( gate_high || fIgnoreGate ) {
      fADC_Hel = ( fADC_hdata > fThreshold ) ? kPlus : kMinus;
      if( fSign<0 )
	fADC_Hel = ( fADC_Hel == kPlus ) ? kMinus : kPlus;
      ret = 1;
    }

    if (fDebug >= 3) {
      cout << "ADC helicity info "<<endl;
      cout << "Gate "<<fADC_Gate<<"  helic. bit "<<fADC_hdata;
      cout << "    resulting helicity "<<fADC_Hel<<endl;
    }

  }
  // fHelicity may be reassigned by derived classes, so we must keep the ADC
  // result separately. But within this class, the two are the same.
  fHelicity = fADC_Hel;

  return ret;
}
 

ClassImp(THaADCHelicity)

