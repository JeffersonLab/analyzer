#ifndef Podd_THaApparatus_h_
#define Podd_THaApparatus_h_

//////////////////////////////////////////////////////////////////////////
//
// THaApparatus
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"

class THaDetector;
class THaEvData;
class TList;

class THaApparatus : public THaAnalysisObject {
  
public:
  virtual ~THaApparatus();
  
  virtual Int_t        AddDetector( THaDetector* det, Bool_t quiet = false,
				    Bool_t first = false );
  virtual Int_t        Begin( THaRunBase* r=0 );
  virtual void         Clear( Option_t* opt="" );
  virtual Int_t        Decode( const THaEvData& );
  virtual Int_t        End( THaRunBase* r=0 );
          Int_t        GetNumDets() const;
  virtual THaDetector* GetDetector( const char* name );
          TList*       GetDetectors() { return fDetectors; }

  virtual EStatus      Init( const TDatime& run_time );
  virtual void         Print( Option_t* opt="" ) const;
  virtual Int_t        CoarseReconstruct() { return 0; }
  virtual Int_t        Reconstruct() = 0;
  virtual void         SetDebugAll( Int_t level );

protected:
  TList*         fDetectors;    // List of all detectors for this apparatus

  THaApparatus( const char* name, const char* description );
  THaApparatus( );

  ClassDef(THaApparatus,1)   //A generic apparatus (collection of detectors)
};

#endif

