#ifndef ROOT_THaAnalysisObject
#define ROOT_THaAnalysisObject

//////////////////////////////////////////////////////////////////////////
//
// THaAnalysisObject
//
//////////////////////////////////////////////////////////////////////////

#include "TNamed.h"

class TDatime;
struct VarDef;
struct RVarDef;

class THaAnalysisObject : public TNamed {
  
public:
  enum EStatus { kOK, kNotinit, kInitError };
  enum EType   { kVarDef, kRVarDef };
  enum EMode   { kDefine, kDelete };

  virtual ~THaAnalysisObject();
  
  virtual const char*  GetDBFileName() const     { return GetPrefix(); }
          Int_t        GetDebug() const          { return fDebug; }
          const char*  GetPrefix() const         { return fPrefix; }
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

protected:

  // General status variables
  char*           fPrefix;    // Name prefix for global variables
  EStatus         fStatus;    // Initialization status flag
  Int_t           fDebug;     // Debug level
  bool            fIsInit;    // Flag indicating that ReadDatabase called.
  bool            fIsSetup;   // Flag indicating that Setup called.

  virtual Int_t        DefineVariables( EMode mode = kDefine )
     { return kOK; }

          Int_t        DefineVarsFromList( const VarDef* list, 
					   EMode mode = kDefine ) const
     { return DefineVarsFromList( list, kVarDef, mode ); }

          Int_t        DefineVarsFromList( const RVarDef* list, 
					   EMode mode = kDefine ) const
     { return DefineVarsFromList( list, kRVarDef, mode ); }
 
          Int_t        DefineVarsFromList( const void* list, 
					   EType type, EMode mode ) const;

  virtual const char*  Here( const char* ) const;
          void         MakePrefix( const char* basename );
  virtual void         MakePrefix() = 0;
  virtual FILE*        OpenFile( const TDatime& date )
     { return OpenFile(GetDBFileName(), date, Here("OpenFile()")); }
  virtual Int_t        ReadDatabase( FILE* file, const TDatime& date )
     { return kOK; }
  virtual Int_t        RemoveVariables()
     { return DefineVariables( kDelete ); }

  //Only derived classes may construct me
  THaAnalysisObject() : fPrefix(NULL), fStatus(kNotinit), 
    fDebug(0), fIsInit(false), fIsSetup(false) {}
  THaAnalysisObject( const char* name, const char* description );

  ClassDef(THaAnalysisObject,0)   //ABC for a data analysis object
};

#endif
