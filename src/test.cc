// Test für MoonBase
// vom 02.05.1996

/*
	Fehlfunktion bei folgender Befehlsfolge festgestellt:

		zu klappts:
		APPEND BLANK
		REPLACE NAME WITH "text"
		SKIP
		? NAME											; das hier bringt den Fehler zum Vorschein
		GO 1
		? NAME
*/

#include <cstddef>
#include <cstring>
#include <moonbase/owndef.hh>
#include <moonbase/dbheader.hh>
#include <moonbase/dbmemoio.hh>
#include <moonbase/dblcklst.hh>
#include <moonbase/dbfileio.hh>
#include <moonbase/dbdata.hh>
#include <moonbase/moonbase.hh>

main()
{
	database db;
	
	printf("open\n");
	if (!db.CreateBegin("test.dbf",3))
	{
		perror("CreateBegin");
		printf("ERROR: %s\n",db.lasterror());
		return 1;
	}
	db.CreateAdd("NAME"		,'C',25,0);
	db.CreateAdd("VORNAME" ,'C',25,0);
	db.CreateAdd("NUMMER"	,'N',10,2);
	if(!db.CreateEnd())
	{
		printf("ERROR: %s\n",db.lasterror());
		return 1;
	}
	db.AppendBlank();
	db.Replace("NAME","Dieses ist das Testfeld.");
	db.Skip();
	printf("NAME: %s\n", (char*)db("NAME"));
	db.Go(1);
	printf("NAME: %s\n", (char*)db("NAME"));
	return 0;
}