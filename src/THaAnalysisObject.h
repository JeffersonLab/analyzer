#ifndef ROOT_THaAnalysisObject
#define ROOT_THaAnalysisObject

//////////////////////////////////////////////////////////////////////////
//
// THaAnalysisObject
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"
#include "THaGlobals.h"
#include <vector>
#include <string>
#include <cstdio>

class THaEvData;
class TDatime;
class TList;
struct VarDef;
struct RVarDef;
struct TagDef;

class THaAnalysisObject : public TNamed {
  
public:
  enum EStatus { kOK, kNotinit, kInitError, kFileError };
  enum EType   { kVarDef, kRVarDef };
  enum EMode   { kDefine, kDelete };

  virtual ~THaAnalysisObject();
  
  virtual const char*  GetDBFileName() const     { return GetPrefix(); }
          Int_t        GetDebug() const          { return fDebug; }
          const char*  GetPrefix() const         { return fPrefix; }
  virtual void         Clear( Option_t* opt="" ) {}
          EStatus      Init();
  virtual EStatus      Init( const TDatime& run_time );
          Bool_t       IsInit() const            { return IsOK(); }
          Bool_t       IsOK() const              { return (fStatus == kOK); }
  virtual void         SetDebug( Int_t level )   { fDebug = level; }
  virtual void         SetName( const char* name );
  virtual void         SetNameTitle( const char* name, const char* title );
          EStatus      Status() const            { return fStatus; }

  // Static function to provide easy access to database files
  // from CINT scripts etc.
  static  FILE*   OpenFile( const char* name, const TDatime& date,
			    const char* here = "OpenFile()",
			    const char* filemode = "r", 
			    const int debug_flag = 1);

  // Access functions for reading tag/value pairs from database files
  static  Int_t   LoadDBvalue( FILE* file, const TDatime& date, 
			       const char* tag, Double_t& value );
  static  Int_t   LoadDBvalue( FILE* file, const TDatime& date, 
			       const char* tag, string& text );
  static  Int_t   LoadDBvalue( FILE* file, const TDatime& date, 
			       const char* tag, TString& text );
  static  Int_t   LoadDB( FILE* file, const TDatime& date, 
			  const TagDef* tags, const char* prefix="" );
  static  Int_t   SeekDBdate( FILE* file, const TDatime& date,
			      bool end_on_tag = false );
  static  Int_t   SeekDBconfig( FILE* file, const char* tag,
				const char* label = "config",
				bool end_on_tag = false );

  // Geometry utility functions
  static  void    GeoToSph( Double_t  th_geo, Double_t  ph_geo,
			    Double_t& th_sph, Double_t& ph_sph );
  static  void    SphToGeo( Double_t  th_sph, Double_t  ph_sph,
			    Double_t& th_geo, Double_t& ph_geo );

protected:

  enum EProperties { kNeedsRunDB = BIT(0) };

  // General status variables
  char*           fPrefix;    // Name prefix for global variables
  EStatus         fStatus;    // Initialization status flag
  Int_t           fDebug;     // Debug level
  bool            fIsInit;    // Flag indicating that ReadDatabase called.
  bool            fIsSetup;   // Flag indicating that Setup called.
  TString         fConfig;    // Configuration to use from database
  UInt_t          fProperties;// Properties of this object (see EProperties)

  virtual Int_t        DefineVariables( EMode mode = kDefine )
     { return kOK; }

          Int_t        DefineVarsFromList( const VarDef* list, 
					   EMode mode = kDefine,
					   const char* var_prefix="" ) const
     { return DefineVarsFromList( list, kVarDef, mode, var_prefix ); }

          Int_t        DefineVarsFromList( const RVarDef* list, 
					   EMode mode = kDefine,
					   const char* var_prefix="" ) const
     { return DefineVarsFromList( list, kRVarDef, mode, var_prefix ); }
 
          Int_t        DefineVarsFromList( const void* list, 
					   EType type, EMode mode,
					   const char* var_prefix="" ) const;

  THaAnalysisObject* FindModule( const char* name, const char* classname );

  virtual const char*  Here( const char* ) const;
          void         MakePrefix( const char* basename );
  virtual void         MakePrefix() = 0;
  virtual FILE*        OpenFile( const TDatime& date )
     { return OpenFile(GetDBFileName(), date, Here("OpenFile()"), "r", fDebug); }
  virtual FILE*        OpenRunDBFile( const TDatime& date )
     { return OpenFile("run", date, Here("OpenFile()"), "r", fDebug); }
  virtual Int_t        ReadDatabase( const TDatime& date )
     { return kOK; }
  virtual Int_t        ReadRunDatabase( const TDatime& date );
  virtual Int_t        RemoveVariables()
     { return DefineVariables( kDelete ); }

  // Support function for reading database files
  static vector<string> GetDBFileList( const char* name, const TDatime& date,
				       const char* here = "GetDBFileList()" );

  // Only derived classes may construct
  THaAnalysisObject( const char* name, const char* description );

  static TList* fgModules;    // List of all currently existing Analysis Modules

private:
  // Support functions for reading database files
  static Int_t IsDBdate( const string& line, TDatime& date, bool warn=true );
  static Int_t IsDBtag ( const string& line, const char* tag, string& text );

  // Prevent default construction, copying, assignment
  THaAnalysisObject();
  THaAnalysisObject( const THaAnalysisObject& );
  THaAnalysisObject& operator=( const THaAnalysisObject& );

  ClassDef(THaAnalysisObject,0)   //ABC for a data analysis object
};

//____________ inline functions _______________________________________________

#endif
