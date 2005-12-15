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
#include <TRef.h>

class THaSubDetector : public THaDetectorBase {
  
public:
  virtual ~THaSubDetector();
  
  THaDetectorBase*  GetDetector() const            {
    return static_cast<THaDetectorBase*>(fDetector.GetObject());
  }
  
  virtual void      SetDetector( THaDetectorBase* );

protected:

  virtual const char* GetDBFileName() const 
    { return GetDetector() ? GetDetector()->GetDBFileName() : GetPrefix(); }

//Only derived classes may construct me

  THaSubDetector( const char* name, const char* description,
		  THaDetectorBase* detector );  

  virtual void MakePrefix();

 private:
  TRef fDetector;        // (Sub)detector containing this subdetector
                         //  Use GetDetector instead to find the parent

 public:
  ClassDef(THaSubDetector,1)   //ABC for a subdetector
};

#endif
