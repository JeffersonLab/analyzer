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

#include "TimeCorrectionModule.h"

class THaDetMap;

class THaTriggerTime : public Podd::TimeCorrectionModule {
public:
  explicit THaTriggerTime( const char* name="trg", const char* description = "" );
  virtual ~THaTriggerTime();

  Int_t               EventType()  const { return fEvtType; }

  virtual void        Clear( Option_t* opt="" );
  virtual Int_t       Process( const THaEvData& );

 protected:
  // Configuration
  THaDetMap* fDetMap;      // Hardware channel map
  Double_t   fTDCRes;      // time-per-channel
  Int_t      fCommonStop;  // default =0 => TDC type is common-start

  std::vector<Int_t>    fTrgTypes; // which trigger-types to watch
  std::vector<Double_t> fToffsets; // array of trigger-timing offsets

  // Event-by-event data
  std::vector<Double_t> fTrgTimes; // array of the read-out trigger times
  Int_t     fEvtType;     // the relevant event type for this spectr.

  virtual Int_t  DefineVariables( EMode mode = kDefine );
  virtual Int_t  ReadDatabase( const TDatime& date );

  ClassDef(THaTriggerTime,0)
};

#endif  /*  Podd_THaTriggerTime_h_  */
