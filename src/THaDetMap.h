
#ifndef ROOT_THaDetMap
#define ROOT_THaDetMap

//////////////////////////////////////////////////////////////////////////
//
// THaDetMap
//
// Standard detector map for a Hall A detector.
// The detector map defines the hardware channels that correspond to a
// single detector. Typically, "channels" are Fastbus addresses 
// characterized by
//
//   Crate, Slot, array of channels
//
// This is a very preliminary version.
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"

class THaDetMap {

public:
  struct Module {
    UShort_t crate;
    UShort_t slot;
    UShort_t lo;
    UShort_t hi;
  };

  THaDetMap();
  THaDetMap( const THaDetMap& );
  THaDetMap& operator=( const THaDetMap& );
  virtual ~THaDetMap();
  
  virtual Int_t     AddModule( UShort_t crate, UShort_t slot, 
			       UShort_t chan_lo, UShort_t chan_hi );
  virtual void      Clear() { fNmodules = 0; }
          Module*   GetModule( UShort_t i ) const { return (Module*)fMap+i; }
          Int_t     GetSize() const { return static_cast<Int_t>(fNmodules); }
  virtual void      Print( Option_t* opt="" ) const;

  static const int kDetMapSize = 32;  //Size of the map

protected:
  UShort_t     fNmodules;    //Number of modules (=crate,slot) with data from this detector
  UShort_t*    fMap;         //Array of modules, each module is a 4-tuple (create,slot,chan_lo,chan_hi)

  ClassDef(THaDetMap,0)   //The standard detector map
};

#endif
