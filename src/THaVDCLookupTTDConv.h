#ifndef ROOT_THaVDCLookupTTDConv
#define ROOT_THaVDCLookupTTDConv

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCLookupTTDConv                                                       //
//                                                                           //
// Uses a pre-generated lookup table to convert from time to distance        // 
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "THaVDCTimeToDistConv.h"

class THaVDCLookupTTDConv : public THaVDCTimeToDistConv {

public:
  THaVDCLookupTTDConv( );
  THaVDCLookupTTDConv(Double_t t0, Int_t num_bins, Float_t *table):
    fT0(t0), fNumBins(num_bins), fTable(table) 
    {
      fLongestDist = 0.01512;   // (m)
      fTimeRes = 0.5e-9;        // (s)
    }

  virtual ~THaVDCLookupTTDConv();

  virtual Double_t ConvertTimeToDist(Double_t time, Double_t tanTheta);

  Double_t GetT0() const { return fT0; }
  Int_t GetNumBins() const { return fNumBins; }
  Float_t *GetTable() const { return fTable; }

  Double_t GetLongestDist() const { return fLongestDist; }
  Double_t GetTimeRes() const { return fTimeRes; }

protected:

  THaVDCLookupTTDConv( const THaVDCLookupTTDConv& ) {}
  THaVDCLookupTTDConv& operator=( const THaVDCLookupTTDConv& ) 
  { return *this; }

  Double_t fT0;     // calculated zero time 
  Int_t fNumBins;   // size of lookup table
  Float_t *fTable;  // time-to-distance lookup table
                    // note that the table is allocated outside of the class

  
  // FIXME: these should be loaded from DB
  Double_t fLongestDist;
  Double_t fTimeRes;

  ClassDef(THaVDCLookupTTDConv,0)             // VDC Lookup TTD Conv class
};


////////////////////////////////////////////////////////////////////////////////

#endif
