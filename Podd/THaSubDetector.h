#ifndef Podd_THaSubDetector_h_
#define Podd_THaSubDetector_h_

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
  
  // Get parent (sub)detector (one level up)
  THaDetectorBase* GetParent() const {
    return static_cast<THaDetectorBase*>(fParent.GetObject());
  }
  // For backward compatibility
  THaDetectorBase* GetDetector() const { return GetParent(); }
  // Search for parent THaDetector (not subdetector)
  THaDetector*     GetMainDetector() const;
  THaApparatus*    GetApparatus() const;
  
  virtual void     SetParent( THaDetectorBase* );
  void             SetDetector( THaDetectorBase* det ) { SetParent(det); }

protected:

  virtual const char* GetDBFileName() const;
  virtual void MakePrefix();

  //Only derived classes may construct me
  THaSubDetector( const char* name, const char* description,
		  THaDetectorBase* parent );  
  THaSubDetector() {} // For ROOT RTTI

 private:
  TRef fParent;        // (Sub)detector containing this subdetector

 public:
  ClassDef(THaSubDetector,1)   //ABC for a subdetector
};

#endif
