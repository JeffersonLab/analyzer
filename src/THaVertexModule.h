#ifndef ROOT_THaVertexModule
#define ROOT_THaVertexModule

//////////////////////////////////////////////////////////////////////////
//
// THaVertexModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"

class TVector3;

class THaVertexModule : public THaPhysicsModule {
  
public:
  virtual ~THaVertexModule();
  
  virtual const TVector3& GetVertex() const = 0;

protected:

  THaVertexModule( const char* name, const char* description );

  ClassDef(THaVertexModule,0)   //ABC for a vertex module

};

#endif
