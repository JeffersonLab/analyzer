#ifndef ROOT_THaApparatus
#define ROOT_THaApparatus

//////////////////////////////////////////////////////////////////////////
//
// THaApparatus
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"
#include "TList.h"

class THaDetector;
class THaEvData;

class THaApparatus : public THaAnalysisObject {
  
public:
  virtual ~THaApparatus();
  
  virtual Int_t        AddDetector( THaDetector* det );
  virtual Int_t        Decode( const THaEvData& );
          Int_t        GetNumDets()   const      { return fDetectors->GetSize(); }
          THaDetector* GetDetector( const char* name );
  virtual EStatus      Init( const TDatime& run_time );
  virtual void         Print( Option_t* opt="" ) const 
                                                 { fDetectors->Print(opt); }
  virtual Int_t        CoarseReconstruct() { return 0; }
  virtual Int_t        Reconstruct() = 0;
  virtual void         SetDebugAll( Int_t level );

protected:
  TList*         fDetectors;    // List of all detectors for this apparatus

  //Only derived classes may construct me  
  THaApparatus( const char* name, const char* description );

  virtual void MakePrefix() { THaAnalysisObject::MakePrefix( NULL ); }

private:
  // Prevent default construction, copying, assignment
  THaApparatus();
  THaApparatus( const THaApparatus& );
  THaApparatus& operator=( const THaApparatus& );

  ClassDef(THaApparatus,0)   //A generic apparatus (collection of detectors)
};

//_____________________________________________________________________________
inline
THaDetector* THaApparatus::GetDetector( const char* name )
{
  // Find the named detector and return a pointer to it. 
  // This is useful for specialized processing.

  return reinterpret_cast<THaDetector*>( fDetectors->FindObject( name ));
}

#endif

