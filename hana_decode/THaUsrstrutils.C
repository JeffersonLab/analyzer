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
#include <string>

ClassImp(THaUsrstrutils)

const int           THaUsrstrutils::MAX     = 5000;
const unsigned long THaUsrstrutils::LONGMAX = 0x7FFFFFFF;

THaUsrstrutils::THaUsrstrutils()
{
   max_size_string = MAX;
};

THaUsrstrutils::~THaUsrstrutils()
{
};

int THaUsrstrutils::getflag(char *s)
{
  char *pos,*val;

  getflagpos(s,&pos,&val);
  if(!pos) return(0);
  if(!val) return(1);
  return(2);
};

char* THaUsrstrutils::getstr(char *s){
  char *pos,*val;
  char *end;
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

  ret = (char *) malloc(slen+1);
  strncpy(ret,val,slen);
  ret[slen] = '\0';
  return(ret);
};

unsigned int THaUsrstrutils::getint(char *s)
{
  char *sval;
  unsigned int retval;
  sval = getstr(s);
  if(!sval) return(0);		/* Just return zero if no value string */
  retval = strtol(sval,0,0);
  if(retval == LONGMAX && (sval[1]=='x' || sval[1]=='X')) {/* Probably hex */
     sscanf(sval,"%x",&retval);
   }
  free(sval);
  return(retval);
};

void THaUsrstrutils::getflagpos_instring(char *constr, char *s,char **pos_ret,char **val_ret)
{
  int slen;
  char *pos;

  slen=strlen(s);
  pos = constr;
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
};
  
void THaUsrstrutils::getflagpos(char *s,char **pos_ret,char **val_ret)
{
  getflagpos_instring(file_configusrstr,s,pos_ret,val_ret);
  if(*pos_ret) return;
  return;
};

// Routine string_from_evbuffer loads the file_configusrstr
// from the event buffer.
// It has the same strengths and weaknesses as the DAQ code,
// hence the interpretation should be the same.

void THaUsrstrutils::string_from_evbuffer(int *evbuffer, int nlen)
{
     max_size_string = nlen;
     string_from_evbuffer(evbuffer);
};

void THaUsrstrutils::string_from_evbuffer(int *evbuffer)
{
     static const int DEBUG = 0;
     union cidata {
        int ibuff[MAX];
        char cbuff[MAX];
     } evbuff; 
     int nmax;
     nmax = (max_size_string < MAX) ? max_size_string : MAX;
     for (int j=0; j < nmax; j++) evbuff.ibuff[j] = evbuffer[j];
     string strbuff;
     char *ctmp = new char[sizeof(evbuff.cbuff)+1];
     memcpy(ctmp, evbuff.cbuff, sizeof(evbuff.cbuff));
     strbuff = ctmp;
     int pos1 = 1;
     int iamlost = 0;
     int ifound = 0;
     while (pos1 > 0) {
       pos1 = strbuff.find("\n",pos1+1);
       int pos2 = strbuff.rfind("\n",pos1-1);
       int pos3 = strbuff.rfind(";",pos1);
       if (iamlost++ > MAX) {
             delete ctmp;
             return;         // should not happen...
       }
       if (DEBUG) {
         cout << "strbuff = " << strbuff << endl;
         cout << "strbuff pos " <<pos1<<" "<<pos2<<" "<<pos3<<endl;
       }
       int tncomm = strbuff.find(";",pos2);
       if (DEBUG) cout << "next comment  " << tncomm << endl;
       if (pos2 > pos3) {
          ifound = 1;
          strbuff.assign(strbuff,pos2+1,tncomm);
          if (DEBUG) cout << "strbuff (pos2 > pos3) = " << strbuff << endl;
          break;
       }
     }
     if(ifound) {
        file_configusrstr = new char[strbuff.size()+100];
        strcpy(file_configusrstr,strbuff.c_str());
        while(*file_configusrstr && isspace(*file_configusrstr)) {
	   file_configusrstr++;
	}
        if (DEBUG) cout << "file_configusrstr = \n"<<file_configusrstr<<endl;
     } else {
        file_configusrstr = (char *) malloc(1);
        file_configusrstr[0] = '\0';
     }
     delete ctmp;
     return;
};
      
// This routine reads a file to load file_configustr.
// It is like what is used by the DAQ code to load prescale factors, etc.

void THaUsrstrutils::string_from_file(char *ffile_name)
{
  FILE *fd;
  char s[256], *flag_line;

/* check that filename exists */
  fd = fopen(ffile_name,"r");
  if(fd == NULL) {
    printf("Failed to open usr flag file %s\n",ffile_name);  
    file_configusrstr = (char *) malloc(1);
    file_configusrstr[0] = '\0';
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
      file_configusrstr = (char *) malloc(strlen(flag_line)+1);
      strcpy(file_configusrstr,flag_line);
      cout << "file_configustr =  " << file_configusrstr << endl;
    } else {
      file_configusrstr = (char *) malloc(1);
      file_configusrstr[0] = '\0';
    }
    fclose(fd);
  }

};



