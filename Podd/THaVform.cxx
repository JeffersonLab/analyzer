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

#include <iostream>
#include <cassert>

using namespace std;
using namespace THaString;

//_____________________________________________________________________________
THaVform::THaVform( const char *type, const char* name, const char* formula,
		    const THaVarList* vlst, const THaCutList* clst )
  : THaFormula(), fNvar(0), fObjSize(0), fEyeOffset(0), fData(0.0),
    fType(kUnknown), fDebug(0), fVarPtr(nullptr), fOdata(nullptr),
    fPrefix(kNoPrefix)
{
  SetName(name);
  SetList(vlst);
  SetCutList(clst);
  string stemp1 = StripPrefix(formula);
  SetTitle(stemp1.c_str());

  if( type && *type ) {
    if( !CmpNoCase(type, "cut") )
      fType = kCut;
    else if( !CmpNoCase(type, "formula") )
      fType = kForm;
    else if( !CmpNoCase(type, "eye") )
      fType = kEye;
    else if( !CmpNoCase(type, "vararray") )
      fType = kVarArray;
  }

  // Call THaFormula's Compile() unless it's an "eye"
  if (IsEye()) return;
  Compile();
}

//_____________________________________________________________________________
THaVform::THaVform( const THaVform& rhs ) :
  THaFormula(rhs), fNvar(rhs.fNvar), fObjSize(rhs.fObjSize),
  fEyeOffset(rhs.fEyeOffset), fData(rhs.fData),
  fType(rhs.fType), fDebug(rhs.fDebug), fAndStr(rhs.fAndStr), fOrStr(rhs.fOrStr),
  fSumStr(rhs.fSumStr), fVarName(rhs.fVarName), fVarStat(rhs.fVarStat),
  fSarray(rhs.fSarray), fVectSform(rhs.fVectSform), fStitle(rhs.fStitle),
  fVarPtr(rhs.fVarPtr), fOdata(nullptr), fPrefix(rhs.fPrefix)
{
  // Copy ctor

  for( const auto* theCut : rhs.fCut ) {
    if( theCut ) {
      //FIXME: do we really need to make copies?
      // The more formulas, the more stuff to evaluate...
      fCut.push_back(new THaCut(*theCut));
    }
  }
  for( const auto* theForm : rhs.fFormula ) {
    if( theForm ) {
      //FIXME: do we really need to make copies?
      fFormula.push_back(new THaFormula(*theForm));
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
  if( &fv != this ) {
    Uncreate();
    Create(fv);
  }
  return *this;
}

//_____________________________________________________________________________
void THaVform::Create(const THaVform &rhs)
{
  THaFormula::operator=(rhs);
  fNvar = rhs.fNvar;
  fObjSize = rhs.fObjSize;
  fEyeOffset = rhs.fEyeOffset;
  fData = rhs.fData;
  fType = rhs.fType;
  fDebug = rhs.fDebug;
  fAndStr = rhs.fAndStr;
  fOrStr = rhs.fOrStr;
  fSumStr = rhs.fSumStr;
  fVarName = rhs.fVarName;
  fVarStat = rhs.fVarStat;

  for( const auto* itf : rhs.fFormula ) {
    if( itf ) {
      //FIXME: do we really need to make copies?
      fFormula.push_back(new THaFormula(*itf));
    }
  }
  for( const auto* itc : rhs.fCut ) {
    if( itc ) {
      //FIXME: do we really need to make copies?
      fCut.push_back(new THaCut(*itc));
    }
  }

  fSarray = rhs.fSarray;
  fVectSform = rhs.fVectSform;
  fStitle = rhs.fStitle;
  fVarPtr = rhs.fVarPtr;
  delete fOdata; fOdata = nullptr;
  if( rhs.fOdata )
    fOdata = new THaOdata(*rhs.fOdata);
  fPrefix = rhs.fPrefix;
}

//_____________________________________________________________________________
void THaVform::Uncreate()
{
  delete fOdata;
  fOdata = nullptr;
  for( auto& itc : fCut ) delete itc;
  for( auto& itf : fFormula ) delete itf;
  fCut.clear();
  fFormula.clear();
}

//_____________________________________________________________________________
void THaVform::LongPrint() const
{
  // Long printout for debugging.
  ShortPrint();
  cout << "Num of variables " << fNvar << endl;
  cout << "Object size " << fObjSize << endl;
  for( Int_t i = 0; i < fNvar; ++i ) {
    cout << "Var # " << i << "    name = " <<
         fVarName[i] << "   stat = " << fVarStat[i] << endl;
  }
  for( const auto* itf : fFormula ) {
    cout << "Formula full printout --> " << endl;
    itf->Print(kPRINTFULL);
  }
  for( const auto* itc : fCut ) {
    cout << "Cut full printout --> " << endl;
    itc->Print(kPRINTFULL);
  }
}

//_____________________________________________________________________________
void THaVform::ShortPrint() const
{
  // Short printout for self identification.
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
      cout << "(case insensitive)"<<endl;
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
  return fVarName;
}


//_____________________________________________________________________________
Int_t THaVform::Init()
{
  // Initialize the variables of this Vform.
  // For explanation of return status see "ExplainErr"

  fObjSize = 0;
  fSarray.clear();

  if (IsEye()) return 0;

  fStitle = GetTitle();
  Int_t status = 0;

  for( Int_t i = 0; i < fNvar; ++i ) {
    if( fVarStat[i] == kVAType ) {  // This obj is a var. sized array.
      if( StripBracket(fStitle) == fVarName[i] ) {
 	 status = 0;
         fVarPtr = fVarList->Find(fVarName[i].c_str());
         if( fVarPtr ) {
           fType = kVarArray;
           fObjSize = fVarPtr->GetLen();
           if( fPrefix == kNoPrefix ) {
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

  for( Int_t i = 0; i < fNvar; ++i ) {

    if( fVarStat[i] == kVAType ) {
      cout << "ERROR:THaVform:: Cannot mix variable arrays with ";
      cout << "other variables or formula."<<endl;
      return kIllMix;
    }
    if (fVarStat[i] != kFAType ) continue;
    auto* pvar1 = fVarList->Find(fVarName[i].c_str());
    fVarPtr = pvar1; // Store one pointer to be able to get the size
                     // later since it may change. This works since all
                     // elements were verified to be the same size.
    if (i == fNvar) continue;
    for( Int_t j = i + 1; j < fNvar; ++j ) {
      if( fVarStat[j] != kFAType )
        continue;
      const auto* pvar2 = fVarList->Find(fVarName[j].c_str());
      if (!pvar1 || !pvar2) {
        status = kArrZer;
        cout << "THaVform:ERROR: Trying to use zero pointer."<<endl;
        continue;
      }
      if( !(pvar1->HasSameSize(pvar2)) ) {
	cout << "THaVform::ERROR: Var "<<fVarName[i]<<"  and ";
        cout << fVarName[j]<<"  have different sizes "<<endl;
        status = kArrSiz;
      }
    }
  }
  if( status != 0 )
    return status;

  for( Int_t i = 0; i < fNvar; ++i ) {
    if( fVarStat[i] == kFAType ) fSarray.push_back(fVarName[i]);
  }

  Int_t varsize = 1;
  if (fVarPtr) varsize = fVarPtr->GetLen();

  status = MakeFormula(0, varsize);

  if (status != 0) return status;

  if (fVarPtr) {
    fObjSize = fVarPtr->GetLen();
    if (fPrefix == kNoPrefix) {
       delete fOdata;
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
  for( auto& itc : fCut ) itc->Compile();
  for( auto& itf : fFormula ) itf->Compile();
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

    string soper;
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

    string sform;
    for (Int_t i = 0; i < fhi; ++i) {
      sform += fVectSform[i];
      if (i < (long)fVectSform.size()-1) sform += soper;
    }

    string cname(GetName());

    if (IsCut()) {
      if( !fCut.empty() ) {  // drop what was there before
        for( auto& itc : fCut ) delete itc;
        fCut.clear();
      }
      cname += "cut";
      //FIXME: avoid duplicate cuts/formulas
      fCut.push_back(new THaCut(cname.c_str(),sform.c_str(),
				"thavsform"));
    } else if (IsFormula()) {
      if( !fFormula.empty() ) {  // drop what was there before
        for( auto& itf : fFormula )
          delete itf;
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
  fAndStr = "AND:";
  fOrStr  = "OR:";
  fSumStr = "SUM:";
  const vector<string> sprefix{ fAndStr, fOrStr, fSumStr };
  static const vector<Int_t> ipflg{ kAnd, kOr, kSum };
// If we ever invent new prefixes, add them here...

  string stitle = string(expr);

  fPrefix = kNoPrefix;
  string result = stitle;
  for (string::size_type i = 0; i < sprefix.size(); ++i) {
     auto pos1 = stitle.find(ToUpper(sprefix[i]),0);
     auto pos2 = stitle.find(ToLower(sprefix[i]),0);
     auto pos  = string::npos;
     if (pos1 != string::npos) pos = pos1;
     if (pos2 != string::npos) pos = pos2;
     if (pos != string::npos) {
       if (fPrefix != kNoPrefix) {
         cerr << "THaVform:ERROR:: Cannot have >1 prefix."<<endl;
       }
       fPrefix = ipflg[i];
       pos = pos + sprefix[i].length();
       result = stitle.substr(pos,pos+stitle.length());
     }
  }

// If the variable is like "L.s1.lt[I]" strip the [I]
// from the end since it is implicit
  const string eye = "[I]";
  string stemp = result;
  auto pos1 = result.find(ToUpper(eye),0);
  auto pos2 = result.find(ToLower(eye),0);
  auto pos = string::npos;
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
  string result;
  auto pos1 = var.find('[',0);
  auto pos2 = var.find(']',0);
  if( (pos1 != string::npos) &&
      (pos2 != string::npos) && pos2 > pos1 ) {
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
  if (fOdata) {
    fOdata->AddBranches(tree, mydata);
  } else {
    assert(!IsVarray() && fObjSize == 1);
    string tinfo;
    tinfo = mydata + "/D";
    tree->Branch(mydata.c_str(), &fData, tinfo.c_str(), 4000);
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
    if( fOdata != nullptr ) {
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
    if( fOdata ) {
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

  if (fDebug) cout << "THaVform::DefinedGlob for name = " << name << endl;

  Int_t ret = THaFormula::DefinedGlobalVariable( name );
  if( ret < 0 )
    return ret;

  // Retrieve some of the results of the THaFormula parser
  FVarDef_t& def = fVarDef[ret];
  const auto* gvar = static_cast<const THaVar*>( def.obj );
  // This makes a certain assumption about the array syntax defined by ROOT
  // and THaArrayString, but in the interest of performance  we don't create
  // a full THaArrayString here just to find out if it is an array element
  Bool_t var_is_array = name.Contains("[");

  fVarName.emplace_back(name.Data());
  FAr stat = kScaler;
  if( gvar->IsArray() ) {

    if( var_is_array ) {
      stat = kAElem;
    } else {
      stat = kFAType;
    }
    if (gvar->GetLen() == 0) {
      stat = kVAType;
    }

  }
  fVarStat.push_back(stat);

  if (fDebug) {
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
  const string open_brack="[";
  const string close_brack="]";

  for (int idx = 0; idx < size; ++idx) {
    string acopy = fStitle;
    string aline = acopy;
    const int LEN = 16;
    char num[LEN];
    snprintf(num,LEN,"%d",idx);
    for( const auto& sb : fSarray ) {
      vector<string::size_type> ipos;
      string::size_type pos1 = 0;
      string::size_type pos = 0;
      aline.clear();
      while( pos != string::npos ) {
        pos = acopy.find(sb, pos1);
        pos1 = pos + sb.length();
        auto pos2 = acopy.find(open_brack, pos1);
        if( pos != string::npos &&
            (pos2 == string::npos || pos2 > pos1))
          ipos.push_back(pos1);
      }
      if( ipos.empty() ) continue;
// Found at least one array,
// now build fSarray for [0], [1], ... size
      if (idx == 0) fVectSform.clear();
      pos = 0;
      for( auto ipo : ipos ) {
        aline.append(acopy.substr(pos, ipo));
        aline.append(open_brack).append(num).append(close_brack);
        pos = ipo;
      }
      aline.append(acopy.substr(ipos[ipos.size()-1],acopy.length()));
      acopy = aline;
    }
    if( aline.length() > 0 )
      fVectSform.push_back(aline);
  }

}


ClassImp(THaVform)
