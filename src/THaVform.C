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
#include "THaArrayString.h"
#include "THaVarList.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "TTree.h"
#include "TROOT.h"

#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace std;

THaVform::THaVform( const char *type, const char* name, const char* formula,
		    const THaVarList* vlst, const THaCutList* clst )
  : THaFormula("[0]", "[0]", vlst, clst)
{
// The title "[0]" and expression "[0]" will be over-written
// depending on the type of THaVform.
  fType = THaString(type);
  SetName(name);
  THaString stemp1 = StripPrefix(formula); 
  SetTitle(stemp1.c_str());
  Clear();
  Compile();  
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
  THaFormula::THaFormula(rhs.GetName(), rhs.GetTitle());
  fType = rhs.fType;
  fNvar = rhs.fNvar;
  fVarPtr = rhs.fVarPtr;
  fObjSize = rhs.fObjSize;
  fOdata = rhs.fOdata;
  fVarName = rhs.fVarName;
  fVarStat = rhs.fVarStat;
  fVectSform = rhs.fVectSform;
  fgAndStr = rhs.fgAndStr;
  fgOrStr = rhs.fgOrStr;
  fgSumStr = rhs.fgSumStr;
  fCut = rhs.fCut;
  fFormula = rhs.fFormula;
}

//_____________________________________________________________________________
void THaVform::Uncreate() 
{
  if (fOdata) delete fOdata;
  for (std::vector<THaCut*>::iterator itc = fCut.begin();
       itc != fCut.end(); itc++) delete *itc;
  for (std::vector<THaFormula*>::iterator itf = fFormula.begin();
       itf != fFormula.end(); itf++) delete *itf;
  fCut.clear();
  fFormula.clear();
}

//_____________________________________________________________________________
void THaVform::LongPrint() 
{ // Long printout for debugging.
  ShortPrint();
  cout << "Num of variables "<<fNvar<<endl;
  cout << "Object size "<<fObjSize<<endl;
  for (Int_t i = 0; i < fNvar; i++) {
      cout << "Var # "<<i<<"    name = "<<
      fVarName[i]<<"   stat = "<<fVarStat[i]<<endl;
  }
  for (std::vector<THaFormula*>::iterator itf = fFormula.begin();
       itf != fFormula.end(); itf++) {
       cout << "Formula full printout --> "<<endl;
       (*itf)->Print(kPRINTFULL);
  }
  for (std::vector<THaCut*>::iterator itc = fCut.begin();
       itc != fCut.end(); itc++) {
       cout << "Cut full printout --> "<<endl;
       (*itc)->Print(kPRINTFULL);
  }
}

//_____________________________________________________________________________
void THaVform::ShortPrint() 
{ // Short printout for self identification.
  cout << "Print of ";
  if (IsVarray()) cout << " variable sized array: ";
  if (IsFormula()) cout << " formula: ";
  if (IsCut()) cout << " cut: ";
  if (IsEye()) cout << " [I]-var: ";
  cout << GetName() << "  = " << GetTitle()<<endl;
}

//_____________________________________________________________________________
void THaVform::ErrPrint(Int_t err)
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
vector<THaString> THaVform::GetArrays() 
{
  // Return all variables that are array elements.
  vector<THaString> result;
  result.clear();
  for (Int_t i = 0; i < fNvar; i++) {
    if (fVarStat[i] == kAElem) result.push_back(fVarName[i]);
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

  fStitle = THaString(GetTitle());
  Int_t status = 0;

  for (Int_t i = 0; i < fNvar; i++) {
    if (fVarStat[i] == kVAType) {  // This obj is a var. sized array.
      if (StripBracket(fStitle) == fVarName[i]) {  
 	 status = 0;
         fVarPtr = gHaVars->Find(fVarName[i].c_str());
         if (fVarPtr) {
           fType = "VarArray";
           fObjSize = fVarPtr->GetLen();
           if (fPrefix == kNoPrefix) {
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
  if (!gHaVars) return kNoGlo;

// Check that all fixed arrays have the same size.

  for (Int_t i = 0; i < fNvar; i++) {

    if (fVarStat[i] == kVAType ) {
      cout << "ERROR:THaVform:: Cannot mix variable arrays with ";
      cout << "other variables or formula."<<endl;
      return kIllMix;
    }
    if (fVarStat[i] != kFAType ) continue; 
    pvar1 = gHaVars->Find(fVarName[i].c_str());
    fVarPtr = pvar1; // Store one pointer to be able to get the size 
                     // later since it may change. This works since all
                     // elements were verified to be the same size.
    if (i == fNvar) continue;
    for (Int_t j = i+1; j < fNvar; j++) {
      if (fVarStat[j] != kFAType ) continue; 
      pvar2 = gHaVars->Find(fVarName[j].c_str());
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

  for (Int_t i = 0; i < fNvar; i++) {
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
  for (Int_t i = 0; i < fNvar; i++) {
    if (fVarStat[i] != kFAType ) continue; 
    fVarPtr = gHaVars->Find(fVarName[i].c_str());
    break;
  }
  for (std::vector<THaCut*>::iterator itc = fCut.begin();
       itc != fCut.end(); itc++) (*itc)->Compile();
  for (std::vector<THaFormula*>::iterator itf = fFormula.begin();
       itf != fFormula.end(); itf++) (*itf)->Compile();
  return;
}


//_____________________________________________________________________________
Int_t THaVform::Clear() 
{
  fNvar = 0;
  fVarPtr = 0;
  fOdata = 0;
  fVarName.clear();
  fVarStat.clear();
  fVectSform.clear();
  return 0;

}

//_____________________________________________________________________________
Int_t THaVform::MakeFormula(Int_t flo, Int_t fhi)
{ // Make the vector formula (fVectSform) from index flo to fhi.
  // Return status :
  //    0 = ok
  // kUnkPre = tried to use an unknown prefix ("OR:", etc)
  // kIllPre = ambiguity of whether this is a cut.

  Int_t status = 0;
  char cname[50],temp[50];

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

  if (fPrefix == kNoPrefix) {

   for (Int_t i = flo; i < fhi; i++) { 
     strcpy(cname,GetName());
     sprintf(temp,"-%d",i);
     strcat(cname,temp);
     if (IsCut()) {
        fCut.push_back(new THaCut(cname,fVectSform[i].c_str(),
               (const char*)"thavcut"));
     } else if (IsFormula()) {
        fFormula.push_back(new THaFormula(cname,fVectSform[i].c_str()));
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
      return status;
      cout << "THaVform:ERROR:: Illegal prefix -- "<<endl;
      cout << "OR:, AND: works only with cuts."<<endl;
      cout << "SUM: works only with both cuts and formulas."<<endl;
    }

    string sform="";
    for (Int_t i = 0; i < fhi; i++) {
      sform += fVectSform[i];
      if (i < (long)fVectSform.size()-1) sform += soper;
    }
    strcpy(cname,GetName());

    if (IsCut()) {
      if ((long)fCut.size() != 0) {  // drop what was there before
         for (std::vector<THaCut* >::iterator itc = fCut.begin();
             itc != fCut.end(); itc++) delete *itc;
         fCut.clear();
      }
      strcat(cname,"cut");
      fCut.push_back(new THaCut(cname,sform.c_str(), 
           (const char*)"thavsform"));
    } else if (IsFormula()) {
      if ((long)fFormula.size() != 0) {  // drop what was there before
         for (std::vector<THaFormula* >::iterator itf = 
           fFormula.begin(); itf != fFormula.end(); itf++) 
               delete *itf;
         fFormula.clear();
      }
      strcat(cname,"form");
      fFormula.push_back(new THaFormula(cname,sform.c_str()));
    }

  }

  return status;
}

//_____________________________________________________________________________
THaString THaVform::StripPrefix(const char* expr)
{
  Int_t pos,pos1,pos2;
  THaString result,stitle;
  vector<THaString> sprefix;
  vector<Int_t> ipflg;
  fgAndStr = "AND";
  fgOrStr  = "OR";
  fgSumStr = "SUM";
  sprefix.push_back(fgAndStr);  ipflg.push_back(kAnd);
  sprefix.push_back(fgOrStr);   ipflg.push_back(kOr);
  sprefix.push_back(fgSumStr);  ipflg.push_back(kSum);
// If we ever invent new prefixes, add them here...

  stitle = THaString(expr);

  fPrefix = kNoPrefix;
  result = stitle;
  for (int i = 0; i < (long)sprefix.size(); i++) {
     pos1 = stitle.find(sprefix[i].ToUpper(),0);
     pos2 = stitle.find(sprefix[i].ToLower(),0);
     pos  = string::npos;
     if (pos1 != string::npos) pos = pos1;
     if (pos2 != string::npos) pos = pos2;
     if (pos != string::npos) {
       if (fPrefix != kNoPrefix) {
         cout << "THaVform:ERROR:: Cannot have >1 prefix."<<endl;
       }
       fPrefix = ipflg[i];
       pos = pos + sprefix[i].length() + 1;
       result = stitle.substr(pos,pos+stitle.length());
     }
  }

// If the variable is like "L.s1.lt[I]" strip the [I]
// from the end since it is implicit
  THaString eye = "[I]";
  THaString stemp = result;
  pos1 = result.find(eye.ToUpper(),0);
  pos2 = result.find(eye.ToLower(),0);
  pos = string::npos;
  if (pos1 != string::npos) pos = pos1;
  if (pos2 != string::npos) pos = pos2;
  if (pos  != string::npos) result = stemp.substr(0,pos);
  return result;
}

//_____________________________________________________________________________
THaString THaVform::StripBracket(THaString& var) const
{
// If the string contains "[anything]", we strip
// it away.
  Int_t pos1,pos2;
  THaString open_brack = "[";
  THaString close_brack = "]";
  THaString result = "";
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
    THaString tinfo;
    tinfo = mydata + "/D";
    THaString tmp = mydata;
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
  if (IsEye()) return 0;  

  if (IsVarray()) {

    switch (fPrefix) {

      case kAnd:
      case kOr:
        break;   // nonsense for a variable sized array

      case kSum:
        for (Int_t i = 0; i < fVarPtr->GetLen(); i++) {
          fData += fVarPtr->GetValue(i);
	}
        fObjSize = 1;
        break;

      default:
	if (fOdata) {
          fObjSize = fVarPtr->GetLen();
          for (Int_t i = 0; i < fVarPtr->GetLen(); i++) {
	    if (fOdata->Fill(i,fVarPtr->GetValue(i)) != 1) {
	      cout << "THaVform::ERROR: storing too much";
	      cout << " variable sized data: ";
	      cout << fVarPtr->GetName() <<endl;
	    }
	  }
	}
    }

  }

  if (IsFormula()) {
    if ((long)fFormula.size() != 0) {
      if ( !fFormula[0]->IsError() ) {
        fData = fFormula[0]->Eval();
      }
    }
    for (Int_t i = 0; i < (long)fFormula.size(); i++) {
      if ( fOdata != 0 && !fFormula[i]->IsError()) {
         fOdata->Fill(i,fFormula[i]->Eval());
      }
    }
    return 0;
  }

  if (IsCut()) {
    fData = 0;
    if ((long)fCut.size() != 0) {
      if (!fCut[0]->IsError()) {
        if (fCut[0]->EvalCut()) fData = 1;
      }
    }
    for (Int_t i = 0; i < (long)fCut.size(); i++) {
      if ( fOdata != 0 && !fCut[i]->IsError() ) {
        if (fCut[i]->EvalCut()) {
	  fOdata->Fill(i,1.0);  // 1 = true
        } else {
	  fOdata->Fill(i,0.0);  // 0 = false
        }
      }
    }
    return 0;
  }

  return 0;
}


//_____________________________________________________________________________
Double_t THaVform::GetData(Int_t index) 
{
   if (IsEye()) return (Double_t)index;
   if (fOdata) {
     if (fObjSize <= 1) return fOdata->Get();
     return fOdata->Get(index);
   } else {
     return fData;
   }
};


//_____________________________________________________________________________
Int_t THaVform::DefinedGlobalVariable( const TString& name )
{
  // We use the parsing functionality of THaFormula to 
  // tell if the variables are global variables and, if so,
  // if they are arrays or elements of arrays.

  if (fgDebug) cout << "THaVform::DefinedGlob for name = "<<name<<endl;
  fVarName.push_back(name.Data());
  fVarStat.push_back(kScaler);
  fNvar++;

  if( !fVarList ) 
    return -1;
  if( fNcodes >= kMAXCODES )
    return -6;

  // Parse name for array syntax
  
  THaArrayString var(name);
  if( var.IsError() ) return -2;

  // Find the variable with this name
  const THaVar* obj = fVarList->Find( var.GetName() );
  if( !obj ) 
    return -3;

  // Error if array requested but the corresponding variable is not an array
  if( var.IsArray() && !obj->IsArray() )
    return -4;

  Int_t index = fNvar-1;
  fVarStat[index] = kScaler;
  if (obj->IsArray()) {
    if (var.IsArray()) {
      fVarStat[index] = kAElem;
    } else {
      fVarStat[index] = kFAType;
    }
    if (obj->GetLen() == 0) fVarStat[index] = kVAType;
  }

  if (fgDebug) {
    if (obj->IsArray()) cout << "obj is array"<<endl;
    if (obj->IsBasic()) cout << "obj is basic"<<endl;
    if (obj->IsPointerArray()) cout << "obj is pointer array"<<endl;
    cout << "Here is obj print "<<endl;
    obj->Print();
    cout << "end of obj print "<<endl<<endl;
    cout << "length of var  "<< obj->GetLen()<<endl;
    cout << "fVarStat "<<fVarStat[index]<<endl;
  }

  // Subscript(s) within bounds?
  index = 0;
  if( var.IsArray() 
      && (index = obj->Index( var )) == kNPOS ) return -5;

  // Check if this variable already used in this formula
  FVarDef_t* def = fVarDef;
  for( Int_t i=0; i<fNcodes; i++, def++ ) {
    if( obj == def->code && index == def->index )
      return i;
  }
  // If this is a new variable, add it to the list
  def->type = kVariable;
  def->code = obj;
  def->index = index;

  // No parameters ever for a THaVform/THaFormula
  fNpar = 0;

  return fNcodes++;
}


//_____________________________________________________________________________
void THaVform::GetForm(Int_t size)
{

  THaString open_brack="[";
  THaString close_brack="]";
  Int_t pos,pos1,pos2;
  vector<Int_t> ipos;

  char num[10]; 
  THaString aline,acopy;
  for (int idx = 0; idx < size; idx++) {
    acopy = fStitle;
    aline = acopy;
    sprintf(num,"%d",idx);
    for (vector<THaString>::iterator is = fSarray.begin();
        is != fSarray.end(); is++) {
      THaString sb = *is;
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
      for (int j = 0; j < (long)ipos.size(); j++) {
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

