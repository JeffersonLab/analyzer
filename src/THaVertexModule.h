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

  //Only derived classes may construct me
  THaVertexModule() {}
  THaVertexModule( const char* name, const char* description );
  THaVertexModule( const THaVertexModule& ) {}
  THaVertexModule& operator=( const THaVertexModule& ) { return *this; }

  ClassDef(THaVertexModule,0)   //ABC for a vertex module

};

#endif
