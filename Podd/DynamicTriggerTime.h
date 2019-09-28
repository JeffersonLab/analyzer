#ifndef Podd_DynamicTriggerTime_h_
#define Podd_DynamicTriggerTime_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::DynamicTriggerTime                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTriggerTime.h"

namespace Podd {

class DynamicTriggerTime : public THaTriggerTime {
public:
  DynamicTriggerTime( const char *name = "trg", const char *description = "",
                      THaApparatus *a = NULL );

  virtual Int_t Decode( const THaEvData & );
  virtual void  Clear( Option_t *opt = "" );
  virtual Int_t DefineVariables( EMode mode = kDefine );

protected:

  virtual Int_t ReadDatabase( const TDatime &date );

  ClassDef( DynamicTriggerTime, 0 ) // Event-by-event trigger time correction
};

} // namespace Podd

#endif  /*  Podd_DynamicTriggerTime_h_  */
