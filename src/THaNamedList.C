//*-- Author :    Ole Hansen    08/05/2000


//////////////////////////////////////////////////////////////////////////
//
// THaNamedList
//
// A list with a name. Used to store blocks of THaCuts (tests) 
// in a THashList, where the name of this list is the hash index.
//
//////////////////////////////////////////////////////////////////////////

#include "THaNamedList.h"
#include "TNamed.h"
#include "TClass.h"
#include <iostream>
#include <cstring>

using namespace std;


//_____________________________________________________________________________
THaNamedList::THaNamedList()
{
  // THaNamedList default constructor

  fNamed = new TNamed();
}

//_____________________________________________________________________________
THaNamedList::THaNamedList( const char* name )
{
  // Normal THaNamedList constructor with single argument

  fNamed = new TNamed( name, name );
}

//_____________________________________________________________________________
THaNamedList::THaNamedList( const char* name, const char* title )
{
  // Normal THaNamedList constructor with name and title

  fNamed = new TNamed( name, title );
}

//_____________________________________________________________________________
THaNamedList::~THaNamedList()
{
  // THaNamed list destructor

  delete fNamed;
}

//_____________________________________________________________________________
void THaNamedList::PrintOpt( Option_t* opt ) const
{
  // Print all objects in the list. Pass option 'opt' through to each object
  // being printed. (This is the old ROOT 2,x behavior).

  TIter next(this);
  TObject* object;
  while((object = next()))
    object->Print(opt);
}

//_____________________________________________________________________________
void THaNamedList::ls( Option_t* opt ) const
{
   // List THaNamedList name and title.

  //  IndentLevel();
  cout <<"OBJ: " << fNamed->IsA()->GetName() << "\t" << GetName() << "\t" << GetTitle() << " : "
       << Int_t(TestBit(kCanDelete)) << endl;
  TList::ls( opt );
}

//_____________________________________________________________________________
void THaNamedList::SetName( const Text_t* name )
{
  // Set the name of the object. 
  // Only set name if it is not set already.
  // This avoids problems with hash tables.
  // Should probably be changed to provide a means of rehashing.

  if( !*fNamed->GetName() )
    fNamed->SetName( name );
}

//_____________________________________________________________________________
void THaNamedList::SetNameTitle( const Text_t* name, const Text_t* title )
{
  // Set name and title of the object, but only if the name is not yet 
  // initialized

  if( !*fNamed->GetName() )
    fNamed->SetNameTitle( name, title );
}

//_____________________________________________________________________________
ClassImp(THaNamedList)
