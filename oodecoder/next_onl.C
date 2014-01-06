{
#include <iostream>
// next_onl.C :  Continue running....

int nevent;
cout << "number of events ? "<<endl;
cin >> nevent;

THaOnlRun* run = new THaOnlRun( "adaqs2", "tst1", 1);
analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
cout << "set out file  "<<endl<<flush;
run->SetLastEvent( nevent );

}




