//*-- Author :    Robert Michaels, April 2003

// Vector formulas, and much more.
// This object inherits from THaFormula to be able to use its
// parsing capabilities.  This object is either a
//   1. Scaler formula
//   2. Vector formula which can involve fixed size arrays.
//   3. Variable sized array
//   4. An "eye" variable ("[I]") used by THaVhist
//   5. A cut, either vector or scaler.
// The usage instructions are explained in documentation
// about THaOutput.


#include "THaVform.h"
#include "THaString.h"
#include "THaVarList.h"
#include "THaCut.h"
#include "TTree.h"
#include "TROOT.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cctype>

using namespace std;

//_____________________________________________________________________________
THaVform::THaVform( const char *type, const char* name, const char* formula,
		    const THaVarList* vlst, const THaCutList* clst )
  : THaFormula(), fNvar(0), fObjSize(0), fData(0.0),
    fType(kUnknown), fVarPtr(NULL), fOdata(NULL), fPrefix(kNoPrefix)
{
  SetName(name);
  SetList(vlst);
  SetCutList(clst);
  string stemp1 = StripPrefix(formula);
  SetTitle(stemp1.c_str());

  if( type && *type ) {
    size_t len = strlen(type); char* buf = new char[len+1];
    strcpy(buf,type); char* c = buf; while((*c = tolower(*c))) ++c;
    if( !strcmp(buf,"cut") )
      fType = kCut;
    else if( !strcmp(buf,"formula") )
      fType = kForm;
    else if( !strcmp(buf,"eye") )
      fType = kEye;
    else if( !strcmp(buf,"vararray") )
      fType = kVarArray;
  }

  // Call THaFormula's Compile()
  Compile();
}

//_____________________________________________________________________________
THaVform::THaVform( const THaVform& rhs ) :
  THaFormula(rhs), fNvar(rhs.fNvar), fObjSize(rhs.fObjSize), fData(rhs.fData),
  fType(rhs.fType), fgAndStr(rhs.fgAndStr), fgOrStr(rhs.fgOrStr),
  fgSumStr(rhs.fgSumStr), fVarName(rhs.fVarName), fVarStat(rhs.fVarStat),
  fSarray(rhs.fSarray), fVectSform(rhs.fVectSform), fStitle(rhs.fStitle),
  fVarPtr(rhs.fVarPtr), fOdata(NULL), fPrefix(rhs.fPrefix)
{
  // Copy ctor

  for (vector<THaCut*>::const_iterator itc = rhs.fCut.begin();
       itc != rhs.fCut.end(); ++itc) {
    if( *itc ) {
      //FIXME: do we really need to make copies?
      // The more formulas, the more stuff to evaluate...
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,10,0)
      fCut.push_back(new THaCut(**itc));
#else
      // work around buggy TFormula copy constructor
      THaCut* c = new THaCut;
      *c = **itc;
      fCut.push_back(c);
#endif
    }
  }
  for (vector<THaFormula*>::const_iterator itf = rhs.fFormula.begin();
       itf != rhs.fFormula.end(); ++itf) {
    if( *itf ) {
      //FIXME: do we really need to make copies?
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,10,0)
      fFormula.push_back(new THaFormula(**itf));
#else
      // work around buggy TFormula copy constructor
      THaFormula* f = new THaFormula;
      *f = **itf;
      fFormula.push_back(f);
#endif
    }
  }
  if( rhs.fOdata )
    fOdata = new THaOdata(*rhs.fOdata);
}

//_____________________________________________________________________________
THaVform::~THaVform()
{
  Uncreate();
}

//_____________________________________________________________________________
THaVform &THaVform::operator=(const THaVform &fv)
{
  if ( &fv != this )
    {
      Uncreate();
      Create (fv);
    }
  return *this;
}

//_____________________________________________________________________________
void THaVform::Create(const THaVform &rhs)
{
  THaFormula::operator=(rhs);
  fType = rhs.fType;
  fNvar = rhs.fNvar;
  fVarPtr = rhs.fVarPtr;
  fObjSize = rhs.fObjSize;
  if( rhs.fOdata )
    fOdata = new THaOdata(*rhs.fOdata);
  fVarName = rhs.fVarName;
  fVarStat = rhs.fVarStat;
  fVectSform = rhs.fVectSform;
  fgAndStr = rhs.fgAndStr;
  fgOrStr = rhs.fgOrStr;
  fgSumStr = rhs.fgSumStr;
  for (vector<THaCut*>::const_iterator itc = rhs.fCut.begin();
       itc != rhs.fCut.end(); ++itc) {
    if( *itc ) {
      //FIXME: do we really need to make copies?
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,10,0)
      fCut.push_back(new THaCut(**itc));
#else
    // work around buggy TFormula copy constructor
      THaCut* c = new THaCut;
      *c = **itc;
      fCut.push_back(c);
#endif
    }
  }
  for (vector<THaFormula*>::const_iterator itf = rhs.fFormula.begin();
       itf != rhs.fFormula.end(); ++itf) {
    if( *itf ) {
      //FIXME: do we really need to make copies?
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,10,0)
      fFormula.push_back(new THaFormula(**itf));
#else
    // work around buggy TFormula copy constructor
      THaFormula* f = new THaFormula;
      *f = **itf;
      fFormula.push_back(f);
#endif
    }
  }
}

//_____________________________________________________________________________
void THaVform::Uncreate()
{
  if (fOdata) delete fOdata;
  fOdata = NULL;
  for (vector<THaCut*>::iterator itc = fCut.begin();
       itc != fCut.end(); ++itc) delete *itc;
  for (vector<THaFormula*>::iterator itf = fFormula.begin();
       itf != fFormula.end(); ++itf) delete *itf;
  fCut.clear();
  fFormula.clear();
}

//_____________________________________________________________________________
void THaVform::LongPrint() const
{ // Long printout for debugging.
  ShortPrint();
  cout << "Num of variables "<<fNvar<<endl;
  cout << "Object size "<<fObjSize<<endl;
  for (Int_t i = 0; i < fNvar; ++i) {
      cout << "Var # "<<i<<"    name = "<<
      fVarName[i]<<"   stat = "<<fVarStat[i]<<endl;
  }
  for (vector<THaFormula*>::const_iterator itf = fFormula.begin();
       itf != fFormula.end(); ++itf) {
       cout << "Formula full printout --> "<<endl;
       (*itf)->Print(kPRINTFULL);
  }
  for (vector<THaCut*>::const_iterator itc = fCut.begin();
       itc != fCut.end(); ++itc) {
       cout << "Cut full printout --> "<<endl;
       (*itc)->Print(kPRINTFULL);
  }
}

//_____________________________________________________________________________
void THaVform::ShortPrint() const
{ // Short printout for self identification.
  cout << "Print of ";
  if (IsVarray()) cout << " variable sized array: ";
  if (IsFormula()) cout << " formula: ";
  if (IsCut()) cout << " cut: ";
  if (IsEye()) cout << " [I]-var: ";
  cout << GetName() << "  = " << GetTitle()<<endl;
}

//_____________________________________________________________________________
void THaVform::ErrPrint(Int_t err) const
{
  // Gives friendly explanation of the usage errors.
  // The possibilities are only bounded by human imagination.

  cout << "THaVform::Explanation of error status -->"<<endl;

  switch(err) {

    case kOK:
      cout << "No error.";
      break;

    case kIllVar:
      cout << "If you make a formula of a variable sized array"<<endl;
      cout << "like L.vdc.v1.wire, then the formula string must"<<endl;
      cout << "ONLY contain that variable, no brackets, NOTHING else."<<endl;
      cout << "Data appears in Tree same as for a variable"<<endl;
      cout << "However, e.g. '2*L.vdc.v1.wire'  is undefined."<<endl;
      break;

    case kIllTyp :
      cout << "Illegal type of THaVform"<<endl;
      cout << "Only options at present are "<<endl;
      cout << "'formula', 'cut', 'eye'"<<endl;
      break;

    case kNoGlo :
      cout << "There are no global variables defined !!"<<endl;
      break;

    case kIllMix :
      cout << "You CANNOT have a formula that mixes"<<endl;
      cout << "variable sized and fixed sized arrays."<<endl;
      cout << "Note: Formula can only defined by fixed sized arrays"<<endl;
      cout << "like L.s1.lt. So e.g.  '2*L.s1.lt - 500'  is ok."<<endl;
      break;

    case kArrZer :
      cout << "Attempting to use a variable with pointer = 0."<<endl;
      cout << "The variables must be defined in the code."<<endl;
      cout << "Try 'gHaVars->Print()' to verify names."<<endl;
      break;

    case kArrSiz :
      cout << "Your formula uses variables of different sizes."<<endl;
      cout << "This is not meaningful."<<endl;
      break;

    case kUnkPre :
      cout << "Unknown prefix."<<endl;
      cout << "The presently supported prefixes are "<<endl;
      cout << "'SUM:', 'AND:', and 'OR:' "<<endl;
      cout << "(case insenstitive)"<<endl;
      break;

    case kIllPre :
      cout << "Illegal usage of prefix."<<endl;
      cout << "'SUM:' can be used with formula and cuts."<<endl;
      cout << "'AND:', and 'OR:' can only be used with cuts."<<endl;
      break;

    default:
      cout << "Unknown error !!"<<endl;

  }

}


//_____________________________________________________________________________
vector<string> THaVform::GetVars() const
{
// Get names of variable that are used by this formula.
  vector<string> result;
  result.clear();
  for (Int_t i = 0; i < fNvar; ++i) {
    result.push_back(fVarName[i]);
  }
  return result;
}


//_____________________________________________________________________________
Int_t THaVform::Init()
{
  // Initialize the variables of this Vform.
  // For explanation of return status see "ExplainErr"

  THaVar *pvar1, *pvar2;
  fObjSize = 0;
  fSarray.clear();

  if (IsEye()) return 0;

  fStitle = GetTitle();
  Int_t status = 0;

  for (Int_t i = 0; i < fNvar; ++i) {
    if (fVarStat[i] == kVAType) {  // This obj is a var. sized array.
      if (StripBracket(fStitle) == fVarName[i]) {
 	 status = 0;
         fVarPtr = fVarList->Find(fVarName[i].c_str());
         if (fVarPtr) {
           fType = kVarArray;
           fObjSize = fVarPtr->GetLen();
           if (fPrefix == kNoPrefix) {
	      delete fOdata;
	      fOdata = new THaOdata();
	   } else {
	      fObjSize = 1;
	   }
	 }
      } else {
	 status = kIllVar;
      }
      return status;
    }
  }

  if (!IsCut() && !IsFormula()) return kIllTyp;
  if (!fVarList) return kNoGlo;

// Check that all fixed arrays have the same size.

  for (Int_t i = 0; i < fNvar; ++i) {

    if (fVarStat[i] == kVAType ) {
      cout << "ERROR:THaVform:: Cannot mix variable arrays with ";
      cout << "other variables or formula."<<endl;
      return kIllMix;
    }
    if (fVarStat[i] != kFAType ) continue;
    pvar1 = fVarList->Find(fVarName[i].c_str());
    fVarPtr = pvar1; // Store one pointer to be able to get the size
                     // later since it may change. This works since all
                     // elements were verified to be the same size.
    if (i == fNvar) continue;
    for (Int_t j = i+1; j < fNvar; ++j) {
      if (fVarStat[j] != kFAType ) continue;
      pvar2 = fVarList->Find(fVarName[j].c_str());
      if (!pvar1 || !pvar2) {
        status = kArrZer;
        cout << "THaVform:ERROR: Trying to use zero pointer."<<endl;
        continue;
      }
      if ( !(pvar1->HasSameSize(pvar2)) ) {
	cout << "THaVform::ERROR: Var "<<fVarName[i]<<"  and ";
        cout << fVarName[j]<<"  have different sizes "<<endl;
        status = kArrSiz;
      }
    }
  }
  if (status != 0) return status;

  for (Int_t i = 0; i < fNvar; ++i) {
    if (fVarStat[i] == kFAType) fSarray.push_back(fVarName[i]);
  }

  Int_t varsize = 1;
  if (fVarPtr) varsize = fVarPtr->GetLen();

  status = MakeFormula(0, varsize);

  if (status != 0) return status;

  if (fVarPtr) {
    fObjSize = fVarPtr->GetLen();
    if (fPrefix == kNoPrefix) {
       fOdata = new THaOdata();
    } else {
       fObjSize = 1;
    }
  } else {
    fObjSize = 1;
  }

  return status;

}

//_____________________________________________________________________________
void THaVform::ReAttach( )
{
// Store one pointer to be able to get the size.
// (see explanation in Init).  Also recompile the
// THaCut's and THaFormula's to reattach to variables.
  for (Int_t i = 0; i < fNvar; ++i) {
    if (fVarStat[i] != kFAType ) continue;
    fVarPtr = fVarList->Find(fVarName[i].c_str());
    break;
  }
  for (vector<THaCut*>::iterator itc = fCut.begin();
       itc != fCut.end(); ++itc) (*itc)->Compile();
  for (vector<THaFormula*>::iterator itf = fFormula.begin();
       itf != fFormula.end(); ++itf) (*itf)->Compile();
  return;
}


//_____________________________________________________________________________
Int_t THaVform::MakeFormula(Int_t flo, Int_t fhi)
{ // Make the vector formula (fVectSform) from index flo to fhi.
  // Return status :
  //    0 = ok
  // kUnkPre = tried to use an unknown prefix ("OR:", etc)
  // kIllPre = ambiguity of whether this is a cut.

  Int_t status = 0;

  if (flo < 0) flo = 0;
  if (flo >= fhi) return 0;
  if (fhi > fgVFORM_HUGE) {
    // fSize cannot be infinite.
    // The following probably indicates a usage error.
    cout << "THaVform::WARNING: Asking for a too-huge";
    cout << " number of formulas !!" << endl;
    cout << "Truncating at " << fgVFORM_HUGE << endl;
    fhi = fgVFORM_HUGE;
  }
  if (fhi > (long)fVectSform.size()) GetForm(fhi);
  // and if the above line doesn't repair fhi...
  if (fhi > (long)fVectSform.size()) return 0;

  if (fPrefix == kNoPrefix) {

   for (Int_t i = flo; i < fhi; ++i) {
     string cname = Form("%s-%d",GetName(),i);
     //FIXME: avoid duplicate cuts/formulas
     if (IsCut()) {
        fCut.push_back(new THaCut(cname.c_str(),fVectSform[i].c_str(),
				  "thavcut"));
     } else if (IsFormula()) {
        fFormula.push_back(new THaFormula(cname.c_str(),
					  fVectSform[i].c_str()));
     }
   }

  } else {  // The formula had a prefix like "OR:"
            // This THaVform is therefore a scaler.

    string soper="";
    int iscut=0;
    switch (fPrefix) {

      case kAnd:
	soper = "&&";
        iscut = 1;
        break;

      case kOr:
	soper = "||";
        iscut = 1;
        break;

      case kSum:
        soper = "+";
        iscut = 0;
        break;

      default:
        status = kUnkPre;
        cout << "THaVform:ERROR:: Unknown prefix"<<endl;
    }
    if (status != 0) return status;

    if (iscut && !IsCut()) {
      status = kIllPre;
      cout << "THaVform:ERROR:: Illegal prefix -- "<<endl;
      cout << "OR:, AND: works only with cuts."<<endl;
      cout << "SUM: works only with both cuts and formulas."<<endl;
      return status;
    }

    string sform="";
    for (Int_t i = 0; i < fhi; ++i) {
      sform += fVectSform[i];
      if (i < (long)fVectSform.size()-1) sform += soper;
    }

    string cname(GetName());

    if (IsCut()) {
      if( !fCut.empty() ) {  // drop what was there before
	for (vector<THaCut* >::iterator itc = fCut.begin();
             itc != fCut.end(); ++itc) delete *itc;
	fCut.clear();
      }
      cname += "cut";
      //FIXME: avoid duplicate cuts/formulas
      fCut.push_back(new THaCut(cname.c_str(),sform.c_str(),
				"thavsform"));
    } else if (IsFormula()) {
      if( !fFormula.empty() ) {  // drop what was there before
	for (vector<THaFormula* >::iterator itf = fFormula.begin();
	     itf != fFormula.end(); ++itf)
	  delete *itf;
	fFormula.clear();
      }
      cname += "form";
      //FIXME: avoid duplicate cuts/formulas
      fFormula.push_back(new THaFormula(cname.c_str(),sform.c_str()));
    }

  }

  return status;
}

//_____________________________________________________________________________
string THaVform::StripPrefix(const char* expr)
{
  string::size_type pos,pos1,pos2;
  string result,stitle;
  vector<string> sprefix;
  vector<Int_t> ipflg;
  fgAndStr = "AND:";
  fgOrStr  = "OR:";
  fgSumStr = "SUM:";
  sprefix.push_back(fgAndStr);  ipflg.push_back(kAnd);
  sprefix.push_back(fgOrStr);   ipflg.push_back(kOr);
  sprefix.push_back(fgSumStr);  ipflg.push_back(kSum);
// If we ever invent new prefixes, add them here...

  stitle = string(expr);

  fPrefix = kNoPrefix;
  result = stitle;
  for (int i = 0; i < (long)sprefix.size(); ++i) {
     pos1 = stitle.find(ToUpper(sprefix[i]),0);
     pos2 = stitle.find(ToLower(sprefix[i]),0);
     pos  = string::npos;
     if (pos1 != string::npos) pos = pos1;
     if (pos2 != string::npos) pos = pos2;
     if (pos != string::npos) {
       if (fPrefix != kNoPrefix) {
         cout << "THaVform:ERROR:: Cannot have >1 prefix."<<endl;
       }
       fPrefix = ipflg[i];
       pos = pos + sprefix[i].length();
       result = stitle.substr(pos,pos+stitle.length());
     }
  }

// If the variable is like "L.s1.lt[I]" strip the [I]
// from the end since it is implicit
  string eye = "[I]";
  string stemp = result;
  pos1 = result.find(ToUpper(eye),0);
  pos2 = result.find(ToLower(eye),0);
  pos = string::npos;
  if (pos1 != string::npos) pos = pos1;
  if (pos2 != string::npos) pos = pos2;
  if (pos  != string::npos) result = stemp.substr(0,pos);
  return result;
}

//_____________________________________________________________________________
string THaVform::StripBracket(const string& var) const
{
// If the string contains "[anything]", we strip
// it away.
  string::size_type pos1,pos2;
  string open_brack = "[";
  string close_brack = "]";
  string result = "";
  pos1 = var.find(open_brack,0);
  pos2 = var.find(close_brack,0);
  if ((pos1 != string::npos) &&
      (pos2 != string::npos)) {
      result = var.substr(0,pos1);
      result += var.substr(pos2+1,var.length());
  } else {
      result = var;
  }
  return result;
}

//_____________________________________________________________________________
Int_t THaVform::SetOutput(TTree *tree)
{
  if (IsEye()) return 0;
  string mydata = string(GetName());
  if (fOdata) fOdata->AddBranches(tree, mydata);
  if (!IsVarray() && fObjSize <= 1) {
    string tinfo;
    tinfo = mydata + "/D";
    string tmp = mydata;
    tree->Branch(tmp.c_str(), &fData, tinfo.c_str(), 4000);
  }
  return 0;

}

//_____________________________________________________________________________
Int_t THaVform::Process()
{
// Process this THaVform.  Must be done once per event.
// fData is the data of 1st element which is relevant
// if this is a scaler.  Otherwise fOdata is vector data.

  if (fOdata) fOdata->Clear();
  fData = 0;

  switch (fType) {

  case kForm:
    if (!fFormula.empty()) {
      THaFormula* theFormula = fFormula[0];
      if ( !theFormula->IsError() ) {
	//FIXME: use cached result
        fData = theFormula->Eval();
      }
    }
    if( fOdata != 0 ) {
      vector<THaFormula*>::size_type i = fFormula.size();
      while( i-- > 0 ) {
	THaFormula* theFormula = fFormula[i];
	if ( !theFormula->IsError()) {
	  //FIXME: use cached result?
	  fOdata->Fill(i,theFormula->Eval());
	}
      }
    }
    return 0;

  case kVarArray:
    switch (fPrefix) {

    case kNoPrefix:
      // Standard case first
      if (fOdata) {
	fObjSize = fVarPtr->GetLen();
	// Fill array in reverse order so that fOdata is resized just once
	Int_t i = fObjSize;
	Bool_t first = true;
	while( i-- > 0 ) {
	  // FIXME: for better efficiency, should use pointer to data and
	  // Fill(int n,double* data) method in case of a contiguous array
	  if (fOdata->Fill(i,fVarPtr->GetValue(i)) != 1 && first ) {
	    cout << "THaVform::ERROR: storing too much";
	    cout << " variable sized data: ";
	    cout << fVarPtr->GetName() <<"  "<<fVarPtr->GetLen()<<endl;
	    first = false;
	  }
	}
      }
      break;

    case kAnd:
    case kOr:
      break;   // nonsense for a variable sized array

    case kSum:
      {
	Int_t i = fVarPtr->GetLen();
	while( i-- > 0 )
	  fData += fVarPtr->GetValue(i);
	fObjSize = 1;
      }
      break;

    default:
      break;
    }
    return 0;

  case kCut:
    if (!fCut.empty()) {
      THaCut* theCut = fCut[0];
      if (!theCut->IsError()) {
	  //FIXME: use cached result?
	//        if (theCut->Result()) fData = 1.0;
        if (theCut->EvalCut()) fData = 1.0;
      }
    }
    if( fOdata != 0 ) {
      vector<THaCut*>::size_type i = fCut.size();
      while( i-- > 0 ) {
	THaCut* theCut = fCut[i];
	if ( !theCut->IsError() )
	  //FIXME: use cached result?
	  //	  fOdata->Fill( i, ((theCut->GetResult()) ? 1.0 : 0.0) );  // 1 = true
	  fOdata->Fill( i, ((theCut->EvalCut()) ? 1.0 : 0.0) );  // 1 = true
      }
    }
    return 0;

  case kEye:
    return 0;

  default:
    return -1;

  }
}


//_____________________________________________________________________________
Int_t THaVform::DefinedGlobalVariable( TString& name )
{
  // We use the parsing functionality of THaFormula to
  // tell if the variables are global variables and, if so,
  // if they are arrays or elements of arrays.

  if (fgDebug) cout << "THaVform::DefinedGlob for name = "<<name<<endl;

  Int_t ret = THaFormula::DefinedGlobalVariable( name );
  if( ret < 0 )
    return ret;

  // Retrieve some of the results of the THaFormula parser
  FVarDef_t& def = fVarDef[ret];
  const THaVar* gvar = static_cast<const THaVar*>( def.obj );
  // This makes a certain assumption about the array syntax defined by ROOT
  // and THaArrayString, but in the interest of performance  we don't create
  // a full THaArrayString here just to find out if it is an array element
  Bool_t var_is_array = name.Contains("[");

  fVarName.push_back(name.Data());
  FAr stat;
  if( gvar->IsArray() ) {

    if( var_is_array ) {
      stat = kAElem;
    } else {
      stat = kFAType;
    }
    if (gvar->GetLen() == 0) {
      stat = kVAType;
    }

  } else {
    stat = kScaler;
  }
  fVarStat.push_back(stat);

  if (fgDebug) {
    if (gvar->IsArray()) cout << "gvar is array"<<endl;
    if (gvar->IsBasic()) cout << "gvar is basic"<<endl;
    if (gvar->IsPointerArray()) cout << "gvar is pointer array"<<endl;
    cout << "Here is gvar print "<<endl;
    gvar->Print();
    cout << "end of gvar print "<<endl<<endl;
    cout << "length of var  "<< gvar->GetLen()<<endl;
    cout << "fVarStat "<<fVarStat[fNvar]<<endl;
  }

  ++fNvar;

  return ret;
}


//_____________________________________________________________________________
void THaVform::GetForm(Int_t size)
{

  string open_brack="[";
  string close_brack="]";
  string::size_type pos,pos1,pos2;
  vector<Int_t> ipos;

  char num[10];
  string aline,acopy;
  for (int idx = 0; idx < size; ++idx) {
    acopy = fStitle;
    aline = acopy;
    sprintf(num,"%d",idx);
    for (vector<string>::iterator is = fSarray.begin();
        is != fSarray.end(); ++is) {
      string sb = *is;
      aline = "";
      ipos.clear();
      pos1 = 0;
      pos  = 0;
      while (pos != string::npos) {
        pos  = acopy.find(sb,pos1);
        pos1 = pos + sb.length();
        pos2 = acopy.find(open_brack,pos1);
        if (pos  != string::npos &&
           (pos2 == string::npos || pos2 > pos1))
               ipos.push_back(pos1);
      }
      if ((long)ipos.size() == 0) continue;
// Found at least one array,
// now build fSarray for [0], [1], ... size
      if (idx == 0) fVectSform.clear();
      pos = 0;
      for (int j = 0; j < (long)ipos.size(); ++j) {
         aline += acopy.substr(pos,ipos[j]) +
                 open_brack + num + close_brack;
         pos = ipos[j];

      }
      aline += acopy.substr(ipos[ipos.size()-1],acopy.length());
      acopy = aline;
    }
    if (aline.length() > 0) fVectSform.push_back(aline);
  }

}


ClassImp(THaVform)

