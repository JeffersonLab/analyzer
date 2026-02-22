#ifndef Podd_THaRaster_h_
#define Podd_THaRaster_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaRaster                                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaBeamDet.h"
#include "TVectorT.h"
#include "TMatrixD.h"

class THaRaster : public THaBeamDet {
public:
  explicit THaRaster( const char* name, const char* description = "",
                      THaApparatus* a = nullptr );
  ~THaRaster() override;

  void       Clear( Option_t* ="" ) override;
  Int_t      Decode( const THaEvData& ) override;
  Int_t      Process() override;

  TVector3 GetPosition()  const override { return fPosition[2]; }
  TVector3 GetDirection() const override { return fDirection; }

  // As soon as someone finds a better solution, the following lines should be
  // changed. It is ridiculous to have nine methods to get the components
  // of the beam position at various locations, but I do not know how else to
  // get them into histograms except for writing my own event loop

  Double_t GetRawPosX() { return fRawPos(0); }
  Double_t GetRawPosY() { return fRawPos(1); }
  Double_t GetRawSlopeX() { return fRawSlope(0); }
  Double_t GetRawSlopeY() { return fRawSlope(1); }

  Double_t GetPosBPMAX() { return fPosition[0](0); }
  Double_t GetPosBPMAY() { return fPosition[0](1); }
  Double_t GetPosBPMAZ() { return fPosition[0](2); }

  Double_t GetPosBPMBX() { return fPosition[1](0); }
  Double_t GetPosBPMBY() { return fPosition[1](1); }
  Double_t GetPosBPMBZ() { return fPosition[1](2); }

  Double_t GetPosTarX() { return fPosition[2](0); }
  Double_t GetPosTarY() { return fPosition[2](1); }
  Double_t GetPosTarZ() { return fPosition[2](2); }

protected:
  Int_t  ReadDatabase( const TDatime& date ) override;
  Int_t  DefineVariables( EMode mode = kDefine ) override;

  bool      CheckHitInfo( const DigitizerHitInfo_t& hitinfo ) const;
  Int_t     StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
  OptUInt_t LoadData( const THaEvData& evdata,
                      const DigitizerHitInfo_t& hitinfo ) override;

  typedef TVectorT<Double_t> TVectorD;

  TVectorD  fRawPos;        // current in Raster ADCs for position
  TVectorD  fRawSlope;      // current in Raster ADCs for the derivative (maybe unused)

  TVector3  fPosition[3];   // Beam position at 1st, 2nd BPM or at the target (meters)
  TVector3  fDirection;  // Beam angle at the target (meters)

  TMatrixD  fRaw2Pos[3];
  TVector3  fPosOff[3];

  TVectorD  fRasterFreq;
  TVectorD  fSlopePedestal;
  TVectorD  fRasterPedestal;

  Int_t fNfired;

  ClassDefOverride(THaRaster,0)   // Generic Raster class
};

////////////////////////////////////////////////////////////////////////////////

#endif
