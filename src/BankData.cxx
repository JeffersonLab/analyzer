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

using namespace std;
typedef string::size_type ssiz_t;
using namespace THaString;

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
  if(dvars) delete[] dvars;
  if(vardata) delete[] vardata;
  RemoveVariables();
}

//_____________________________________________________________________________
void BankData::Clear( Option_t* opt )
{
  // Clear all internal variables.

  THaPhysicsModule::Clear(opt);

  fDataValid = false;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus BankData::Init( const TDatime& run_time )
{
  // Standard initialization. Calls this object's ReadDatabase(),
  // ReadRunDatabase(), and DefineVariables()
  // (see THaAnalysisObject::Init)

  cout << "BankData init called "<<endl;

  if( THaPhysicsModule::Init( run_time ) != kOK ) {
    cout << "init not ok ?"<<endl;
  }

  fStatus = kOK;
  return kOK;
}

//_____________________________________________________________________________
Int_t BankData::Process( const THaEvData& evdata )
{

  if( !IsOK() ) return -1;

  Int_t k=0;
  for (UInt_t i=0; i<banklocs.size(); i++) {
    //    cout << "BankData proc "<<dec<<i<<"  "<<banklocs[i]->roc<<"  "<<banklocs[i]->bank<<"  "<<banklocs[i]->numwords<<endl;
   evdata.FillBankData(vardata, banklocs[i]->roc, banklocs[i]->bank, banklocs[i]->offset, banklocs[i]->numwords);
   //   cout << "numwords "<<i<<"  "<<banklocs[i]->numwords<<endl;
    for (Int_t j=0; j<banklocs[i]->numwords; j++) {
      //      cout << "     data  "<<j<<"   "<<k<<"  0x"<<hex<<vardata[j]<<dec<<endl;
      dvars[k] = 1.0*vardata[j];
      k++;
    }
  }
  //  for (Int_t i=0; i<k; i++) cout << "dvars["<<i<<"] = "<<dvars[i]<<endl;

#ifdef TEST1
  Int_t myroc=20;
  Int_t mybank=7;
  Int_t mydata[100];
  cout << "BankData:: process, evnum =  "<<evdata.GetEvNum()<<"  "<<evdata.GetEvType()<<endl;
  evdata.FillBankData(mydata,20,7,0,10);
  for (UInt_t i=0; i<10; i++) {
    cout << "Mydata 20, 7, 0, 10 "<<i<<"   0x"<<hex<<mydata[i]<<"   dec "<<dec<<mydata[i]<<endl;
  }
  cout << "-----------------------"<<endl;
  evdata.FillBankData(mydata,20,7,5,20);
  for (UInt_t i=0; i<20; i++) {
    cout << "Mydata 20, 7, 5, 20 "<<i<<"   0x"<<hex<<mydata[i]<<"   dec "<<dec<<mydata[i]<<endl;
  }
  cout << "-----------------------"<<endl;
  evdata.FillBankData(mydata,20,250,0,40);
  for (UInt_t i=0; i<40; i++) {
    cout << "Mydata 20, 250, 0, 40 "<<i<<"   0x"<<hex<<mydata[i]<<"   dec "<<dec<<mydata[i]<<endl;
  }
#endif

  // That's it
  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
Int_t BankData::ReadRunDatabase( const TDatime& date )
{
  // Read the parameters of this module from the run database

  int ldebug=1;
  const int LEN = 200;
  char cbuf[LEN];
  string::size_type minus1 = string::npos;
  string::size_type pos1;
  const string scomment = "#";
  vector<string> dbline;
  FILE *fi;

  Int_t err = THaPhysicsModule::ReadRunDatabase( date );
  if( err ) return err;

  FILE* f = OpenRunDBFile( date );
  if( !f ) return kFileError;

  vector<string> fnames (GetDBFileList(GetName(), date, ""));
 
  if(ldebug) cout << "fnames size "<<fnames.size()<<endl;

  for (UInt_t i=0; i<fnames.size(); i++) {
    if (ldebug) cout << "BankData:  trying database file "<<fnames[i]<<endl;
    fi = OpenFile(fnames[i].c_str(), date);
    if (fi != NULL) break;
  }

  if (fi == NULL) {
    cout << "Failed to open database file for BankData"<<endl;
    return -1;
  }

  Int_t iroc, ibank, ioff, inum;
  std::string svar;

  while( fgets(cbuf, LEN, fi) != NULL) {
    std::string sinput(cbuf);
    if(ldebug) cout << "database line = "<<sinput<<endl;
    dbline = vsplit(sinput);
    svar="";
    if(dbline.size() > 2) {
      pos1 = FindNoCase(dbline[0],scomment);
      if (pos1 != minus1) continue;
      iroc = 0;  ibank = 0; ioff = 0;  inum = 1;
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
     if (ldebug) cout << "svar "<<svar<<"  len "<<svar.length()<<"  iroc "<<iroc<<"  ibank "<<ibank<<"  offset "<<ioff<<"  num "<<inum<<endl;      
     banklocs.push_back( new BankLoc(svar, iroc, ibank, ioff, inum) );

  }

  cout << "banklocs size "<<banklocs.size()<<endl;

// warning to myself:  do NOT call DefineVariables() here, it will lead to
// stale global data.  Let the analyzer mechanisms call it (in the right sequence).

  fStatus = kOK;

  fclose(fi);
  return fStatus;
}

//_____________________________________________________________________________
Int_t BankData::DefineVariables( EMode mode ) {
  // Define/delete global variables.
  Int_t ldebug=1;
  char svarstem[100],svarname[100],cdesc[100];
  Nvars = 0;
  Int_t maxwords=-999;
  Int_t too_big = 1000000;
  for (UInt_t i=0; i<banklocs.size(); i++) {
      if (ldebug) cout << "bankloc "<<i<<"   "<<banklocs[i]->roc<<"   "<<banklocs[i]->bank<<"  "<<banklocs[i]->offset<<"  "<<banklocs[i]->numwords<<endl;
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
  vardata = new UInt_t[maxwords];
  dvars = new Double_t[Nvars];
  for (Int_t i=0; i<Nvars; i++) dvars[i]=40+i;
  if (!gHaVars) {
    cerr << "BankData::ERROR: No gHaVars ?!  Well, that's a problem !!"<<endl;
    return kInitError;
  }
  if (ldebug) cout << "BankData:: DefVars "<<fName<<"  "<<Nvars<<endl;
  const Int_t* count = 0;
  Int_t k=0;
  for (UInt_t i = 0; i < banklocs.size(); i++) {
    sprintf(svarname,fName);
    strcat(svarname,".");
    strcat(svarname,banklocs[i]->svarname.c_str());
    sprintf(cdesc,"Bank Data ");
    strcat(cdesc,banklocs[i]->svarname.c_str());
    if (banklocs[i]->numwords == 1) {
       if (ldebug) cout << "numwords = 1, svarname = "<<svarname<<endl;
       gHaVars->DefineByType(svarname, cdesc, &dvars[k], kDouble, count);
       k++;
    } else {
      for (Int_t j=0; j<banklocs[i]->numwords; j++) {
        char cnum[100];
        sprintf(cnum,"%d",j);
        strcpy(svarstem,svarname);
        strcat(svarstem,cnum);
        if (ldebug) cout << "numwords > 1, svarname = "<<svarstem<<endl;
        gHaVars->DefineByType(svarstem, cdesc, &dvars[k], kDouble, count);
        k++;
      }
    }
  }
  fIsSetup = ( mode == kDefine );
  return kOK;
} 

  
//_____________________________________________________________________________
void BankData::PrintInitError( const char* here )
{
  Error( Here(here), "Cannot set. Module already initialized." );
}



//_____________________________________________________________________________
ClassImp(BankData)

