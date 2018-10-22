#ifndef Podd_THaVertexModule_h_
#define Podd_THaVertexModule_h_

//////////////////////////////////////////////////////////////////////////
//
// THaVertexModule
//
//////////////////////////////////////////////////////////////////////////

#include "TVector3.h"

struct RVarDef;

class THaVertexModule {
  
public:
  virtual ~THaVertexModule();
  
  virtual const TVector3&   GetVertex()      const { return fVertex; }
  virtual const TVector3&   GetVertexError() const { return fVertexError; }
  virtual       Bool_t      HasVertex()      const { return fVertexOK; }
  virtual       void        VertexClear();
  static  const RVarDef*    GetRVarDef();

protected:

  TVector3  fVertex;      // Vertex position (m) 
  TVector3  fVertexError; // Uncertainties in fVertex coordinates
  Bool_t    fVertexOK;

  THaVertexModule();

  ClassDef(THaVertexModule,0)   //ABC for a vertex module

};

#endif
