#ifndef ROOT_THaHelicity
#define ROOT_THaHelicity

//////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// In-time helicity detector - both from ADC and G0 electronics 
// (with delay=0).
// Provides redundancy for cross-checks.
//
//////////////////////////////////////////////////////////////////////////


#include "THaADCHelicity.h"
#include "THaG0HelicityReader.h"

class THaHelicity : public THaADCHelicity, public THaG0HelicityReader {
  
public:
  THaHelicity( const char* name, const char* description, 
	       THaApparatus* a = NULL );
  virtual ~THaHelicity();

  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );

  THaHelicity();
  
protected:


  Bool_t fCheckRedund;      // Warn if G0 and ADC helicities disagree

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaHelicity,2)   // 

};

#endif

