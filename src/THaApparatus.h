#ifndef ROOT_THaApparatus
#define ROOT_THaApparatus

//////////////////////////////////////////////////////////////////////////
//
// THaApparatus
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "TList.h"

class THaDetector;
class THaEvData;
class TDatime;

class THaApparatus : public TNamed {
  
public:
  enum EStatus { kOK, kNotinit, kInitError };

  virtual ~THaApparatus();
  
  virtual Int_t        AddDetector( THaDetector* det );
  virtual Int_t        Decode( const THaEvData& );
          Int_t        GetDebug() const          { return fDebug; }
          Int_t        GetNumDets()   const      { return fDetectors->GetSize(); }
          Int_t        GetNumMyDets() const      { return fNmydets; }
          THaDetector* GetDetector( const char* name );
  virtual Int_t        Init();
  virtual Int_t        Init( const TDatime& run_time );
          Bool_t       IsOK() const              { return (fStatus == kOK); }
          Bool_t       IsInit() const            { return IsOK(); }
  virtual void         Print( Option_t* opt="" ) const 
                                                 { fDetectors->Print(opt); }
  virtual Int_t        Reconstruct() = 0;
  virtual void         SetDebugAll( Int_t level );
          void         SetDebug( Int_t level )   { fDebug = level; }
          EStatus      Status() const            { return fStatus; }

protected:
  Int_t          fDebug;        // Debug level
  TList*         fDetectors;    // List of all detectors for this apparatus
  Int_t          fNmydets;      // Number of detectors defined in the constructor
  THaDetector**  fMydets;       // Array of pointers to the detectors defined by me
  EStatus        fStatus;       // Initialization status flag

  //Only derived classes may construct me  
  THaApparatus() : 
    fDebug(0), fDetectors(0), fNmydets(0), fMydets(0), fStatus(kNotinit) {}
  THaApparatus( const char* name, const char* description );
  THaApparatus( const THaApparatus& ) {};
  THaApparatus& operator=( const THaApparatus& ) { return *this; }

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

