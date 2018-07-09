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
#include "THaAnalysisObject.h"
#include "Varargs.h"
#include "VarDef.h"
#include "THaString.h"
#include "THaVar.h"
#include "VarType.h"
#include "THaVarList.h"
#include "Helper.h"
#include <sstream>

using namespace std;
typedef string::size_type ssiz_t;
using namespace THaString;

//_____________________________________________________________________________
class BankLoc { // Utility class used by BankData
public:
  BankLoc(std::string svar, Int_t iroc, Int_t ibank, Int_t ioff, Int_t inum)
    : svarname(svar), roc(iroc), bank(ibank), offset(ioff), numwords(inum) {}
  std::string svarname;
  Int_t roc,bank,offset,numwords;
};

//_____________________________________________________________________________
BankData::BankData( const char* name, const char* description) :
  THaPhysicsModule(name,description), Nvars(0), dvars(0), vardata(0)
{
  // Normal constructor.


}

//_____________________________________________________________________________
BankData::~BankData()
{
  // Destructor
  RemoveVariables();
  Podd::DeleteContainer(banklocs);
  delete[] dvars;
  delete[] vardata;
}

//_____________________________________________________________________________
Int_t BankData::Process( const THaEvData& evdata )
{

  if( !IsOK() ) return -1;

  Int_t k=0;
  for (UInt_t i=0; i<banklocs.size(); i++) {
   evdata.FillBankData(vardata, banklocs[i]->roc, banklocs[i]->bank, banklocs[i]->offset, banklocs[i]->numwords);
    for (Int_t j=0; j<banklocs[i]->numwords; j++) {
      dvars[k] = 1.0*vardata[j];
      k++;
    }
  }

  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
Int_t BankData::ReadDatabase( const TDatime& date )
{
  // Read the parameters of this module from the run database

  const int ldebug=0;
  const int LEN = 200;
  char cbuf[LEN];
  const string::size_type minus1 = string::npos;
  string::size_type pos1;
  const string scomment = "#";
  vector<string> dbline;

//  const char* const here = "ReadDatabase";

  // Read database

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  RemoveVariables();
  Podd::DeleteContainer(banklocs);

  while( fgets(cbuf, LEN, fi) != NULL) {
    std::string sinput(cbuf);
    if(ldebug) cout << "database line = "<<sinput<<endl;
    dbline = vsplit(sinput);
    std::string svar="";
    Int_t iroc = 0,  ibank = 0, ioff = 0,  inum = 1;
    if(dbline.size() > 2) {
      pos1 = FindNoCase(dbline[0],scomment);
      if (pos1 != minus1) continue;
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
     if(svar.length()==0) continue;
     if (ldebug) cout << "svar "<<svar<<"  len "<<svar.length()
		      <<"  iroc "<<iroc<<"  ibank "<<ibank
		      <<"  offset "<<ioff<<"  num "<<inum<<endl;
     banklocs.push_back( new BankLoc(svar, iroc, ibank, ioff, inum) );

  }

  Int_t maxwords=-999;
  const Int_t too_big = 1000000;
  Nvars = 0;
  for (UInt_t i=0; i<banklocs.size(); i++) {
    if (ldebug) cout << "bankloc "<<i<<"   "<<banklocs[i]->roc<<"   "
		     <<banklocs[i]->bank<<"  "<<banklocs[i]->offset<<"  "
		     <<banklocs[i]->numwords<<endl;
    if(banklocs[i]->numwords > maxwords) maxwords = banklocs[i]->numwords;
    Nvars += banklocs[i]->numwords;
  }
  // maxwords is the biggest number of words for all banklocs.
  // dvars must be of length Nvars in order to fit all the data
  // for this class; it's the global data.
  // Check that neither of these are "too_big" for some reason.
  if (maxwords > too_big || Nvars > too_big) {
    cerr << "BankData::ERROR:  very huge number of words "<<maxwords<<"  "<<Nvars<<endl;
    return kInitError;
  }

  delete [] vardata;
  delete [] dvars;
  vardata = new UInt_t[maxwords];
  dvars = new Double_t[Nvars];
  //DEBUG?
  for (Int_t i=0; i<Nvars; i++)
    dvars[i]=40+i;

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

  const Int_t ldebug=0;
  if (!gHaVars) {
    cerr << "BankData::ERROR: No gHaVars ?!  Well, that's a problem !!"<<endl;
    return kInitError;
  }

  if (ldebug) cout << "BankData:: DefVars "<<fName<<"  "<<Nvars<<endl;
  Int_t k=0;
  for (UInt_t i = 0; i < banklocs.size(); i++) {
    string svarname = std::string(fName.Data()) + "." + banklocs[i]->svarname;
    string cdesc = "Bank Data " + banklocs[i]->svarname;
    if (banklocs[i]->numwords == 1) {
       if (ldebug) cout << "numwords = 1, svarname = "<<svarname<<endl;
       if( mode == kDefine )
	 gHaVars->Define(svarname.c_str(), cdesc.c_str(), dvars[k]);
       else
	 gHaVars->RemoveName(svarname.c_str());
       k++;
    } else {
      for (Int_t j=0; j<banklocs[i]->numwords; j++) {
	ostringstream os;
	os << svarname << j;
	if (ldebug) cout << "numwords > 1, svarname = "<<os.str()<<endl;
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
