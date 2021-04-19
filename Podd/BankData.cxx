//*-- Author :    Bob Michaels, April 2018
// This is based on Ole Hansen's example UserModule

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// BankData                                                           //
//                                                                      //
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "BankData.h"
#include "THaString.h"
#include "THaVar.h"
#include "THaVarList.h"
#include "THaEvData.h"
#include "THaGlobals.h"
#include "Helper.h"
#include <sstream>
#include <iterator>

using namespace std;
using namespace THaString;

//_____________________________________________________________________________
class BankLoc { // Utility class used by BankData
public:
  BankLoc(std::string svar, Int_t iroc, Int_t ibank, Int_t ioff, Int_t inum)
    : svarname(std::move(svar)), roc(iroc), bank(ibank), offset(ioff),
      numwords(inum) {}
  std::string svarname;
  Int_t roc,bank,offset,numwords;
};

//_____________________________________________________________________________
BankData::BankData( const char* name, const char* description) :
  THaPhysicsModule(name,description), Nvars(0), dvars(nullptr), vardata(nullptr)
{
  // Normal constructor.
}

//_____________________________________________________________________________
BankData::~BankData()
{
  // Destructor
  RemoveVariables();
  delete[] dvars;
  delete[] vardata;
}

//_____________________________________________________________________________
Int_t BankData::Process( const THaEvData& evdata )
{

  if( !IsOK() ) return -1;

  Int_t k=0;
  for( const auto& bankloc : banklocs ) {
    evdata.FillBankData(vardata, bankloc->roc, bankloc->bank,
        bankloc->offset, bankloc->numwords);
    for( Int_t j = 0; j < bankloc->numwords; j++, k++ ) {
      dvars[k] = vardata[j];
    }
  }

  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
Int_t BankData::ReadDatabase( const TDatime& date )
{
  // Read the parameters of this module from the run database

  const int LEN = 200;
  char cbuf[LEN];
  const string scomment = "#";
  vector<string> dbline;

//  const char* const here = "ReadDatabase";

  // Read database

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  RemoveVariables();

  while( fgets(cbuf, LEN, fi) != nullptr) {
    std::string sinput(cbuf);
    if(fDebug) cout << "database line = " << sinput << endl;
    dbline = vsplit(sinput);
    std::string svar;
    Int_t iroc = 0,  ibank = 0, ioff = 0,  inum = 1;
    if(dbline.size() > 2) {
      auto pos1 = FindNoCase(dbline[0],scomment);
      if (pos1 != string::npos) continue;
      svar = dbline[0];
      iroc  = atoi(dbline[1].c_str());
      ibank = atoi(dbline[2].c_str());
      if (dbline.size()>3) {
	  ioff = atoi(dbline[3].c_str());
	  if (dbline.size()>4) {
	    inum = atoi(dbline[4].c_str());
	  }
	}
      }
     if(svar.empty()) continue;
     if (fDebug) cout << "svar " << svar << "  len " << svar.length()
                      << "  iroc " << iroc << "  ibank " << ibank
                      << "  offset " << ioff << "  num " << inum << endl;
    banklocs.emplace_back(new BankLoc(svar, iroc, ibank, ioff, inum));

  }

  Int_t maxwords = 1, ibank = 0;
  Nvars = 0;
  for( const auto& bankloc : banklocs) {
    if (fDebug) cout << "bankloc " << ibank++ << "   " << bankloc->roc << "   "
                     << bankloc->bank << "  " << bankloc->offset << "  "
                     << bankloc->numwords << endl;
    if( bankloc->numwords > maxwords) maxwords = bankloc->numwords;
    Nvars += bankloc->numwords;
  }
  // maxwords is the biggest number of words for all banklocs.
  // dvars must be of length Nvars in order to fit all the data
  // for this class; it's the global data.
  // Check that neither of these are "too_big" for some reason.
  const Int_t too_big = 1000000;
  if (maxwords > too_big || Nvars > too_big) {
    cerr << "BankData::ERROR:  very huge number of words "
         <<maxwords<<"  "<<Nvars<<endl;
    return kInitError;
  }

  delete [] vardata;
  delete [] dvars; dvars = nullptr;
  vardata = new UInt_t[maxwords];
  if( Nvars > 0 ) {
    dvars = new Double_t[Nvars];
    //DEBUG?
    for (Int_t i=0; i<Nvars; i++)
      dvars[i]=40+i;
  }

  // warning to myself:  do NOT call DefineVariables() here, it will lead to
  // stale global data.  Let the analyzer mechanisms call it (in the
  // right sequence).

  fStatus = kOK;

  fclose(fi);
  return fStatus;
}

//_____________________________________________________________________________
Int_t BankData::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if (!gHaVars) {
    cerr << "BankData::ERROR: No gHaVars ?!  Well, that's a problem !!"<<endl;
    return kInitError;
  }

  if (fDebug) cout << "BankData:: DefVars " << fName << "  " << Nvars << endl;
  Int_t k=0;
  for( const auto& bankloc : banklocs ) {
    string svarname = std::string(fName.Data()) + "." + bankloc->svarname;
    string cdesc = "Bank Data " + bankloc->svarname;
    if (bankloc->numwords == 1) {
       if (fDebug) cout << "numwords = 1, svarname = " << svarname << endl;
       if( mode == kDefine )
	 gHaVars->Define(svarname.c_str(), cdesc.c_str(), dvars[k]);
       else
	 gHaVars->RemoveName(svarname.c_str());
       k++;
    } else {
      for (Int_t j=0; j<bankloc->numwords; j++) {
	ostringstream os;
	os << svarname << j;
	if (fDebug) cout << "numwords > 1, svarname = " << os.str() << endl;
	if( mode == kDefine )
	  gHaVars->Define(os.str().c_str(), cdesc.c_str(), dvars[k]);
	else
	  gHaVars->RemoveName(os.str().c_str());
	k++;
      }
    }
  }
  fIsSetup = ( mode == kDefine );
  return kOK;
}

//_____________________________________________________________________________
ClassImp(BankData)
