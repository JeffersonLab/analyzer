#ifndef ROOT_THaVDCHit
#define ROOT_THaVDCHit

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCHit                                                                 //
//                                                                           //
// Class representing a single hit for the VDC                               //
///////////////////////////////////////////////////////////////////////////////
#include "TObject.h"

#include "THaVDCWire.h"
#include <stdio.h>


class THaVDCHit : public TObject {

public:
  THaVDCHit( THaVDCWire* wire=NULL, Int_t rawtime=0, Double_t time=0.0 ) : 
    fWire(wire), fRawTime(rawtime), fTime(time), fDist(0.0) {}
  virtual ~THaVDCHit();

  virtual Double_t ConvertTimeToDist(Double_t slope);
  Int_t Compare (TObject * obj);
  Bool_t IsSortable () const { return kTRUE; }
  
  // Get and Set Functions
  THaVDCWire* GetWire() { return fWire; }
  Int_t GetRawTime() { return fRawTime; }
  Double_t GetTime() { return fTime; }
  Double_t GetDist() { return fDist; }
  Double_t GetPos()  { return fWire->GetPos(); } //Position of hit wire

  void SetWire(THaVDCWire * wire) { fWire = wire; }
  void SetRawTime(Int_t time)    { fRawTime = time; }
  void SetTime(Double_t time)    { fTime = time; }
  void SetDist(Double_t dist) { fDist = dist; }
  
protected:
  THaVDCWire* fWire;     // Wire on which the hit occurred
  Int_t       fRawTime;  // TDC value (channels)
  Double_t    fTime;     // Time corrected for time offset of wire (s)
  Double_t    fDist;     // (Perpendicular) Drift Distance
  
  
  THaVDCHit( const THaVDCHit& ) {}
  THaVDCHit& operator=( const THaVDCHit& ) { return *this; }

  
  ClassDef(THaVDCHit,0)             // VDCHit class
};

////////////////////////////////////////////////////////////////////////////////

#endif
