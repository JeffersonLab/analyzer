//*-- Author :    Ole Hansen   03/05/2000


//////////////////////////////////////////////////////////////////////////
//
// THaCut -- Hall A analyzer cut (a.k.a. test)
// This is a slightly expanded version of a THaFormula
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "THaCut.h"
#include "THaPrintOption.h"


ClassImp(THaCut);

//_____________________________________________________________________________
void THaCut::Print( Option_t* option ) const
{
  // Print this cut.
  // Options:
  //   "BRIEF" -- short (default)
  //   "FULL"  -- extended
  //   "LINE"  -- one-line (for listings)
  //   "STATS" -- one-line (listing of statistics only)

  THaPrintOption s(option);
  Int_t nn = max( s.GetValue(1), (Int_t)strlen(GetName()) );
  Int_t nt = max( s.GetValue(2), (Int_t)strlen(GetTitle()) );
  Int_t nb = max( s.GetValue(3), fBlockname.Length() );
  Int_t np = max( s.GetValue(4), (Int_t)strlen(s.GetOption(4)) );
    
  // Print data according to the requested format

  if ( s.IsLine() ) {

    cout.flags( ios::left );
    cout << setw(nn) << GetName() << "  " 
	 << setw(nt) << GetTitle() << "  ";
    if( !strcmp( s.GetOption(), kPRINTLINE )) {
      cout << setw(1)  << (bool)fLastResult << "  "
	   << setw(nb) << fBlockname << "  ";
    }
    cout << setw(9)  << fNCalled << "  "
	 << setw(np) << fNPassed << " ";
    cout << setprecision(3);
    if( fNCalled > 0 )
      cout << "(" << 100.0*((float)fNPassed)/((float)fNCalled) << "%)";
    else
      cout << "(0.0%)";

    cout << endl;

  } else {

    cout.flags( ios::right );
    THaFormula::Print( s.GetOption() );
    cout << "Curval: " << setw(9) << (bool)fLastResult << "  "
	 << "Block:  " << fBlockname << endl;
    cout << "Called: " << setw(9) << fNCalled << "  "
	 << "Passed: " << setw(9) << fNPassed;
    if( fNCalled > 0 )
      cout << setprecision(3)
	   <<"  (" << 100.0*((float)fNPassed)/((float)fNCalled) << "%)\n";
    else
      cout << "  (0.00%)\n";

  }
}

//_____________________________________________________________________________
void THaCut::SetName( const Text_t* name )
{
  // Set name of the cut
  // This method overrides the one provided by TNamed
  // so that the name can only be set if it is empty.
  // This avoids problems with hash values since THaCuts are
  // elements of a THashList.
  // Alternatively, we could rehash the associated table(s), but that 
  // would be much more involved.

  if( fName.Length() == 0 )
    TNamed::SetName( name );
}

//_____________________________________________________________________________
void THaCut::SetNameTitle( const Text_t* name, const Text_t* formula )
{
  // Set name and formula of the object, only if not yet initialized

  if( fName.Length() == 0 )
    TNamed::SetNameTitle( name, formula );
}

