#ifndef ROOT_VarDef
#define ROOT_VarDef

//////////////////////////////////////////////////////////////////////////
//
// VarDef
//
// Structures for calls to THaVarList::DefineVariables() and derived
// functions.
// 
// Also defines structure for THaAnalysisModule::LoadRunDB().
//
//////////////////////////////////////////////////////////////////////////

struct VarDef {
  const char*      name;     // Variable name
  const char*      desc;     // Variable description
  Int_t            type;     // Variable data type (see VarType.h)
  Int_t            size;     // Size of array (0/1 = scalar)
  const void*      loc;      // Location of data
  const Int_t*     count;    // Optional: Actual size of variable size array
};

struct RVarDef {
  const char*      name;     // Variable name
  const char*      desc;     // Variable description
  const char*      def;      // Definition of data (data member or method name)
};

struct TagDef {
  const char*      name;     // Tag name
  Double_t*        var;      // Destination variable
  Int_t            fatal;    // Error to return if tag not found (0=ignore)
};

#endif
