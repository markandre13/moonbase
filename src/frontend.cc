// berlin, 5 am
// you just take your pills and you'll be fine
//                              (dance or die)

/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1995 Mark-André Hopf                                        *
 *  TCL Interface Definitionen von Gita Siegert                             *
 *  filename: frontend.cpp                                                  *
 *  relation: database.cpp                                                  *
 *  author  : Mark-André Hopf                                              *
 *  date    : 13.04.1996                                                    *
 *                                                                          *
 * This is an simple frontend for MoonBase with dBASE like commands.        *
 *                                                                          *
 *--------------------------------------------------------------------------*
 *                                                                          *
 * 15.02.1995 : 6 Befehle probeweise implementiert                          *
 * 16.02.1995 : An die Pipe Kommunikation mit Tcl angepasst                 *
 * 25.02.1995 : Weitere Funktionen ueber 'PRINT'                            *
 * 26.02.1995 : SELECT notduerftig installiert und bemerkt, dass dies eine  *
 *              innere Funktion von 'database' sein sollte.                 *
 * 31.03.1995 : Kommandos zum Record Locking aufgenommen.                   *
 * 03.04.1995 : scanf durch fgets ersetzt, SELECT wieder entfernt           *
 * 12.04.1995 : COPY MEMO <memofield> TO <filename> [ADDITIVE]              *
 *              Die wahnsinnige switch-Anweisung macht deutlich, dass ich   *
 *              mal etwas Gehirnschmalz in den Parser investieren sollte    *
 * 16.04.1995 : DELETED,DELETE und RECALL hinzugefuscht                     *
 *                                                                          *
 ****************************************************************************/
const char *FRONTEND_VERSION   = "v0.01 16.04.1996";

//#include <typeinfo>
#include <cstddef>
#include <cstring>
#include <moonbase/owndef.hh>
#include <moonbase/dbheader.hh>
#include <moonbase/dbmemoio.hh>
#include <moonbase/dblcklst.hh>
#include <moonbase/dbfileio.hh>
#include <moonbase/dbdata.hh>
#include <moonbase/moonbase.hh>

#include <fstream.h>
#include <iomanip.h>
#include <stdlib.h>
#include <new.h>
#ifdef __MSDOS__
  #include <alloc.h>
#endif
#include <stdio.h>
#include <time.h>

inline void dbResult(bool f){printf(f?".T.\n":".F.\n");}

/****************************************************************************
 *                                                                          *
 * LANGUAGE DEFINITION                                                      *
 *                                                                          *
 ****************************************************************************/
struct lang_token
{
  const char  *key;   // Schluesselwort
  short token;        // Nummer des zugehoerigen Tokens
  const char *syntax; // Beschreibung der Ein-/Ausgabe Parameter:
                      // [0]  : Rueckgabewert
                      // [1..]: Aufrufparameter
                      // v: void, kein Wert
                      // k: ein weiters Schluesselwort
                      // s: ein String
                      // n: eine Nummer
                      // b: bool
                      // Kleinbuchstaben bedeuten optional und Grossbuchstaben,
                      // dass ein Wert ausgewertet, bzw. uebergeben werden muss
};

enum {TKN_UNKNOWN, TKN_QUIT, TKN_USE, TKN_PRINT, TKN_SKIP, TKN_GO,
      TKN_REPLACE, TKN_UNLOCK, TKN_COPY, TKN_APPEND, TKN_DELETE, TKN_RECALL,

      TKN_LIST, TKN_HELP,

      TKN_STRINGCONST, TKN_TRUECONST, TKN_FALSECONST, TKN_SYMBOL, TKN_NOTOKEN,

      TK2_STRUCTURE, TK2_WITH, TK2_ALL, TK2_TO, TK2_ADDITIVE,
      TK2_MEMO, TK2_BLANK,

      TK2_RECCOUNT, TK2_FIELD, TK2_FIELDNO, TK2_RECNO, TK2_EOF, TK2_BOF,
      TK2_DBF, TK2_RLOCK, TK2_DELETED
     };

lang_token language[]=
{
  {""         , TKN_UNKNOWN   ,"v"},     // must be the first in list !!!

  // commands
  {"QUIT"     , TKN_QUIT      ,"v"},
  {"USE"      , TKN_USE       ,"v"},
  {"?"        , TKN_PRINT     ,"v"},
  {"PRINT"    , TKN_PRINT     ,"v"},
  {"SKIP"     , TKN_SKIP      ,"v"},
  {"GO"       , TKN_GO        ,"v"},
  {"REPLACE"  , TKN_REPLACE   ,"v"},
  {"UNLOCK"   , TKN_UNLOCK    ,"v"},
  {"LIST"     , TKN_LIST      ,"v"},
  {"HELP"     , TKN_HELP      ,"v"},
  {"COPY"     , TKN_COPY      ,"v"},
  {"APPEND"   , TKN_APPEND    ,"v"},
  {"DELETE"   , TKN_DELETE    ,"v"},
  {"RECALL"   , TKN_RECALL    ,"v"},

  // functions
  {"RECCOUNT" , TK2_RECCOUNT  ,""},
  {"FIELD"    , TK2_FIELD     ,""},
  {"FIELDNO"  , TK2_FIELDNO   ,""},
  {"RECNO"    , TK2_RECNO     ,""},
  {"RLOCK"    , TK2_RLOCK     ,""},
  {"BOF"      , TK2_BOF       ,""},
  {"EOF"      , TK2_EOF       ,""},
  {"DBF"      , TK2_DBF       ,""},
  {"DELETED"  , TK2_DELETED   ,""},

  // xtras
  {"STRUCTURE", TK2_STRUCTURE ,"v"},
  {"WITH"     , TK2_WITH      ,"v"},
  {"MEMO"     , TK2_MEMO      ,"v"},
  {"TO"       , TK2_TO        ,"v"},
  {"ADDITIVE" , TK2_ADDITIVE  ,"v"},
  {"ALL"      , TK2_ALL       ,"v"},
  {"BLANK"    , TK2_BLANK     ,"v"},
};

const int nTokens = sizeof(language)/sizeof(lang_token);

struct idelem
{
  char* str;
  int   len;
  WORD  token;
  const char* syntax;
} idlist[100];
WORD idptr;
char idstring[256];

void frontend();
bool gettoken(idelem*);
const char* disolve(char*);

/****************************************************************************
 *                                                                          *
 * MAIN                                                                     *
 *                                                                          *
 ****************************************************************************/

void mem_warn();

int main()
{
  #ifdef DEBUG
  FILE *handle=fopen("database.log","w");
  time_t t;
  time(&t);
  fprintf(handle,"%s\n",ctime(&t));
  fclose(handle);
  ERRORMSG("entered main()\n");
  #endif

  set_new_handler(mem_warn);

  frontend();

  ERRORMSG("finished main()\n");
  return 0;
}

/****************************************************************************
 *                                                                          *
 * FRONTEND                                                                 *
 *                                                                          *
 ****************************************************************************/
void frontend()
{
  printf("Frontend for MoonBase %s\n"
         "(based upon MoonBase %s)\n"
         "(C)opyright 1993/95 MAGIC INC.\n"
         "(C)opyright 1993/95 Mark-Andre'Hopf\n\n"
         "there is a 'help'-command...\n\n",
         FRONTEND_VERSION, MOONBASE_VERSION );
  fflush(stdout);

  database db;

  bool quit=false;
  while(!quit)
  {
    char input[256];
    const char* error;
    //lang_token* token_1st;
    //lang_token* token_2nd;

    // read input
    fflush(stdout);
    printf(">");
    fflush(stdout);
    fgets(input,255,stdin);

    // disolve input
    if ((error=disolve(input))!=NULL)
    {
      printf("ERROR: %s\n",error);
      continue;
    }
    if (idptr==0) continue; // (empty line)

    gettoken(&idlist[0]);

    switch(idlist[0].token)
    {
      case TKN_UNKNOWN:
      case TKN_NOTOKEN:
        if (idlist[0].len>0)
          printf("ERROR: \"%s\" is an unknown keyword\n",idlist[0].str);
        break;
      case TKN_QUIT:
      	printf("Bye...\n");
        quit=true;
        break;
      case TKN_USE:
        if (idptr==1 || *idlist[1].str=='\0')
        {
          db.Use();
          break;
        }
        if (!db.Use(idlist[1].str))
          printf("%s\n",db.lasterror());
        break;
      case TKN_REPLACE:
        {
          int i;
          gettoken(&idlist[2]);
          if (idlist[2].token!=TK2_WITH)
          {
            printf("ERROR: syntax error\n");
            break;
          }
          i=db.FieldNo(idlist[1].str);
          if (i==0)
          {
            printf("ERROR: %s is an unknown field\n.T.\n",idlist[1].str);
            break;
          }
          if (!db.Replace(i,idlist[3].str))
            printf("%s\n",db.lasterror());
          break;
        }
      case TKN_PRINT:
        {
          int i;
          if (idlist[1].token==TKN_STRINGCONST)
          {
            printf("%s\n",idlist[1].str);
            break;
          }
          gettoken(&idlist[1]);
          switch(idlist[1].token)
          {
            case TK2_FIELD:
              i=atoi(idlist[2].str);
              if (db.Field(i)!=NULL)
                printf("%s\n",db.Field(i));
              else
                printf("\n");
              break;
            case TK2_FIELDNO:
              printf("%i\n",db.FieldNo(idlist[2].str));
              break;
            case TK2_RECCOUNT:
              printf("%i\n",db.RecCount());
              break;
            case TK2_RECNO:
              printf("%lu\n",db.RecNo());
              break;
            case TK2_BOF:
              dbResult(db.Bof());
              break;
            case TK2_EOF:
              dbResult(db.Eof());
              break;
            case TK2_DBF:
              if (db.Dbf()==NULL)
                printf("\n");
              else
                printf("%s\n",db.Dbf());
              break;
            case TK2_RLOCK:
              dbResult(db.RLock());
              break;
            case TK2_DELETED:
              dbResult(db.Deleted());
              break;
            default:
              i=db.FieldNo(idlist[1].str);
              if (i==0)
                printf("ERROR: \"%s\" is an unknown keyword\n",idlist[1].str);
              else
                printf("%s\n",(char*)db(i));
          }
        }
        break;
      case TKN_SKIP:
        dbResult(db.Skip());
        break;
      case TKN_DELETE:
        dbResult(db.Delete());
        break;
      case TKN_RECALL:
        dbResult(db.Recall());
        break;
      case TKN_GO:
        dbResult(db.Go(atoi(idlist[1].str)));
        break;
      case TKN_UNLOCK:
        switch(idptr)
        {
          case 1:
            dbResult(db.Unlock());
            break;
          case 2:
            gettoken(&idlist[1]);
            if(idlist[1].token==TK2_ALL)
            {
              dbResult(db.UnlockAll());
            }
          default:
            printf("ERROR: Syntax error\n");
        }
        break;
      case TKN_COPY:
        if (idptr<5 || idptr>6)
        {
          printf("ERROR: Syntax error\n.F.\n");
          break;
        }
        gettoken(&idlist[1]);
        gettoken(&idlist[3]);
        if (idptr==6)
        {
          gettoken(&idlist[5]);
          if (idlist[5].token!=TK2_ADDITIVE)
          {
            printf("ERROR: Syntax error\n");
            break;
          }
        }
        if (idlist[1].token!=TK2_MEMO && idlist[3].token!=TK2_TO)
        {
          printf("ERROR: Syntax error\n");
          break;
        }

        if ( !db.CopyMemoTo(idlist[2].str,idlist[4].str,idptr==6?true:false) )
          printf("ERROR: %s.\n",db.lasterror());
        break;
      case TKN_APPEND:
        gettoken(&idlist[1]);
        switch(idlist[1].token)
        {
          case TK2_BLANK:
            if (!db.AppendBlank())
              printf("ERROR: %s.\n",db.lasterror());
            break;
          default:
            printf("ERROR: unknown argument\n");
        }
        break;
      case TKN_LIST:
        gettoken(&idlist[1]);
        switch(idlist[1].token)
        {
          case TK2_STRUCTURE:
            db.ListStructure();
            break;
          default:
            printf("ERROR: unknown argument\n");
        }
        break;
      case TKN_HELP:
        printf("The following commands are, in a way, ready to use:\n"
         "  ?/PRINT ...         : print a field, a functions result or string\n"
         "  APPEND BLANK        : append a new blank record\n"
         "  COPY MEMO <field> TO <file> [ADDITIVE]:\n"
         "  DELETE              : delete record\n"
         "  GO <recordnumber>   : select a record\n"
         "  LIST STRUCTURE      : list the structure of the opened db-file\n"
         "  RECALL              : recall deleted record\n"
         "  REPLACE <f> WITH <s>: replace contents of field f with s\n"
         "  SKIP                : go to the next record\n"
         "  UNLOCK              : unlock record\n"
         "  UNLOCKALL           : unlock all records\n"
         "  USE <filename>      : open db-file\n"
         "  QUIT                : leave this program\n"
         "Functions:\n"
         "  BOF                 : .T. = begin of file\n"
         "  DBF                 : name of the dbFile\n"
         "  DELETED             : .T. = record is marked as deleted\n"
         "  EOF                 : .T. = end of file\n"
         "  FIELD <nr>          : name of field nr\n"
         "  FIELDNO <fieldname> : number of field fieldname\n"
         "  RECCOUNT            : number of records\n"
         "  RLOCK               : lock record, .T. on success\n\n");
        break;
      case TK2_STRUCTURE:
        printf("ERROR: syntax error\n");
        break;
      default:
        if (idlist[0].syntax[0]!='v')
          printf("ERROR: %s is a function, use PRINT %s\n",
          idlist[0].str,idlist[0].str);
        else
          printf("ERROR: keyword is not implemented yet\n");
        break;
    }
  }
}

/****************************************************************************
 *                                                                          *
 * gettoken                                                                 *
 * Sucht das zum uebergebenen Schluesselwort passende Token heraus.         *
 *                                                                          *
 ****************************************************************************/
bool gettoken(idelem *id)
{
  lang_token* ptoken_def=language;
  int token_nr=0;

  if (id->token!=TKN_UNKNOWN) return false;
  while(token_nr<nTokens)
  {
    if (strnicmp(id->str, ptoken_def->key, id->len)==0)
    {
      // id->str   =ptoken_def->key;
      id->token =ptoken_def->token;
      id->syntax=ptoken_def->syntax;
      return true;
    }
    ptoken_def++;
    token_nr++;
  }
  id->token=TKN_NOTOKEN;
  return false;
}


/****************************************************************************
 *                                                                          *
 * diverses aus der Entstehungszeit...                                      *
 *                                                                          *
 ****************************************************************************/

void mem_warn()
{
  printf("\nnew failed to allocate memory\n");
  #ifdef __MSDOS__
    printf("heap   : %lu Bytes free\n", (ULONG)coreleft());
    printf("farheap: %lu Bytes free\n", farcoreleft());
  #endif
  exit(1);
}

const char* disolve(char* input)
{
  int pos=0;
  idptr=0;
  int len;
  bool b;
  bool bEOW;
  bool bEOL=false;
  do
  {
    // skip whitespace
    while(input[pos]==' ' || input[pos]=='\t') pos++;

    // analyse word
    bEOW=false;                     // End Of Word = false
    len=0;                          // lenght of the word is now zero
    idlist[idptr].str  =input+pos;  // store begining of the word
    idlist[idptr].token=TKN_UNKNOWN;// words token is now unknown
    do
    {
      switch(input[pos])
      {
        // end of word
        case '\n':
        case '\0':
          bEOL=true;
        case ' ':
        case '\t':
          bEOW=true;
          input[pos]='\0';          // terminate previous string
          pos++;                    // skip the terminating symbol
          break;
        // special single char symbols that can mark an end of word
        case '(':
        case ')':
        case '+':
        case '-':
        case '*':
        case '/':
        case ',':
        case ';':
        case '!':
        case '<':
        case '>':
        case '=':
        case '@':
          if (len==0) // 1st char: take it as an own word..
          {
            idlist[idptr].token=TKN_SYMBOL;
            input[pos]='\0';  // terminate previous string
                              // TKN_SYMBOL is not enough identification
            pos++;
          }
          bEOW=true;   // ..otherwise take as the end of the previous word
          break;
        // special multiple char symbols that can mark an end of word
        case '.':
          if (len==0)
          {
            b=false;
            switch(input[pos+1])
            {
              case't':case'T':
                b=true;
              case'f':case'F':
                if (input[pos+2]=='.')
                {
                  input[pos]='\0'; // terminate previous string
                  idlist[idptr].token=b?TKN_TRUECONST:TKN_FALSECONST;
                  len=3;
                  pos+=3;
                  bEOW=true;
                  break;
                }
            }
          }
          break;
        // string constant : BEGIN
        case '"':
          if (len!=0)             // end of an previous string
          {
            bEOW=true;
            break;
          }
          input[pos]='\0';        // terminate previous string
          pos++;
          idlist[idptr].str=input+pos;
          idlist[idptr].token=TKN_STRINGCONST;
          do
          {
            switch(input[pos])
            {
              case '\0':
              case '\n':
                bEOL=true;
                return "Unterminated string or character constant";
              case '"':
                input[pos]='\0';
                pos++;
                bEOW=true;
                break;
              default:
                len++;
                pos++;
            }
          }while(!bEOW);
          break;
        // string constant : END
      }
      pos++;
      len++;
    }while(!bEOW&&!bEOL);
    pos--;
    len--;
    idlist[idptr].len=len;
    idptr++;
  }while(!bEOL);
  return NULL;
}
