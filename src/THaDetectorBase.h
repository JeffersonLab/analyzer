#ifndef ROOT_THaDetectorBase
#define ROOT_THaDetectorBase

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "TVector3.h"
#include <cstdio>

class THaDetMap;
class THaEvData;
class THaApparatus;
class TDatime;
struct VarDef;
struct RVarDef;

class THaDetectorBase : public TNamed {
  
public:
  enum EStatus { kOK, kNotinit, kInitError };

  virtual ~THaDetectorBase();
  
  virtual Int_t   Decode( const THaEvData& ) = 0;
  Int_t           GetDebug() const                { return fDebug; }
  Int_t           GetNelem() const                { return fNelem; }
  virtual const char* GetDBFileName() const = 0;
  const TVector3& GetOrigin() const               { return fOrigin; }
  const char*     GetPrefix() const               { return fPrefix; }
  const Float_t*  GetSize() const                 { return fSize; }
  EStatus         Init();
  virtual EStatus Init( const TDatime& date );
  Bool_t          IsOK() const                    { return (fStatus == kOK); }
  Bool_t          IsInit() const                  { return IsOK(); }
  void            PrintDetMap( Option_t* opt="") const;
  virtual void    SetDebug( Int_t level )         { fDebug = level; }
  EStatus         Status() const                  { return fStatus; }
  virtual void    SetName( const char* name );
  virtual void    SetNameTitle( const char* name, const char* title );

  static  FILE*   OpenFile( const char *name, const TDatime& date,
			    const char *here = "OpenFile()",
			    const char *filemode = "r", 
			    const int debug_flag = 1);

protected:

  // Mapping
  THaDetMap*      fDetMap;    // Hardware channel map for this detector

  // Configuration
  Int_t           fNelem;     // Number of detector elements (paddles, mirrors)

  // Geometry 
  TVector3        fOrigin;    // Origin of detector plane in detector coordinates
  Float_t         fSize[3];   // Detector size in x,y,z (cm)
  
  // General status variables
  char*           fPrefix;    // Name prefix for global variables
  EStatus         fStatus;    // Flag indicating status of initialization
  Int_t           fDebug;     // Debug level
  bool            fIsInit;    // Flag indicating that ReadDatabase called.
  bool            fIsSetup;   // Flag indicating that SetupDetector called.

  virtual Int_t   DefineVariables( const VarDef* list ) const;
  virtual Int_t   DefineVariables( const RVarDef* list ) const;
  virtual const char* Here( const char* ) const;
  void            MakePrefix( const char* basename );
  virtual void    MakePrefix() = 0;
  virtual FILE*   OpenFile( const TDatime& date )
    { return OpenFile(GetDBFileName(), date, Here("OpenFile()")); }
  virtual Int_t   ReadDatabase( FILE* file, const TDatime& date )
    { return kOK; }
  virtual Int_t   RemoveVariables() const;
  virtual Int_t   SetupDetector( const TDatime& date )   { return kOK; }

  //Only derived classes may construct me
  THaDetectorBase() : fDetMap(NULL), fPrefix(NULL), fStatus(kNotinit), 
    fDebug(0), fIsInit(false), fIsSetup(false) {}
  THaDetectorBase( const char* name, const char* description );

  ClassDef(THaDetectorBase,0)   //ABC for a detector or subdetector

};

#endif
