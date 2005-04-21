#ifndef ROOT_THaCherenkov
#define ROOT_THaCherenkov

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCherenkov                                                              //
//                                                                           //
// Generic Cherenkov detector.                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"
#include <TClonesArray.h>

class THaCherenkov : public THaPidDetector {

public:
  THaCherenkov( const char* name, const char* description = "",
	      THaApparatus* a = NULL );
  virtual ~THaCherenkov();

  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
          Float_t    GetAsum() const { return fASUM_c; }

          Int_t      GetNTracks() const;
  const TClonesArray* GetTrackHits() const { return fTrackProj; }

protected:

  // Calibration
  Float_t*   fOff;        // [fNelem] TDC offsets (chan)
  Float_t*   fPed;        // [fNelem] ADC pedestals (chan)
  Float_t*   fGain;       // [fNelem] ADC gains

  // Per-event data
  Int_t      fNThit;      // Number of mirrors with non zero TDC times
  Float_t*   fT;          // [fNelem] Array of TDC times of channels
  Float_t*   fT_c;        // [fNelem] Array of corrected TDC times of channels
  Int_t      fNAhit;      // Number of mirrors with non zero ADC amplitudes
  Float_t*   fA;          // [fNelem] Array of ADC amplitudes of channels
  Float_t*   fA_p;        // [fNelem] Array of ADC minus pedestal values of chans
  Float_t*   fA_c;        // [fNelem] Array of corrected ADC amplitudes of chans
  Float_t    fASUM_p;     // Sum of ADC minus pedestal values of channels
  Float_t    fASUM_c;     // Sum of corrected ADC amplitudes of channels
  Float_t    fTRX;        // Xcoord(cm) of track cross point with Aero pl
  Float_t    fTRY;        // Ycoord(cm) of track cross point with Aero pl

  // Useful derived quantities
  double tan_angle, sin_angle, cos_angle; // Rotation angle of the detector plane

  TClonesArray*  fTrackProj;  // projection of track onto cerenkov plane

          void   ClearEvent();
  virtual Int_t  DefineVariables( EMode mode = kDefine );
          void   DeleteArrays();
  virtual Int_t  ReadDatabase( const TDatime& date );

  ClassDef(THaCherenkov,0)    //Generic Cherenkov class
};

////////////////////////////////////////////////////////////////////////////////

#endif
