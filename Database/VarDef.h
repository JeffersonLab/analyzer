#ifndef Podd_VarDef_h_
#define Podd_VarDef_h_

//////////////////////////////////////////////////////////////////////////
//
// VarDef
//
// Structures for calls to THaVarList::DefineVariables() and derived
// functions.
//
// Also defines structure for THaAnalysisModule::LoadDB().
//
//////////////////////////////////////////////////////////////////////////

#include "Rtypes.h"   // IWYU pragma: export
#include "VarType.h"  // IWYU pragma: export

struct VarDef {
  const char*  name     {nullptr};   // Variable name
  const char*  desc     {""};        // Variable description
  Int_t        type     {kDouble};   // Variable data type (see VarType.h)
  Int_t        size     {0};         // Size of array (0/1 = scalar)
  const void*  loc      {nullptr};   // Location of data
  const Int_t* count    {nullptr};   // (opt) Actual size of variable size array
};

struct RVarDef {
  const char*  name     {nullptr};   // Variable name
  const char*  desc     {""};        // Variable description
  const char*  def      {nullptr};   // Definition of data (data member or method name)
};

struct DBRequest {
  const char*  name     {nullptr};   // Key name
  void*        var      {nullptr};   // Pointer to data (default to Double*)
  VarType      type     {kDouble};   // (opt) data type (see VarType.h, default Double_t)
  UInt_t       nelem    {0};         // (opt) number of array elements (0/1 = 1 or auto)
  Bool_t       optional {false};     // (opt) If true, missing key is ok
  Int_t        search   {0};         // (opt) Search for key along name tree
  const char*  descript {nullptr};   // (opt) Key description (if 0, same as name)
};

#endif
