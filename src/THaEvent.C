//*-- Author :    Ole Hansen     17/05/2000

////////////////////////////////////////////////////////////////////////
//
// THaEvent
//
// This is the base class for "event" objects, which define the 
// structure of the event data to be written to the output DST 
// via ROOT object I/O.  
//
// Any derived class must implement at least the following:
// 
//  - Define all member variables that are to appear in the output
//    for each event (a la ntuple).
//  - Set up the fDataMap array to link these member variables to 
//    global analyzer variables.
//
// Data members can be simple types (Double_t etc.) and any ROOT objects
// capable of object I/O (i.e. with a valid Streamer() class).
// For any data members other than simple types defined in fDataMap,
// the Fill() method has to be extended accordingly.
//
// There is support for simple 1-d histograms that are filled
// at the end of processing for each event.
// Derived classes may add such histograms by extending/replacing
// the contents of the fHistDef array.
//
// This class will be obsoleted in v0.80.
//
////////////////////////////////////////////////////////////////////////

#include "THaEvent.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TROOT.h"
#include "TH1.h"
#include "TMath.h"
#include "THaArrayString.h"

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <strstream.h>

ClassImp(THaEventHeader)
ClassImp(THaEvent)

const char* const THaEvent::kDefaultHistFile = "histdef.dat";

//______________________________________________________________________________
THaEvent::THaEvent() : 
  fInit(kFALSE), fDataMap(NULL), fHistDef(NULL)
{
  // Create a THaEvent object.
}

//______________________________________________________________________________
THaEvent::~THaEvent()
{
  // Destructor. Clean up all my objects.

  // Delete all histograms that still actually exist

  if( HistDef* hdef = fHistDef ) {
    while( hdef->name ) {
      if( gROOT->FindObject( hdef->hname )) {
	delete hdef->h1;
      }
      hdef->h1 = NULL;
      delete [] hdef->name;  hdef->name  = NULL;
      delete [] hdef->hname; hdef->hname = NULL;
      delete [] hdef->title; hdef->title = NULL;
      delete hdef->parsed_name; hdef->parsed_name = NULL;
      hdef++;
    }
  }
  delete [] fHistDef; fHistDef = NULL;
  delete [] fDataMap; fDataMap = NULL;
}

//______________________________________________________________________________
void THaEvent::Clear( Option_t* opt )
{
  // Reset all histograms

  if( HistDef* hdef = fHistDef ) {
    while( hdef->name ) {
      if( hdef->h1 )
	hdef->h1->Reset( opt );
      hdef++;
    }
  }
}

//______________________________________________________________________________
Int_t THaEvent::DefineHist( const char* varname, 
			    const char* histname, const char* title,
			    Int_t nbins, Axis_t xlo, Axis_t xhi )
{
  // Define 1-d histogram with the specified parameters on the global
  // variable 'varname'.

  //FIXME: This is ugly and inefficient.

  if( !varname || !histname || !title || nbins <= 0 ) {
    Error("DefineHist", "Invalid parameter(s). Histogram not defined.");
    return -1;
  }

  HistDef hdef;
  hdef.name  = new char[ strlen(varname)+1 ];
  hdef.hname = new char[ strlen(histname)+1 ];
  hdef.title = new char[ strlen(title)+1 ];
  strcpy( const_cast<char*>(hdef.name),  varname );
  strcpy( const_cast<char*>(hdef.hname), histname );
  strcpy( const_cast<char*>(hdef.title), title );
  hdef.nbins = nbins;
  hdef.xlo   = xlo;
  hdef.xhi   = xhi;
  hdef.h1    = NULL;
  hdef.pvar  = NULL;
  hdef.type  = 0;
  hdef.parsed_name = new THaArrayString;

  HistDef* h = fHistDef;
  Int_t n = 0;
  if( h )
    while( (h++)->name )
      n++;

  h = new HistDef[ n+2 ];
  if( n )
    memcpy( h, fHistDef, n*sizeof(HistDef) );
  memcpy( h+n, &hdef, sizeof(HistDef) );
  memset( h+n+1, 0, sizeof(HistDef) );

  delete [] fHistDef;
  fHistDef = h;

  return 0;
}

//______________________________________________________________________________
Int_t THaEvent::HistFill()
{
  // Fill the defined histograms with current event data.

  Int_t ndat = 0;

  if( HistDef* hdef = fHistDef ) {
    while( hdef->name ) {
      if( hdef->h1 && hdef->pvar ) {

	if( !hdef->pvar->IsArray()) {
	  hdef->h1->Fill( hdef->pvar->GetValue());

	} else {
	  if( !hdef->parsed_name->IsArray() ) {
	    // Loop over all array indices if full array chosen
	      for( int i=0; i<hdef->pvar->GetLen(); i++ )
		hdef->h1->Fill( hdef->pvar->GetValue(i) );
	  } else {
	    // Fill histogram with the selected array element
	    Int_t index = hdef->pvar->Index( *hdef->parsed_name ); 
	    if( index < hdef->pvar->GetLen() )
	      hdef->h1->Fill( hdef->pvar->GetValue( index ));
	  }
	}
	ndat++;
      }
      hdef++;
    }
  }
  return ndat;
}

//______________________________________________________________________________
Int_t THaEvent::HistInit()
{
  // Set up histograms. 

  static const char* const here = "HistInit()";

  if( !gHaVars ) return -2;

  if( HistDef* hdef = fHistDef ) {
    while( hdef->name ) {

      // Create new histograms if necessary
      THaVar* pvar;
      if( !hdef->h1 && (pvar = gHaVars->Find( hdef->name )) ) {
	// Save the pointer to the global variable so we needn't
	// Find() it again later
	hdef->pvar = pvar;

	// Deal with arrays

	*hdef->parsed_name = hdef->name;
	if( hdef->parsed_name->IsError() ) {
	  Error( here, "Error parsing variable name %s. "
		 "No histogram created.", hdef->name );
	  continue;
	}
	if( hdef->parsed_name->IsArray() && !hdef->pvar->IsArray() ) {
	  Error( here, "Global variable %s is not an array, but "
		 "array element requested. No histogram created.",
		 hdef->parsed_name->GetName() );
	  continue;
	}
	if( hdef->parsed_name->IsArray() 
	    && (hdef->parsed_name->GetNdim() != hdef->pvar->GetNdim()) ) {
	  Error( here, "Number of array dimensions does not match for "
		 "variable %s. No histogram created.", hdef->name );
	  continue;
	}
	
	// Create new histogram capable of holding the data type of
	// the corresponding global variable

	switch( pvar->GetType() ) {
	case kDouble: case kDoubleP:  case kDouble2P:
	  hdef->h1 = new TH1D( hdef->hname, hdef->title, 
			       hdef->nbins, hdef->xlo, hdef->xhi );
	  hdef->type = 3;
	  break;
	
	case kFloat:  case kFloatP:   case kFloat2P:
	case kLong:   case kLongP:    case kLong2P:
	case kULong:  case kULongP:   case kULong2P:
	case kInt:    case kIntP:     case kInt2P:
	case kUInt:   case kUIntP:    case kUInt2P:
	case kUShort: case kUShortP:  case kUShort2P:
	  hdef->h1 = new TH1F( hdef->hname, hdef->title, 
			       hdef->nbins, hdef->xlo, hdef->xhi );
	  hdef->type = 2;
	  break;
	
	case kShort: case kShortP:    case kShort2P:
	case kByte:  case kByteP:     case kByte2P:
	  hdef->h1 = new TH1S( hdef->hname, hdef->title, 
			       hdef->nbins, hdef->xlo, hdef->xhi );
	  hdef->type = 1;
	  break;

	case kChar:  case kCharP:     case kChar2P:
	  hdef->h1 = new TH1C( hdef->hname, hdef->title, 
			       hdef->nbins, hdef->xlo, hdef->xhi );
	  hdef->type = 0;
	  break;

	default:
	  Error( here, "Unknown type for global variable %s: %d "
		 "No histogram created.", hdef->name, pvar->GetType() );
	  hdef->pvar = NULL;
	  break;
	}
      } else {
	Warning( here, "Global variable %s not found. "
		 "No histogram created.", hdef->name );
	hdef->pvar = NULL;
      }

      hdef++;
    }
  }
  return 0;
}

//______________________________________________________________________________
Int_t THaEvent::Fill()
{
  // Copy global variables specified in the data map to the event structure.

  static const char* const here = "Fill()";

  // Initialize datamap if not yet done
  if( !fInit ) {
    Int_t status = Init();
    if( status )
      return status;
  }

  // Move the data into the members variables of this Event object
  //
  // For basic types, this can be reduced to a memcpy. Knowledge of the
  // type of the destination is not necessary, BUT the user needs to ensure
  // the destination type matches the source type -> potential for error!
  //
  // For object types, this is no longer that easy (without rewriting the
  // variable system); we really have to convert the data retrieved via
  // GetData() to the type of the destination.
  //
  // These complications will go away completely once this class is rewritten
  // with an intelligent streamer.
  //

  Int_t nvar = 0;
  Int_t ncopy;
  THaVar* pvar;
  if( DataMap* datamap = fDataMap ) {
    while( datamap->ncopy ) {
      if( (pvar = datamap->pvar) ) {
	if( datamap->ncopy>0 )
	  ncopy = datamap->ncopy;
	else if( datamap->ncopyvar )
	  ncopy = *datamap->ncopyvar;
	else {
	  ncopy = pvar->GetLen();
	  if( ncopy == THaVar::kInvalidInt ) 
	    continue;
	}
	Int_t       type = pvar->GetType();
	size_t      size = pvar->GetTypeSize();
	const void* src  = pvar->GetValuePointer();
	if( !src ) continue;
	if( pvar->IsBasic() ) {
	  if( type >= kDoubleP ) {
	    // Pointers to pointers - get the pointer they currently point to
	    void** loc = (void**)src;
	           src = (const void*)(*loc);
	  }
	  if( !src ) continue;
	  if( !pvar->IsPointerArray() )
	    memcpy( datamap->dest, src, ncopy*size );
	  else {
	    // For pointer arrays, we need to copy the elements one by one
	    // Type doesn't matter for memcpy, but size does ;)
	    for( int i=0; i<ncopy; i++ ) {
	      const int** psrc = (const int**)src;
	      int*        dest = static_cast<int*>( datamap->dest );
	      memcpy( dest+i, *(psrc+i), size );
	    }
	  }
	} else {
	  // Object variables: Now we have to worry about the type of
	  // the destination. We assume it is the same type as the
	  // source. Ugly and inefficient, but good enough for now I guess.
	  // Note: Function return values are either Long_t or Double_t,
	  // regardless of the actual return value of the function.
	  // This is a ROOT limitation.
	  for( int i=0, j=0; i<ncopy; i++, j++ ) {
	    Double_t val = pvar->GetValue(i);
	    if( val == THaVar::kInvalid )
	      continue;
	    switch( type ) {
	    case kDouble: case kDoubleP:
	      *(((Double_t*)datamap->dest)+j) = val;
	      break;
	    case kFloat:  case kFloatP:
	      *(((Float_t*)datamap->dest)+j)  = static_cast<Float_t>(val);
	      break;
	    case kInt:    case kIntP:
	      *(((Int_t*)datamap->dest)+j)    = static_cast<Int_t>(val);
	      break;
	    case kUInt:   case kUIntP:
	      *(((UInt_t*)datamap->dest)+j)   = static_cast<UInt_t>(val);
	      break;
	    case kShort:  case kShortP:
	      *(((Short_t*)datamap->dest)+j)  = static_cast<Short_t>(val);
	      break;
	    case kUShort: case kUShortP:
	      *(((UShort_t*)datamap->dest)+j) = static_cast<UShort_t>(val);
	      break;
	    case kLong:   case kLongP:
	      *(((Long_t*)datamap->dest)+j)   = static_cast<Long_t>(val);
	      break;
	    case kULong:  case kULongP:
	      *(((ULong_t*)datamap->dest)+j)  = static_cast<ULong_t>(val);
	      break;
	    case kChar:   case kCharP:
	      *(((Char_t*)datamap->dest)+j)   = static_cast<Char_t>(val);
	      break;
	    case kByte:   case kByteP:
	      *(((Byte_t*)datamap->dest)+j)   = static_cast<Byte_t>(val);
	      break;
	    default:
	      Warning( here, "Unknown type for variable %s "
		 "Not filled.", pvar->GetName() );
	      j--; nvar--;
	      break;
	    }
	  }
	}
	nvar += ncopy;
      }
      datamap++;
    }
  }

  // Fill histograms

  HistFill();

  return nvar;
}

//______________________________________________________________________________
Int_t THaEvent::Init()
{
  // Initialize fDataMap. Called automatically by Fill() as necessary.

  if( !gHaVars ) return -2;

  if( DataMap* datamap = fDataMap ) {
    while( datamap->ncopy ) {
      if( THaVar* pvar = gHaVars->Find( datamap->name )) {
	datamap->pvar = pvar;
      } else {
	Warning("Init()", "Global variable %s not found. "
		"Will be filled with zero.", datamap->name );
	datamap->pvar = NULL;
      }
      datamap++;
    }
  }

  Int_t status = HistInit();
  if( status ) 
    return status;

  fInit = kTRUE;
  return 0;
}

//______________________________________________________________________________
Int_t THaEvent::LoadHist( const char* filename )
{
  // Load histogram defintions from file. 
  // Return zero if successful, -1 otherwise.

  static const char* const here = "LoadHist";

  if( !filename || strlen(filename) == 0
      || strspn(filename," ") == strlen(filename) ) {
    Error( here, "Invalid file name. No histograms loaded.");
    return -1;
  }

  ifstream infile( filename );
  if( !infile ) {
    Error( here, "Unable to open input file %s. No histograms loaded.",
	   filename );
    return -2;
  }

  // Read the file line by line

  Int_t nlines_read = 0, nlines_ok = 0;

  string line;
  const string comment_chars( "#" );
  const string whitespace( " \t" );
  const char kQuot = '\"';
  const string separators =  whitespace + kQuot;
  string::size_type pos, prev_pos;

  while( getline( infile, line )) {

    // Blank line or comment?

    if( line.empty()
	|| (pos = line.find_first_not_of( whitespace )) == string::npos
	|| comment_chars.find(line[pos]) != string::npos )
      continue;

    // Valid line ... start processing

    vector<string> tokens;
    bool quote = false, gotquote = false;
    line.append(" ");
    prev_pos = pos;
    ++nlines_read;

    // Split the line into tokens separated by whitespace and quotes.
    // Preserve whitespace inside quotes and strip the quotes.

    while( (pos = line.find_first_of( separators, pos )) != string::npos ) {
      if( !quote && pos > prev_pos ) {
	tokens.push_back( line.substr( prev_pos, pos-prev_pos ));
      }
      gotquote = (line[pos] == kQuot);
      ++pos;
      if( !quote )
	prev_pos = pos;
      if( gotquote ) {
	if( quote ) {
	  tokens.push_back( line.substr( prev_pos, pos-prev_pos-1 ));
	  prev_pos = pos;
	}
	quote = !quote;
      }
    }
    // Check for problems

    if( quote ) {
      Error( here, "Unbalanced quotes, histogram not loaded:\n"
	     ">>>>>> %s", line.c_str());
      continue;
    }
    if( tokens.size() != 6 ) {
      Error( here, "Incorrect number of parameters, histogram not loaded:\n"
	     ">>>>>> %s", line.c_str());
      continue;
    }
    Int_t  nbins = std::atoi(tokens[3].c_str());
    Axis_t xlo   = std::atof(tokens[4].c_str());
    Axis_t xhi   = std::atof(tokens[5].c_str());
    if( nbins <= 0 ) {
      Error( here, "Number of bins <= 0. Histogram not loaded:\n"
	     ">>>>>> %s", line.c_str());
      continue;
    }
    if( xlo>=xhi ) {
      Error( here, "Lower end >= upper end. Histogram not loaded:\n"
	     ">>>>>> %s", line.c_str());
      continue;
    }

    // At last, define the histogram

    DefineHist( tokens[0].c_str(), tokens[1].c_str(), tokens[2].c_str(),
		nbins, xlo, xhi );
    ++nlines_ok;
  }

  infile.close();
  Int_t nbad = nlines_read-nlines_ok;
  if( nbad>0 ) Warning( here, "%d hsitograms(s) could not be defined, "
			"check input file %s", nbad, filename );
  return nbad;
}

//______________________________________________________________________________
void THaEvent::PrintHist( Option_t* opt ) const
{
  // Print current histogram definitions

  HistDef* hdef = fHistDef;
  if( !hdef) return;

  // Determine widths of print columns
  const int NF = 9;
  UInt_t width[NF] = { 0 };
  while( hdef->name ) {
    width[0] = max( width[0], strlen(hdef->name));
    width[1] = max( width[1], strlen(hdef->hname));
    width[2] = max( width[2], strlen(hdef->title));
    width[3] = max( width[3], IntDigits( hdef->nbins ));
    width[4] = max( width[4], FloatDigits( static_cast<Float_t>(hdef->xlo)));
    width[5] = max( width[5], FloatDigits( static_cast<Float_t>(hdef->xhi)));
    if( hdef->h1 ) {
      width[6] = max( width[6], strlen(hdef->h1->ClassName()));
      width[7] = max( width[7], IntDigits( static_cast<Int_t>
					   ( hdef->h1->GetEntries())));
      width[8] = max( width[8], FloatDigits( static_cast<Float_t>
					     (hdef->h1->GetSumOfWeights())));
    }
    hdef++;
  }

  // Print the histogram definitions
  hdef = fHistDef;
  char* quoted_title = new char[ width[2]+3 ];
  while( hdef->name ) {
    ostrstream ostr( quoted_title, width[2]+3 );
    ostr << '\"' << hdef->title << '\"' << '\0';
    cout.flags( ios::left );
    cout << setw(width[0]) << hdef->name << " "
	 << setw(width[1]) << hdef->hname << " "
	 << setw(width[2]+2) << quoted_title << " ";
    cout.flags( ios::right );
    cout << setw(width[3]) << hdef->nbins << " "
	 << setw(width[4]) << hdef->xlo << " "
	 << setw(width[5]) << hdef->xhi << " ";
    if( hdef->h1 ) {
      cout.flags( ios::left );
      cout << setw(width[6]) << hdef->h1->ClassName() << " ";
      cout.flags( ios::right );
      cout << setw(width[7]) << hdef->h1->GetEntries() << " "
	   << setw(width[8]) << hdef->h1->GetSumOfWeights();
    }
    cout << endl;
    hdef++;
  }
  delete [] quoted_title;
  cout.flags( ios::right );
}

//______________________________________________________________________________
void THaEvent::Reset( Option_t* opt )
{
  // Reset pointers to global variables. Forces Init() to be executed
  // at next Fill(), which re-associates global variables with the
  // event structure and histograms.

  // Force histgrams that no longer exist to be recreated
  if( HistDef* hdef = fHistDef ) {
    while( hdef->name ) {
      if( !gROOT->FindObject( hdef->hname ))
	hdef->h1 = NULL;
      hdef++;
    }
    fInit = kFALSE;
  }
}

//______________________________________________________________________________
UInt_t THaEvent::IntDigits( Int_t n ) const
{ 
  //Get number of printable digits of integer n.
  //Internal utility function.

  if( n == 0 ) return 1;
  int j = 0;
  if( n<0 ) {
    j++;
    n = -n;
  }
  while( n>0 ) {
    n /= 10;
    j++;
  }
  return j;
}

//______________________________________________________________________________
UInt_t THaEvent::FloatDigits( Float_t f ) const
{ 
  //Get number of printable digits of float f.
  //Internal utility function.

  const int len = 16;
  char buf[len+1] = { 0 };
  ostrstream ostr( buf, len );
  ostr << f;
  int i = len;
  while( i && buf[--i] == 0);
  if( i == 0 ) return 1;
  if( strchr(buf,'.'))
    while( i>=0 && ( buf[i] == '\n' || buf[i] == 0 )) i--;
  else
    while( i>=0 && ( buf[i] == '\n' )) i--;
  if( i < 0 ) return 1;
  return i+1;
}

