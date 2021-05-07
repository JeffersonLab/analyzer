#ifndef Podd_THaShower_h_
#define Podd_THaShower_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaShower                                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"
#include "DetectorData.h"
#include <vector>

class TClonesArray;

class THaShower : public THaPidDetector {

public:
  explicit THaShower( const char* name, const char* description = "",
                      THaApparatus* a = nullptr );
  THaShower(); // for ROOT I/O
  virtual ~THaShower();

  // THaShower now uses THaDetectorBase::Decode()
  virtual void       Clear( Option_t* ="" );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
          UInt_t     GetMainClusterSize() const { return fClBlk.size(); }
          UInt_t     GetNclust() const { return fNclust; }
          UInt_t     GetNhits() const  { return fADCData->GetHitCount(); }
          Data_t     GetE() const      { return fE; }
          Data_t     GetX() const      { return fX; }
          Data_t     GetY() const      { return fY; }

  // Extension of standard ADCData to handle channel mapping
  class ShowerADCData : public Podd::ADCData {
  public:
    ShowerADCData( const char* name, const char* desc, Int_t nelem,
                   const std::vector<Int_t>& chanmap )
      : Podd::ADCData(name,desc,nelem), fChanMap(chanmap) {}
    Int_t GetLogicalChannel( const DigitizerHitInfo_t& hitinfo ) const override;
  protected:
    const std::vector<Int_t>& fChanMap;
  };

protected:

  // Mapping (see also fDetMap)
  std::vector<Int_t> fChanMap; // Map physical -> logical channel numbers

  // Configuration
  Int_t      fNrows;     // Number of rows
  Data_t     fEmin;      // Minimum energy for a cluster center

  // Geometry
  class CenterPos {
  public:
    Data_t x;  // x-position of block center (m)
    Data_t y;  // y-position of block center (m)
  };
  std::vector<CenterPos> fBlockPos;  // Block center positions

  // Per-event data
  Data_t     fAsum_p;    // Sum of blocks ADC minus pedestal values
  Data_t     fAsum_c;    // Sum of blocks corrected ADC amplitudes
  UInt_t     fNclust;    // Number of clusters
  Data_t     fE;         // Energy (MeV) of main cluster
  Data_t     fX;         // Energy-weighted x position (m) of main cluster
  Data_t     fY;         // Energy-weighted y position (m) of main cluster

  class ClusterBlock {
  public:
    Int_t  n;  // Block number (0..fNelem-1)
    Data_t E;  // Energy deposit (MeV) for current event
  };
  std::vector<ClusterBlock> fClBlk; // Blocks of main cluster

  ShowerADCData* fADCData; // Convenience pointer to ADC data in fDetectorData

  virtual Int_t  StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data );
  virtual void   PrintDecodedData( const THaEvData& evdata ) const;

  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

  ClassDef(THaShower,0)     //Generic shower detector class
};

////////////////////////////////////////////////////////////////////////////////

#endif
