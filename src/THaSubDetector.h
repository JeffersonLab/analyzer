#ifndef ROOT_THaSubDetector
#define ROOT_THaSubDetector

//////////////////////////////////////////////////////////////////////////
//
// THaSubDetector
//
// Abstract base class for a generic Hall A detector. This class
// describes a subdetector (part of an actual detector). It cannot
// be added to an apparatus, but it must be contained in either
// another detector or subdetector.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetectorBase.h"

class THaSubDetector : public THaDetectorBase {
  
public:
  virtual ~THaSubDetector();
  
  THaDetectorBase*  GetDetector() const            { return fDetector; }
  virtual void      SetDetector( THaDetectorBase* );

protected:
  THaDetectorBase* fDetector;   // (Sub)detector containing this subdetector

//Only derived classes may construct me

  THaSubDetector() : fDetector(0) {}     
  THaSubDetector( const char* name, const char* description,
		  THaDetectorBase* detector );  

  void MakePrefix();

  ClassDef(THaSubDetector,0)   //ABC for a subdetector
};

#endif
