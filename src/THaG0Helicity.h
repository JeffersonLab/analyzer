#ifndef Podd_THaG0Helicity_h_
#define Podd_THaG0Helicity_h_

////////////////////////////////////////////////////////////////////////
//
// THaG0Helicity
//
// Helicity of the beam - from G0 electronics in delayed mode
// 
////////////////////////////////////////////////////////////////////////

#include "THaHelicityDet.h"
#include "THaG0HelicityReader.h"

class TH1F;

class THaG0Helicity : public THaHelicityDet, public THaG0HelicityReader {

public:

  THaG0Helicity( const char* name, const char* description, 
		 THaApparatus* a = NULL );
  THaG0Helicity();
  virtual ~THaG0Helicity();

  virtual Int_t  Begin( THaRunBase* r=0 );
  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );
  virtual Int_t  End( THaRunBase* r=0 );
  virtual void   SetDebug( Int_t level );
  virtual Bool_t HelicityValid() const { return fValidHel; }

  Int_t     GetQuad()  const { return fQuad; }
  Double_t  GetTdiff() const { return fTdiff; }
  // Set functions that can be used instead of database values
  void      SetG0Delay( Int_t delay );
  void      SetTdavg( Double_t tdavg );
  void      SetTtol( Double_t ttol );
  void      SetMaxMsQrt( Int_t missq );

protected:

  void      TimingEvent(); // Check for and process timing events
  void      QuadCalib();
  void      LoadHelicity();
  void      QuadHelicity(Int_t cond=0);
  EHelicity RanBit(Int_t i);
  UInt_t    GetSeed();
  Bool_t    CompHel();

  enum { kNbits = 24 };

  Int_t     fG0delay;  // delay of helicity (# windows)

  Int_t     fEvtype;               // Current CODA event type
  Double_t  fTdavg, fTdiff, fT0, fT9;
  Bool_t    fT0T9; // Was fT0 computed using fT9?
  Bool_t    fQuad_calibrated;
  Bool_t    fValidHel;
  Bool_t    fRecovery_flag;
  Double_t  fTlastquad, fTtol;
  Int_t     fQuad, fFirstquad;

  Double_t  fLastTimestamp;
  Double_t  fTimeLastQ1;
  Int_t     fT9count;
  Int_t     fPredicted_reading, fQ1_reading;
  EHelicity fPresent_helicity, fSaved_helicity, fQ1_present_helicity;
  Int_t     fMaxMissed;   // maximum number of missed quads before resetting
  Int_t     fHbits[kNbits];
  UInt_t    fNqrt;
  TH1F *    fHisto;  // Histogram of time diffs, filled in calibration mode
  Int_t     fNB;
  UInt_t    fIseed, fIseed_earlier, fInquad;
  
  Int_t     fTET9Index;         // Count of T9s (missing and found) since QRT=1
  Int_t     fTELastEvtQrt;      // QRT of last event
  Double_t  fTELastEvtTime;     // Timestamp of last event
  Double_t  fTELastTime;        // Time of last timing event (before this)
  Int_t     fTEPresentReadingQ1;// present_reading at last QRT==1 timing event
  Int_t     fTEStartup;         // Nonzero if starting up
  Double_t  fTETime;            // Time of timing event (this or most recent)
  Bool_t    fTEType9;           // True if timing event was T9

  UInt_t    fManuallySet;       // Bit pattern of manually set parameters

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaG0Helicity,2)   // Beam helicity from G0 electronics in delayed mode

};

#endif
