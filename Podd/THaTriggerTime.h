#ifndef Podd_THaTriggerTime_h_
#define Podd_THaTriggerTime_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaTriggerTime                                                            //
//                                                                           //
//  Simple processing: report the global offset of the common-start/stop     //
//   for a given spectrometer, for use as a common offset to fix-up          //
//   places where the absolute time (ie VDC) is used.                        //
//                                                                           //
//  Lookup in the database what the offset is for each trigger-type, and     //
//  report it for each appropriate event.                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"

class THaTriggerTime : public THaNonTrackingDetector {
 public:
  THaTriggerTime( const char* name="trg", const char* description = "",
      THaApparatus* a = NULL );

  ~THaTriggerTime();
  
  virtual Int_t       Decode( const THaEvData& );
  Double_t            TimeOffset() const { return fEvtTime; }
  Int_t               EventType() const { return fEvtType; }
  
  virtual void        Clear( Option_t* opt="" );
  
  virtual Int_t       DefineVariables( EMode mode = kDefine );

  virtual Int_t       CoarseProcess(TClonesArray&) { return 0; }
  virtual Int_t       FineProcess(TClonesArray&) { return 0; }

 protected:
  Double_t  fEvtTime;     // the offset for this event
  Int_t     fEvtType;     // the relevant event type for this spectr.

  Double_t  fTDCRes;      // time-per-channel
  Double_t  fGlOffset;    // overall offset shared by all
  Int_t     fCommonStop;     // default =0 => TDC type is common-start 

  Int_t     fNTrgType;    // number of trigger types
  Double_t *fTrgTimes;    //[fNTrgType] array of the read-out trigger times
  
  std::vector<Double_t> fToffsets;   // array of trigger-timing offsets
  std::vector<Int_t>    fTrgTypes;   // which trigger-types to watch
  
  virtual Int_t  ReadDatabase( const TDatime& date );

  ClassDef(THaTriggerTime,0)
};

#endif  /*  Podd_THaTriggerTime_h_  */
