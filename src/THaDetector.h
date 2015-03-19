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

class THaApparatus;

class THaDetector : public THaDetectorBase {

public:
  virtual ~THaDetector();
  THaApparatus*  GetApparatus() const;
  virtual void   SetApparatus( THaApparatus* );

  THaDetector();  // for ROOT I/O only

protected:

  virtual void MakePrefix();

  //Only derived classes may construct me
  THaDetector( const char* name, const char* description,
	       THaApparatus* apparatus = 0 );

private:
  TRef  fApparatus;         // Apparatus containing this detector

  ClassDef(THaDetector,1)   //Abstract base class for a Hall A detector
};

#endif
