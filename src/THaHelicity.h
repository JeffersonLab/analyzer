////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// Helicity of the beam.  See implementation for description.
// 
// author: R. Michaels, Sept 2002
// updated: Jan 2006 by V. Sulkosky
// Changed into an implementation of THaHelicityDet, Aug 2006, Ole Hansen
//
// Debug levels for this detector: (set via SetDebug(n))
// = 1 for messages about unusual helicity bit handling,
//     e.g. handling missing quadruples
// = 2 for messages relating to scalers
// = 3 for verbose debugging output
// = -1 to get fTdiff (calib time)
//
////////////////////////////////////////////////////////////////////////


#ifndef ROOT_THaHelicity
#define ROOT_THaHelicity

#include "THaHelicityDet.h"

class THaEvData;
class TString;
class TH1F;

class THaHelicity : public THaHelicityDet {

public:

  THaHelicity( const char* name, const char* description, 
	       THaApparatus* a = NULL );
  virtual ~THaHelicity();

  virtual Int_t  Begin( THaRunBase* r=0 );
  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );

  // Helicity sources: left or right spectrometer. Use as argument to
  // GetHelicity() etc. 
  // kDefault selects the spectrometer arm specified in the database 
  // (or last call to SetState, resp.)
  enum EArm { kDefault = -1, kLeft, kRight };

  virtual Int_t  GetHelicity() const;
          Int_t  GetHelicity( const char* spec ) const;
          Int_t  GetHelicity( EArm spec ) const;
  virtual Bool_t HelicityValid() const;
          Bool_t HelicityValid( EArm spec ) const;

  // Get helicity clock timestamp (~105 kHz)
  virtual Double_t GetTime() const;

  // 
  // Get ROC 10/11 (R/L) helicity information (for debugging)
  Int_t    GetQrt( EArm arm=kDefault ) const;
  Int_t    GetQuad( EArm arm=kDefault ) const;
  Double_t GetTdiff( EArm arm=kDefault ) const;
  Int_t    GetGate( EArm arm=kDefault ) const; 
  Int_t    GetReading( EArm arm=kDefault ) const;
  Int_t    GetValidTime( EArm arm=kDefault ) const;
  Int_t    GetValidHelicity( EArm arm=kDefault ) const;
  Int_t    GetNumRead() const { return fNumread; } // Lastest valid reading
  Int_t    GetBadRead() const { return fBadread; } // Latest problematic reading

  // Get methods for Ring Buffer scalers
  Int_t GetRingClk()      const { return fRing_clock; }
  Int_t GetRingQrt()      const { return fRing_qrt; }
  Int_t GetRingHelicity() const { return fRing_helicity; }
  Int_t GetRingTrig()     const { return fRing_trig; }
  Int_t GetRingBCM()      const { return fRing_bcm; }
  Int_t GetRingl1a()      const { return fRing_l1a; }
  Int_t GetRingV2fh()     const { return fRing_v2fh; }

  // The user may set the state of this class (see implementation)
  void SetState(Int_t mode, Int_t delay, Int_t sign, 
		Int_t spec, Int_t redund);
  void SetROC (Int_t arm, Int_t roc, Int_t helheader, Int_t helindex,
	       Int_t timeheader, Int_t timeindex);
  void SetRTimeROC (Int_t arm, 
		    Int_t roct2, Int_t t2header, Int_t t2index, 
		    Int_t roct3, Int_t t3header, Int_t t3index);

  THaHelicity();  // For ROOT I/O only

protected:

  virtual void ReadData ( const THaEvData& evdata );
  void TimingEvent(); // Check for and process timing events
  void QuadCalib();
  void LoadHelicity();
  void QuadHelicity(Int_t cond=0);
  Int_t RanBit(Int_t i);
  UInt_t GetSeed();
  Bool_t CompHel();
  Int_t FindWord (const THaEvData& evdata, Int_t roc, Int_t header, 
		  Int_t index) const;

// These variables define the state of this object ---------
// The fG0mode flag turns G0 mode on (1) or off (0)
// G0 mode is the delayed helicity mode, else it is in-time.
  Bool_t fG0mode;
  Int_t fG0delay;  // delay of helicity (# windows)
// Overall sign (as determined by Moller)
  Int_t fSign;
// Which spectrometer do we believe ?
  Int_t fWhichSpec;  // = fgLarm or fgRarm
// Check redundancy ? (yes=1, no=0)
  Bool_t fCheckRedund;
// ---- end of state variables -----------------

  enum { kNbits = 24 };
  Double_t *fTdavg, *fTdiff, *fT0, *fT9;
  Bool_t* fT0T9; // Was fT0 computed using fT9?
  Double_t *fTlastquad, *fTtol;
  Int_t *fQrt, *fGate, *fFirstquad, *fEvtype;
  Int_t *fQuad;
  Double_t *fTimestamp;
  Double_t *fLastTimestamp;
  Double_t *fTimeLastQ1;
  Int_t *validTime, *validHel;
  Int_t *t9count;
  Int_t *present_reading, *predicted_reading; 
  Int_t *q1_reading, *present_helicity, *saved_helicity;
  Int_t *q1_present_helicity, *quad_calibrated;
  Int_t *hbits;
  UInt_t *fNqrt;
  TH1F *hahel1, *hahel2;
  Int_t recovery_flag;
  int nb;
  UInt_t iseed, iseed_earlier, inquad;
  Int_t fArm;  // Spectrometer = kRight or kLeft

  Int_t fNumread, fBadread;
  // ring buffer scalers
  Int_t fRing_clock, fRing_qrt, fRing_helicity;
  Int_t fRing_trig, fRing_bcm, fRing_l1a, fRing_v2fh;

// ADC data for helicity and gate (redundant).
  Double_t  Ladc_helicity, Ladc_gate;
  Double_t  Radc_helicity, Radc_gate;
  Int_t  Ladc_hbit, Radc_hbit;

  Int_t     Lhel,Rhel;
  Double_t  timestamp;                         

  // ROC information

  // Index is 0, 1 for (nominally) left, right arm
  // If a header is zero the index is taken to be from the start of
  // the ROC (0 = first word of ROC), otherwise it's from the header
  // (0 = first word after header).

  Int_t fROC[2];                 // ROC for left, right arm
  Int_t fHelHeader[2];           // Header for helicity bit
  Int_t fHelIndex[2];            // Index from header
  Int_t fTimeHeader[2];          // Header for timestamp
  Int_t fTimeIndex[2];           // Index from header
  // Redundant clocks
  Int_t fRTimeROC2[2];                  // ROC 
  Int_t fRTimeHeader2[2];    // Header for timestamp
  Int_t fRTimeIndex2[2];      // Index from header
  Int_t fRTimeROC3[2];                  // ROC 
  Int_t fRTimeHeader3[2];    // Header for timestamp
  Int_t fRTimeIndex3[2];      // Index from header

  // Following members are used by TimingEvent():
  
  Int_t* fTET9Index;           // Count of T9s (missing and found) since QRT==1
  Int_t* fTELastEvtQrt;        // QRT of last event
  Double_t* fTELastEvtTime;    // Timestamp of last event
  Double_t* fTELastTime;       // Time of last timing event (before this)
  Int_t* fTEPresentReadingQ1;  // present_reading at last QRT==1 timing event
  Int_t* fTEStartup;           // Nonzero if starting up
  Double_t* fTETime;           // Time of timing event (this or most recent)
  Bool_t* fTEType9;            // True if timing event was T9

  static const Int_t OK       =  1;
  static const Int_t HCUT     = 5000;  
  static const Int_t Plus     =  1;    
  static const Int_t Minus    = -1;    
  static const Int_t Unknown  =  0;    

  void   InitG0();
  void   InitMemory();

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaHelicity,1)       // Beam helicity information.
};

#endif 

