#ifndef Podd_THaAnalysisObject_h_
#define Podd_THaAnalysisObject_h_

//////////////////////////////////////////////////////////////////////////
//
// THaAnalysisObject
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "THaGlobals.h"
#include "TDatime.h"
#include "Database.h"
#include "DataType.h"
#include "OptionalType.h"

#include <vector>
#include <string>
#include <cstdio>
#include <map>
#include <cstdarg>
#include <utility>

class THaEvData; //needed by derived classes
class TList;
class TVector3;
class THaRunBase;
class THaOutput;
class TObjArray;

class THaAnalysisObject : public TNamed {
  
public:
  enum EStatus { kOK = 0, kInitError = -8, kFileError = -9, kNotinit = -10 };
  enum EType   { kVarDef, kRVarDef };
  enum EMode   { kDefine, kDelete };

  THaAnalysisObject();  // only for ROOT I/O
  THaAnalysisObject( const THaAnalysisObject& ) = delete;
  THaAnalysisObject( const THaAnalysisObject&& ) = delete;
  THaAnalysisObject& operator=( const THaAnalysisObject& ) = delete;
  THaAnalysisObject& operator=( const THaAnalysisObject&& ) = delete;

  virtual ~THaAnalysisObject();
  
  virtual Int_t        Begin( THaRunBase* r=nullptr );
  virtual void         Clear( Option_t* ="" ) {} // override TNamed::Clear()
  virtual Int_t        End( THaRunBase* r=nullptr );
  virtual const char*  GetDBFileName() const;
          const char*  GetClassName() const;
          const char*  GetConfig() const         { return fConfig.Data(); }
          Int_t        GetDebug() const          { return fDebug; }
          const char*  GetPrefix() const         { return fPrefix; }
          TString      GetPrefixName() const;
          EStatus      Init();
  virtual EStatus      Init( const TDatime& run_time );
          Bool_t       IsInit() const            { return IsOK(); }
          Bool_t       IsOK() const              { return (fStatus == kOK); }

	  TDatime      GetInitDate() const       { return fInitDate; }

          void         SetConfig( const char* label );
  virtual void         SetDebug( Int_t level );
  virtual void         SetName( const char* name );
  virtual void         SetNameTitle( const char* name, const char* title );
          EStatus      Status() const            { return fStatus; }

  virtual Int_t        InitOutput( THaOutput * );
          Bool_t       IsOKOut() const           { return fOKOut; }
  virtual FILE*        OpenFile( const TDatime& date );
  virtual FILE*        OpenRunDBFile( const TDatime& date );
  virtual void         Print( Option_t* opt="" ) const;

  // For backwards compatibility
  static Int_t    LoadDB( FILE* file, const TDatime& date,
                          const DBRequest* request, const char* prefix,
                          Int_t search = 0,
                          const char* here = "THaAnalysisObject::LoadDB" )
  {
    return Podd::LoadDatabase(file, date, request, prefix, search, here);
  }

  // Geometry utility functions
  static  void    GeoToSph( Double_t  th_geo, Double_t  ph_geo,
			    Double_t& th_sph, Double_t& ph_sph );
  static  void    SphToGeo( Double_t  th_sph, Double_t  ph_sph,
			    Double_t& th_geo, Double_t& ph_geo );

  static  Bool_t  IntersectPlaneWithRay( const TVector3& xax,
					 const TVector3& yax,
					 const TVector3& org,
					 const TVector3& ray_start,
					 const TVector3& ray_vect,
					 Double_t& length,
					 TVector3& intersect );

  static Int_t    DefineVarsFromList( const void* list,
                                      EType type, EMode mode,
                                      const char* def_prefix,
                                      const TObject* obj,
                                      const char* prefix,
                                      const char* here,
                                      const char* comment_subst = "" );

  static void     PrintObjects( Option_t* opt="" );

protected:

  enum EProperties { kNeedsRunDB = BIT(0), kConfigOverride = BIT(1) };

  // General status variables
  char*           fPrefix;    // Name prefix for global variables
  EStatus         fStatus;    // Initialization status flag
  Int_t           fDebug;     // Debug level
  Bool_t          fIsInit;    // Flag indicating that ReadDatabase done
  Bool_t          fIsSetup;   // Flag indicating that DefineVariables done.
  TString         fConfig;    // Configuration to use from database
  UInt_t          fProperties;// Properties of this object (see EProperties)
  Bool_t          fOKOut;     // Flag indicating object-output prepared
  TDatime         fInitDate;  // Date passed to Init

  std::map<std::string,UInt_t> fMessages; // Warning messages & count
  UInt_t          fNEventsWithWarnings;   // Events with warnings

  TObject*        fExtra;     // Additional member data (for binary compat.)

  virtual Int_t        DefineVariables( EMode mode = kDefine );
          Int_t        DefineVarsFromList( const VarDef* list,
                                           EMode mode = kDefine,
                                           const char* def_prefix = "",
                                           const char* comment_subst = "" ) const;
          Int_t        DefineVarsFromList( const RVarDef* list, EMode mode,
                                           const char* def_prefix = "",
                                           const char* comment_subst = "" ) const;
          Int_t        DefineVarsFromList( const void* list, EType type,
                                           EMode mode, const char* def_prefix = "",
                                           const char* comment_subst = "" ) const;

  virtual void         DoError( int level, const char* location,
				const char* fmt, va_list va) const;

  THaAnalysisObject*   FindModule( const char* name, const char* classname,
				   bool do_error = true );

  virtual const char*  Here( const char* ) const;
  virtual const char*  ClassNameHere( const char* ) const;
          Int_t        LoadDB( FILE* f, const TDatime& date,
			       const DBRequest* req, Int_t search = 0 ) const;
          void         MakePrefix( const char* basename );
  virtual void         MakePrefix();
  virtual Int_t        ReadDatabase( const TDatime& date );
  virtual Int_t        ReadRunDatabase( const TDatime& date );
          Int_t        RemoveVariables();

#ifdef WITH_DEBUG
  void DebugPrint( const DBRequest* list ) const;

  template <typename T>  // available for Double_t, Float_t, UInt_t and Int_t
  static void WriteValue( T val, int p=0, int w=5 );
#endif

  // Only derived classes may construct
  THaAnalysisObject( const char* name, const char* description );

private:
  Int_t DefineVariablesWrapper( EMode mode = kDefine );

  static TList* fgModules;  // List of all currently existing Analysis Modules

  ClassDef(THaAnalysisObject,2)   //ABC for a data analysis object
};

//____________ inline functions _______________________________________________

#endif
