#ifndef ROOT_THaApparatus
#define ROOT_THaApparatus

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
  
  virtual Int_t        AddDetector( THaDetector* det );
  virtual Int_t        Begin( THaRunBase* r=0 );
  virtual void         Clear( Option_t* opt="" );
  virtual Int_t        Decode( const THaEvData& );
  virtual Int_t        End( THaRunBase* r=0 );
          Int_t        GetNumDets() const;
  virtual THaDetector* GetDetector( const char* name );
  const   TList*       GetDetectors() { return fDetectors; } // for inspection

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

