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

#include "THaDetector.h"
#include "TRef.h"

class THaApparatus;

class THaSubDetector : public THaDetectorBase {
  
public:
  virtual ~THaSubDetector();
  
  // Get parent (sub)detector
  THaDetectorBase* GetParent() const {
    return static_cast<THaDetectorBase*>(fParent.GetObject());
  }
  THaDetector*     GetDetector() const;
  THaApparatus*    GetApparatus() const;
  
  virtual void     SetDetector( THaDetectorBase* );

protected:

  virtual const char* GetDBFileName() const;
  virtual void MakePrefix();

  //Only derived classes may construct me
  THaSubDetector( const char* name, const char* description,
		  THaDetectorBase* parent );  

 private:
  TRef fParent;        // (Sub)detector containing this subdetector

 public:
  ClassDef(THaSubDetector,1)   //ABC for a subdetector
};

//_____________________________________________________________________________
inline
THaDetector* THaSubDetector::GetDetector() const
{
  // Return parent detector (not subdetector)

  THaDetectorBase* parent = GetParent();
  while( parent && dynamic_cast<THaSubDetector*>(parent) )
    parent = static_cast<THaSubDetector*>(parent)->GetParent();
  return dynamic_cast<THaDetector*>(parent);
}

//_____________________________________________________________________________
inline
THaApparatus* THaSubDetector::GetApparatus() const
{
  // Return parent apparatus (parent of parent detector)

  THaDetector* det = GetDetector();
  return det ? det->GetApparatus() : NULL;
}

///////////////////////////////////////////////////////////////////////////////
#endif
