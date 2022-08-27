#ifndef Podd_THaVDCHit_h_
#define Podd_THaVDCHit_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCHit                                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "THaVDCWire.h"
#include "DataType.h"
#include <cassert>
#include <functional>

class THaVDCHit : public TObject {

public:
  explicit THaVDCHit( THaVDCWire* wire=nullptr, UInt_t rawtime=0,
                      Double_t time=0.0, UInt_t nthit=0 )
    : fWire(wire), fRawTime(rawtime), fTime(time),
      fNthit(nthit), fDist(kBig), fdDist(1.0), ftrDist(kBig),
      fltrDist(kBig), fTrkNum(0), fClsNum(-1) {}
  THaVDCHit( const THaVDCHit& ) = default;
  THaVDCHit& operator=( const THaVDCHit& ) = default;

  virtual Double_t ConvertTimeToDist(Double_t slope);
  Int_t  Compare ( const TObject* obj ) const;
  Bool_t IsSortable () const { return true; }

  // Get and Set Functions
  THaVDCWire* GetWire() const { return fWire; }
  Int_t    GetWireNum() const { return fWire->GetNum(); }
  UInt_t   GetRawTime() const { return fRawTime; }
  Double_t GetTime()    const { return fTime; }
  Double_t GetDist()    const { return fDist; }
  Double_t GetPos()     const { return fWire->GetPos(); } //Position of hit wire
  Double_t GetdDist()   const { return fdDist; }
  Int_t    GetTrkNum()  const { return fTrkNum; }
  Int_t    GetClsNum()  const { return fClsNum; }
  UInt_t   GetNthit()   const { return fNthit; }

  void     SetWire(THaVDCWire * wire) { fWire = wire; }
  void     SetRawTime(UInt_t time)    { fRawTime = time; }
  void     SetTime(Double_t time)     { fTime = time; }
  void     SetDist(Double_t dist)     { fDist = dist; }
  void     SetdDist(Double_t ddist)   { fdDist = ddist; }
  void     SetFitDist(Double_t dist)  { ftrDist = dist; }
  void     SetLocalFitDist(Double_t dist)  { fltrDist = dist; }
  void     SetTrkNum(Int_t num)       { fTrkNum = num; }
  void     SetClsNum(Int_t num)       { fClsNum = num; }
  void     SetNthit(UInt_t num)       { fNthit  = num; }

  // Functor for ordering hits
  class ByWireThenTime {
  public:
    bool operator() ( const THaVDCHit* a, const THaVDCHit* b ) const
    {
      assert( a && b );
      if( a->GetWireNum() != b->GetWireNum() )
	return ( a->GetWireNum() < b->GetWireNum() );
      return ( a->GetTime() < b->GetTime() );
    }
  };

protected:

  THaVDCWire* fWire;     // Wire on which the hit occurred
  UInt_t      fRawTime;  // TDC value (channels)
  Double_t    fTime;     // Measured drift time, corrected for trigger time (s)
  UInt_t      fNthit;    // Number of TDC hits per channel per event
  Double_t    fDist;     // (Perpendicular) Drift Distance
  Double_t    fdDist;    // Uncertainty in fDist (for chi2 calc)
  Double_t    ftrDist;   // (Perpendicular) distance from the global track (m)
  Double_t    fltrDist;  // (Perpendicular) distance from the local track (m)
  Int_t       fTrkNum;   // Number of the track using this hit (0 = unused)
  Int_t       fClsNum;   // Number of the cluster using this hit (-1 = unused)

  ClassDef(THaVDCHit,4)     // VDCHit class
};

///////////////////////////////////////////////////////////////////////////////
#endif
