
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
    UInt_t   first;  // logical number of first channel
    UInt_t   model;  // model number of module (for ADC/TDC identification).
                     // Upper two bytes of model -> ADC/TDC-ness of module
  };

  THaDetMap();
  THaDetMap( const THaDetMap& );
  THaDetMap& operator=( const THaDetMap& );
  virtual ~THaDetMap();
  
  virtual Int_t     AddModule( UShort_t crate, UShort_t slot, 
			       UShort_t chan_lo, UShort_t chan_hi,
			       UInt_t first=0, UInt_t model=0 );
  virtual void      Clear() { fNmodules = 0; }
          Module*   GetModule( UShort_t i ) const { return (Module*)fMap+i; }
          Int_t     GetSize() const { return static_cast<Int_t>(fNmodules); }

	  UInt_t    GetModel( Module *d );
	  Bool_t    IsADC(Module *d);
	  Bool_t    IsTDC(Module *d);

  virtual void      Print( Option_t* opt="" ) const;

  static const int kDetMapSize = 100;  //Maximum size of the map (sanity check)

protected:
  UShort_t     fNmodules;    //Number of modules (=crate,slot) with data
                             // from this detector

  Module*      fMap;         //Array of modules, each module is a 6-tuple
                             // (create,slot,chan_lo,chan_hi,first_logical, model)

  Int_t        fMaplength;   // current size of the fMap array
  
  static const UInt_t ADCBit = 0x1<<31;
  static const UInt_t TDCBit = 0x1<<30;
  static const UInt_t ModelMask = 0xffff0000;
  
  ClassDef(THaDetMap,0)   //The standard detector map
};

inline Bool_t THaDetMap::IsADC(Module* d) {
  return d->model & ADCBit;
}

inline Bool_t THaDetMap::IsTDC(Module* d) {
  return d->model & TDCBit;
}

inline UInt_t THaDetMap::GetModel(Module* d) {
  return d->model & ModelMask;
}


#endif
