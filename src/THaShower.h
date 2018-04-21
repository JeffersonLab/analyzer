#ifndef Podd_THaShower_h_
#define Podd_THaShower_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaShower                                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"
#include <vector>

class TClonesArray;

class THaShower : public THaPidDetector {

public:
  THaShower( const char* name, const char* description = "",
	     THaApparatus* a = NULL );
  THaShower();
  virtual ~THaShower();

  virtual void       Clear( Option_t* ="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
          Int_t      GetNclust() const { return fNclust; }
          Int_t      GetNhits() const  { return fNhits; }
          Float_t    GetE() const      { return fE; }
          Float_t    GetX() const      { return fX; }
          Float_t    GetY() const      { return fY; }

protected:

  // Mapping (see also fDetMap)
  std::vector< std::vector<UShort_t> > fChanMap; // Logical channel numbers
                                                 // for each detector map module

  // Configuration
  Int_t      fNclublk;   // Max. number of blocks composing a cluster
  Int_t      fNrows;     // Number of rows

  // Geometry
  Float_t*   fBlockX;    // [fNelem] x positions (cm) of block centers
  Float_t*   fBlockY;    // [fNelem] y positions (cm) of block centers

  // Calibration
  Float_t*   fPed;       // [fNelem] Pedestals for each block
  Float_t*   fGain;      // [fNelem] Gains for each block

  // Other parameters
  Float_t    fEmin;      // Minimum energy for a cluster center

  // Per-event data
  Int_t      fNhits;     // Number of hits
  Float_t*   fA;         // [fNelem] Array of ADC amplitudes of blocks
  Float_t*   fA_p;       // [fNelem] Array of ADC minus pedestal values of blocks
  Float_t*   fA_c;       // [fNelem] Array of corrected ADC amplitudes of blocks
  Float_t    fAsum_p;    // Sum of blocks ADC minus pedestal values
  Float_t    fAsum_c;    // Sum of blocks corrected ADC amplitudes
  Int_t      fNclust;    // Number of clusters
  Float_t    fE;         // Energy (MeV) of main cluster
  Float_t    fX;         // x position (cm) of main cluster
  Float_t    fY;         // y position (cm) of main cluster
  Int_t      fMult;      // Number of blocks in main cluster
  Int_t*     fNblk;      // [fNclublk] Numbers of blocks composing main cluster
  Float_t*   fEblk;      // [fNclublk] Energies of blocks composing main cluster

  void           DeleteArrays();
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

  ClassDef(THaShower,0)     //Generic shower detector class
};

////////////////////////////////////////////////////////////////////////////////

#endif
