/////////////////////////////////////////////////////////////////////
//
// R. Michaels, March 2000
// THaUsrstrutils = USeR STRing UTILitieS.
// The code below is what is used by DAQ to interpret strings
// like prescale factors 
// Yes, this is mostly old-style C, but it
// has the advantage that the interpretation should
// be identical to what the DAQ used.  
//
/////////////////////////////////////////////////////////////////////

#include "THaUsrstrutils.h"
#include "TMath.h"
#include "TRegexp.h"
#include <cctype>

using namespace std;

const int           THaUsrstrutils::MAX     = 5000;
const unsigned long THaUsrstrutils::LONGMAX = 0x7FFFFFFF;
static const int DEBUG = 0;

int THaUsrstrutils::getflag(const char *s) const
{
  const char *pos,*val;

  getflagpos(s,&pos,&val);
  if(!pos) return(0);
  if(!val) return(1);
  return(2);
}

char* THaUsrstrutils::getstr(const char *s) const
{
  const char *pos,*val;
  const char *end;
  char *ret;
  int slen;

  getflagpos(s,&pos,&val);
  if(!val){
    return(0);
  }
  end = strchr(val,',');	/* value string ends at next keyword */
  if(end)
    slen = end - val;
  else				/* No more keywords, value is rest of string */
    slen = strlen(val);

  ret = new char[slen+1];
  strncpy(ret,val,slen);
  ret[slen] = '\0';
  return(ret);
}

unsigned int THaUsrstrutils::getint(const char *s) const
{
  char* sval = getstr(s);
  if(!sval) return(0);		/* Just return zero if no value string */
  unsigned int retval = strtol(sval,0,0);
  if(retval == LONGMAX && (sval[1]=='x' || sval[1]=='X')) {/* Probably hex */
     sscanf(sval,"%x",&retval);
   }
  delete [] sval;
  return(retval);
}

void THaUsrstrutils::getflagpos_instring(const char *constr, const char *s,
					 const char **pos_ret, const char **val_ret)
{
  int slen = strlen(s);
  const char *pos = constr;

  while(pos) {
    pos = strstr(pos,s);
    if(pos) {			/* Make sure it is really the keyword */
      /* Make sure it is an isolated keyword */
      if((pos != constr && pos[-1] != ',') ||
	 (pos[slen] != '=' && pos[slen] != ',' && pos[slen] != '\0')) {
	pos += 1;	continue;
      } else break;		/* It's good */
    }
  }
  *pos_ret = pos;
  if(pos) {
    if(pos[slen] == '=') {
      *val_ret = pos + slen + 1;
    } else 
      *val_ret = 0;
  } else
    *val_ret = 0;
  return;
}
  
// Routine string_from_evbuffer loads the configstr from the event buffer.
// It has the same strengths and weaknesses as the DAQ code,
// hence the interpretation should be the same.

void THaUsrstrutils::string_from_evbuffer(const int *evbuffer, int nlen )
{
//       union cidata {
//          int ibuff[MAX];
//          char cbuff[MAX];
//       } evbuff; 
//       int nmax;
//       nmax = (max_size_string < MAX) ? max_size_string : MAX;
//       for (int j=0; j < nmax; j++) evbuff.ibuff[j] = evbuffer[j];
//       string strbuff;
//       char *ctmp = new char[sizeof(evbuff.cbuff)+1];
//       memcpy(ctmp, evbuff.cbuff, sizeof(evbuff.cbuff));
//       strbuff = ctemp;
//
// If evbuffer always terminated (as implied above):
//     string strbuff = (const char*)evbuffer;  
// but since we can't be sure:
     int  bufsize = TMath::Min(nlen,MAX);
     char* ctemp = new char[bufsize+1];
     memcpy(ctemp,evbuffer,bufsize);
     ctemp[bufsize] = 0;  // terminate C-string for sure
     TString strbuff = ctemp;
     delete [] ctemp;
     Ssiz_t pos1, ext;
     // Note: These expressions designed to provide backwards-compatibility.
     // They will skip lines that have text before the comment char ";".
     TRegexp re1("\n[^;]*\n[^;]*");
     TRegexp re2("^[^;]*\n[^;]*");
     TRegexp re3("\n[^;]*$");
     TRegexp re4("^[^;]*$");
     if( (pos1 = re1.Index(strbuff,&ext)) != kNPOS ||
	 (pos1 = re2.Index(strbuff,&ext)) != kNPOS || 
	 (pos1 = re3.Index(strbuff,&ext)) != kNPOS ||
	 (pos1 = re4.Index(strbuff,&ext)) != kNPOS ) {
       strbuff = strbuff(pos1,ext); //note: may contain a leading \n, stripped below
       const char* p = strbuff.Data();
       while(isspace(*p)) p++;   //Removes leading \n's too
       configstr = p;
       if (DEBUG) cout << "configstr = \n"<<configstr<<endl;
     } else {
       configstr = "";
     }
     return;
}
      
// This routine reads a file to load file_configustr.
// It is like what is used by the DAQ code to load prescale factors, etc.

void THaUsrstrutils::string_from_file(const char *ffile_name)
{
  FILE *fd;
  char s[256], *flag_line;

/* check that filename exists */
  configstr = "";
  fd = fopen(ffile_name,"r");
  if(fd == NULL) {
    printf("Failed to open usr flag file %s\n",ffile_name);  
  } else {
    /* Read till an uncommented line is found */
    flag_line = 0;
    while(fgets(s,255,fd)){
      char *arg;
      arg = strchr(s,COMMENT_CHAR);
      if(arg) *arg = '\0'; /* Blow away comments */
      arg = s;			/* Skip whitespace */
      while(*arg && isspace(*arg)){
	arg++;
      }
      if(*arg) {
	flag_line = arg;
	break;
      }
    }
    if(flag_line) {		/* We have a config usrstr */
      configstr = flag_line;
      if(DEBUG) cout << "configstr =  " << configstr << endl;
    }
    fclose(fd);
  }
}


ClassImp(THaUsrstrutils)

