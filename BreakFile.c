/* ---------------------------------------------------------------------- */
/* Plot - 10-8-94 SSi     -    BreakFile.c                                */
/* ---------------------------------------------------------------------- */

#include "Plot.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <string.h>

/* Aufbau eines Breakfiles (.brk) ------------------------------------------- *
 *
 * 1. IFF-Bild der Funtkion
 *
 * 2. aktive PlotPrefs Struktur
 *
 * 3. FnctStruct der Funktion
 *
 * 4. BreakFile Struktur
 *
 * -------------------------------------------------------------------------- */

/* globale Daten ------------------------------------------------------------ */

extern struct Window *Opt2W;

extern struct DrawingInfo DI;
extern short Options;
extern struct PlotPrefs *activePrefs;

extern struct Gadget *_OPT_RG3d_G;

/* Speicherroutine ---------------------------------------------------------- */

int SaveBreakFile(char *Filename)	/* lädt einen Breakfile */
{								/* RETURN: Error */
	struct FnctStruct *fs=0;
	int error=0;
	BPTR File;
	
	if (!(File=Open(Filename,MODE_NEWFILE))) { Error(l[70]); return(-1); }

	if (error=_SavePicture(File)) goto _EXIT;
	if (error=_Save_Prefs(File,activePrefs)) goto _EXIT;
	if (!(fs=BuildFnctStruct())) { Error(l[63]); error=-1; goto _EXIT; }
	if (error=_SaveFnctStruct(File,fs)) goto _EXIT;
	error=_SaveBreakStruct(File);
_EXIT:
	if (fs) FreeFnctStruct(fs);
	Close(File);

	CreateIcon(Filename);
	
	return(error);
}

int _SaveBreakStruct(BPTR File)	/* baut eine BreakFile-Strukt aus DI auf */
{								/* und speichert sie */
	struct Break brk;			/* RETURN: Error */
	int error=0;
	
	brk.FloatLen=sizeof(double);
	brk.FloatType=FLOATTYPE;
	brk.SX1=DI.SX1;
	brk.SX2=DI.SX2;
	brk.p2=DI.p2;
	brk._p2=DI._p2;
	brk.v2=DI.v2;
	brk.PatX1Cnt=DI.PatX1Cnt;
	brk.PatX2Cnt=DI.PatX2Cnt;
	brk.PointsToDo=DI.PointsToDo;
	brk.PointCnt=DI.PointCnt;
	brk.PSize=DI.PSize;
	brk.Options=Options;
	brk.Flags=(DI.P?BRK_PFIELD:0)|(DI._P?BRK__PFIELD:0)|(DI.V?BRK_VFIELD:0);
	
	if (Write(File,&brk,sizeof(struct Break))!=sizeof(struct Break)) goto _SBERR;
	
	if (DI.P) if (Write(File,DI.P,DI.PSize*sizeof(point))!=DI.PSize*sizeof(point)) goto _SBERR;
	if (DI._P) if (Write(File,DI._P,DI.PSize*sizeof(point))!=DI.PSize*sizeof(point)) goto _SBERR;
	if (DI.V) if (Write(File,DI.V,DI.PSize*sizeof(vector))!=DI.PSize*sizeof(vector)) goto _SBERR;
	
	goto _SBNERR;
_SBERR:
	Error(l[71]);
	error=-1;
_SBNERR:
	return(error);
}

/* Laderoutine -------------------------------------------------------------- */

int LoadBreakFile(char *Filename)	/* lädt einen Breakfile */
{								/* RETURN: Error */
	struct FnctStruct *fs=0;
	struct PlotPrefs *pp;
	int error=0;
	BPTR File;
	
	if (!(File=Open(Filename,MODE_OLDFILE))) { Error(l[69]); return(-1); }

	if (error=_LoadPicture(File,0)) goto _EXIT;
	
	if (!(pp=_Load_Prefs(File))) { error=-1; goto _EXIT; }
	
	/*** Options & Prefs ***/
	
	Options&=~OPT_RG3d;							/* OPT_RG3d */
	Options|=pp->Options&OPT_RG3d;
	GT_SetGadgetAttrs(_OPT_RG3d_G,Opt2W,0,GTCB_Checked,(Options&OPT_RG3d)?TRUE:FALSE,TAG_END);
	if (Options&OPT_ACR) ReduceColors(1);
	SetColors((Options&OPT_RG3d)?&activePrefs->RG3dCols:&activePrefs->StdCols);
	if (Options&OPT_ACR) ReduceColors(0);

	/* -------------------- */
	
	strcpy(pp->FontName,activePrefs->FontName); /* irrelev. Prefs */
	pp->FontYSize=activePrefs->FontYSize;
	pp->FontStyle=activePrefs->FontStyle;
	
	if (Use_Prefs(pp)) { Free_Prefs(pp); error=-1; goto _EXIT; }
	if (Options&OPT_ACR) ReduceColors(1);
	if (!(fs=_LoadFnctStruct(File))) { error=-1; goto _EXIT; }
	if (fs->Version!=PLOTVERSION || fs->Subversion!=PLOTSUBVERSION) { Error(l[111]); error=-1; goto _EXIT; }
	if (UseFnctStruct(fs,0)) { error=-1; Error(l[63]); goto _EXIT; }
	MakeDrawingInfo();
	if (error=_LoadBreakStruct(File)) goto _EXIT;
	Seek(File,0,OFFSET_BEGINNING);
	if (error=_LoadPicture(File,1)) goto _EXIT;
_EXIT:
	if (fs) FreeFnctStruct(fs);
	Close(File);
	return(error);
}

int _LoadBreakStruct(BPTR File)	/* lädt eine Break Struktur und init. die */
{								/* DI-Strukt. entspr. (nur die geladenen */
	struct Break brk;			/* Einträge(!)) / RETURN: Error */

	if (Read(File,&brk,sizeof(struct Break))!=sizeof(struct Break)) { Error(l[74]); return(-1); }

	if (brk.FloatLen!=sizeof(double) ||  /* richtiges Float Format ? */
		  brk.FloatType!=FLOATTYPE) { Error(l[75]); return(-1); }
	
	DI.SX1=brk.SX1;
	DI.SX2=brk.SX2;
	DI.p2=brk.p2;
	DI._p2=brk._p2;
	DI.v2=brk.v2;
	DI.PatX1Cnt=brk.PatX1Cnt;
	DI.PatX2Cnt=brk.PatX2Cnt;
	DI.PointsToDo=brk.PointsToDo;
	DI.PointCnt=brk.PointCnt;
	DI.PSize=brk.PSize;

	/*** Options & Prefs ***/
	
	if (brk.Flags&BRK_PFIELD)
	{
		if (DI.P)
		{
			if (Read(File,DI.P,DI.PSize*sizeof(point))!=DI.PSize*sizeof(point)) { Error(l[74]); return(-1); }
		}
		else Seek(File,DI.PSize*sizeof(point),OFFSET_CURRENT);
	}
	if (brk.Flags&BRK__PFIELD)
	{
		if (DI._P)
		{
			if (Read(File,DI._P,DI.PSize*sizeof(point))!=DI.PSize*sizeof(point)) { Error(l[74]); return(-1); }
		}
		else Seek(File,DI.PSize*sizeof(point),OFFSET_CURRENT);
	}
	if (brk.Flags&BRK_VFIELD)
	{
		if (DI.V)
		{
			if (Read(File,DI.V,DI.PSize*sizeof(vector))!=DI.PSize*sizeof(vector)) { Error(l[74]); return(-1); }
		}
		else Seek(File,DI.PSize*sizeof(vector),OFFSET_CURRENT);
	}
	return(0);
}

/* IFF Laderoutine (prelink) ----------------------------------------------- */

int _LoadPicture(BPTR File,short load)
{
	long buf[2];

	if (Read(File,buf,8)!=8) { Error(l[74]); return(-1); }

	if (buf[0]==('F'<<24|'O'<<16|'R'<<8|'M'))
	{
		if (!load) Seek(File,buf[1],OFFSET_CURRENT);/* Bild überspringen */
		else
		{		/* Bild einlesen */

			buf[0]=Seek(File,-8,OFFSET_CURRENT);	/* pos. speichern */
		
			Mode(MODE_DOCLEAR);	/* Bild laden */
		
			if (_ReadIFF_BMHD(File)) return(-1);
		
			Seek(File,buf[0]+buf[1],OFFSET_BEGINNING);
		}
	}
	else
	{
		Error(l[112]);
		return(-1);
	}
	return(0);
}
