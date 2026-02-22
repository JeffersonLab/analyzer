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
  ~THaApparatus() override;
  
  virtual Int_t        AddDetector( THaDetector* det, Bool_t quiet = false,
				    Bool_t first = false );
          Int_t        Begin( THaRunBase* r = nullptr ) override;
          void         Clear( Option_t* opt = "" ) override;
  virtual Int_t        Decode( const THaEvData& );
          Int_t        End( THaRunBase* r=nullptr ) override;
          Int_t        GetNumDets() const;
  virtual THaDetector* GetDetector( const char* name );
          TList*       GetDetectors() { return fDetectors; }

          EStatus      Init( const TDatime& run_time ) override;
          void         Print( Option_t* opt = "" ) const override;
  virtual Int_t        CoarseReconstruct() { return 0; }
  virtual Int_t        Reconstruct() = 0;
  virtual void         SetDebugAll( Int_t level );

protected:
  TList*         fDetectors;    // List of all detectors for this apparatus

  THaApparatus( const char* name, const char* description );
  THaApparatus( );

  ClassDefOverride(THaApparatus,1)   //A generic apparatus (collection of detectors)
};

#endif

