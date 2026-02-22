#ifndef Podd_THaNamedList_h_
#define Podd_THaNamedList_h_

//////////////////////////////////////////////////////////////////////////
//
// THaNamedList
//
// A list with a name. Used to store blocks of THaCuts (tests) in a 
// THashTable. 
//
//////////////////////////////////////////////////////////////////////////

#include "TList.h"
#include "TNamed.h"

class THaNamedList : public TList {
  
public:
  THaNamedList();
  explicit THaNamedList( const char* name );
  THaNamedList( const char* name, const char* descript );
  ~THaNamedList() override;

          Int_t    Compare( const TObject* obj) const override
    { return fNamed->Compare(obj); }
  virtual void     FillBuffer(char*& buffer)       { fNamed->FillBuffer(buffer); }
          const Text_t*  GetName() const override  { return fNamed->GetName(); }
          const Text_t*  GetTitle() const override { return fNamed->GetTitle(); }
          ULong_t  Hash() const override           { return fNamed->Hash(); }
          Bool_t   IsSortable() const override     { return true; }
  virtual void     PrintOpt( Option_t* opt="" ) const;
  virtual void     SetName(const Text_t *name); // *MENU*
  virtual void     SetNameTitle(const Text_t *name, const Text_t *title);
  virtual void     SetTitle(const Text_t *title="") 
    { fNamed->SetTitle(title); } // *MENU*
          void     ls(Option_t *option="") const override;
          Int_t    Sizeof() const                  { return fNamed->Sizeof(); }

protected:
  TNamed*    fNamed;   //Name of the list

  ClassDefOverride(THaNamedList,0)   //A list with a name
};

#endif

