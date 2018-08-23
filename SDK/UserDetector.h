#ifndef Podd_UserDetector_h_
#define Podd_UserDetector_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// UserDetector                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"
#include <vector>

class UserDetector : public THaNonTrackingDetector {

public:
  UserDetector( const char* name, const char* description = "",
		THaApparatus* a = 0 );
  UserDetector() {}
  virtual ~UserDetector();

  virtual void       Clear( Option_t* opt="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
  virtual void       Print( Option_t* opt="" ) const;

  Int_t GetNhits() const { return fEvtData.size(); }

protected:
  // Typical experimental data are far less certain than even single precision,
  // so floats are usually just fine for storing them.
  typedef Float_t Data_t;
  typedef std::vector<Data_t> Vec_t;

  // Calibration data from database
  Vec_t     fPed;       // ADC pedestals
  Vec_t     fGain;      // ADC gains

  // Per-event data
  // Define a structure to hold the information of one hit
  struct EventData {
    Int_t   fChannel;   // Channel number
    Data_t  fRawADC;    // Raw ADC data
    Data_t  fCorADC;    // ADC data corrected for pedestal and gain
    // Define a constructor so we can fill all fields in one line
    EventData(Int_t chan, Data_t raw, Data_t cor)
      : fChannel(chan), fRawADC(raw), fCorADC(cor) {}
  };

  // Vector with the hit information for the current event
  std::vector<EventData> fEvtData;

  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode );

  ClassDef(UserDetector,0)   // Example detector
};

////////////////////////////////////////////////////////////////////////////////

#endif
