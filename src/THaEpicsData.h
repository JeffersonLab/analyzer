#ifndef ROOT_THaEpicsData
#define ROOT_THaEpicsData

//////////////////////////////////////////////////////////////////////////
//
// THaEpicsData
//
// A very simple class for decoding some useful EPICS data.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"

class THaEpicsData : public THaDetector {
  
public:
  THaEpicsData( const char* name, const char* description = "",
		THaApparatus* a = NULL );
  virtual ~THaEpicsData();

  virtual Int_t        Decode( const THaEvData& );
  virtual EStatus      Init( const TDatime& run_time );

protected:

  Double_t fXposa;      // EpicsData A X-axis position
  Double_t fYposa;      // EpicsData A Y-axis position
  Double_t fXposb;      // EpicsData B X-axis position
  Double_t fYposb;      // EpicsData B Y-axis position
  Double_t favgcur;     // EpicsData Average Current

  TString  fEpicsTags[5]; // EPICS tags for position/current variables

  THaEpicsData() {}
  THaEpicsData( const THaEpicsData& ) {}
  THaEpicsData& operator=( const THaEpicsData& ) { return *this; }
  
private:

  ClassDef(THaEpicsData,0)   //Generic EpicsData class
};

#endif 







