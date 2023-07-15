/* 3dPlot - 29-1-94 SSi - WorkbenchStartup - Auswertung der Tooltypes */

/* erkannte Tooltypes:
 *
 * DIR=<Pfad>				; Pfad für Laden/Speichern setzen
 * INTERLACE=<ON/OFF>		; Interlace-Schalter (zur Kompatibilität)
 * HELP=<Filename>		; Help-Textfile (ASCII)
 * LANGUAGE=<Filename>	; zu verwendender Sprachfile (s.Languages/(...).lng)
 * SETOPT=<Option(s)>		; setzt Optionen
 * CLROPT=<OPtion(s)>		; löscht Optionen
 * RESETFNCT=<Filename>   ; setzt Filenamen der Resetfnct (Default:"Reset.fnct")
 * PREFS=<Filename>     ;Filenamen des .3dp Files 
 * ICONS=<Pfad>			; Pfad für Icons (Default: ProgHomeDir)
 */

char *TOOLTYPES="\
  DIR=<Path>           sets up path for load/save operations\n\
  INTERLACE=<ON/OFF>   chooses screen mode (obsolete - don't use!)\n\
  HELP=<Filename>      redefines help file\n\
  LANGUAGE=<Filename>  defines language infofile\n\
  BACKUP=<Filename>    defines backup file\n\
  SETOPT=<Option(s)>   activates one or more options(see below)\n\
  CLROPT=<Option(s)>   deactivates one or more options(see below)\n\
  RESETFNCT=<Filename> redefines reset function file\n\
  PREFS=<Filename>     defines custom prefs file\n\
  ICONS=<Path>         redefines directory to take icons from\n\
\n\
  Options for SETOPT/CLROPT: ACR, TIME, LOWPRI, APS, AWH, BAK, WB, RG3D\n";
 
#include "Plot.h"
#include <workbench/startup.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <string.h>

/* Globale Parameter -------------------------------------------------------- */

extern char *_Filename;			/* Filename für Disk.c */
extern char *HelpFile;

extern char *Language;			/* bevorz. Languagefilename */

extern short Options;

struct WBStartup *argmsg;
struct WBArg *wbarg;

extern char BackupFileName[];

char Reset_fnct_Name[80]="Reset.fnct";

char Prefs_1st_Name[80]="";

extern BPTR HomeDir_Lock;	/* Homedir Lock des Programms (needs not to be UnLock()-ed */

BPTR DefaultDir_Lock=0;	/* needs not to be UnLock()-ed */
char *Default_fnct_Name=0;	/* needs not to be free()-ed */

BPTR FnctDir_Lock=0;		/* gets UnLock()-ed in CloseAll() */

char Icons_Dir[520];

/* -------------------------------------------------------------------------- */

void ComputeWBMsg(int argc, char **argv)	/* wertet die Übergabeparam. aus */
{										/* und benutzt gegf. die Tooltypes */
	BPTR OldCD;
	struct DiskObject *dobj=0;
	char **toolarray=0;
	char *s;
	
	if (!argc)
	{
		if (!(argmsg=(struct WBStartup *)argv)) return;
	}

	if (argmsg)
	{
		if (!(wbarg=argmsg->sm_ArgList)) return;
		OldCD=CurrentDir(wbarg->wa_Lock);

		if (dobj=GetDiskObject(wbarg->wa_Name))	/* .info laden */
		{
			toolarray=(char **)dobj->do_ToolTypes;
		}
	}
	else
	{
		if (argc==1)
		{
			if (dobj=GetDiskObject(argv[0]))	/* .info laden */
			{
				toolarray=(char **)dobj->do_ToolTypes;
			}
		}
		else toolarray=&argv[1];
	}
	
	if (toolarray) /* Tooltypes auswerten - - - - - - - - - - - - - - - - */
	{
		if (s=(char *)FindToolType(toolarray,"DIR"))
		{
			FnctDir_Lock=Lock(s,ACCESS_READ);
		}
		if (s=(char *)FindToolType(toolarray,"LANGUAGE"))
		{
			Language=getstr(s);
		}
		else Language=0;
		if (s=(char *)FindToolType(toolarray,"HELP"))
		{
			HelpFile=getstr(s);
		}
		else HelpFile=0;
		if (s=(char *)FindToolType(toolarray,"INTERLACE"))
		{
			Options=(Options&~OPT_INTERLACE)|(!stricmp(s,"ON")?OPT_INTERLACE:0);
		}
		if (s=(char *)FindToolType(toolarray,"BACKUP"))
		{
			strncpy(BackupFileName,s,559);
		}
		else
		{
			if (FnctDir_Lock) NameFromLock(FnctDir_Lock,BackupFileName,559);
			strmfp(BackupFileName,BackupFileName,".bak");
		}
		if (s=(char *)FindToolType(toolarray,"CLROPT"))
		{
			if (MatchToolValue(s,"ACR")) Options&=~OPT_ACR;
			if (MatchToolValue(s,"TIME")) Options&=~OPT_Time;
			if (MatchToolValue(s,"LOWPRI")) Options&=~OPT_LowPri;
			if (MatchToolValue(s,"APS")) Options&=~OPT_APS;
			if (MatchToolValue(s,"AWH")) Options&=~OPT_AWH;
			if (MatchToolValue(s,"BAK")) Options&=~OPT_Backup;
			if (MatchToolValue(s,"WB")) Options&=~OPT_WorkBench;
			if (MatchToolValue(s,"RG3D")) Options&=~OPT_RG3d;
		}
		if (s=(char *)FindToolType(toolarray,"SETOPT"))
		{
			if (MatchToolValue(s,"ACR")) Options|=OPT_ACR;
			if (MatchToolValue(s,"TIME")) Options|=OPT_Time;
			if (MatchToolValue(s,"LOWPRI")) Options|=OPT_LowPri;
			if (MatchToolValue(s,"APS")) Options|=OPT_APS;
			if (MatchToolValue(s,"AWH")) Options|=OPT_AWH;
			if (MatchToolValue(s,"BAK")) Options|=OPT_Backup;
			if (MatchToolValue(s,"WB")) Options|=OPT_WorkBench;
			if (MatchToolValue(s,"RG3D")) Options|=OPT_RG3d;
		}
		if (s=(char *)FindToolType(toolarray,"RESETFNCT"))
		{
			strncpy(Reset_fnct_Name,s,79);
		}
		if (s=(char *)FindToolType(toolarray,"PREFS"))
		{
			strncpy(Prefs_1st_Name,s,79);
		}
		if (s=(char *)FindToolType(toolarray,"ICONS"))
		{
			strncpy(Icons_Dir,s,519);
		}
		else Icons_Dir[0]=0;
	}
	
	if (argmsg)	/* Anfangsfunktion ggf. setzen */
	{
		if (argmsg->sm_NumArgs>=2)
		{
			if (wbarg[1].wa_Name)
			{
				DefaultDir_Lock=wbarg[1].wa_Lock;
				_Filename=Default_fnct_Name=wbarg[1].wa_Name;
			}
		}
	}
	
	if (dobj) FreeDiskObject(dobj);
	if (argmsg) CurrentDir(OldCD);

	LoadLanguage();
	
	if (!HelpFile) HelpFile=l[87];
}

/* -------------------------------------------------------------------------- */

/* CreateIcon() produziert für einen File dann ein Icon,
 * wenn im Programhomedir ein Icon mit dem Namen "<Ext>.info" existiert,
 * wobei <Ext> die Extension des Filenamens ist.
 */

void CreateIcon(char *Name)	/* s.o. (CD=Programhomedir) */ 
{
	struct DiskObject *dobj;
	char ext[10];
	static char buf[530];
	
	stcgfe(ext,Name);
	strmfp(buf,Icons_Dir,ext);
	
	if (dobj=GetDiskObject(buf))
	{
		PutDiskObject(Name,dobj);
		FreeDiskObject(dobj);
	}
}
