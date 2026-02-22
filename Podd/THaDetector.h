#ifndef Podd_THaDetector_h_
#define Podd_THaDetector_h_

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
class THaRunBase;

class THaDetector : public THaDetectorBase {

public:
  ~THaDetector() override;

  Int_t          End( THaRunBase* r=nullptr ) override;
  THaApparatus*  GetApparatus() const;
  virtual void   SetApparatus( THaApparatus* );

  THaDetector();  // for ROOT I/O only

protected:
  void MakePrefix() override;

  //Only derived classes may construct me
  THaDetector( const char* name, const char* description,
	       THaApparatus* apparatus = nullptr );

private:
  TRef  fApparatus;         // Apparatus containing this detector

  ClassDefOverride(THaDetector,1)   //Abstract base class for a Hall A detector
};

#endif
