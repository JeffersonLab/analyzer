#ifndef Podd_THaQWEAKHelicity_h_
#define Podd_THaQWEAKHelicity_h_

////////////////////////////////////////////////////////////////////////
//
// THaQWEAKHelicity
//
// Helicity of the beam - from QWEAK electronics in delayed mode
// 
////////////////////////////////////////////////////////////////////////

#include "THaHelicityDet.h"
#include "THaQWEAKHelicityReader.h"

class TH1F;

class THaQWEAKHelicity : public THaHelicityDet, public THaQWEAKHelicityReader {

public:

  THaQWEAKHelicity( const char* name, const char* description, 
		 THaApparatus* a = NULL );
  THaQWEAKHelicity();
  virtual ~THaQWEAKHelicity();

  virtual Int_t  Begin( THaRunBase* r=0 );
  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );
  virtual Int_t  End( THaRunBase* r=0 );
  virtual void   SetDebug( Int_t level );
  virtual Bool_t HelicityValid() const { return fValidHel; }

  void PrintEvent(Int_t evtnum);

protected:
  void  FillHisto();
  void  CheckTIRvsRing(Int_t eventnumber);
  void  LoadHelicity(Int_t eventnumber);
  Int_t RanBit30(Int_t& ranseed);
  THaHelicityDet::EHelicity SetHelicity(Int_t polarity, Int_t phase);

  
  // variables that need to be read from the database
  Int_t fOffsetTIRvsRing; 
  // Offset between the ring reported value and the TIR reported value
  Int_t fQWEAKDelay;  
  // delay of helicity (# windows)
  Int_t fMAXBIT; 
  //number of bit in the pseudo random helcity generator
  std::vector<Int_t> fPatternSequence; // sequence of 0 and 1 in the pattern
  Int_t fQWEAKNPattern; // maximum of event in the pattern
  Bool_t HWPIN;


  Int_t fQrt;
  Int_t fTSettle;
  Bool_t fValidHel;

  Int_t fHelicityLastTIR;
  Int_t fPatternLastTIR;
  void SetErrorCode(Int_t error);
  Double_t fErrorCode;
 

  // Ring related data
  Int_t fRing_NSeed; //number of event collected for seed
  Int_t fRingU3plus, fRingU3minus;
  Int_t fRingT3plus, fRingT3minus;
  Int_t fRingT5plus, fRingT5minus;
  Int_t fRingT10plus, fRingT10minus;
  Int_t fRingTimeplus, fRingTimeminus;
  Int_t fRingSeed_reported;
  Int_t fRingSeed_actual;
  Int_t fRingPhase_reported;
  Int_t fRing_reported_polarity;
  Int_t fRing_actual_polarity;

  Int_t fEvtype; // Current CODA event type
 
  static const Int_t NHIST = 2;
  TH1F* fHisto[NHIST];  

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaQWEAKHelicity,0)   // Beam helicity from QWEAK electronics in delayed mode

};

#endif

