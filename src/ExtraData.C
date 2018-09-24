/////////////////////////////////////////////////////////////////////////////
//
// Podd::ExtraData
//
// Base class for class-specific extra data members, to be assigned to
// fExtra pointer. Used to maintain binary compatibility between patch
// versions.
//
/////////////////////////////////////////////////////////////////////////////

#include "ExtraData.h"
#include "TNamed.h"
#include <cassert>
#include <iostream>

using namespace std;

namespace Podd {

//___________________________________________________________________________
ExtraData::ExtraData( TObject* obj, TObject* prev )
  : fOwner(obj)
{
  // Constructor

  assert(fOwner);
  assert( prev == 0 || dynamic_cast<ExtraData*>(prev) );
  fNext = static_cast<ExtraData*>(prev);
}

//___________________________________________________________________________
ExtraData::~ExtraData()
{
  // Destructor. Deletes the entire linked list of ExtraData objects

  // NB: this will delete all nodes in turn until fNext == 0
  delete fNext; fNext = 0;
}

//___________________________________________________________________________
ExtraData* ExtraData::Find( TObject* obj ) const
{
  // Get the actual ExtraData object for the owner 'obj'

  assert(obj);

  ExtraData* pextra = const_cast<ExtraData*>(this);
  while( pextra && obj != pextra->fOwner ) {
    pextra = pextra->fNext;
  }
  return pextra;
}

//___________________________________________________________________________
void ExtraData::Print( Option_t* ) const
{
  TNamed* named = dynamic_cast<TNamed*>(fOwner);
  if( named ) {
    cout << "Extra data for object \""
         << named->GetName() << "\" (\""
         << named->GetTitle() << "\"):" << endl;
  }
}

//===========================================================================

} // namespace Podd
