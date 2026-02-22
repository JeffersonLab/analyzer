#ifndef Podd_THaBPM_h_
#define Podd_THaBPM_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaBPM                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaBeamDet.h"
#include "TVectorT.h"
#include "TMatrixD.h"

class THaBPM : public THaBeamDet {

public:
  explicit THaBPM( const char* name, const char* description = "",
                   THaApparatus* a = nullptr );
  THaBPM() : fCalibRot(0) {}
  THaBPM( const THaBPM& ) = delete;
  THaBPM& operator=( const THaBPM& ) = delete;
  ~THaBPM() override;

  void   Clear( Option_t* ="" ) override;
  Int_t  Decode( const THaEvData& ) override;
  Int_t  Process() override;

  TVector3 GetPosition()  const override { return fPosition; }
  TVector3 GetDirection()  const override { return fDirection; }

  Double_t GetRawSignal0() {return fRawSignal(0);}
  Double_t GetRawSignal1() {return fRawSignal(1);}
  Double_t GetRawSignal2() {return fRawSignal(2);}
  Double_t GetRawSignal3() {return fRawSignal(3);}

  Double_t GetRotPosX() {return fRotPos(0); }
  Double_t GetRotPosY() {return fRotPos(1); }

protected:

  TVectorD fRawSignal;     // induced signal of the antennas
  TVectorD fPedestals;
  TVectorD fCorSignal;     // pedestal subtracted signal
  TVectorD fRotPos;        // position in the BPM system, arbitrary units
  TMatrixD fRot2HCSPos;
  TVector3 fOffset;
  TVector3 fPosition;      // Beam position at the BPM (meters)
  TVector3 fDirection;     // Beam direction at the BPM
                           // always points along z-axis
  Double_t fCalibRot;

  Int_t  ReadDatabase( const TDatime& date ) override;
  Int_t  DefineVariables( EMode mode = kDefine ) override;

  bool      CheckHitInfo( const DigitizerHitInfo_t& hitinfo ) const;
  OptUInt_t LoadData( const THaEvData& evdata,
                      const DigitizerHitInfo_t& hitinfo ) override;
  Int_t     StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;

  ClassDefOverride(THaBPM,0)   // Generic BPM class
};

////////////////////////////////////////////////////////////////////////////////

#endif
