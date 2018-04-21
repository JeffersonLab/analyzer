#ifndef Podd_THaHRS_h_
#define Podd_THaHRS_h_

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

  Bool_t GetTrSorting() const;
  Bool_t SetTrSorting( Bool_t set = kFALSE );
  Bool_t AutoStandardDetectors( Bool_t set = kTRUE );
  
  virtual EStatus Init( const TDatime& run_time );

protected:
  THaNonTrackingDetector* fRefDet;  // calculate time track hits this plane

  // Bit flags
  enum {
    kSortTracks   = BIT(14), // Tracks are to be sorted by chi2
    kAutoStdDets  = BIT(15)  // Auto-create standard detectors if no "vdc"
  };

  ClassDef(THaHRS,0) //A Hall A High Resolution Spectrometer
};

#endif

