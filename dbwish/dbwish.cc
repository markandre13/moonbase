/*****************************************************************************
 *                                                                           *
 * MoonBase - a dBASE compatible database for C++                            *
 * dbwish - a Tcl/Tk window shell with MoonBase                              *
 * (C)opyright 1995 by Mark-André Hopf                                       *
 * (C)opyright 1995 by MAGIC INCLUDED RESEARCH                               *
 *                                                                           *
 * dBASE is a (TM) of Borland International, Inc.                            *
 *                                                                           *
 *****************************************************************************/

// wake up in the dark, the aftertaste of anger in my mouth...

#include <tk.h>
#include <moonbase.hh>
#include <stdio.h>

database db;

int DB_Init(Tcl_Interp *interp);

int dbUse				(ClientData,Tcl_Interp*,int,char**);
int dbDbf				(ClientData,Tcl_Interp*,int,char**);
int dbSelect		(ClientData,Tcl_Interp*,int,char**);
int dbCreateBegin(ClientData,Tcl_Interp*,int,char**);
int dbCreateAdd	(ClientData,Tcl_Interp*,int,char**);
int dbCreateEnd	(ClientData,Tcl_Interp*,int,char**);

int dbRead			(ClientData,Tcl_Interp*,int,char**);
int dbReplace		(ClientData,Tcl_Interp*,int,char**);
int dbAppendBlank(ClientData,Tcl_Interp*,int,char**);
int dbGo				(ClientData,Tcl_Interp*,int,char**);
int dbSkip			(ClientData,Tcl_Interp*,int,char**);
int dbEof				(ClientData,Tcl_Interp*,int,char**);
int dbBof				(ClientData,Tcl_Interp*,int,char**);

int dbRecCount	(ClientData,Tcl_Interp*,int,char**);
int dbRecNo			(ClientData,Tcl_Interp*,int,char**);
int dbFldCount	(ClientData,Tcl_Interp*,int,char**);
int dbField			(ClientData,Tcl_Interp*,int,char**);
int dbFieldNo		(ClientData,Tcl_Interp*,int,char**);

int dbRLock			(ClientData,Tcl_Interp*,int,char**);
int dbUnlock		(ClientData,Tcl_Interp*,int,char**);
int dbUnlockAll	(ClientData,Tcl_Interp*,int,char**);

int dbDelete		(ClientData,Tcl_Interp*,int,char**);
int dbRecall		(ClientData,Tcl_Interp*,int,char**);
int dbIsDeleted (ClientData,Tcl_Interp*,int,char**);

int dbCopyMemoTo(ClientData,Tcl_Interp*,int,char**);

int dbLastError	(ClientData,Tcl_Interp*,int,char**);
int dbVersion		(ClientData,Tcl_Interp*,int,char**);

/****************************************************************************
 *                                                                          *
 * Init Tcl/Tk and create new commands for MoonBase                         *
 *                                                                          *
 ****************************************************************************/ 
int Tcl_AppInit(Tcl_Interp *interp)
{
	Tk_Window main = Tk_MainWindow(interp);
	
	if (Tcl_Init(interp) == TCL_ERROR) return TCL_ERROR;
	if (Tk_Init(interp) == TCL_ERROR) return TCL_ERROR;	
	if (DB_Init(interp) == TCL_ERROR) return TCL_ERROR;
	
	return TCL_OK;
}

/****************************************************************************
 *                                                                          *
 * Add MoonBase commands to TCL                                             *
 *                                                                          *
 ****************************************************************************/ 

int DB_Init(Tcl_Interp *interp)
{
  Tcl_CreateCommand(interp,"dbUse"				,dbUse				,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbDbf"				,dbDbf				,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbSelect"			,dbSelect			,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbCreateBegin",dbCreateBegin,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbCreateAdd"	,dbCreateAdd	,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbCreateEnd"	,dbCreateEnd	,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbRead"				,dbRead				,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbReplace"		,dbReplace		,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbAppendBlank",dbAppendBlank,NULL,(void(*)(ClientData))0);
  Tcl_CreateCommand(interp,"dbGo"					,dbGo					,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbSkip"				,dbSkip				,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbEof"				,dbEof				,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbBof"				,dbBof				,NULL,(void(*)(ClientData))0);
  Tcl_CreateCommand(interp,"dbRecCount"		,dbRecCount		,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbRecNo"			,dbRecNo			,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbFldCount"		,dbFldCount		,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbField"			,dbField			,NULL,(void(*)(ClientData))0);
  Tcl_CreateCommand(interp,"dbFieldNo"		,dbFieldNo		,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbRLock"			,dbRLock			,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbUnlock"			,dbUnlock			,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbUnlockAll"	,dbUnlockAll	,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbDelete"			,dbDelete			,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbRecall"			,dbRecall			,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbIsDeleted"	,dbIsDeleted	,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbCopyMemoTo"	,dbCopyMemoTo	,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbLastError"	,dbLastError	,NULL,(void(*)(ClientData))0);
	Tcl_CreateCommand(interp,"dbVersion"		,dbVersion		,NULL,(void(*)(ClientData))0);
	return TCL_OK;
}

char* dbBool(bool b)
{
	return b?".T.":".F.";
}

//#undef TCL_STATIC
//#define TCL_STATIC TCL_VOLATILE

void  dbBool(Tcl_Interp* interp, bool b)
{
	if (b)
		Tcl_SetResult(interp, ".T.", TCL_STATIC);
	else
		Tcl_SetResult(interp, ".F.", TCL_STATIC);	
}

/****************************************************************************
 * dbUse                                                                    *
 ****************************************************************************/
int dbUse(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	switch(argc)
	{
		case 1:
			dbBool(interp, db.Use() );
			break;
		case 2:
			dbBool(interp, db.Use(argv[1]) );
			break;	
		default:
			Tcl_SetResult(interp, "ERROR: Syntax is 'dbUse [<db-file>]'", TCL_STATIC);
			return TCL_ERROR;
	}
	return TCL_OK;
}

/****************************************************************************
 * dbDbf                                                                    *
 ****************************************************************************/
int dbDbf(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if(argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbDbf'", TCL_STATIC);
		return TCL_ERROR;
	}
	
	Tcl_SetResult(interp, db.Dbf(), TCL_VOLATILE);
	return TCL_OK;
}

/****************************************************************************
 * dbSelect                                                                 *
 ****************************************************************************/
int dbSelect(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if(argc!=2)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbSelect <name>'", TCL_STATIC);
		return TCL_ERROR;
	}
	
	dbBool(interp, db.Select(argv[1]));
	return TCL_OK;
}

/****************************************************************************
 * dbCreateBegin                                                            *
 ****************************************************************************/
int dbCreateBegin(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	argc--;
	if(argc!=2)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbCreateBegin <db-file> <fieldcount>'", TCL_STATIC);
		return TCL_ERROR;
	}
	char *endptr;
	ULONG fieldcount=strtoul(argv[2],&endptr,0);
	if (*endptr!='\0')
	{
		Tcl_SetResult(interp, "ERROR: parameter 'fieldcount' is not a number", TCL_STATIC );
		return TCL_ERROR;
	}
	if (fieldcount==0 || fieldcount>255)
	{
		Tcl_SetResult(interp, "ERROR: fieldcount must be between 1 and 255", TCL_STATIC);
		return TCL_ERROR; 
	}
	dbBool(interp, db.CreateBegin(argv[1], (BYTE)fieldcount));
	return TCL_OK;
}

/****************************************************************************
 * dbCreateAdd                                                              *
 ****************************************************************************/
int dbCreateAdd(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	argc--;
	if(argc!=3 && argc!=4)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbCreateAdd <fieldname> <type> <len> [<decimals>]'", TCL_STATIC);
		return TCL_ERROR;
	}
	
	// 1.Parameter
	if (*argv[1]>='0' && *argv[1]<='9')
	{
		Tcl_SetResult(interp, "ERROR: fieldname must not begin with a number", TCL_STATIC);
		return TCL_ERROR;
	}
	
	// 2.Parameter
	*argv[2]=upcase(*argv[2]);
	if (argv[2][1]!='\0')
	{
		Tcl_SetResult(interp, "ERROR: type must be a single char", TCL_STATIC);
		return TCL_ERROR;
	}
	switch(*argv[2])
	{
		case 'B':case 'C':case 'D':case 'M':case 'N':break;
		default:
			Tcl_SetResult(interp, "ERROR: type must be 'B'ool, 'C'har, 'D'ate, 'M'emo or 'N'umeric.",TCL_STATIC);
			return TCL_ERROR;
	}
	
	// 3.Parameter
	char *endptr;
	ULONG len=strtoul(argv[3],&endptr,0);
	if (*endptr!='\0')
	{
		Tcl_SetResult(interp, "ERROR: parameter 'len' is not a number", TCL_STATIC );
		return TCL_ERROR;
	}
	if (len==0 || len>255)
	{
		Tcl_SetResult(interp, "ERROR: 'len' must be between 1 and 255", TCL_STATIC);
		return TCL_ERROR; 
	}

	// 4.Parameter
	ULONG dec=0;
	if (argc==4)
	{
		if (*argv[2]!='N')
		{
			Tcl_SetResult(interp, "ERROR: 'dec' must be zero when type is not 'N'umeric",TCL_STATIC);
			return TCL_ERROR; 
		}
		dec=strtoul(argv[4],&endptr,0);
		if (*endptr!='\0')
		{
			Tcl_SetResult(interp, "ERROR: parameter 'dec' is not a number", TCL_STATIC );
			return TCL_ERROR;
		}
		if (dec>len)
		{
			Tcl_SetResult(interp, "ERROR: 'dec' is above 'len'", TCL_STATIC);
			return TCL_ERROR; 
		}
	}
		
	if (! db.CreateAdd(argv[1],*argv[2],(BYTE)len,(BYTE)dec) )
	{
		Tcl_SetResult( interp, db.lasterror(), TCL_VOLATILE );
		return TCL_ERROR;
	}
	dbBool(interp, true);
	return TCL_OK;
}

/****************************************************************************
 * dbCreateEnd                                                              *
 ****************************************************************************/
int dbCreateEnd(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	argc--;
	if(argc!=0)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbCreateEnd'", TCL_STATIC);
		return TCL_ERROR;
	}
	if (!db.CreateEnd())
	{
		Tcl_SetResult( interp, db.lasterror(), TCL_VOLATILE );
		return TCL_ERROR;
	}
	dbBool(interp, true);
	return TCL_OK;
}


/****************************************************************************
 * dbRead                                                                   *
 ****************************************************************************/
int dbRead(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=2)
	{
		interp->result = "ERROR: Syntax is 'dbRead <fieldname>'";
		return TCL_ERROR;
	}
	
	WORD	nr = db.FieldNo(argv[1]);
	if (nr==0)
	{
		interp->result = "ERROR: unknown fieldname";
		return TCL_ERROR;
	}
	Tcl_SetResult(interp, (char*)db(nr), TCL_VOLATILE);
	return TCL_OK;
}

/****************************************************************************
 * dbReplace                                                                *
 ****************************************************************************/
int dbReplace(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=3)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbReplace <fieldname> <value>'", TCL_STATIC);
		return TCL_ERROR;
	}
	
	int	nr = db.FieldNo(argv[1]);
	if (nr==0)
	{
		Tcl_SetResult(interp, "ERROR: unknown fieldname", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool(interp, db.Replace(nr,argv[2]) );
	return TCL_OK;
}

/****************************************************************************
 * dbAppendBlank                                                            *
 ****************************************************************************/
int dbAppendBlank(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbAppendBlank'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.AppendBlank() );
	return TCL_OK;
}

/****************************************************************************
 * dbGo                                                                     *
 ****************************************************************************/
int dbGo(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=2)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbGo <recordnumber>'", TCL_STATIC);
		return TCL_ERROR;
	}
	char *endptr;
	ULONG nr=strtoul(argv[1],&endptr,0);
	if (*endptr!='\0')
	{
		Tcl_SetResult(interp, "ERROR: parameter is not a number", TCL_STATIC );
		return TCL_ERROR;
	}
	dbBool( interp, db.Go(nr) );
	return TCL_OK;
}

/****************************************************************************
 * dbSkip                                                                   *
 ****************************************************************************/
int dbSkip(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	switch(argc)
	{
		case 1:
			dbBool( interp, db.Skip(1) );
			break;
		case 2:
			char *endptr;
			long nr=strtol(argv[1],&endptr,0);
			if (*endptr!='\0')
			{
				Tcl_SetResult(interp, "ERROR: parameter is not a number", TCL_STATIC );
				return TCL_ERROR;
			}
			dbBool( interp, db.Skip(nr) );
			break;	
		default:
			Tcl_SetResult(interp, "ERROR: Syntax is 'dbSkip [<count>]'", TCL_STATIC);
			return TCL_ERROR;
	}
	return TCL_OK;
}

/****************************************************************************
 * dbEof                                                                    *
 ****************************************************************************/
int dbEof(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbEof'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.Eof() );
	return TCL_OK;
}

/****************************************************************************
 * dbBof                                                                    *
 ****************************************************************************/
int dbBof(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbBof'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.Bof() );
	return TCL_OK;
}

/****************************************************************************
 * dbRecCount                                                               *
 ****************************************************************************/
int dbRecCount(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbRecCount'", TCL_STATIC);
		return TCL_ERROR;
	}
	char str[20];
	sprintf(str,"%lu",db.RecCount());
	Tcl_SetResult(interp, str, TCL_VOLATILE);
	return TCL_OK;
}

/****************************************************************************
 * dbRecNo                                                                  *
 ****************************************************************************/
int dbRecNo(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbRecNo'", TCL_STATIC);
		return TCL_ERROR;
	}
	char str[20];
	sprintf(str,"%lu",db.RecNo() );
	Tcl_SetResult(interp, str, TCL_VOLATILE);
	return TCL_OK;
}

/****************************************************************************
 * dbFldCount                                                               *
 ****************************************************************************/
int dbFldCount(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbFldCount'", TCL_STATIC);
		return TCL_ERROR;
	}
	char str[20];
	sprintf(str,"%lu",db.FldCount() );
	Tcl_SetResult(interp, str, TCL_VOLATILE);
	return TCL_OK;
}

/****************************************************************************
 * dbField                                                                  *
 ****************************************************************************/
int dbField(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=2)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbField <fieldnumber>'", TCL_STATIC);
		return TCL_ERROR;
	}
	char *endptr;
	ULONG nr=strtoul(argv[1],&endptr,0);
	if (*endptr!='\0')
	{
		Tcl_SetResult(interp, "ERROR: parameter is not a number", TCL_STATIC );
		return TCL_ERROR;
	}
	if (nr>(ULONG)db.FldCount() || nr==0)
	{
		Tcl_SetResult(interp, "ERROR: invalid fieldnumber", TCL_STATIC );
		return TCL_ERROR;
	}
	
	Tcl_SetResult( interp, db.Field(nr), TCL_STATIC );
	return TCL_OK;
}

/****************************************************************************
 * dbFieldNo                                                                *
 ****************************************************************************/
int dbFieldNo(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=2)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbFieldNo <fieldname>'", TCL_STATIC);
		return TCL_ERROR;
	}
	char str[20];
	sprintf(str,"%lu",db.FieldNo(argv[1]) );
	Tcl_SetResult(interp, str, TCL_VOLATILE);
	return TCL_OK;
}

/****************************************************************************
 * dbRLock                                                                  *
 ****************************************************************************/
int dbRLock(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbRLock'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.RLock() );
	return TCL_OK;
}

/****************************************************************************
 * dbUnlock                                                                 *
 ****************************************************************************/
int dbUnlock(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbUnlock'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.Unlock() );
	return TCL_OK;
}

/****************************************************************************
 * dbUnlockAll                                                              *
 ****************************************************************************/
int dbUnlockAll(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbUnlockAll'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.UnlockAll() );
	return TCL_OK;
}

/****************************************************************************
 * dbDelete                                                                 *
 ****************************************************************************/
int dbDelete(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbDelete'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.Delete() );
	return TCL_OK;
}

/****************************************************************************
 * dbRecall                                                                 *
 ****************************************************************************/
int dbRecall(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbDelete'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.Delete() );
	return TCL_OK;
}

/****************************************************************************
 * dbIsDeleted                                                              *
 ****************************************************************************/
int dbIsDeleted(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbIsDeleted'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.Deleted() );
	return TCL_OK;
}

/****************************************************************************
 * dbCopyMemoTo                                                             *
 ****************************************************************************/
int dbCopyMemoTo(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=3)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbCopyMemoTo <memofield> <filename>'", TCL_STATIC);
		return TCL_ERROR;
	}
	dbBool( interp, db.CopyMemoTo(argv[1],argv[2]) );
	return TCL_OK;
}

/****************************************************************************
 * dbLastError                                                              *
 ****************************************************************************/
int dbLastError(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbError'", TCL_STATIC);
		return TCL_ERROR;
	}
	Tcl_SetResult( interp, db.lasterror(), TCL_VOLATILE );
	return TCL_OK;
}

/****************************************************************************
 * dbVersion                                                                *
 ****************************************************************************/
int dbVersion(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	if (argc!=1)
	{
		Tcl_SetResult(interp, "ERROR: Syntax is 'dbVersion'", TCL_STATIC);
		return TCL_ERROR;
	}
	Tcl_SetResult( interp,(char*)MOONBASE_VERSION, TCL_STATIC );
	return TCL_OK;
}
