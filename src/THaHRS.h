#ifndef ROOT_THaHRS
#define ROOT_THaHRS

//////////////////////////////////////////////////////////////////////////
//
// THaHRS
//
//////////////////////////////////////////////////////////////////////////

#include "THaSpectrometer.h"

class THaNonTrackingDetector;

class THaHRS : public THaSpectrometer {
  
public:
  THaHRS( const char* name, const char* description );
  virtual ~THaHRS();

  virtual Int_t   FindVertices( TClonesArray& tracks );
  virtual Int_t   TrackCalc();
  virtual Int_t   TrackTimes( TClonesArray* tracks );

  virtual Int_t   SetRefDet( const char* name );
  virtual Int_t   SetRefDet( const THaNonTrackingDetector* obj );

  THaNonTrackingDetector* GetRefDet() const { return fRefDet; }

  Bool_t SetTrSorting( Bool_t set = kFALSE );
  Bool_t GetTrSorting() const;
  
protected:
  THaNonTrackingDetector* fRefDet;  // calculate time track hits this plane

  // Flag for fProperties indicating that tracks are to be sorted by chi2
  static const UInt_t kSortTracks = BIT(16);

  ClassDef(THaHRS,0) //A Hall A High Resolution Spectrometer
};

#endif

