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

class THaApparatus;

class THaDetector : public THaDetectorBase {
  
public:
  virtual ~THaDetector();
  
  THaApparatus*  GetApparatus() const   { return fApparatus; }
  virtual void   SetApparatus( THaApparatus* );

protected:
  THaApparatus*  fApparatus;        // Apparatus containing this detector

//Only derived classes may construct me

  THaDetector() : fApparatus(NULL) {}     
  THaDetector( const char* name, const char* description, 
	       THaApparatus* apparatus = NULL );  

  void MakePrefix();

  ClassDef(THaDetector,0)   //Abstract base class for a Hall A detector
};

#endif
