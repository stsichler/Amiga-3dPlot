/* ---------------------------------------------------------------------- */
/* Plot - 25-12-93 SSi    -    HAUPTMODUL  (Teil 2)                       */
/* ---------------------------------------------------------------------- */
/* Gadgethandler 1-63 / 80 / 92 / 93 */

#include "Plot.h"

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <user/functions.h>

/* globale Zeiger etc. ------------------------------------------------- */

extern struct Screen *OurS;
extern struct Window *OurW;
extern struct Window *Opt1W;
extern struct Window *Opt2W;
extern struct Window *DiskW;
extern struct Window *ApsW;

extern struct Gadget *_FuncG,*_xResG,*_yResG,*_xMinG,*_xMaxG,*_yMinG,*_yMaxG,
		*_zMinG,*_zMaxG,*_OPattG,*_OxSpaceG,*_OySpaceG,*_OBodyG,
		*_OxLightG,*_OyLightG,*_OzLightG,*_OPT_ACR_G,*_OPT_ZLimit_G,
		*_OPT_Time_G,*_OPT_Quad_G,*_OPT_WorkBench_G,*_OPT_DitherCol_G,
		*_OPT_APS_G,*_OGLightStateG,*_dRfxG,*_pRfxG,*_OGPatColG,
		*_OLIntensG,*_OPT_RG3d_G,*_APS_Int_G,*_OGBakFile,*_OGBodyL,*_OGPatL;

extern char *TOOLTYPES;		/* Tooltype Definitionen */
extern char *PROGTYPE;	/* Konfigurationsabhängige Anzeige für Screentitel */

extern short BorderCleared;			/* Zeigt an, daß der BorderDraw Bereich */
							/* verändert ist */
extern short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY;	/* min. und max. Koor. allgem. */
extern short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. für die Funktion */
								/* auf dem Bildschirm */
extern double Midx,Midy;					/* Koordinatenmitte (veränderlich) */
extern double _Midx,_Midy;				/*      -"-         (fest) */

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

extern double PatternCol,PatternDeltaCol;	/* Funktionsfarben */
extern double BodyCol;

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

extern short ConsoleState;

/* Verbindung zum Disktask --------------------------------------------- */

extern struct Process *DiskTaskPtr;

extern char *_Filename;			/* Filename vom Disktask */

extern char Filename[];					/* Filename für Laden/Speichern */

extern BPTR CD_at_start;					/* Current Dir bei Programmstart */

/* Taskkommunikation ---------------------------------------------------- */

#define SendMainMsg(type) { while (MainMsg.Type); MainMsg.Msg.mn_Length=2; MainMsg.Type=type; if (DiskTask_MsgPort) PutMsg(DiskTask_MsgPort,(struct Message *)&MainMsg); else MainMsg.Type=0; }
#define ReplyOwnMsg(msg) ((struct OwnMsg *)msg)->Type=0;

extern struct MsgPort *MainTask_MsgPort;
extern struct MsgPort *DiskTask_MsgPort;

extern volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */
extern volatile struct OwnMsg DiskMsg;		/* Nachricht von Disk->Main */

extern struct IntuiMessage *ActMsg,*ActGtmsg;/* "laufende" Nachrichten */

/* Prototypen deklarierter Gadgethandler ------------------------------- */

void GDH4to9(struct Gadget *,USHORT);

void GUH1(struct Gadget *,USHORT);
void GUH2(struct Gadget *,USHORT);
void GUH3(struct Gadget *,USHORT);
void GUH4to9(struct Gadget *,USHORT);
void GUH10(struct Gadget *,USHORT);
void GUH11(struct Gadget *,USHORT);
void GUH12(struct Gadget *,USHORT);
void GUH13(struct Gadget *,USHORT);
void GUH14(struct Gadget *,USHORT);
void GUH15(struct Gadget *,USHORT);
void GUH16(struct Gadget *,USHORT);
void GUH17(struct Gadget *,USHORT);
void GUH18(struct Gadget *,USHORT);
void GUH19(struct Gadget *,USHORT);
void GUH20(struct Gadget *,USHORT);
void GUH21(struct Gadget *,USHORT);
void GUH22(struct Gadget *,USHORT);
void GUH25(struct Gadget *,USHORT);
void GUH28(struct Gadget *,USHORT);
void GUH29(struct Gadget *,USHORT);
void GUH30(struct Gadget *,USHORT);
void GUH31(struct Gadget *,USHORT);
void GUH33(struct Gadget *,USHORT);
void GUH34(struct Gadget *,USHORT);
void GUH35(struct Gadget *,USHORT);
void GUH36(struct Gadget *,USHORT);
void GUH37(struct Gadget *,USHORT);
void GUH38(struct Gadget *,USHORT);
void GUH39(struct Gadget *,USHORT);
void GUH43(struct Gadget *,USHORT);
void GUH44(struct Gadget *,USHORT);
void GUH45(struct Gadget *,USHORT);
void GUH46(struct Gadget *,USHORT);
void GUH47(struct Gadget *,USHORT);
void GUH48(struct Gadget *,USHORT);
void GUH49(struct Gadget *,USHORT);
void GUH50(struct Gadget *,USHORT);
void GUH51(struct Gadget *,USHORT);
void GUH52(struct Gadget *,USHORT);
void GUH53(struct Gadget *,USHORT);
void GUH54(struct Gadget *,USHORT);
void GUH55(struct Gadget *,USHORT);
void GUH56(struct Gadget *,USHORT);
void GUH57(struct Gadget *,USHORT);
void GUH58(struct Gadget *,USHORT);
void GUH59(struct Gadget *,USHORT);
void GUH60(struct Gadget *,USHORT);
void GUH61(struct Gadget *,USHORT);
void GUH62(struct Gadget *,USHORT);
void GUH63(struct Gadget *,USHORT);
void GUH80(struct Gadget *,USHORT);
void GUH92(struct Gadget *,USHORT);
void GUH93(struct Gadget *,USHORT);

/* Gadgethandler ------------------------------------------------------- */

void GUH1(gad,code)			/* Size : A */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldSizeFact;
		
		OldSizeFact=SizeFact;
		CompuSizeFact();
		
		if (!Mode(MODE_DISPL)) { SizeFact=OldSizeFact; ComputeBaseVects(); }
	}
}

void GUH2(gad,code)			/* Angle y = 0 */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldyAngle;
		
		OldyAngle=yAngle;
		yAngle=0.0;
		ComputeBaseVects();
				
		if (!Mode(MODE_DISPL)) { yAngle=OldyAngle; ComputeBaseVects(); }
	}
}

void GUH3(gad,code)			/* Angle z = 0 */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldzAngle;
		
		OldzAngle=zAngle;
		zAngle=0.0;
		ComputeBaseVects();
		
		if (!Mode(MODE_DISPL)) { zAngle=OldzAngle; ComputeBaseVects(); }
	}
}

void GDH4to9(gad,code)			/* für Gadget 4-9 */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		short oldsm=submode;
		submode=gad->GadgetID;
		if (!Mode(MODE_STARTEDIT)) submode=oldsm;
	}
}

void GUH4to9(gad,code)			/* für Gadget 4-9 */
struct Gadget *gad;
USHORT code;			/* ACHTUNG: gad und code können sein. */
{
	if (ConsoleState==CS_MAIN && (mode==MODE_EDIT || mode==MODE_STARTEDIT)) Mode(MODE_DISPL);
}

void GUH10(gad,code)			/* Funktion */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		short err;
		
		if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_FuncG,OurW,0,GTST_String,FunctionString,TAG_END);
		else
		{
			strncpy(FunctionString,((struct StringInfo *)(gad->SpecialInfo))->Buffer,256);
			err=CompileFunction(&FunctionCode);
		
			if (err)
			{
				ClearFunctionCode(&FunctionCode); 
				Status|=ST_NO_FUNC;
				if (err!=-1) ((struct StringInfo *)(gad->SpecialInfo))->BufferPos=err-1;
				Error(l[60]);
				ActivateGadget(gad,OurW,0);
			}	
			else
			{
				Status&=~ST_NO_FUNC;
				if (Options&OPT_AWH) WindowToFront(Opt2W);
			}
		}	
		return;
	}
}

void GUH11(gad,code)			/* Opt1... */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		WindowToFront(Opt2W);
		WindowToBack(Opt1W);
	}
}

void GUH12(gad,code)			/* Resolution x */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_xResG,OurW,0,GTIN_Number,(ULONG)xRes,TAG_END);
		else
		{
			Status&=~ST_RES;
			xRes=(short)(((struct StringInfo *)(gad->SpecialInfo))->LongInt);
			if (xRes<1 || xRes>MAXXRES)
			{
				Error(l[61]);
				Status|=ST_RES;
				ActivateGadget(gad,OurW,0);
			}
												 /* neue Spacingwerte berechnen */
			SpacingX=(short)((double)xRes/xResPac+.5)-1;
			if (SpacingX<0) SpacingX=0;
			GT_SetGadgetAttrs(_OxSpaceG,Opt2W,0,GTIN_Number,(ULONG)SpacingX,TAG_END);
		}
	}
}

void GUH13(gad,code)			/* Resolution y */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_yResG,OurW,0,GTIN_Number,(ULONG)yRes,TAG_END);
		else
		{
			Status&=~ST_RES;
			yRes=(short)(((struct StringInfo *)(gad->SpecialInfo))->LongInt);
			if (yRes<1 || yRes>MAXYRES)
			{
				Error(l[61]);
				Status|=ST_RES;
				ActivateGadget(gad,OurW,0);
			}
												 /* neue Spacingwerte berechnen */
			SpacingY=(short)((double)yRes/yResPac+.5)-1;
			if (SpacingY<0) SpacingY=0;
			GT_SetGadgetAttrs(_OySpaceG,Opt2W,0,GTIN_Number,(ULONG)SpacingY,TAG_END);
		}
	}
}

void GUH14(gad,code)			/* Range X min */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldxMin;
		
		OldxMin=xMin;
		xMin=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		switch (_TestRange(0))
		{
			case -1: ActivateGadget(gad,OurW,0); break;
			case 1: xMin=OldxMin; GT_SetGadgetAttrs(_xMinG,OurW,0,GTST_String,ftoa(xMin),TAG_END); break;
		}
	}
}

void GUH15(gad,code)			/* Range X max */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldxMax;
		
		OldxMax=xMax;
		xMax=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		switch (_TestRange(0))
		{
			case -1: ActivateGadget(gad,OurW,0); break;
			case 1: xMax=OldxMax; GT_SetGadgetAttrs(_xMaxG,OurW,0,GTST_String,ftoa(xMax),TAG_END); break;
		}
	}
}

void GUH16(gad,code)			/* Range Y min */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldyMin;
		
		OldyMin=yMin;
		yMin=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		switch (_TestRange(0))
		{
			case -1: ActivateGadget(gad,OurW,0); break;
			case 1: yMin=OldyMin; GT_SetGadgetAttrs(_yMinG,OurW,0,GTST_String,ftoa(yMin),TAG_END); break;
		}
	}
}

void GUH17(gad,code)			/* Range Y max */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldyMax;
		
		OldyMax=yMax;
		yMax=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		switch (_TestRange(0))
		{
			case -1: ActivateGadget(gad,OurW,0); break;
			case 1: yMax=OldyMax; GT_SetGadgetAttrs(_yMaxG,OurW,0,GTST_String,ftoa(yMax),TAG_END); break;
		}
	}
}

void GUH18(gad,code)			/* Range Z min */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldzMin;
		
		OldzMin=zMin;
		zMin=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		switch (_TestRange(0))
		{
			case -1: ActivateGadget(gad,OurW,0); break;
			case 1: zMin=OldzMin; GT_SetGadgetAttrs(_zMinG,OurW,0,GTST_String,ftoa(zMin),TAG_END); break;
		}
	}
}

void GUH19(gad,code)			/* Range Z max */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldzMax;
		
		OldzMax=zMax;
		zMax=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
		switch (_TestRange(0))
		{
			case -1: ActivateGadget(gad,OurW,0); break;
			case 1: zMax=OldzMax; GT_SetGadgetAttrs(_zMaxG,OurW,0,GTST_String,ftoa(zMax),TAG_END); break;
		}
	}
}

void GUH20(gad,code)			/* Draw */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		if (mode==MODE_DRAW)
		{
			if (Mode(MODE_WAIT) && Options&OPT_AWH) WindowToFront(Opt2W);			
		}
		else
		{
			if (Mode(MODE_TEST))
			{
				if (Status) Error(l[63]);
				else Mode(MODE_DRAW);
			}
		}
	}
}

void GUH21(gad,code)			/* Color Reduction */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN) ReduceColors(-1);
}

void GUH22(gad,code)			/* Disk... */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN) WindowToFront(DiskW);
}

void GUH25(gad,code)			/* Bild zentrieren */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		double OldMidx,OldMidy;
		
		OldMidx=Midx;
		OldMidy=Midy;
		Midx=_Midx;
		Midy=_Midy;
		if (!Mode(MODE_DISPL)) { Midx=OldMidx; Midy=OldMidy; }
	}
}

void GUH28(gad,code)			/* Pattern Auswahl */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT))
	{		
		short active=0;
		if (DrawMode&DMF_LX) active=1;
		if (DrawMode&DMF_LY) active+=2;
		GT_SetGadgetAttrs(_OPattG,Opt2W,0,GTCY_Active,active,TAG_END);
	}
	else
	{
		DrawMode&=~(DMF_LX|DMF_LY);
		switch (code)
		{
			case 0: break;
			case 1: DrawMode|=DMF_LX; break;
			case 2: DrawMode|=DMF_LY; break;
			case 3: DrawMode|=DMF_LX|DMF_LY; break;
		}
	}
}

void GUH29(gad,code)			/* Spacing x */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_OxSpaceG,Opt2W,0,GTIN_Number,(ULONG)SpacingX,TAG_END);
	else
	{
		Status&=~ST_SPACE;
		SpacingX=(short)(((struct StringInfo *)(gad->SpecialInfo))->LongInt);
		if (SpacingX<0 || SpacingX>5000)
		{
			Error(l[61]);
			Status|=ST_SPACE;
			ActivateGadget(gad,Opt2W,0);
		}
										/* neuen ResPac Quotienten berechnen */
		xResPac=(double)xRes/(double)(SpacingX+1);
	}
}

void GUH30(gad,code)			/* Spacing y */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_OySpaceG,Opt2W,0,GTIN_Number,(ULONG)SpacingY,TAG_END);
	else
	{
		Status&=~ST_SPACE;
		SpacingY=(short)(((struct StringInfo *)(gad->SpecialInfo))->LongInt);
		if (SpacingY<0 || SpacingY>5000)
		{
			Error(l[61]);
			Status|=ST_SPACE;
			ActivateGadget(gad,Opt2W,0);
		}
										/* neuen ResPac Quotienten berechnen */
		yResPac=(double)yRes/(double)(SpacingY+1);
	}
}

void GUH31(gad,code)			/* Body Färbung Auswahl */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT))
	{
		short active=0;
		if (DrawMode&DMF_FL) { active=1; if (DrawMode&DMF_FLL) active=2; if (DrawMode&DMF_COL) active=3; }
		GT_SetGadgetAttrs(_OBodyG,Opt2W,0,GTCY_Active,active,TAG_END);
	}
	else
	{
		DrawMode&=~(DMF_FL|DMF_FLL|DMF_COL);
		switch (code)
		{
			case 0: break;
			case 1: DrawMode|=DMF_FL; break;
			case 2: DrawMode|=DMF_FL|DMF_FLL; break;
			case 3: DrawMode|=DMF_FL|DMF_COL; break;
		}
		{
			BOOL dis=(DrawMode&(DMF_COL|DMF_LCOL))?FALSE:TRUE,
				dis2;
				
			dis2=(dis||!(LightsOn&(1<<SLS)))?TRUE:FALSE;
			
			GadDisable(_dRfxG,Opt2W,dis);	
			GadDisable(_pRfxG,Opt2W,dis);	
			GadDisable(_OLIntensG,Opt2W,dis2);	
			GadDisable(LightSelG,Opt2W,dis);	
			GadDisable(_OGLightStateG,Opt2W,dis);	
			GadDisable(_OxLightG,Opt2W,dis2);	
			GadDisable(_OyLightG,Opt2W,dis2);	
			GadDisable(_OzLightG,Opt2W,dis2);	
		
			GadDisable(_OGBodyL,Opt2W,((DrawMode&(DMF_FL|DMF_FLL|DMF_COL))==(DMF_FL|DMF_FLL))?FALSE:TRUE);
		}
	}
}

void GUH33(gad,code)			/* Lichtquelle x */
struct Gadget *gad;
USHORT code;
{
	double OldLvx;
	
	OldLvx=LightVect[SLS].x;
	LightVect[SLS].x=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
	if (!Mode(MODE_DISPL)) { LightVect[SLS].x=OldLvx; GT_SetGadgetAttrs(_OxLightG,Opt2W,0,GTST_String,ftoa(LightVect[SLS].x),TAG_END); }
}

void GUH34(gad,code)			/* Lichtquelle y */
struct Gadget *gad;
USHORT code;
{
	double OldLvy;
	
	OldLvy=LightVect[SLS].y;
	LightVect[SLS].y=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
	if (!Mode(MODE_DISPL)) { LightVect[SLS].y=OldLvy; GT_SetGadgetAttrs(_OyLightG,Opt2W,0,GTST_String,ftoa(LightVect[SLS].y),TAG_END); }
}

void GUH35(gad,code)			/* Lichtquelle z */
struct Gadget *gad;
USHORT code;
{
	double OldLvz;
	
	OldLvz=LightVect[SLS].z;
	LightVect[SLS].z=atof(((struct StringInfo *)(gad->SpecialInfo))->Buffer);
	if (!Mode(MODE_DISPL)) { LightVect[SLS].z=OldLvz; GT_SetGadgetAttrs(_OzLightG,Opt2W,0,GTST_String,ftoa(LightVect[SLS].z),TAG_END); }
}

void GUH36(gad,code)			/* ACR-Schalter */
struct Gadget *gad;
USHORT code;
{
	Options&=~OPT_ACR;
	Options|=(gad->Flags&GFLG_SELECTED)?OPT_ACR:0;
}

void GUH37(gad,code)			/* Limit z-vaule - Schalter */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_OPT_ZLimit_G,Opt2W,0,GTCB_Checked,(Options&OPT_ZLimit)?TRUE:FALSE,TAG_END);
	else
	{
		Options&=~OPT_ZLimit;
		Options|=(gad->Flags&GFLG_SELECTED)?OPT_ZLimit:0;
	}
}

void GUH38(gad,code)			/* Estimate Time - Schalter */
struct Gadget *gad;
USHORT code;
{
	Options&=~OPT_Time;
	Options|=(gad->Flags&GFLG_SELECTED)?OPT_Time:0;
	DisplayMode();
}

void GUH39(gad,code)			/* Quadrangle only - Schalter */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_OPT_Quad_G,Opt2W,0,GTCB_Checked,(Options&OPT_Quad)?TRUE:FALSE,TAG_END);
	else
	{
		Options&=~OPT_Quad;
		Options|=(gad->Flags&GFLG_SELECTED)?OPT_Quad:0;
	}
}

void GUH43(gad,code)			/* Load function */
struct Gadget *gad;
USHORT code;
{
	struct FnctStruct *fs;

	if (!Mode(MODE_WAIT)) return;
	Forbid();
	if (!_Filename || !_Filename[0]) { Permit(); Error(l[65]); return; }
	strncpy(Filename,_Filename,559);
	Permit();
	Mode(MODE_LOAD);
										/* .brk - File ?? */
	if (GetFirstLWofFile(Filename)!=('F'<<24|'O'<<16|'R'<<8|'M'))
	{
		if (fs=LoadFnctStruct())	/* nein, also Funktion */
		{
			if (UseFnctStruct(fs,0)) Error(l[63]);
			else Message(l[53]);
			FreeFnctStruct(fs);
			if (_TestRange(0)==-1) Error(l[62]);
		}
		else
		{
			Error(l[64]);
		}
		Mode(MODE_WAIT);
	}
	else
	{
		if (Options&OPT_ACR) ReduceColors(1);
		if (LoadBreakFile(Filename)) { Error(l[64]); Mode(MODE_WAIT); }
		else
		{
			if (_TestRange(-1)==-1) { Error(l[62]); Mode(MODE_WAIT); PicSecWorth=0; }
			else
			{
				Message(l[53]);
				mode=MODE_DRAW;		/* Zeichenvorgang einleiten */
				submode=1;
				DisplayMode();
				SetDrawGadget(1);
				
				DateStamp(&PicLastSave);/* letzte Bildspeicherung festlegen */
				PicSecWorth=1;
	
				if (Options&OPT_ACR) ReduceColors(1);
				if (Options&OPT_AWH) WindowToFront(OurW);

				GetAPSFilename();			/* APS ggf. einblenden */
				StartTime(APS_TH,APS_MinIntervall*60);
			}
		}
	}
}

void GUH44(gad,code)			/* Save function */
struct Gadget *gad;
USHORT code;
{
	struct FnctStruct *fs;
	Forbid();
	if (!_Filename || !_Filename[0]) { Permit(); Error(l[65]); return; }
	strncpy(Filename,_Filename,554);
	Permit();
	strmfe(Filename,Filename,"fnct");	/* Funktion speichern */
		
	if (fs=BuildFnctStruct())
	{
		if (AskWriteIfExists())
		{
			Mode(MODE_SAVE);
			if (Options&OPT_Backup) BackupFile(Filename);
			if (SaveFnctStruct(fs)) Error(l[66]);
			else
			{
				/* Liste auffrischen -> Nachricht an Disktask */

				SendMainMsg(MT_DISKREFRESHREQ);
				
				Message(l[54]);
			}
			Mode(MODE_OLD);
		}
		FreeFnctStruct(fs);
	}
	else Error(l[63]);
}

void GUH45(gad,code)			/* Save Picture */
struct Gadget *gad;
USHORT code;
{
	int err=0;
	Forbid();
	if (!_Filename || !_Filename[0]) { Permit(); Error(l[65]); return; }
	strncpy(Filename,_Filename,554);
	Permit();
	if (mode==MODE_WAIT)
	{
		if (PicSecWorth<0) { Error(l[67]); return; }
		strmfe(Filename,Filename,"iff");
		if (AskWriteIfExists())
		{
			Mode(MODE_SAVE);
			if (Options&OPT_ACR) ReduceColors(1);
			if (Options&OPT_Backup) BackupFile(Filename);
			err=SavePicture();
			if (Options&OPT_ACR) ReduceColors(0);
			if (err) Error(l[66]);
			else
			{
				/* Liste auffrischen -> Nachricht an Disktask */

				SendMainMsg(MT_DISKREFRESHREQ);

				Message(l[54]);
				PicSecWorth=0;		/* Bild für "unwichtig" erklären */
			}
			Mode(MODE_OLD);
		}
	}
	else	/* wenn noch gezeichnet wird, BreakFile speichern */
	{
		if (mode==MODE_DRAW && submode==1)
		{
			strmfe(Filename,Filename,"brk");
			
			Mode(MODE_SAVE);
			if (Options&OPT_ACR) ReduceColors(1);
			if (Options&OPT_Backup) BackupFile(Filename);
			if (SaveBreakFile(Filename)) Error(l[66]);
			else
			{
				/* Liste auffrischen -> Nachricht an Disktask */

				SendMainMsg(MT_DISKREFRESHREQ);
					
				Message(l[54]);
				PicSecWorth=1;
				DateStamp(&PicLastSave);/* letzte Bildspeicherung festlegen */
				DI.PointsMeasured=0;		/* und Zeitschätzung neu starten */
				DI.LastSec=DI.LastMic=0;
			}
			Mode(MODE_OLD);
		}
		else Error(l[67]);
	}
}

void GUH46(gad,code)			/* WorkBench - Schalter */
struct Gadget *gad;
USHORT code;
{
	Options&=~OPT_WorkBench;
	if (gad->Flags&GFLG_SELECTED) Options|=OpenWorkBench()?OPT_WorkBench:0;
	else Options|=CloseWorkBench()?0:OPT_WorkBench;
	
	GT_SetGadgetAttrs(gad,Opt1W,0,GTCB_Checked,(Options&OPT_WorkBench)?TRUE:FALSE,TAG_END);
}

void GUH47(gad,code)			/* Color dithering - Schalter */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_OPT_DitherCol_G,Opt2W,0,GTCB_Checked,(Options&OPT_DitherCol)?TRUE:FALSE,TAG_END);
 	else
	{
		Options&=~OPT_DitherCol;
		Options|=(gad->Flags&GFLG_SELECTED)?OPT_DitherCol:0;
		if (Options&OPT_ACR) ReduceColors(1);
		DrawCCross(0);	
		if (Options&OPT_ACR) ReduceColors(0);
	}
}

void GUH48(gad,code)			/* Opt2... */
struct Gadget *gad;
USHORT code;
{
	if (ConsoleState==CS_MAIN)
	{
		WindowToFront(Opt1W);
		WindowToBack(Opt2W);
	}
}

void GUH49(gad,code)			/* Low Priority - Schalter */
struct Gadget *gad;
USHORT code;
{
	Options&=~OPT_LowPri;
	Options|=(gad->Flags&GFLG_SELECTED)?OPT_LowPri:0;
	SetOurPri();
}

void GUH50(gad,code)			/* Einstellungen (Preferences PPrefs) ... */
struct Gadget *gad;
USHORT code;
{
	Edit_Prefs();
}

void GUH51(gad,code)			/* Auto Pic Save (APS) - Schalter */
struct Gadget *gad;
USHORT code;
{
	Options&=~OPT_APS;
	Options|=(gad->Flags&GFLG_SELECTED)?OPT_APS:0;

	GadDisable(_APS_Int_G,Opt1W,(Options&OPT_APS)?FALSE:TRUE);

	GetAPSFilename();
	StartTime(APS_TH,APS_MinIntervall*60);
}

void GUH52(gad,code)			/* APS-Zeitintervall */
struct Gadget *gad;
USHORT code;
{
	APS_MinIntervall=(LONG)(((struct StringInfo *)(gad->SpecialInfo))->LongInt);
	if (APS_MinIntervall<1) APS_MinIntervall=1;
	GT_SetGadgetAttrs(gad,Opt1W,0,GTIN_Number,(LONG)APS_MinIntervall,TAG_END);
	StartTime(APS_TH,APS_MinIntervall*60);
}

void GUH53(gad,code)			/* Time Threshold für Requester */
struct Gadget *gad;
USHORT code;
{
	TimeMinThreshold=(LONG)(((struct StringInfo *)(gad->SpecialInfo))->LongInt);
	if (TimeMinThreshold<0) TimeMinThreshold=0;
	GT_SetGadgetAttrs(gad,Opt1W,0,GTIN_Number,(LONG)TimeMinThreshold,TAG_END);
}

void GUH54(gad,code)			/* Auto Window Handling (AWH) - Schalter */
struct Gadget *gad;
USHORT code;
{
	Options&=~OPT_AWH;
	Options|=(gad->Flags&GFLG_SELECTED)?OPT_AWH:0;
}

void GUH55(gad,code)		/* LightSelect Slider Gadget */
struct Gadget *gad;
USHORT code;
{
	SLS=code-1;
	RefreshLightGadgets();
}

void GUH56(gad,code)		/* LightSelect State Gadget */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT))
	{
		GT_SetGadgetAttrs(_OGLightStateG,Opt2W,0,GTCY_Active,(LightsOn&(1<<SLS))?1:0,TAG_END);
	}
	else
	{
		LightsOn&=~(1<<SLS);
		if (code) LightsOn|=1<<SLS;
		{
			BOOL dis=((DrawMode&(DMF_COL|DMF_LCOL))&&(LightsOn&(1<<SLS)))?FALSE:TRUE;
			
			GadDisable(_OLIntensG,Opt2W,dis);	
			GadDisable(_OxLightG,Opt2W,dis);	
			GadDisable(_OyLightG,Opt2W,dis);	
			GadDisable(_OzLightG,Opt2W,dis);	
		}
		Mode(MODE_DISPL);
	}
}

void GUH57(gad,code)		/* diffuse(d) Reflex Slider Gadget */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_dRfxG,Opt2W,0,GTSL_Level,(short)(Diffuse_Rfx*20.0),TAG_END);
	else Diffuse_Rfx=(double)code/20.0;
}

void GUH58(gad,code)		/* direct(p)(=punctual) Reflex Slider Gadget */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_pRfxG,Opt2W,0,GTSL_Level,(short)(Direct_Rfx*20.0),TAG_END);
	else Direct_Rfx=(double)code/20.0;
}

void GUH59(gad,code)			/* Musterfärbung Auswahl */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT))
	{
		short active=0;
		if (DrawMode&DMF_PAT) active=(DrawMode&DMF_LCOL)?2:1; 
		GT_SetGadgetAttrs(_OGPatColG,Opt2W,0,GTCY_Active,active,TAG_END);
	}
	else
	{
		DrawMode&=~(DMF_PAT|DMF_LCOL);
		switch (code)
		{
			case 0: break;
			case 1: DrawMode|=DMF_PAT; break;
			case 2: DrawMode|=DMF_PAT|DMF_LCOL; break;
		}
		{
			BOOL dis=(DrawMode&(DMF_COL|DMF_LCOL))?FALSE:TRUE,
				dis2;
				
			dis2=(dis||!(LightsOn&(1<<SLS)))?TRUE:FALSE;
			
			GadDisable(_dRfxG,Opt2W,dis);	
			GadDisable(_pRfxG,Opt2W,dis);	
			GadDisable(_OLIntensG,Opt2W,dis2);	
			GadDisable(LightSelG,Opt2W,dis);	
			GadDisable(_OGLightStateG,Opt2W,dis);	
			GadDisable(_OxLightG,Opt2W,dis2);	
			GadDisable(_OyLightG,Opt2W,dis2);	
			GadDisable(_OzLightG,Opt2W,dis2);	
			dis2=(DrawMode&DMF_PAT)?FALSE:TRUE;
			GadDisable(_OxSpaceG,Opt2W,dis2);
			GadDisable(_OySpaceG,Opt2W,dis2);
			GadDisable(_OPattG,Opt2W,dis2);
						
			GT_SetGadgetAttrs(_OGPatL,Opt2W,0,GA_Disabled,dis2,GTSL_Min,(DrawMode&DMF_LCOL)?-20:0,GTSL_Level,(WORD)(((DrawMode&DMF_LCOL)?PatternDeltaCol:PatternCol)*20.0),TAG_END);
		}
	}
}

void GUH60(gad,code)		/* Light Intensity Slider Gadget */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_OLIntensG,Opt2W,0,GTSL_Level,(short)(LightIntens[SLS]*20.0),TAG_END);
	else LightIntens[SLS]=(double)code/20.0;
}

void GUH61(gad,code)			/* Backup - Schalter */
struct Gadget *gad;
USHORT code;
{
	Options&=~OPT_Backup;
	Options|=(gad->Flags&GFLG_SELECTED)?OPT_Backup:0;

	GadDisable(_OGBakFile,Opt1W,(Options&OPT_Backup)?FALSE:TRUE);
}

void GUH62(gad,code)			/* Backup - Filename */
struct Gadget *gad;
USHORT code;
{
	strncpy(BackupFileName,((struct StringInfo *)(gad->SpecialInfo))->Buffer,559);
}

void GUH63(gad,code)		/* Help Scroller Gadget */
struct Gadget *gad;
USHORT code;
{
	PrintLines(code);
}

void GUH80(gad,code)			/* Rot-Grün-3d - Schalter */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(_OPT_RG3d_G,Opt2W,0,GTCB_Checked,(Options&OPT_RG3d)?TRUE:FALSE,TAG_END);
 	else
	{
		Options&=~OPT_RG3d;
		Options|=(gad->Flags&GFLG_SELECTED)?OPT_RG3d:0;
		if (Options&OPT_ACR) ReduceColors(1);
		SetColors((Options&OPT_RG3d)?&activePrefs->RG3dCols:&activePrefs->StdCols);
		DrawCCross(0);	
		if (Options&OPT_ACR) ReduceColors(0);
	}
}

void GUH92(gad,code)		/* Körper Farbe Slider Gadget */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(gad,Opt2W,0,GTSL_Level,(UWORD)(BodyCol*20.0),TAG_END);
	else BodyCol=(double)code/20.0;
}

void GUH93(gad,code)		/* Muster Farbe Slider Gadget */
struct Gadget *gad;
USHORT code;
{
	if (!Mode(MODE_WAIT)) GT_SetGadgetAttrs(gad,Opt2W,0,GTSL_Level,(WORD)(((DrawMode&DMF_LCOL)?PatternDeltaCol:PatternCol)*20.0),TAG_END);
	else
	{
		if (DrawMode&DMF_LCOL) PatternDeltaCol=(double)((WORD)code)/20.0;
		else PatternCol=(double)code/20.0;
	}
}
