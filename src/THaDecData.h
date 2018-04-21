#ifndef Podd_THaDecData_h_
#define Podd_THaDecData_h_

//////////////////////////////////////////////////////////////////////////
//
// THaDecData
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"
#include "THashList.h"
#include "BdataLoc.h"

class TString;

class THaDecData : public THaApparatus {
  
public:
  THaDecData( const char* name = "D",
	      const char* description = "Raw decoder data" );
  virtual ~THaDecData();

  virtual EStatus Init( const TDatime& run_time );
  virtual void    Clear( Option_t* opt="" );
  virtual Int_t   Decode( const THaEvData& );
  virtual void    Print( Option_t* opt="" ) const;
  virtual void    Reset( Option_t* opt="" );

  // Disabled functions from THaApparatus
  virtual Int_t   AddDetector( THaDetector*, Bool_t, Bool_t ) { return 0; }
  virtual Int_t   Reconstruct() { return 0; }

protected:
  UInt_t          evtype;      // CODA event type
  UInt_t          evtypebits;  // Bitpattern of active trigger numbers
  THashList       fBdataLoc;   // Raw data channels

  virtual Int_t   DefineVariables( EMode mode = kDefine );
  virtual FILE*   OpenFile( const TDatime& date );
  virtual Int_t   ReadDatabase( const TDatime& date );

  Int_t           DefineLocType( const BdataLoc::BdataLocType& loctype,
				 const TString& configstr, bool re_init );

  ClassDef(THaDecData,0)
};

#endif
