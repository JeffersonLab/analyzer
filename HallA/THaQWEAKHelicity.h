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
#include <vector>

class TH1F;

class THaQWEAKHelicity : public THaHelicityDet, public THaQWEAKHelicityReader {

public:

  THaQWEAKHelicity( const char* name, const char* description, 
                    THaApparatus* a = nullptr );
  THaQWEAKHelicity();
  virtual ~THaQWEAKHelicity();

  virtual Int_t  Begin( THaRunBase* r=nullptr );
  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );
  virtual Int_t  End( THaRunBase* r=nullptr );
  virtual void   SetDebug( Int_t level );
  virtual Bool_t HelicityValid() const { return fValidHel; }

  void PrintEvent( UInt_t evtnum );

protected:
  virtual void  FillHisto();
  void  CheckTIRvsRing( UInt_t eventnumber );
  void  LoadHelicity( UInt_t eventnumber );
  UInt_t RanBit30( UInt_t& ranseed );
  THaHelicityDet::EHelicity SetHelicity( UInt_t polarity, UInt_t phase);

  
  // variables that need to be read from the database
  UInt_t fOffsetTIRvsRing;
  // Offset between the ring reported value and the TIR reported value
  UInt_t fQWEAKDelay;
  // delay of helicity (# windows)
  UInt_t fMAXBIT;
  //number of bit in the pseudo random helicity generator
  std::vector<Int_t> fPatternSequence; // sequence of 0 and 1 in the pattern
  UInt_t fQWEAKNPattern; // maximum of event in the pattern
  Bool_t HWPIN;


  Int_t fQrt;
  Int_t fTSettle;
  Bool_t fValidHel;

  UInt_t fHelicityLastTIR;
  UInt_t fPatternLastTIR;
  void SetErrorCode(Int_t error);
  Double_t fErrorCode;
 

  // Ring related data
  UInt_t fRing_NSeed; //number of event collected for seed
  UInt_t fRingU3plus, fRingU3minus;
  UInt_t fRingT3plus, fRingT3minus;
  UInt_t fRingT5plus, fRingT5minus;
  UInt_t fRingT10plus, fRingT10minus;
  UInt_t fRingTimeplus, fRingTimeminus;
  UInt_t fRingSeed_reported;
  UInt_t fRingSeed_actual;
  UInt_t fRingPhase_reported;
  UInt_t fRing_reported_polarity;
  UInt_t fRing_actual_polarity;

  UInt_t fEvtype; // Current CODA event type
 
  static const Int_t NHIST = 2;
  std::vector<TH1F*> fHisto;

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaQWEAKHelicity,0)   // Beam helicity from QWEAK electronics in delayed mode

};

#endif

