/* MakeLanguage.c - 23-5-94 - ®SSi/94 ---- (3dPlot) ---------------------- */

/* zur Produktion eines Languagefiles mit dem File linken und ausführen.
 *
 * Bei Änderung der Versions/Revisionsnummer von 3dPlot muß MakeLanguage
 * neu gelinkt werden.
 */

/* Bsp: für Englisch:
 *
 * 1. English.c -> English.o
 * 2. MakeLanguage.o+English.o -> MakeLanguage (s.u.)
 * 3. MakeLanguage -> English.lng
 *
 * Linkeroptionen: from LIB:c.o+MakeLanguage.o+<Sprache>.o to MakeLanguage
 *                 LIB /Version.o LIB:user.lib LIB:sc.lib LIB:amiga.lib
 */

/* ------------------------------------------------------------------------- */

/* Format des produzierten Language Files: *******************************
 *
 * 16 byte Kennung "3dPlot_Language"
 * 2 byte Versionsnummer
 * 2 byte Revisionsnummer
 *
 * verbleibende Filelänge ab hier ( = Länge der eigentl. Daten )
 *
 * Offsettabelle Stringszahl+1 Einträge, die den Offset der Stringanfänge
 * relativ zum Beginn dieser Tabelle beinhalten;
 * letzter Eintrag 0
 *
 * Strings...
 *
 *************************************************************************
 */
 
extern short PLOTVERSION,PLOTSUBVERSION;

extern char *name;
extern char *lang[];

#include <stdio.h>

void main(void)				/* Produktion eines .lng File */
{
	FILE *fp;
	
	if (!(fp=fopen(name,"wb"))) return;
	    /* 1234567890123456 */
	fputs("3dPlot__Language",fp);	/* Kennung schreiben */
	
	fwrite(&PLOTVERSION,sizeof(short),1,fp);
	fwrite(&PLOTSUBVERSION,sizeof(short),1,fp);	/* Versionskennung schr. */

	fwrite(&fp,sizeof(long),1,fp);	/* dummy für Datenlänge */
	
	{
		short Anzahl;	/* Anzahl der Strings */
		short a;
		long ListPos;	/* Position der Pointerliste im File */
		
		for (Anzahl=0;lang[Anzahl];Anzahl++);
		
		ListPos=ftell(fp);		
		fwrite(lang,sizeof(char *),Anzahl+1,fp); /* Liste (dummy) schreiben */
		
		/* Strings in den File schreiben */
		
		for (a=0;a<Anzahl;a++)
		{
			long Pos;
			
			if (ftell(fp)&1) fputc(0,fp);	/* Adr. begradigen */

			Pos=ftell(fp);	/* Pos merken und String schreiben */
			fputs(lang[a],fp);
			fputc(0,fp);
			
			lang[a]=(char *)(Pos-ListPos);	/* und Pos. in die Liste eintragen */
		}
		
		{
			long size;					/* Datenlänge schreiben */
			size=ftell(fp)-ListPos;
			
			fseek(fp,ListPos-sizeof(long),SEEK_SET);
			fwrite(&size,sizeof(long),1,fp);
		}
		
		/* und Tabelle in den File schreiben */
		
		fseek(fp,ListPos,SEEK_SET);
		
		fwrite(lang,sizeof(char *),Anzahl+1,fp);
		
		fclose(fp);
	}
}
