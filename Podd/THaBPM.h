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
  virtual ~THaBPM();

  virtual void   Clear( Option_t* ="" );
  virtual Int_t  Decode( const THaEvData& );
  virtual Int_t  Process();

  virtual TVector3 GetPosition()  const { return fPosition; }
  virtual TVector3 GetDirection()  const { return fDirection; }

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

  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

  bool              CheckHitInfo( const DigitizerHitInfo_t& hitinfo ) const;
  virtual OptUInt_t LoadData( const THaEvData& evdata,
                              const DigitizerHitInfo_t& hitinfo );
  virtual Int_t     StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data );

  ClassDef(THaBPM,0)   // Generic BPM class
};

////////////////////////////////////////////////////////////////////////////////

#endif
