#ifndef ROOT_THaDetector
#define ROOT_THaDetector

//////////////////////////////////////////////////////////////////////////
//
// THaDetector
//
// Abstract base class for a generic Hall A detector. This class 
// describes an actual detector (not subdetector) and can be added to
// an apparatus.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetectorBase.h"
#include <TRef.h>
#include "THaApparatus.h"

//class THaApparatus;

class THaDetector : public THaDetectorBase {
  
public:
  virtual ~THaDetector();
  THaApparatus*  GetApparatus() const   {
    return static_cast<THaApparatus*>(fApparatus.GetObject());
  }
  
  virtual void   SetApparatus( THaApparatus* );

  THaDetector();  // for ROOT I/O only
protected:
  TRef  fApparatus;          // Apparatus containing this detector

//Only derived classes may construct me

  THaDetector( const char* name, const char* description, 
	       THaApparatus* apparatus = NULL );  

  virtual void MakePrefix();

  ClassDef(THaDetector,1)   //Abstract base class for a Hall A detector
};

#endif
