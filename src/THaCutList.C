//*-- Author :    Ole Hansen     05-May-00


//////////////////////////////////////////////////////////////////////////
//
// THaCutList
//
// Class to manage dynamically-defined cuts (tests).
//
//////////////////////////////////////////////////////////////////////////

#include "THaCut.h"
#include "THaNamedList.h"
#include "THaCutList.h"
#include "THaPrintOption.h"
#include "TError.h"
#include "TList.h"
#include "TString.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cctype>

using namespace std;

const char* const THaCutList::kDefaultBlockName = "Default";
const char* const THaCutList::kDefaultCutFile   = "default.cuts";

//_____________________________________________________________________________
void THaHashList::PrintOpt( Option_t* opt ) const
{
  // Print all objects in the list. Pass option 'opt' through to each object
  // being printed. (This is the old ROOT 2,x behavior).

  TIter next(this);
  TObject* object;
  while((object = next()))
    object->Print(opt);
}

//______________________________________________________________________________
THaCutList::THaCutList() : fVarList(NULL)
{
  // Default constructor. No variable list is defined. Either define it
  // later with SetList() or pass the list as an argument to Define().
  // Allowing this constructor is not very safe ...

  fCuts   = new THaHashList();
  fBlocks = new THaHashList();
}

//______________________________________________________________________________
THaCutList::THaCutList( const THaVarList* lst ) : fVarList(lst)
{
  // Normal constructor. Create the main lists and set the variable list.

  fCuts   = new THaHashList();
  fBlocks = new THaHashList();
}

//______________________________________________________________________________
THaCutList::~THaCutList()
{
  // Default destructor. Deletes all cuts and lists.

  fCuts->Clear();
  fBlocks->Delete();
  delete fCuts;
  delete fBlocks;
}

//______________________________________________________________________________
void THaCutList::Clear( Option_t* option )
{
  // Remove all cuts and all blocks

  fBlocks->Delete();
  fCuts->Delete();
}

//______________________________________________________________________________
void THaCutList::ClearAll( Option_t* opt )
{
  // Clear the results of all defined cuts

  TIter next( fCuts );
  while( THaCut* pcut = static_cast<THaCut*>( next() ))
    pcut->ClearResult();
}

//______________________________________________________________________________
void THaCutList::ClearBlock( const char* block, Option_t* opt )
{
  // Clear the results of the defined cuts in the named block

  TIter next( FindBlock(block) );
  while( THaCut* pcut = static_cast<THaCut*>( next() ))
    pcut->ClearResult();
}

//______________________________________________________________________________
void THaCutList::Compile()
{
  // Compile all cuts in the list.
  // Since cuts are compiled when we Define() them, this routine typically only 
  // needs to be called when global variable pointers need to be updated.

  TList* bad_cuts = NULL;
  bool have_bad = false;

  TIter next( fCuts );
  while( THaCut* pcut = static_cast<THaCut*>( next() )) {
    pcut->Compile();
    if( pcut->IsError() ) { 
      Error( "Compile", "expression error, cut removed: %s %s block: %s",
	     pcut->GetName(), pcut->GetTitle(), pcut->GetBlockname() );
      if( !bad_cuts ) bad_cuts = new TList;
      bad_cuts->Add( new TNamed( *pcut ));
      have_bad = true;
    }
  }
  if( have_bad ) {
    TIter next_bad( bad_cuts );
    while( TNamed* pbad = static_cast<TNamed*>( next_bad() )) {
      Remove( pbad->GetName() );
    }
    bad_cuts->Delete();
  }
  delete bad_cuts;
}

//______________________________________________________________________________
Int_t THaCutList::Define( const char* cutname, const char* expr, 
			  const char* block )
{
  // Define a new cut with given name in given block. If the block does not
  // exist, it is created.  This is the normal way to define a cut since
  // there is usually just one variable list. 
  //
  // Cut names MUST be unique and cannot be empty. Quotes and whitespace are
  // illegal in cut names; any other characters are allowed.
  //
  // Symbolic variables in expressions must only refer to variables defined 
  // in the variable list OR to previously defined cuts.
  //
  // It is possible to use cut names from one block in definitions for a 
  // different block. This should be done with care, however, since such
  // "external" cuts are not necessarily updated with the same frequency
  // as the current block, depending on the design of the application.
  //
  // A return value of 0 indicates success, negative numbers indicate error.
  //

  if( !fVarList ) { 
    Error( "Define", "no variable list set, cut not created" );
    return -1;
  }
  return Define( cutname, expr, fVarList, block );
}

//______________________________________________________________________________
Int_t THaCutList::Define( const char* cutname, const char* expr, 
			  const THaVarList* lst, const char* block )
{
  // Define a new cut with given name in given block with variables from
  // given list.  See description of Define() above.
  //

  static const char* here = "THaCutList::Define";

  if( !cutname || !*cutname || (strspn(cutname," ")==strlen(cutname)) ) {
    Error( here, "empty cut name, cut not created" );
    return -4;
  }
  if( !expr || !*expr || (strspn(expr," ")==strlen(expr)) ) {
    Error( here, "empty expression string, cut not created: %s", cutname );
    return -5;
  }
  if( !block || !*block || (strspn(block," ")==strlen(block)) ) {
    Error( here, "empty block name, cut not created: %s %s", cutname, expr );
    return -6;
  }
  if( strpbrk(cutname,"\"'` ") ) {
    Error( here, "illegal character(s) in cut name, cut not created: "
	   "%s %s block: %s", cutname, expr, block );
    return -7;
  }
  if( fCuts->FindObject(cutname) ) {
    Error( here, "duplicate cut name, cut not created: %s %s block: %s",
	   cutname, expr, block );
    return -8;
  }

  // Create new cut using the given expression. Bail out if error.

  THaCut* pcut = new THaCut( cutname, expr, block, lst, this );
  if( !pcut ) return -2;
  if( pcut->IsError() ) { 
    Error( here, "expression error, cut not created: %s %s block: %s",
	   cutname, expr, block );
    delete pcut; 
    return -3;
  }

  // Formula ok -> add it to the lists. If this is a new block, create it.

  THaNamedList* plist = FindBlock( block );
  if( !plist ) {
    plist = new THaNamedList( block );
    fBlocks->Add( plist );
  }

  fCuts->AddLast( pcut );
  plist->AddLast( pcut );
  return 0;
}

//______________________________________________________________________________
Int_t THaCutList::Eval()
{
  // Evaluate all tests in all blocks.  Because of possible dependences between
  // blocks, each block is evaluated separately in the order in which the blocks
  // were defined.

  Int_t i = 0;
  TIter next( fBlocks );
  while( THaNamedList* plist = static_cast<THaNamedList*>( next() ))
    i += EvalBlock( plist );

  return i;
}

//______________________________________________________________________________
Int_t THaCutList::EvalBlock( const TList* plist )
{
  // Evaluate all cuts in the given list in the order in which they were defined.

  if( !plist ) return -1;
  Int_t i = 0;
  TIter next( plist );
  while( THaCut* pcut = static_cast<THaCut*>( next() )) {
    pcut->EvalCut();
    i++;
  }
  return i;
}

//______________________________________________________________________________
Int_t THaCutList::EvalBlock( const char* block )
{
  // Evaluate all tests in the given block in the order in which they were defined.
  // If no argument is given, the default block is evaluated.

  return EvalBlock( FindBlock( block ) );
}

//______________________________________________________________________________
static inline bool IsEnd( const char* s )
{
  // True if s points to a comment ("#" or "//") or zero.
  // Utility function used by Load().

  return ( !*s || *s == '#' || (*s == '/' && *(s+1) == '/' ));
}

//______________________________________________________________________________
Int_t THaCutList::Load( const char* filename )
{
  // Read cut definitions from a file and create the cuts. If no filename is
  // given, the file ./cutdef.dat is used.
  //
  // Lines starting with # or "//" are treated as comments. 
  // Blank lines and leading spaces are ignored.
  //
  // A valid cut definition consists of two fields, a cut name and the 
  // corresponding expression.  The fields are separated by whitespace.  
  // Comments following the cut expression are ignored.
  //
  // Block names can be set using "Block:" as the cut name (without quotes)
  // followed by the name of the block.
  //
  // Cuts are defined via the Define() method; see the description of 
  // Define() for more details.  Examples:
  //
  // # This is a comment. Blank lines are ignored, so are leading spaces.
  //
  // xcut  x>1
  // cut1  x+y<10
  //
  // Block: Target_cuts
  // cut2  x<10 || x>20   // Comment goes here
  // zcut  (z^2-2)>0      # This text is ignored
  //
  // A return value of 0 indicates success. Negative values indicate severe 
  // errors (e.g. file not found, read error).  Positive numbers indicate the 
  // number of unprocessable lines (e.g. illegal expression, bad cutname) for 
  // which no cuts were defined.
  //

  static const char* const here   = "THaCutList::Load";
  static const char* const whtspc = " \t";

  if( !filename || !*filename || strspn(filename," ") == strlen(filename) ) {
    Error( here, "invalid file name, no cuts loaded" );
    return -1;
  }

  ifstream ifile( filename );
  if( !ifile ) {
    Error( here, "error opening input file %s, no cuts loaded",
	   filename );
    return -2;
  }

  // Read the file line by line

  const unsigned LEN = 256;
  unsigned line_size = LEN;
  char* line  = new char[ line_size ];
  char block[ LEN ];
  strcpy( block, kDefaultBlockName );
  Int_t nlines_read = 0, nlines_ok = 0;

  while( !ifile.eof() ) {
    streampos pos = ifile.tellg();
    ifile.getline( line, line_size );

    // Buffer too small?

    if( (unsigned)ifile.gcount() == line_size-1 ) {
      ifile.clear();
      ifile.seekg( pos );
      delete [] line;
      line_size += LEN;
      line = new char[ line_size ];
      continue;
    }

    // Blank line or comment?

    size_t i = strspn( line, whtspc );
    if( IsEnd(line+i) ) continue;

    // Valid line ... start processing

    nlines_read++;

    // Extract first argument (cut name or "Block:")
    const char* arg1 = line+i, *arg2 = 0;
    while( line[i] && !isspace(line[i]) ) i++;
    if( line[i] == '\0' ) goto noexpr;
    line[i++] = '\0';

    // Extract second argument (expression or block name)
    while( isspace(line[i])) i++;
    if( IsEnd(line+i) ) goto noexpr;
    arg2 = line+i;

    i++;
    while( !IsEnd(line+i) ) i++;
    while( isspace(line[i-1]) ) i--;
    line[i] = '\0';

    // Set block name

    if( !strcmp( arg1, "Block:" ) ) {
      if( strlen(arg2) >= LEN ) {
	Error( here, "block name too long, aborting: %s", arg2 );
	return -3;
      }
      strcpy( block, arg2 );
      nlines_ok++;
      continue;
    }

    // Define the cut. Errors are reported by Define()

    if( strlen(arg1) >= LEN ) {
      Error( here, "cut name too long, cut not loaded: %s %s", 
	     arg1, arg2 );
      continue;
    }

    if( !Define( arg1, arg2, block ) ) nlines_ok++;
    continue;

  noexpr:    
    Warning( here, "ignoring label without expression, %s", arg1 );
    continue;

  }

  ifile.close();
  delete [] line;
  Int_t nbad = nlines_read-nlines_ok;
  if( nbad>0 ) Warning( here, "%d cut(s) could not be defined, check input "
			"file %s", nbad, filename );
  return nbad;
}

//______________________________________________________________________________
void THaCutList::MakePrintOption( THaPrintOption& opt, const TList* plist )
{ 
  // If the print option is either "LINE" or "STATS", determine the widths 
  // of the text fields of all the cuts in the given list and append them
  // to the print option.
  // This is an internal utility function used by Print() and PrintBlock().

  const int NF = 4;         // Number of fields
  if( opt.IsLine() && !strcmp(opt.GetOption(1),"") ) {
    UInt_t width[NF] = { 0, 0, 0, 0 };
    TIter next( plist );
    while( THaCut* pcut = (THaCut*)next() ) {
      width[0] = max( width[0], static_cast<UInt_t>
		      (strlen(pcut->GetName())) );
      width[1] = max( width[1], static_cast<UInt_t>
		      (strlen(pcut->GetTitle())) );
      width[2] = max( width[2], static_cast<UInt_t>
		      (strlen(pcut->GetBlockname())) );
      width[3] = max( width[3], IntDigits( static_cast<Int_t>
					   (pcut->GetNPassed()) ));
    }
    TString newopt = opt.GetOption(0);
    for( int i=0; i<NF; i++ ) {
      newopt += ",";
      newopt += width[i];
    }
    opt = newopt;
  }
}

//______________________________________________________________________________
void THaCutList::Print( Option_t* option ) const
{
  // Print all cuts in all blocks.
  // For options see THaCut.Print(). The default mode is "LINE". The 
  // widths of the text fields are determined automatically.
  // For "LINE" and "STATS", cuts are printed grouped in blocks.

  THaPrintOption opt(option);
  if( !strcmp(opt.GetOption(0),"") ) opt = kPRINTLINE;
  MakePrintOption( opt, fCuts );
  PrintHeader( opt );
  if( opt.IsLine() ) {
    TIter next( fBlocks );
    while( THaNamedList* plist = static_cast<THaNamedList*>( next() )) {
      bool is_stats = !strcmp( opt.GetOption(0), kPRINTSTATS );
      if(  is_stats && strlen( plist->GetName() ) )
	cout << "BLOCK: " << plist->GetName() << endl;
      plist->PrintOpt( opt.Data() );
      if ( is_stats ) cout << endl;
    }
  } else
    fCuts->PrintOpt( opt.Data() );
}

//______________________________________________________________________________
void THaCutList::PrintBlock( const char* block, Option_t* option ) const
{
  // Print all cuts in the named block.

  THaNamedList* plist = FindBlock( block );
  if( !plist ) return;
  THaPrintOption opt(option);
  if( !strcmp(opt.GetOption(0),"") ) opt = kPRINTLINE;
  MakePrintOption( opt, plist );
  PrintHeader( opt );
  plist->PrintOpt( opt.Data() );
}

//______________________________________________________________________________
void THaCutList::PrintCut( const char* cutname, Option_t* option ) const
{
  // Print the definition of a single cut

  THaCut* pcut = static_cast<THaCut*>( fCuts->FindObject( cutname ));
  if( !pcut ) return;
  pcut->Print( option );
}

//______________________________________________________________________________
void THaCutList::PrintHeader( const THaPrintOption& opt ) const
{
  // Print header for Print() and PrintBlock().
  // This is an internal function.

  if( !opt.IsLine() ) return;
  cout.flags( ios::left );
  cout << setw( opt.GetValue(1) ) << "Name" << "  "
       << setw( opt.GetValue(2) ) << "Def"  << "  ";
  if( !strcmp( opt.GetOption(), kPRINTLINE )) {
    cout << setw(1) << "T" << "  "
	 << setw( opt.GetValue(3) ) << "Block" << "  ";
  }
  cout << setw(9) << "Called" << "  "
                  << "Passed" << endl;
  int len = max( opt.GetValue(1) + opt.GetValue(2) 
		 + opt.GetValue(4) + 24, 30 );
  if( !strcmp( opt.GetOption(), kPRINTLINE ))
    len += max( opt.GetValue(3) + 5, 10 );
  char* line = new char[ len+1 ];
  for( int i=0; i<len; i++ ) line[i] = '-';
  line[len] = '\0';
  cout << line << endl;
  delete [] line;
}

//______________________________________________________________________________
void THaCutList::Reset()
{
  // Reset all cut and block counters to zero

  TIter next( fCuts );
  while( THaCut* pcut = static_cast<THaCut*>( next() ))
    pcut->Reset();
}

//______________________________________________________________________________
Int_t THaCutList::Result( const char* cutname, EWarnMode mode )
{
  // Return result of the last evaluation of the named cut
  // (0 if false, 1 if true).
  // If cut does not exist, return -1. Also, print warning if mode=kWarn.

  THaCut* pcut = static_cast<THaCut*>( fCuts->FindObject( cutname ));
  if( !pcut ) {
    if( mode == kWarn )
      Warning("Result", "No such cut: %s", cutname );
    return -1;
  }
  return static_cast<Int_t>( pcut->GetResult() );
}

//______________________________________________________________________________
Int_t THaCutList::Remove( const char* cutname )
{
  // Remove the named cut completely

  THaCut* pcut = static_cast<THaCut*>( fCuts->FindObject( cutname ));
  if ( !pcut ) return 0;
  const char* block = pcut->GetBlockname();
  THaNamedList* plist = static_cast<THaNamedList*>(fBlocks->FindObject( block ));
  if ( plist ) plist->Remove( pcut );
  fCuts->Remove( pcut );
  delete pcut;
  return 1;
}

//______________________________________________________________________________
Int_t THaCutList::RemoveBlock( const char* block )
{
  // Remove all cuts contained in the named block.

  THaNamedList* plist = FindBlock( block );
  if( !plist ) return -1;
  Int_t i = 0;
  TObjLink* lnk = plist->FirstLink();
  while( lnk ) {
    THaCut* pcut = static_cast<THaCut*>( lnk->GetObject() );
    if( pcut ) i++;
    lnk = lnk->Next();
    fCuts->Remove( pcut );
  }
  plist->Delete();   // this should delete all pcuts
  fBlocks->Remove( plist );
  delete plist;

  return i;
}

//______________________________________________________________________________
void THaCutList::SetList( THaVarList* lst )
{
  // Set the pointer to the list of global variables to be used by default.
  // Other variable lists can be used for individual cut definitions.

  fVarList = lst;
}

//______________________________________________________________________________
UInt_t IntDigits( Int_t n )
{ 
  //Get number of printable digits of integer n.
  //Global utility function.

  if( n == 0 ) return 1;
  int j = 0;
  if( n<0 ) {
    j++;
    n *= -1;
  }
  while( n>0 ) {
    n /= 10;
    j++;
  }
  return j;
}

//______________________________________________________________________________
ClassImp(THaCutList)
ClassImp(THaHashList)
