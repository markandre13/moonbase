/****************************************************************************
 *                                                                          *
 *                 MoonBase - a dBASE compatible C++ database               *
 *                                                                          *
 *  (C)opyright 1994,95 Mark-Andre' Hopf                                    *
 *  filename: dbfileio.h                                                    *
 *  relation: dbfileio.cpp                                                  *
 *  author  : Mark-Andre' Hopf                                              *
 *  date    : 10.05.1995                                                    *
 *                                                                          *
 ****************************************************************************/

#ifndef __DBFILEIO_HH
#define __DBFILEIO_HH

/*
 * 'defRecNo' verwaltet den Datensatzzeiger der Datenbank.
 * Wird der Datensatzzeiger veraendert, so wird die Funktion 'RecNoFunc' im
 * dazugehoerigen dbFileIO Objekt aufgerufen.
 */
class dbFileIO;
class defRecNo
{
  private:
    ULONG RecNo;
    dbFileIO* o;
  public:
    void Init(dbFileIO *obj){o=obj;}
    void operator =(ULONG ul);
    operator ULONG(){return RecNo;}
};

class database;

class dbFileIO: public dbHeader, public dbMemoIO
{
	friend database;
	
  private:
    friend defRecNo;
    defRecNo ulRecNo;             // Nummer des aktuellen Datensatzes,..
                                  // .. 0=vor dem ersten Datensatz
    bool bRecNoChanged;           // Datensatzzeiger wurde verändert
    void RecNoFunc();

    char *buffer;                 // Buffer fuer den aktuellen Datensatz
    ULONG ulBufferRec;            // Nummer des Records, im Buffer
    bool bSureBufferIsLocked;     // true, wenn Record im Buffer auf Sperrung
                                  // überprueft wurde und gesperrt ist
    bool bLockedByWrite;          // true, wenn der Buffer lediglich durch
                                  // einen Schreibzugriff gesperrt wurde
    bool bBufferModified;         // Bufferinhalt wurde verändert

    bool updatebuffer();          // aktualisiert den Buffer; true, wenn sich
                                  // dadurch der Bufferinhalt verändert hat

    bool loadbuffer();            // Aktuellen Datensatz in Buffer einlesen;
                                  // berücksichtigt Locked.
    bool savebuffer();            // Buffer auf ursprünglichen Datensatz
                                  // schreiben; berücksichtigt Locked.

    bool readrecord(ULONG,char*); // Einen Record in den angegeben Buffer
                                  // kopieren; keine Berücksichtigung von Locked !!!
    bool writerecord(ULONG,char*);// Einen Record aus dem angegebenen Buffer
                                  // in die Datenbank schreiben; keine Berück-
                                  // sichtigung von Locked !!!

    bool lock(ULONG nr);          // Datensatz sperren (Multiuser)
    bool unlock(ULONG nr);        // Datensatz freigeben (Multiuser)
    dbLockList locklist;          // Klasse, die Liste über gesperrte Datensätze führt

    bool set_delete_flag(bool del); // Datensatz als gelöscht, bzw. nicht gelöscht markieren
    bool bIgnoreDeleted;          // true, means, that functions like SKIP or
                                  // LOCATE must ignore records marked as
                                  // deleted
  protected:
    dbFileIO();
    ~dbFileIO();
    bool fileopen(const char*);   // setzt buffer, ruft dbHeader::fileopen(), öffnet Memodatei
    bool fileclose();             // löscht buffer, ruft dbHeader::fileclose()
		void create_buffer();					// setzt buffer
		bool create_begin(const char *file, WORD fieldcount);
		// bool create_add(...);
		bool create_end();
  public:
    // these two functions cannot stay public, or?
    bool writefield(WORD,const char*);      // Feld im Buffer beschreiben
    bool readfield(WORD,char*,short); // Feld im Buffer lesen

    bool Go(ULONG nr);            // Datensatzzeiger setzen
    bool Skip(long dir=1);        // Datensatzzeiger bewegen
    ULONG RecNo();                // Nummer des aktuellen Datensatzes
    bool Bof();                   // true, wenn vor dem 1.Datensatz
    bool Eof();                   // true, wenn hinter letztem Datensatz

    bool AppendBlank();           // append a new blank record to the db-file

    bool RLock();                 // lock present record
    bool Unlock();                // unlock present record
    bool UnlockAll();             // unlock all records

    bool Deleted();               // true: record is marked as deleted
    bool Delete();                // delete record
    bool Recall();                // recall record
    bool SetDeleted(bool on);     // if on is true, functions like SKIP or
                                  // LOCATE do ignore deleted records

    bool CopyMemoTo(const char* field,  // Memofeld in Datei kopieren und wenn..
                    const char *file,   // ..add==true, ggf. anhaengen
                    bool add=false);

};

inline void defRecNo::operator =(ULONG ul)
{
  RecNo=ul;
  o->RecNoFunc();
}
#endif
