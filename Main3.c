/* ---------------------------------------------------------------------- */
/* Plot - 16-10-94 SSi    -    HAUPTMODUL  (Teil 3)                       */
/* ---------------------------------------------------------------------- */
/* Gadgethandler 64- (Einstellungen Konsole) */

#include "Plot.h"

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <graphics/displayinfo.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <exec/lists.h>


/* globale Zeiger etc. ------------------------------------------------- */

extern struct Screen *OurS;
extern struct Window *OurW;
extern struct Window *Opt1W;
extern struct Window *Opt2W;
extern struct Window *DiskW;
extern struct Window *ApsW;
extern struct Window *ResW;
extern struct Window *PalW;
extern struct RGColSupp *RGColSupp;

extern short BorderCleared;			/* Zeigt an, daß der BorderDraw Bereich */
							/* verändert ist */
extern short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY;	/* min. und max. Koor. allgem. */
extern short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. für die Funktion */
								/* auf dem Bildschirm */
extern double Midx,Midy;					/* Koordinatenmitte (veränderlich) */
extern double _Midx,_Midy;				/*      -"-         (fest) */

extern struct PlotPrefs *editPrefs;
extern struct DimensionInfo editPrefs_DIMS;

/* Daten der Funktion *****************************************************/

extern char FunctionString[MAX_FNCTSTR_LEN+2];	/* Funktion im ASCII-Format */

extern fnctcode *FunctionCode;		/* RAW-Code der Funktion */

extern short Status;	/* Funktion zu berechnen 0 - ja / !=0 - nein */

extern struct VarList Vars;					/* VARIABLEN-WURZELSTRUKTUR */

extern double *X;					/* Zeiger auf x,y Variablen */
extern double *Y;

extern double xMin;
extern double xMax;
extern double yMin;
extern double yMax;			/* Bereich der Funktion */
extern double zMin;
extern double zMax;

extern vector FuncMid;	// Funktionsmitte

extern short xRes;			/* Berechnungsschritte */
extern short yRes;

extern short SpacingX,SpacingY;		/* x/y - Musterabstand */
extern double xResPac,yResPac;	/* =Resol./(Spacing+1) */

extern double yAngle;			/* Kippwinkel der Funktion */
extern double zAngle;

extern double SizeFact;			/* Vergrößerungsfaktor der Funktion */

extern vector LightVect[MAXLIGHTS];	/* Lichtquellen */
extern double LightIntens[MAXLIGHTS];	/* Lichtquellenintensitäten (0-1) */
extern short LightsOn;
extern short SLS;	/* (ShowedLightSource) Nummer der gerade edit. Lichtquelle */
extern struct Gadget *LightSelG;	/* Lichtselektionsgadgets (ID 55) */

extern double Diffuse_Rfx;		/* Reflexionsdaten der Funktion für diffuse */
extern double Direct_Rfx;			/* und direkte Reflexion */

extern struct DateStamp PicLastSave;/* Datum+Zeit der letzten Bildspeicherung */
extern LONG PicSecWorth;	/* Anzahl Sekunden, die das Bild "wert" ist. */
						/* -1: kein Bild auf dem Schirm */
						/*  0: wertloses Bild af dem Schirm */
						/* >0: Bild, das ## sec wert ist, auf dem Schirm */
						
/*************************************************************************/

/* Globale Daten ------------------------------------------------------- */

extern short mode;		/* derz. Arbeitsmodus des Hauptprogramms */

extern short submode;				/* im Editmode : GadgetID des Editgadgets */
							/* im Drawmode :	0 - Zeichenbeginn */
							/*					1 - beim Zeichnen */
							/*					-1 - Zeichenende */
extern short DrawMode;		/* Zeichenmodus */

extern short Options; /* Optionen */
			
extern LONG TimeMinThreshold;		/* Anzahl der Minuten, die ein Bild wert */
							/* sein muß, daß bei Löschgefahr Sicherheits- */
							/* Abfragen gemacht werden (in min.) */
				
extern LONG APS_MinIntervall;		/* Zeitintervall, in dem bei OPT_APS das */
							/* Bild automatisch gesichert wird. (in min.) */

extern char APS_Filename[];	/* AutoPicSave-Filename */

extern TIME *APS_TH;					/* TimeHandle */

extern struct DrawingInfo DI;

extern char BackupFileName[];	/* Filename für OPT_Backup */

extern short OpenAllHitCnt;	/* Zähler für OpenAll-Durchläufe */

extern struct List *DIList;
extern UWORD chosen_Res;

extern short ConsoleState;

extern struct IntuiMessage *ActGtmsg;

/* Daten des PaletteWindow ------------------------------------------------ */

extern short NewGfxChips;	/* !=0 bei ECS/AGA Chips */
extern short EditedColor;
extern struct PP_ColorFile *EditedCf;	/* Zeigt entweder aud EditCf1 oder2 */
extern struct PP_ColorFile EditCf1,EditCf2;/* Kopie zum Editieren */

/* Gadgetzeiger ----------------------------------------------------------- */

extern struct Gadget *_PGXMeasure,*_PGYMeasure,*_PGEyeMeasure,*_PGDAreaX,*_PGDAreaY,
	*_PGColNum,*_PaGRSlider,*_PaGGSlider,*_PaGBSlider,*_PaGRText,*_PaGGText,
	*_PaGBText;
	
/* Prototypen deklarierter Gadgethandler ------------------------------- */

void GUH64(struct Gadget *,USHORT);
void GUH65(struct Gadget *,USHORT);
void GUH66(struct Gadget *,USHORT);
void GUH67(struct Gadget *,USHORT);
void GUH68(struct Gadget *,USHORT);
void GUH69(struct Gadget *,USHORT);
void GUH70(struct Gadget *,USHORT);
void GUH72(struct Gadget *,USHORT);
void GUH73(struct Gadget *,USHORT);
void GUH74(struct Gadget *,USHORT);
void GUH75(struct Gadget *,USHORT);
void GUH76(struct Gadget *,USHORT);
void GUH77(struct Gadget *,USHORT);
void GUH78(struct Gadget *,USHORT);
void GUH79(struct Gadget *,USHORT);
void GUH82(struct Gadget *,USHORT);
void GUH83(struct Gadget *,USHORT);
void GUH84(struct Gadget *,USHORT);
void GUH85(struct Gadget *,USHORT);
void GUH86(struct Gadget *,USHORT);
void GUH87(struct Gadget *,USHORT);
void GUH88(struct Gadget *,USHORT);
void GUH89(struct Gadget *,USHORT);
void GUH90(struct Gadget *,USHORT);
void GUH91(struct Gadget *,USHORT);

/* private Prototypen -------------------------------------------------- */

void _ColorChanged(void);
void _HandlePaGTextError(int);

/* Gadgethandler ------------------------------------------------------- */

void GUH64(gad,code)			/* Bildschirmbreite[cm] */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
			double val;
		
		val=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		if (val<10.0 || val>1000.0)
		{
			if (val<10.0) { val=10.0; Error(l[132]); }
			else { val=1000.0; Error(l[133]); }
			GT_SetGadgetAttrs(_PGXMeasure,OurW,0,GTST_String,ftoa(val),TAG_END);
		}
		editPrefs->ScreenXMeasure=val;
		editPrefs->XAspect=editPrefs->DisplayWidth/editPrefs->ScreenXMeasure;
		DrawTestScreen();
	}
}

void GUH65(gad,code)			/* Bildschirmhöhe[cm] */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
			double val;
		
		val=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		if (val<10.0 || val>1000.0)
		{
			if (val<10.0) { val=10.0; Error(l[132]); }
			else { val=1000.0; Error(l[133]); }
			GT_SetGadgetAttrs(_PGYMeasure,OurW,0,GTST_String,ftoa(val),TAG_END);
		}
		editPrefs->ScreenYMeasure=val;
		editPrefs->YAspect=editPrefs->DisplayHeight/editPrefs->ScreenYMeasure;
		DrawTestScreen();
	}
}

void GUH66(gad,code)			/* Augenvermaßung */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
		char buf[18];
		double OldESDist,OldEEDist;
		short i;
		
		OldESDist=editPrefs->EyeScreenDist;
		OldEEDist=editPrefs->EyeEyeDist;
		
		strcpy(buf,((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		
		for (i=0;buf[i];i++)
		{
			if (buf[i]==';')
			{
				buf[i]=0;
				
				editPrefs->EyeScreenDist=atof(buf);
				editPrefs->EyeEyeDist=atof(&buf[i+1]);
				break;
			}
		}
		
		if (editPrefs->EyeScreenDist<2.0) editPrefs->EyeScreenDist=2.0;
		if (editPrefs->EyeScreenDist>10000.0) editPrefs->EyeScreenDist=10000.0;
		if (editPrefs->EyeEyeDist<-50.0) editPrefs->EyeEyeDist=-50.0;
		if (editPrefs->EyeEyeDist>50.0) editPrefs->EyeEyeDist=50.0;
			
		strncpy(buf,ftoa(editPrefs->EyeScreenDist),8);
		strcat(buf,";");
		strncat(buf,ftoa(editPrefs->EyeEyeDist),7);
	
		GT_SetGadgetAttrs(_PGEyeMeasure,OurW,0,GTST_String,buf,TAG_END);
		DrawTestScreen();
	}		
}

void GUH67(gad,code)			/* Zeichenbereich x */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
		LONG val,maxval;
		val=((struct StringInfo *)(gad->SpecialInfo))->LongInt;
		
		maxval=editPrefs_DIMS.MaxRasterWidth-7;
		
		if (val<10 || val>maxval)
		{
			if (val==0) val=editPrefs_DIMS.TxtOScan.MaxX-editPrefs_DIMS.TxtOScan.MinX-7;
			else
			{
				if (val<10) { val=10; Error(l[132]); }
				else { val=maxval; Error(l[133]); }
			}
			GT_SetGadgetAttrs(_PGDAreaX,OurW,0,GTIN_Number,(ULONG)val,TAG_END);
		}
		editPrefs->DrawAreaWidth=val;
		DrawTestScreen();
	}
}

void GUH68(gad,code)			/* Zeichenbereich y */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
		LONG val,maxval;
		val=((struct StringInfo *)(gad->SpecialInfo))->LongInt;
		
		maxval=editPrefs_DIMS.MaxRasterHeight-H(64)-2;
		
		if (val<10 || val>maxval)
		{
			if (val==0) val=editPrefs_DIMS.TxtOScan.MaxY-editPrefs_DIMS.TxtOScan.MinY-H(64)-2;
			else
			{
				if (val<10) { val=10; Error(l[132]); }
				else { val=maxval; Error(l[133]); }
			}
			GT_SetGadgetAttrs(_PGDAreaY,OurW,0,GTIN_Number,(ULONG)val,TAG_END);
		}
		editPrefs->DrawAreaHeight=val;
		DrawTestScreen();
	}
}

void GUH69(gad,code)			/* Auflösung... */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS) OpenResolutionWindow();
}

void GUH70(gad,code)		/* Farbzahl Slidergadget */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS) editPrefs->DisplayDepth=code;
}

void GUH72(gad,code)			/* Palette... */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS) OpenPaletteWindow();
}

void GUH73(gad,code)		/* Einst. Benutzen */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
		short error,oldOpenAllHitCnt;
	
		oldOpenAllHitCnt=OpenAllHitCnt;
	
		error=Use_Prefs(editPrefs);
		
		if (error<0) { CloseAll(0); goto_exit(20); }
		
/*	if (oldOpenAllHitCnt==OpenAllHitCnt) End_Prefs();	*/
		if (!error) End_Prefs();
	}
}

void GUH74(gad,code)		/* Einst. Abbrechen */
struct Gadget *gad;
USHORT code;
{
	End_Prefs();
}

void GUH75(gad,code)		/* Einstellungen laden */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
		struct PlotPrefs *new;
		
		if (new=Load_Prefs())
		{
			Free_Prefs(editPrefs);
			editPrefs=new;
	
			GetDisplayInfoData(0,(UBYTE *)&editPrefs_DIMS,sizeof(editPrefs_DIMS),DTAG_DIMS,editPrefs->DisplayID);
	
			/* editPrefs_DIMS initialisieren */
			
			editPrefs->DisplayDepth=min(editPrefs->DisplayDepth,editPrefs_DIMS.MaxDepth);
			
			/***** alle Anzeigen auffrischen *****/
		
			GT_SetGadgetAttrs(_PGXMeasure,OurW,0,GTST_String,ftoa(editPrefs->ScreenXMeasure),TAG_END);
			GT_SetGadgetAttrs(_PGYMeasure,OurW,0,GTST_String,ftoa(editPrefs->ScreenYMeasure),TAG_END);
			{
				char buf[18];
				
				strncpy(buf,ftoa(editPrefs->EyeScreenDist),8);
				strcat(buf,";");
				strncat(buf,ftoa(editPrefs->EyeEyeDist),7);
				
				GT_SetGadgetAttrs(_PGEyeMeasure,OurW,0,GTST_String,buf,TAG_END);
			}		
			GT_SetGadgetAttrs(_PGDAreaX,OurW,0,GTIN_Number,(ULONG)editPrefs->DrawAreaWidth,TAG_END);
			GT_SetGadgetAttrs(_PGDAreaY,OurW,0,GTIN_Number,(ULONG)editPrefs->DrawAreaHeight,TAG_END);
			GT_SetGadgetAttrs(_PGColNum,OurW,0,GTSL_Max,min(editPrefs_DIMS.MaxDepth,8),GTSL_Level,editPrefs->DisplayDepth,TAG_END);
		
			SetColors((Options&OPT_RG3d)?&editPrefs->RG3dCols:&editPrefs->StdCols);
			DrawTestScreen();
			CloseResolutionWindow(1);
			ClosePaletteWindow(1);
		}
	}
}

void GUH76(gad,code)		/* Einstellungen speichern */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
		Save_Prefs(editPrefs);
		GUH73(gad,code);	/* 'Einst. benutzen' aufrufen */
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void GUH77(gad,code)		/* Auflösung: Listview */
struct Gadget *gad;
USHORT code;
{
	static ULONG LastSec=0,LastMic=0,ActSec,ActMic;
	static short Last_chosen=-1;
	
	ActSec=ActGtmsg->Seconds;
	ActMic=ActGtmsg->Micros;
	
	chosen_Res=code;
	
	if (DoubleClick(LastSec,LastMic,ActSec,ActMic) && chosen_Res==Last_chosen)
	{
		Last_chosen=-1;
		GUH78(gad,code);
	}
	LastSec=ActSec;
	LastMic=LastMic;
	Last_chosen=chosen_Res;
}

void GUH78(gad,code)		/* Auflösung: OK */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_PREFS)
	{
		struct DINode *n;
		UWORD num;
		
		/* gewählte Auflösung suchen und in editPrefs eintragen */
		
		if (chosen_Res!=0xffff)
		{
			num=0;
			n=(struct DINode *)DIList->lh_Head;
			
			while (num!=chosen_Res && n->node.ln_Succ->ln_Succ)
			{
				n=(struct DINode *)n->node.ln_Succ;
				num++;
			}
			if (n) editPrefs->DisplayID=n->DisplayID;
			
			GetDisplayInfoData(0,(UBYTE *)&editPrefs_DIMS,sizeof(editPrefs_DIMS),DTAG_DIMS,editPrefs->DisplayID);
			
			/* Farbenzahl anpassen */
			
			editPrefs->DisplayDepth=min(editPrefs->DisplayDepth,editPrefs_DIMS.MaxDepth);
			GT_SetGadgetAttrs(_PGColNum,OurW,0,GTSL_Max,min(editPrefs_DIMS.MaxDepth,8),GTSL_Level,editPrefs->DisplayDepth,TAG_END);
	
			editPrefs->DisplayWidth=editPrefs_DIMS.TxtOScan.MaxX-editPrefs_DIMS.TxtOScan.MinX+1;
			editPrefs->DisplayHeight=editPrefs_DIMS.TxtOScan.MaxY-editPrefs_DIMS.TxtOScan.MinY+1;		
			editPrefs->XAspect=editPrefs->DisplayWidth/editPrefs->ScreenXMeasure;
			editPrefs->YAspect=editPrefs->DisplayHeight/editPrefs->ScreenYMeasure;
			
			DrawTestScreen();
		}
		CloseResolutionWindow(1);
	}
}

void GUH79(gad,code)		/* Auflösung: Abbruch */
struct Gadget *gad;
USHORT code;
{
	CloseResolutionWindow(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


void GUH82(gad,code)		/* Palette: Rot Slider */
struct Gadget *gad;
USHORT code;
{
	ULONG val;
	val=code;
	if (!NewGfxChips) val|=val<<4;
	val|=val<<8; val|=val<<16;
	EditedCf->StdCol[EditedColor].red=val;
	_ColorChanged();
}

void GUH83(gad,code)		/* Palette: Grün Slider */
struct Gadget *gad;
USHORT code;
{
	ULONG val;
	val=code;
	if (!NewGfxChips) val|=val<<4;
	val|=val<<8; val|=val<<16;
	EditedCf->StdCol[EditedColor].green=val;
	_ColorChanged();
}

void GUH84(gad,code)		/* Palette: Blau Slider */
struct Gadget *gad;
USHORT code;
{
	ULONG val;
	val=code;
	if (!NewGfxChips) val|=val<<4;
	val|=val<<8; val|=val<<16;
	EditedCf->StdCol[EditedColor].blue=val;
	_ColorChanged();
}

void GUH85(gad,code)		/* Palette: Auswahlgadget */
struct Gadget *gad;
USHORT code;
{
	BOOL Disable=FALSE;
	EditedColor=code;
	if (EditedColor==3 && (EditedCf->Flags&PPCF_RG3d) && RGColSupp->BGCol==3) Disable=TRUE;
	GT_SetGadgetAttrs(_PaGRSlider,PalW,0,GTSL_Level,(EditedCf->StdCol[EditedColor].red)>>(NewGfxChips?24:28),GA_Disabled,Disable,TAG_END);
	GT_SetGadgetAttrs(_PaGGSlider,PalW,0,GTSL_Level,(EditedCf->StdCol[EditedColor].green)>>(NewGfxChips?24:28),GA_Disabled,Disable,TAG_END);
	GT_SetGadgetAttrs(_PaGBSlider,PalW,0,GTSL_Level,(EditedCf->StdCol[EditedColor].blue)>>(NewGfxChips?24:28),GA_Disabled,Disable,TAG_END);
}

void GUH86(gad,code)		/* Palette: Formel für Farbanteil Rot */
struct Gadget *gad;
USHORT code;
{
	strncpy(EditedCf->Red,((struct StringInfo *)(gad->SpecialInfo))->Buffer,79);
	_HandlePaGTextError(SetColors(EditedCf));
}

void GUH87(gad,code)		/* Palette: Formel für Farbanteil Grün */
struct Gadget *gad;
USHORT code;
{
	strncpy(EditedCf->Green,((struct StringInfo *)(gad->SpecialInfo))->Buffer,79);
	_HandlePaGTextError(SetColors(EditedCf));
}

void GUH88(gad,code)		/* Palette: Formel für Farbanteil Blau */
struct Gadget *gad;
USHORT code;
{
	strncpy(EditedCf->Blue,((struct StringInfo *)(gad->SpecialInfo))->Buffer,79);
	_HandlePaGTextError(SetColors(EditedCf));
}

void GUH89(gad,code)		/* Palette: Modus wechseln */
struct Gadget *gad;
USHORT code;
{
	ClosePaletteWindow(3);
	Options^=OPT_RG3d;
	DrawTestScreen();
	OpenPaletteWindow();
}

void GUH90(gad,code)		/* Palette: OK */
struct Gadget *gad;
USHORT code;
{
	if (EditCf1.Flags&PPCF_ERR || EditCf2.Flags&PPCF_ERR) Error(l[138]);
	else
	{
		memcpy(&editPrefs->StdCols,&EditCf1,sizeof(struct PP_ColorFile));
		memcpy(&editPrefs->RG3dCols,&EditCf2,sizeof(struct PP_ColorFile));
		ClosePaletteWindow(1);
	}
}

void GUH91(gad,code)		/* Palette: Abbruch */
struct Gadget *gad;
USHORT code;
{
	if (Options&OPT_ACR) ReduceColors(1);
	ClosePaletteWindow(1);
	SetColors((Options&OPT_RG3d)?&editPrefs->RG3dCols:&editPrefs->StdCols);
	if (Options&OPT_ACR) ReduceColors(0);
}

/* Hilfsroutinen ----------------------------------------------------------- */

void _ColorChanged(void)  /* Ändert die Bildschirmfarbe EditedColor */
{                         /* anhand der in ^EditedCf abgelegten Werte */
	SetRGB(&OurS->ViewPort,EditedColor,
		EditedCf->StdCol[EditedColor].red,
		EditedCf->StdCol[EditedColor].green,
		EditedCf->StdCol[EditedColor].blue);
}

void _HandlePaGTextError(int err)  /* behandelt einen evntl. Fehler von */
{                                  /* SetColors() */
	short Feld=-1;
	struct Gadget *gad=0;
	if (err)
	{
		if (err==-1) Error(l[58]);
		else
		{
			if (err&0xffff0000) Feld=err>>16;
			err&=0xffff;
			switch (Feld)
			{
				case 1: gad=_PaGRText; break;
				case 2: gad=_PaGGText; break;
				case 3: gad=_PaGBText; break;
			}
			if (gad)
			{
				((struct StringInfo *)(gad->SpecialInfo))->BufferPos=err-1;
				Error(l[60]);
				ActivateGadget(gad,PalW,0);
			}
		}
	}
}
