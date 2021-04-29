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
  // Constructor(s) create an instance of this detector object.
  // This is how detectors are created in analysis scripts.
  explicit UserDetector( const char* name, const char* description = "",
                         THaApparatus* a = nullptr );

  // We must provide a default constructor (for technical reasons)!
  // If the detector class defines any raw pointers as data members, they
  // must be initialized to nullptr here. Otherwise "default" will do.
  UserDetector() = default;

  // If any global variables are created by DefineVariables, the destructor
  // must be implemented to clean them up. Likewise for any memory allocated
  // on the heap with "new".
  virtual ~UserDetector();

  // Public base class functions that one typically overrides
  // (see comments in UserDetector.cxx for details)
  virtual void   Clear( Option_t* opt="" );
  virtual Int_t  StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data );
  virtual Int_t  CoarseProcess( TClonesArray& tracks );
  virtual Int_t  FineProcess( TClonesArray& tracks );
  virtual void   Print( Option_t* opt="" ) const;

  Int_t GetNhits() const { return static_cast<Int_t>(fEventData.size()); }

protected:
  // Almost every detector needs to override these functions:
  // Read configuration parameters from the database
  virtual Int_t  ReadDatabase( const TDatime& date );
  // Define "global variables" holding results from analyzing this detector
  virtual Int_t  DefineVariables( EMode mode );

  //---- Data stored with this detector follow here ----

  typedef std::vector<Data_t> DataVec_t;

  // Calibration data from database
  DataVec_t fPed;       // ADC pedestals
  DataVec_t fGain;      // ADC gains

  // Per-event data
  // Define a structure to hold the information of one hit
  class EventData {
  public:
    Int_t   fChannel;   // Logical channel number
    Data_t  fRawADC;    // Raw ADC data
    Data_t  fCalADC;    // Pedestal-subtracted and gain-calibrated ADC data
    // Define a constructor so we can fill all fields in one line
    EventData(Int_t chan, Data_t raw, Data_t cal)
      : fChannel(chan), fRawADC(raw), fCalADC(cal) {}
  };

  // Vector with the hit information for the current event
  std::vector<EventData> fEventData;

  ClassDef(UserDetector,0)   // Example detector
};

////////////////////////////////////////////////////////////////////////////////

#endif
