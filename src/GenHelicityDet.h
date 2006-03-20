#ifndef GENHELICITYDET_H

#define GENHELICITYDET_H

// Here is an example of usage

//  // beam and helicity
//  THaApparatus* a = new THaIdealBeam("Beam", "Ideal beam");
//  GenHelicityDet *h = new GenHelicityDet("H","Beam helicity");
//  h->SetState (1, 8, -1, 1, 0);  // G0 mode; 8 window delay; sign -1;
//                                 // right arm; no redund
//  h->SetROC (1, 28, 0xfadcb0b9, 2, 0xfabc0007, 6); // "Right arm" is ROC
//                                   //            28; headers and indices
//  a->AddDetector(h);
//  gHaApps->Add( a );

////////////////////////////////////////////////////////////////////////
//
// GenHelicityDet
//
// Derived class from THaDetector.  Provides an interface between
// the GenHelicity class and the Analyzer's interface for the 
// decode/reconstruct cycle and the global variables on an event
// by event basis.
// 
// author: V. Sulkosky and R. Feuerbach, Jan 2006
//
////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"
#include "GenHelicity.h"

class THaEvData;

class GenHelicityDet :  public THaDetector {
  
public:

  GenHelicityDet(const char* name, const char* description,
		 THaApparatus* a = NULL);
  virtual ~GenHelicityDet();
  virtual Int_t Decode( const THaEvData& ev );
  virtual Int_t DefineVariables( EMode mode = kDefine );

// The user may set the state of this class (see implementation)
// These variables define the state of this object ---------
// The mode flag turns G0 mode on (1) or off (0)
// G0 mode is the delayed helicity mode, else it is in-time.
//  int mode;
//  int delay;  // delay of helicity (# windows)
// Overall sign (as determined by Moller)
//  int sign;
// Which spectrometer do we believe ?
//  int spec;  // = fgLarm or fgRarm (0 or 1)
// Check redundancy ? (yes=1, no=0)
//  int redund;
// ---- end of state variables -----------------
  void SetState(int mode, int delay, int sign, int spec, int redund);
  void Print(Option_t *option="") const;
  void SetROC (int arm, int roc,
	       int helheader, int helindex,
	       int timeheader, int timeindex);

private:

  //GenHelicityDet( const GenHelicityDet& ) {}
  //GenHelicityDet& operator=( const GenHelicityDet& ) { return *this; }
  GenHelicity fHelicity;

  Int_t fMode, fDelay, fSign, fSpec, fCheck;
  static const Int_t NotOK = -1;

  // Index is 0, 1 for (nominally) left, right arm
  // If a header is zero the index is taken to be from the start of
  // the ROC (0 = first word of ROC), otherwise it's from the header
  // (0 = first word after header).

  Int_t fRoc[2];                 // ROC for left, right arm
  Int_t fHelHeader[2];           // Header for helicity bit
  Int_t fHelIndex[2];            // Index from header
  Int_t fTimeHeader[2];          // Header for timestamp
  Int_t fTimeIndex[2];           // Index from header

  ClassDef(GenHelicityDet,0)       // Beam helicity information.
};

#endif
