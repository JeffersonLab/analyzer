#ifndef ROOT_THaVertexModule
#define ROOT_THaVertexModule

//////////////////////////////////////////////////////////////////////////
//
// THaVertexModule
//
//////////////////////////////////////////////////////////////////////////

#include "TVector3.h"

class THaVertexModule {
  
public:
  virtual ~THaVertexModule();
  
  const TVector3&   GetVertex()  const { return fVertex; }

protected:

  TVector3  fVertex;   // Vertex position (m) 

  THaVertexModule();

  ClassDef(THaVertexModule,0)   //ABC for a vertex module

};

#endif
