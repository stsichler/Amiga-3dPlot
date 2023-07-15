/* ------------------------------------------------------------------------- *
 * 3dPlot - PPrefs.c (PlotPreferences) - 28-8-94 - ÆSSi
 * ------------------------------------------------------------------------- */

#include "Plot.h"
#include <graphics/displayinfo.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <intuition/intuition.h>
#include <exec/lists.h>
#include <graphics/text.h>
#include <graphics/gfxmacros.h>
#include <stddef.h>

/* globale Daten ----------------------------------------------------------- */

extern short mode,submode,ConsoleState;

struct PlotPrefs *activePrefs;
struct PlotPrefs *editPrefs;
struct DimensionInfo editPrefs_DIMS;

extern short Options;

extern struct Screen *OurS;

extern struct Window *OurW;
extern struct Gadget *FirstGad;	/* erstes Gadget von GadTools.lib (Main-window) */
extern struct Window *Opt2W;

struct Window *ResW=0;
struct Gadget *FirstResGad;

extern short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY; /* (fest) */

extern short maxx;
extern short maxy;

struct Gadget *FirstPrefsGad=0;

extern LONG PicSecWorth;	/* Anzahl Sekunden, die das Bild "wert" ist. */

extern void *VisualInfo;

extern short COLm,COLcnt;
extern double COL_CC1,COL_CC2,COL_CC3;

extern short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. f¸r die Funktion */
								/* auf dem Bildschirm */
extern double Midx,Midy;					/* Koordinatenmitte (ver‰nderlich) */
extern double SizeFact;			/* Vergrˆﬂerungsfaktor der Funktion */

extern short BorderCleared;

extern struct MsgPort *MainTask_MsgPort;

extern struct List *DIList; /* von MakeDIList() */
UWORD chosen_Res;	/* Position von RGList */

extern struct TextFont *textfont;

extern struct RGColSupp *RGColSupp;

extern char Prefs_1st_Name[];

extern ULONG Workbench_DisplayID;

extern struct PlotPrefs _defaultPrefs;

short ResWx,ResWy;
	
/* modulinterne Prototypen  ------------------------------------------------ */

LONG __interrupt __ColNumSliderConv(struct Gadget *,WORD);

/* speziell importierte Prototypen ----------------------------------------- */

void __interrupt CloseWindowSafely(struct Window *);

/* Gadgetzeiger ----------------------------------------------------------- */

struct Gadget *_PGXMeasure,*_PGYMeasure,*_PGEyeMeasure,*_PGDAreaX,*_PGDAreaY,
	*_PGColNum;

extern struct Gadget *_OPT_RG3d_G;

/* Gadgetprototypen ------------------------------------------------------- */

struct NewGadget PGMeasure,PGSpecial,PGAction,PGDArea,PGResol,PGColNum,
	RGList,RGAct;

/* ------------------------------------------------------------------------ */

struct PlotPrefs *Load_default_Prefs(void)
							/* vervollst‰ndigt die Defaultprefs */
{							/* und ¸bergibt sie (als Kopie) */
	static short notFirst=0;/* Routine kann wie Load_Prefs() behandlet */
							/* werden. */
	struct PlotPrefs *pp=0;
	
	if (!notFirst)
	{
		struct DimensionInfo di;
		
		_defaultPrefs.Version=PLOTVERSION;
		_defaultPrefs.Subversion=PLOTSUBVERSION;
		_defaultPrefs.StructLen=sizeof(_defaultPrefs);
		_defaultPrefs.FloatLen=sizeof(double);
		_defaultPrefs.FloatType=FLOATTYPE;
		_defaultPrefs.DisplayID=Workbench_DisplayID;
		
		GetDisplayInfoData(0,(UBYTE *)&di,sizeof(di),DTAG_DIMS,_defaultPrefs.DisplayID);
		
		_defaultPrefs.DrawAreaWidth=di.TxtOScan.MaxX-di.TxtOScan.MinX-7;
		_defaultPrefs.DrawAreaHeight=di.TxtOScan.MaxY-di.TxtOScan.MinY-66;
		_defaultPrefs.DisplayWidth=di.TxtOScan.MaxX-di.TxtOScan.MinX+1;
		_defaultPrefs.DisplayHeight=di.TxtOScan.MaxY-di.TxtOScan.MinY+1;
		_defaultPrefs.XAspect=_defaultPrefs.DisplayWidth/_defaultPrefs.ScreenXMeasure;
		_defaultPrefs.YAspect=_defaultPrefs.DisplayHeight/_defaultPrefs.ScreenYMeasure;

		_defaultPrefs.Options=Options;
	}	
	
	if (pp=Copy_Prefs(&_defaultPrefs))
	{
		/****** obsolete *****/
		if (Options&OPT_INTERLACE)
		{
			struct DimensionInfo di;
	
			pp->DisplayID|=HIRESLACE_KEY;
	
			GetDisplayInfoData(0,(UBYTE *)&di,sizeof(di),DTAG_DIMS,pp->DisplayID);
			pp->DrawAreaWidth=di.TxtOScan.MaxX-di.TxtOScan.MinX-7;
			pp->DrawAreaHeight=di.TxtOScan.MaxY-di.TxtOScan.MinY-66;
			pp->DisplayWidth=di.TxtOScan.MaxX-di.TxtOScan.MinX+1;
			pp->DisplayHeight=di.TxtOScan.MaxY-di.TxtOScan.MinY+1;
			pp->YAspect=pp->DisplayHeight/pp->ScreenYMeasure;
		}
		/*********************/
	}
	return(pp);
}

/* ------------------------------------------------------------------------- */

/* Suchreihenfolge f¸r Load_Prefs() :
 *
 * 1. Prefs_1st_Name (CD ist Programhomedir)
 * 2. "3dPlot-Prefs.3dp" (CD ist Programhomedir)
 * 3. "s:3dPlot-Prefs.3dp"
 * 4. Default-Prefs von Load_default_Prefs()
 */

struct PlotPrefs *Load_Prefs(void)	/* l‰dt Einstellungen (s.o.) */
{
	short i;
	BPTR fp;
	struct PlotPrefs *pp=0;
	
	for (i=0,fp=0;i<3 && !fp;i++)
	{
		switch(i)
		{
			case 0: if (Prefs_1st_Name[0]) fp=Open(Prefs_1st_Name,MODE_OLDFILE); break;
			case 1: fp=Open("3dPlot-Prefs.3dp",MODE_OLDFILE); break;
			case 2: fp=Open("S:3dPlot-Prefs.3dp",MODE_OLDFILE); break;
		}
	}
	if (fp) pp=_Load_Prefs(fp);
	if (!pp) pp=Load_default_Prefs();
	if (fp) Close(fp);
	return(pp);
}

/* ------------------------------------------------------------------------- */

struct PlotPrefs *_Load_Prefs(BPTR File) /* l‰dt Preferences (0 bei Fehler) */
{
	char SH[offsetof(struct PlotPrefs,Flags)];
	struct PlotPrefs *pp=0;

	long FileBegin;

	FileBegin=Seek(File,0,OFFSET_CURRENT);
	
	/* Strukturkopf einlesen */
	
	if (Read(File,SH,offsetof(struct PlotPrefs,Flags))!=-1)
	{
		ULONG Len;
		struct PlotPrefs *pph;	/* Zeiger auf Headerpuffer */
		
		pph=(struct PlotPrefs *)SH;
									/* Legalit‰t ¸berpr¸fen */
		if (!strcmp(pph->IDString,PP_IDSTRING))
		{
			if (pph->Version==PLOTVERSION && !(pph->Subversion/10-PLOTSUBVERSION/10))
			{					
				Len=sizeof(struct PlotPrefs);
				
				if (pph->FloatLen==sizeof(double) && pph->FloatType==FLOATTYPE)
				{
					if (pp=(struct PlotPrefs *)AllocMem(Len,MEMF_CLEAR))
					{
						ULONG ReadStrLen;
						Seek(File,FileBegin,OFFSET_BEGINNING);
						
						ReadStrLen=min(pph->StructLen,sizeof(struct PlotPrefs));
		
						if (Read(File,pp,ReadStrLen)!=ReadStrLen)	/* Struktur einlesen */
						{
							Error(l[74]);
							FreeMem(pp,Len);
							pp=0;
						}
						/* Bei ‰lteren Prefs Restparam. vn activePrefs holen */
						if (pp->StructLen<Len && activePrefs)
						{
							memcpy((APTR)((ULONG)pp+pp->StructLen),(APTR)((ULONG)activePrefs+pp->StructLen),Len-pp->StructLen);
						}
						pp->StructLen=Len;	/* auf "heimisches Format" anpassen */
					} else Error(l[68]);
				} else Error(l[75]);
			}	else Error(FileBegin?l[111]:l[149]);
		} else Error(FileBegin?l[112]:l[144]);
	}
	return(pp);
}

/* ------------------------------------------------------------------------- */

/* Save_Prefs() speichert :
 *
 * entweder in den bei 'PREFS=' angegebenen File (Prefs_1st_Name), oder,
 * falls keiner angegeben wurde, in '3dPlot-Prefs.3dp' (CD ist Programhomedir).
 */

int Save_Prefs(struct PlotPrefs *pp)	/* speichert die Struktur (s.o.) */
{																		/* RETURN: Fehler */
	BPTR fp=0;
	char *fn="3dPlot-Prefs.3dp";
	
	if (Prefs_1st_Name[0]) fn=Prefs_1st_Name;
	
	if (fp=Open(fn,MODE_NEWFILE))
	{
		_Save_Prefs(fp,pp);
		Close(fp);
		CreateIcon(fn);
		return(0);
	}
	else Error(l[70]);
	return(-1);
}

/* ------------------------------------------------------------------------- */

int _Save_Prefs(BPTR File,struct PlotPrefs *pp)/* speichert die Struktur */
{											/* RETURN: Error */
	if (!pp) return(-1);
	
	pp->Options=Options;
	
	if (Write(File,pp,pp->StructLen)!=pp->StructLen) /* und schreiben */
	{
		Error(l[71]);
		return(-1);
	}
	
	return(0);
}

/* ------------------------------------------------------------------------- */

int Use_Prefs(struct PlotPrefs *pp)	/* ˆffnet alles nach den geg. */
{                                   /* Parametern neu */
                                    /* RETURN: error: pos. - Bild offen */
	struct PlotPrefs *oldPrefs,     /*         error: neg. - fatal */
	                 *newPrefs;			/*         null:  alles ok. */
	int ret;
	short oDrawx,oDrawy,oDrawX,oDrawY;
	double oMidx,oMidy;
	short neccesary=0;
	
	/* alte Werte retten - - - - - - - - - - - - - - -  */
	
	oDrawx=Drawx;
	oDrawy=Drawy;
	oDrawX=DrawX;
	oDrawY=DrawY;
	oMidx=Midx;
	oMidy=Midy;
	
	oldPrefs=activePrefs;
	if (!(newPrefs=Copy_Prefs(pp))) return(1);

	/* Test, ob CloseAll/OpenAll nˆtig ist - - - - - - - */

	if (!oldPrefs) neccesary=1;		/****** ACHTUNG: Verb. nˆtig *********/
	else
	{
		if (oldPrefs->DrawAreaWidth!=newPrefs->DrawAreaWidth) neccesary=1;
		if (oldPrefs->DrawAreaHeight!=newPrefs->DrawAreaHeight) neccesary=1;
		if (oldPrefs->DisplayID!=newPrefs->DisplayID) neccesary=1;
		if (oldPrefs->DisplayDepth!=newPrefs->DisplayDepth) neccesary=1;
		if (stricmp(oldPrefs->FontName,newPrefs->FontName)) neccesary=1;
		if (oldPrefs->FontYSize!=newPrefs->FontYSize) neccesary=1;
		if (oldPrefs->FontStyle!=newPrefs->FontStyle) neccesary=1;
	}
	
	if (neccesary)
	{
		CloseAll(-1);
		activePrefs=newPrefs;
		if (!OpenAll(-1)) { ret=0; goto __up_1; }
		CloseAll(-1);
		Free_Prefs(newPrefs);
		activePrefs=oldPrefs;
		if (!OpenAll(-1)) { return(1); }
		oldPrefs=activePrefs;
		if (!(activePrefs=Load_default_Prefs())) { Free_Prefs(oldPrefs); return(-1); }
		if (!OpenAll(-1)) { ret=1; goto __up_1; }	
		return(-1);
	}
	else
	{
		ret=0;
		activePrefs=newPrefs;	/* Prefs per Hand init. */
		SetColors((Options&OPT_RG3d)?&activePrefs->RG3dCols:&activePrefs->StdCols);
	}
__up_1:
	/* nachtr‰gliche Anpassungen oldPrefs->activePrefs - - - - - - - */
	
	if (oldPrefs)
	{
		double SizeQuot,MidxQuot,MidyQuot;

		SizeQuot=SizeFact/((double)(oDrawX-oDrawx)/oldPrefs->XAspect+(double)(DrawY-Drawy)/oldPrefs->YAspect);
		MidxQuot=(oMidx-(double)oDrawx)/(double)(oDrawX-oDrawx+1);
		MidyQuot=(oMidy-(double)oDrawy)/(double)(oDrawY-oDrawy+1);

		SizeFact=SizeQuot*((double)(DrawX-Drawx)/activePrefs->XAspect+(double)(DrawY-Drawy)/activePrefs->YAspect);
		ComputeBaseVects();
		Midx=MidxQuot*(double)(DrawX-Drawx+1)+(double)Drawx;
		Midy=MidyQuot*(double)(DrawY-Drawy+1)+(double)Drawy;
		
		Free_Prefs(oldPrefs);
	}
	return(ret);
}

/* ------------------------------------------------------------------------- */

void Free_Prefs(struct PlotPrefs *pp)
{
	if (pp) free(pp);
}

/* ------------------------------------------------------------------------- */

struct PlotPrefs *Copy_Prefs(struct PlotPrefs *old)
{
	struct PlotPrefs *new=0;
	
	if (old)
	{
		if (!(new=malloc(old->StructLen))) { Error(l[68]); }
		else memcpy(new,old,old->StructLen);
	}
	return(new);
}

/* ------------------------------------------------------------------------ */

void Edit_Prefs(void)		/* ruft "Einstellungen" Men¸ auf */
{
	struct Gadget *gad;
	
	if (ConsoleState==CS_PREFS) return;
	
	if (!Mode(MODE_TEST)) return;
	if (!Mode(MODE_CLEAR)) return;	
	if (!editPrefs) if (!(editPrefs=Copy_Prefs(activePrefs))) Error(l[68]);

	/* Vorarbeiten leisten - - - - - - - - - - - - - - */
	
	WindowToFront(OurW);
	
	if (Options&OPT_ACR) ReduceColors(1);
	
	GetDisplayInfoData(0,(UBYTE *)&editPrefs_DIMS,sizeof(editPrefs_DIMS),DTAG_DIMS,editPrefs->DisplayID);

	if (ConsoleState==CS_MAIN) { SetDrawGadget(-1); RemoveGList(OurW,FirstGad,-1);  }
	SetAPen(OurW->RPort,0);		/* Konsole lˆschen */
	RectFill(OurW->RPort,4,maxy+2,maxx-4,OurW->Height-3);
	ConsoleState=CS_CLEAR;
	
	/* Gadgets "Einstellungen" -------------------------------------- */
	
	gad=CreateContext(&FirstPrefsGad);
	
	{
		double dd;		/* Abstand zwischen den Funktionsgruppen */
		
		dd=(double)(maxx-10-W(600))/3.0;
		
		/* Funktionsgruppe Systemmaﬂe - - - - - - - - - - - */

		PGMeasure.ng_VisualInfo=VisualInfo;
		PGMeasure.ng_LeftEdge=5+W(200)-PGMeasure.ng_Width;
		PGMeasure.ng_TopEdge=maxy+2+H(4);
		PGMeasure.ng_GadgetText=l[120];
		PGMeasure.ng_GadgetID=64;
		_PGXMeasure=gad=CreateGadget(STRING_KIND,gad,&PGMeasure,GTST_String,ftoa(editPrefs->ScreenXMeasure),GTST_MaxChars,8,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		
		PGMeasure.ng_TopEdge+=H(15);
		PGMeasure.ng_GadgetText=l[121];
		PGMeasure.ng_GadgetID++;
		_PGYMeasure=gad=CreateGadget(STRING_KIND,gad,&PGMeasure,GTST_String,ftoa(editPrefs->ScreenYMeasure),GTST_MaxChars,8,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;

		{
			char buf[18];
			
			strncpy(buf,ftoa(editPrefs->EyeScreenDist),8);
			strcat(buf,";");
			strncat(buf,ftoa(editPrefs->EyeEyeDist),7);
			
			PGMeasure.ng_TopEdge+=H(15);
			PGMeasure.ng_GadgetText=l[122];
			PGMeasure.ng_GadgetID++;
			_PGEyeMeasure=gad=CreateGadget(STRING_KIND,gad,&PGMeasure,GTST_String,buf,GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,TAG_END);
			if (gad) gad->Activation|=GACT_STRINGRIGHT;
		}		

		/* Funktionsgruppe Zeichenbereich - - - - - - - - - */

		PGDArea.ng_VisualInfo=VisualInfo;
		PGDArea.ng_LeftEdge=5+W(400)-PGDArea.ng_Width+(short)dd;
		PGDArea.ng_TopEdge=maxy+2+H(4);
		PGDArea.ng_GadgetText=l[123];
		l[123][strlen(l[123])-1]='x';
		PGDArea.ng_GadgetID=67;
		_PGDAreaX=gad=CreateGadget(INTEGER_KIND,gad,&PGDArea,GTIN_Number,(ULONG)editPrefs->DrawAreaWidth,GTST_MaxChars,6,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;

		PGDArea.ng_TopEdge+=H(15);
		PGDArea.ng_GadgetText="y";
		PGDArea.ng_GadgetID=68;
		_PGDAreaY=gad=CreateGadget(INTEGER_KIND,gad,&PGDArea,GTIN_Number,(ULONG)editPrefs->DrawAreaHeight,GTST_MaxChars,6,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;

		PGResol.ng_VisualInfo=VisualInfo;
		PGResol.ng_LeftEdge=5+W(200)+(short)dd;
		PGResol.ng_TopEdge=maxy+2+H(4+15);
		PGResol.ng_GadgetText=l[124];
		PGResol.ng_GadgetID=69;
		gad=CreateGadget(BUTTON_KIND,gad,&PGResol,TAG_END);

		PGColNum.ng_VisualInfo=VisualInfo;
		PGColNum.ng_LeftEdge=5+W(400)-PGColNum.ng_Width+(short)dd;;
		PGColNum.ng_TopEdge=maxy+2+H(4+15+15+2);
		PGColNum.ng_GadgetID=70;
		PGColNum.ng_GadgetText=l[125];
		
		_PGColNum=gad=CreateGadget(SLIDER_KIND,gad,&PGColNum,GTSL_Min,(UWORD)3,GTSL_Max,min(editPrefs_DIMS.MaxDepth,8),GTSL_LevelFormat,"%3ld",GTSL_Level,editPrefs->DisplayDepth,
				GTSL_MaxLevelLen,(UWORD)3,GA_RelVerify,TRUE,GTSL_DispFunc,&__ColNumSliderConv,TAG_END);
		
		/* Funktionsgruppe Sondereinstellungen  - - - - - - */

		PGSpecial.ng_VisualInfo=VisualInfo;
		PGSpecial.ng_LeftEdge=5+W(405)+(short)(2.0*dd);
		PGSpecial.ng_TopEdge=maxy+2+H(4);
		PGSpecial.ng_GadgetText=l[126];
		PGSpecial.ng_GadgetID=71;
		gad=CreateGadget(BUTTON_KIND,gad,&PGSpecial,GA_Disabled,TRUE,TAG_END);

		PGSpecial.ng_TopEdge+=H(15);
		PGSpecial.ng_GadgetText=l[127];
		PGSpecial.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&PGSpecial,TAG_END);
		
		/* Funktionsgruppe "Aktion" - - - - - - - - - - - - */

		PGAction.ng_VisualInfo=VisualInfo;
		PGAction.ng_LeftEdge=5+W(505)+(short)(3.0*dd);
		PGAction.ng_TopEdge=maxy+2+H(2);
		PGAction.ng_GadgetText=l[128];
		PGAction.ng_GadgetID=73;
		gad=CreateGadget(BUTTON_KIND,gad,&PGAction,TAG_END);

		PGAction.ng_TopEdge+=H(12);
		PGAction.ng_GadgetText=l[129];
		PGAction.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&PGAction,TAG_END);

		PGAction.ng_TopEdge+=H(12);
		PGAction.ng_GadgetText=l[130];
		PGAction.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&PGAction,TAG_END);

		PGAction.ng_TopEdge+=H(12);
		PGAction.ng_GadgetText=l[131];
		PGAction.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&PGAction,TAG_END);
		
	}
		
	if (!gad) { Error(l[80]); End_Prefs(); }
	else
	{
		AddGList(OurW,FirstPrefsGad,-1,-1,0);
		GT_RefreshWindow(OurW,0);
		RefreshGadgets(FirstPrefsGad,OurW,0);
		ConsoleState=CS_PREFS;
	}
		
	/* -------------------------------------------------------------- */

	DrawTestScreen();
	
	if (Options&OPT_ACR) ReduceColors(0);
	PicSecWorth=-1;
}

/* ------------------------------------------------------------------------- */

void End_Prefs(void)	/* Standard Konsole wieder einblenden */
{
	if (ConsoleState!=CS_MAIN)
	{
		if (Options&OPT_ACR) ReduceColors(1);
	
		CloseResolutionWindow(-1);
		ClosePaletteWindow(-1);
		
		if (FirstPrefsGad)
		{
			if (ConsoleState==CS_PREFS) RemoveGList(OurW,FirstPrefsGad,-1);
			FreeGadgets(FirstPrefsGad);
			SetAPen(OurW->RPort,0);		/* Konsole lˆschen */
			RectFill(OurW->RPort,4,maxy+2,maxx-4,OurW->Height-3);
			FirstPrefsGad=0;
		}
		
		ConsoleState=CS_MAIN;

		SetColors((Options&OPT_RG3d)?&activePrefs->RG3dCols:&activePrefs->StdCols);

		AddGList(OurW,FirstGad,-1,-1,0);
		GT_RefreshWindow(OurW,0);
		RefreshGadgets(FirstGad,OurW,0);			

		GT_SetGadgetAttrs(_OPT_RG3d_G,Opt2W,0,GTCB_Checked,(Options&OPT_RG3d)?TRUE:FALSE,TAG_END);

		RenderMainWindow();
		SetDrawGadget(0);
		
		Free_Prefs(editPrefs);
		editPrefs=0;
		
		BorderCleared=1;
		Mode(MODE_WAIT);
		Mode(MODE_DISPL);
		if (Options&OPT_ACR) ReduceColors(0);
	}
}

/* ------------------------------------------------------------------------- */

void DrawTestScreen(void)		/* Zeichnet das Prefs-Testbild (nutz.editPrefs) */
{
	short Drawx,Drawy,DrawX,DrawY;
	int Border;
	double xasp,yasp;

	/* spezielle Aspect-Werte */

	xasp=activePrefs->DisplayWidth/editPrefs->ScreenXMeasure;
	yasp=activePrefs->DisplayHeight/editPrefs->ScreenYMeasure;	

	EndDisplayHelp();
	
	if (Options&OPT_ACR) ReduceColors(1);
	
	NormalColor();
	SetAPen(OurW->RPort,2);				/* Hintergrund zeichnen */
	SetDrMd(OurW->RPort,JAM1);
	RectFill(OurW->RPort,BorderDrawx,BorderDrawy,BorderDrawX,BorderDrawY);
	
	Border=BorderDrawX-BorderDrawx+1-editPrefs->DrawAreaWidth;
	if (Border<0) Border=0;
	Drawx=BorderDrawx+Border/2;
	DrawX=Drawx+editPrefs->DrawAreaWidth-1;
	DrawX=min(DrawX,BorderDrawX);
	Border=BorderDrawY-BorderDrawy+1-editPrefs->DrawAreaHeight;
	if (Border<0) Border=0;
	Drawy=BorderDrawy+Border/2;
	DrawY=Drawy+editPrefs->DrawAreaHeight-1;
	DrawY=min(DrawY,BorderDrawY);
	
	SetAPen(OurW->RPort,(Options&OPT_RG3d)?RGColSupp->BGCol:3);
	RectFill(OurW->RPort,Drawx,Drawy,DrawX,DrawY);
	
						/* Testw¸rfel zeichnen */

	DrawCCross(1);

	{					/* Testkreis zeichnen */
		double rad,radx,rady;
		
		radx=(double)((BorderDrawX-BorderDrawx-1)/2)/xasp;
		rady=(double)((BorderDrawY-BorderDrawy-1)/2)/yasp;
		rad=min(radx,rady)*.95;

		NormalColor();
		SetAPen(OurW->RPort,1);
		SetDrPt(OurW->RPort,0xe0e0);
		DrawEllipse(OurW->RPort,(Drawx+DrawX)/2,(Drawy+DrawY)/2,
		            (short)(rad*xasp),
		            (short)(rad*yasp));
		SetDrPt(OurW->RPort,0xffff);
	}
	
	/* Einstellungen aufs Bild drucken */
	
	{
		struct NameInfo ni;
		char Buf[80];
		short y=BorderDrawy+H(4)+textfont->tf_Baseline;
		
		if (GetDisplayInfoData(0,(UBYTE *)&ni,sizeof(ni),DTAG_NAME,activePrefs->DisplayID))
		{
			sprintf(Buf,l[139],ni.Name,OurS->Width,OurS->Height);
			Move(OurW->RPort,BorderDrawx+W(4),y);
			Text(OurW->RPort,Buf,strlen(Buf));
			y+=H(8);
		}
		if (activePrefs->DisplayID!=editPrefs->DisplayID)
		{
			if (GetDisplayInfoData(0,(UBYTE *)&ni,sizeof(ni),DTAG_NAME,editPrefs->DisplayID))
			{
				sprintf(Buf,l[140],ni.Name);
				Move(OurW->RPort,BorderDrawx+W(4),y);
				Text(OurW->RPort,Buf,strlen(Buf));
				y+=H(8);
			}
		}
				/* Fonteinstellungen drucken */
		SetAPen(OurW->RPort,1);
		sprintf(Buf,l[141],activePrefs->FontName,activePrefs->FontYSize);
		Move(OurW->RPort,BorderDrawx+W(4),y);
		Text(OurW->RPort,Buf,strlen(Buf));
		y+=H(8);
		if (strcmp(activePrefs->FontName,editPrefs->FontName) || activePrefs->FontYSize!=activePrefs->FontYSize)
		{
			sprintf(Buf,l[142],editPrefs->FontName,editPrefs->FontYSize);
			Move(OurW->RPort,BorderDrawx+W(4),y);
			Text(OurW->RPort,Buf,strlen(Buf));
			y+=H(8);
		}
	}		
	if (Options&OPT_ACR) ReduceColors(0);
}

/* ------------------------------------------------------------------------- */

void OpenResolutionWindow(void)	/* ˆffnet das Resolution-Window */
{	
	struct Gadget *gad;
	
	if (!ResW)
	{
		if (Options&OPT_ACR) ReduceColors(1);
		
		/* Auflˆsungsliste erstellen */
		
		chosen_Res=MakeDIList(editPrefs->DisplayID);

		if (DIList)
		{
			/* Gadgets initialisieren */
	
			gad=CreateContext(&FirstResGad);
	
			RGList.ng_VisualInfo=VisualInfo;
			gad=CreateGadget(LISTVIEW_KIND,gad,&RGList,GTLV_ShowSelected,0,GTLV_Labels,(ULONG)DIList,GTLV_Selected,chosen_Res,TAG_END);

			RGAct.ng_VisualInfo=VisualInfo;
			RGAct.ng_LeftEdge=W(8);
			RGAct.ng_TopEdge=H(89);
			RGAct.ng_GadgetText=l[137];
			RGAct.ng_GadgetID=78;
			gad=CreateGadget(BUTTON_KIND,gad,&RGAct,TAG_END);

			RGAct.ng_LeftEdge=W(270);
			RGAct.ng_GadgetText=l[129];
			RGAct.ng_GadgetID=79;
			gad=CreateGadget(BUTTON_KIND,gad,&RGAct,TAG_END);
			
			/* Window ˆffnen */
		
			if (!(ResW=OpenWindowTags(0,WA_Left,ResWx==-1?(ResWx=((short)BorderDrawx+(short)BorderDrawX-W(368))/2):ResWx,
				WA_Top,ResWy==-1?(ResWy=((short)BorderDrawy+(short)BorderDrawY-H(104))/2):ResWy,
				WA_Width,W(368),WA_Height,H(104),
				WA_Flags,/*WFLG_BACKDROP|*/WFLG_CLOSEGADGET|WFLG_DRAGBAR,
				WA_CustomScreen,(ULONG)OurS,
				WA_Title,l[136],
				WA_Gadgets,FirstResGad,
				TAG_DONE))) { Error(l[68]); return; }
		
			/* IDCMP-Port installieren */
		
			ResW->UserPort=MainTask_MsgPort;
			ModifyIDCMP(ResW, IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|LISTVIEWIDCMP|IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_CHANGEWINDOW);

			/* vollst‰ndige Aktivierung der Gadgets */
			
			GT_RefreshWindow(ResW,0);
		}
		else Error(l[68]);

		if (Options&OPT_ACR) ReduceColors(0);
	}
	if (ResW) WindowToFront(ResW);
}

void CloseResolutionWindow(short mode)  /* bei mode=0 nichts */
{                                       /*          1 Anfrage auf schlieﬂen */
	static short requested=0;           /*          2 Schlieﬂen bei Anfrage */
	                                    /*         -1 sofort schlieﬂen */ 
	if (mode==1 || mode==-1) requested=-1;
	if (requested && mode==2 || mode==-1)
	{
		CloseWindowSafely(ResW);
		ResW=0;
		if (FirstResGad) FreeGadgets(FirstResGad);
		FirstResGad=0;
		FreeDIList();
		requested=0;
	}
}

/* Gadgethilfsroutinen ----------------------------------------------------- */

LONG __interrupt __ColNumSliderConv(struct Gadget *gadget,WORD level)
{
	return((LONG)/*1<<(BYTE)*/level);	/* Conversion f¸r Slidergadgets 57/58 */
}

