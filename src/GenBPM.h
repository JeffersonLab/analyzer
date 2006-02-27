#ifndef ROOT_GenBPM
#define ROOT_GenBPM

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// GenBPM                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaBeamDet.h"
#include "TVector.h"
#include <iostream>

class GenBPM : public THaBeamDet {

public:
  GenBPM( const char* name, const char* description = "",
		   THaApparatus* a = NULL );
  virtual ~GenBPM();

  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      Process();

  virtual TVector3 GetPosition()  const { return fPosition; }
  virtual TVector3 GetDirection()  const { return fDirection; }

  Double_t GetRawSignal0() {return fRawSignal(0);}
  Double_t GetRawSignal1() {return fRawSignal(1);}
  Double_t GetRawSignal2() {return fRawSignal(2);}
  Double_t GetRawSignal3() {return fRawSignal(3);}

  Double_t GetRotPosX() {return fRotPos(0); }
  Double_t GetRotPosY() {return fRotPos(1); }


protected:

  void           ClearEvent();
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );
  
  static UInt_t header_str_to_base16(const char* hdr);


  //  GenBPM() {}
  //  GenBPM( const GenBPM& ) {}
  GenBPM& operator=( const GenBPM& ) { return *this; }


  TVector  fRawSignal;     // induced signal of the antennas
  TVector  fPedestals;
  TVector  fCorSignal;     // pedestal subtracted signal

  TVector fRotPos;         // position in the BPM system, arbitrary units
  
  TMatrix fRot2HCSPos;     

  UInt_t fDaqCrate, fDaqHead, fDaqOff;
  
  TVector3  fOffset;
  TVector3  fPosition;   // Beam position at the BPM (meters)
  TVector3  fDirection;  // Beam direction at the BPM
                         // always points along z-axis

  Int_t fNfired;
  Float_t fCalibRot;

  ClassDef(GenBPM,0)   // GeN BPM class
};

////////////////////////////////////////////////////////////////////////////////

#endif
