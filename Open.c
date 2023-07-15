/* -------------------------------------------------------------------------- *
 * Plot - 25-12-93 SSi  -  Open.c  ------------------------------------------ *
 * -------------------------------------------------------------------------- */

#include "Plot.h"

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <proto/gadtools.h>
#include <graphics/rastport.h>
#include <graphics/displayinfo.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <stdio.h>
#include <user/functions.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <hardware/blit.h>

#define Line(RPort,x1,y1,x2,y2,col) { SetAPen(RPort,col); Move(RPort,x1,y1); Draw(RPort,x2,y2); }

/* Taskkommunikation ----------------------------------------------------- */

#define SendMainMsg(type) { while (MainMsg.Type); MainMsg.Msg.mn_Length=2; MainMsg.Type=type; if (DiskTask_MsgPort) PutMsg(DiskTask_MsgPort,(struct Message *)&MainMsg); else MainMsg.Type=0; }
#define ReplyOwnMsg(msg) ((struct OwnMsg *)msg)->Type=0;
#define ClearMsgPort(port) { struct Message *msg; if (port) while (msg=GetMsg(port)) ReplyMsg(msg); }

extern struct MsgPort *MainTask_MsgPort;
extern struct MsgPort *DiskTask_MsgPort;

extern volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */
extern volatile struct OwnMsg DiskMsg;		/* Nachricht von Disk->Main */

extern struct IntuiMessage *ActMsg,*ActGtmsg;	/* "laufende" Nachrichten */

/* externe Grafikdaten ---------------------------------------------- */

extern far struct Image _3dPlotImage;	/* Image mit Tiefe 4 */
																		/* Farbnummer 0 ist Hintergrundf. */
extern far struct BitMap _3dPlotImBorderBM; /* BitMap mit Tiefe 1 */
																		/* ist 1, wo Image Hintergrundf. ist */
																		
/* private Daten ---------------------------------------------------- */

TIME *Image_TH;

/* Globale Zeiger --------------------------------------------------- */

extern struct PlotPrefs *activePrefs,*editPrefs;

void *VisualInfo;

struct Gadget *FirstGad=0;	/* erstes Gadget von GadTools.lib */
struct Gadget *FirstOpt1Gad=0;/* erstes Gadget des Options(1)-window */
struct Gadget *FirstOpt2Gad=0;/* erstes Gadget des Options(2)-window */
struct Gadget *FirstDiskGad=0;/*         -"-       Disk-window */
extern struct Gadget *FirstPrefsGad;	/* siehe PPrefs.c */

short maxx;		/* letzte Bildspalte (Bildbreite-1) */
short maxy;		/* zeigt auf die max. unterste Zeile des Fnkt-Zeichenbereichs */

extern struct Screen *OurS;
struct DimensionInfo OurS_DIM;

struct Screen *TempS;	/* temporärer Screen zur Überbrückung von CloseAll- */
                      /* OpenAll- Durchläufen (damit WB nicht geöffnet wird) */

extern struct Window *OurW;
extern struct Window *Opt1W;	/* Options-window für Programm (ACHTUNG: Optionen(2)) */
extern struct Window *Opt2W;	/* Options-window für Zeichnen (ACHTUNG: Optionen(1)) */
extern struct Window *DiskW;	/* Disk-window */
extern struct Window *ErrW;	/* Error-window */
extern struct Window *ApsW;	/* AutoPicSave-window */

extern short UseErrorWindow;

extern short BorderCleared;
extern short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY; /* (fest) */
extern short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. für die Funktion */
									/* auf dem Bildschirm (verän.) */
extern double Midx,Midy;				/* Bildschirmmitte (veränderlich) */
extern double _Midx,_Midy;				/* Bildschirmmitte (fest) */

extern short COLm,COLcnt;	/* min. Farbe und Farbzahl für die Funktion */
extern double PatternCol,PatternDeltaCol;	/* Funktionsfarben */
extern double BodyCol;

extern double COL_CC1,COL_CC2,COL_CC3;	/* Farben für DrawCCross() (0-1) */

extern short ModeDisplay_x;	/* x/y-Koordinaten der Modus-Anzeige */
extern short ModeDisplay_y;

extern short DrawMode;	/* Zeichenmodus */

extern short Options;

extern short SpacingX,SpacingY;	/* x/y-Zeichenmusterabstand */
extern double xMin,xMax,yMin,yMax,zMin,zMax;
extern short xRes,yRes;

extern vector LightVect[MAXLIGHTS];	/* Lichtquellen */
extern double LightIntens[MAXLIGHTS];	/* Lichtintensitäten */
extern short LightsOn;
extern short SLS;	/* (ShowedLightSource) Nummer der gerade edit. Lichtquelle */
struct Gadget *LightSelG;	/* Lichtselektionsgadget (ID 55) */

extern double Diffuse_Rfx;			/* Reflexionsdaten der Funktion für diffuse(d) */
extern double Direct_Rfx;			/* und direkte(p) Reflexion */

extern TIME *TimeEstTH;	/* für OPT_Time */
extern TIME *APS_TH;		/* für OPT_APS */
extern TIME *Edit_TH;		/* für Edit function */

extern fnctcode *FunctionCode;

extern char *PROGTYPE;	/* Konfigurationsabhängige Anzeige für Screentitel */

extern LONG TimeMinThreshold,APS_MinIntervall;

extern char BackupFileName[];

extern char FunctionString[];		/* Funktion im ASCII-Format */

extern BPTR CD_at_start;			/* CD bei Start des Programms */
extern BPTR FnctDir_Lock;		/* gets UnLock()-ed in CloseAll() */
extern BPTR HomeDir_Lock;	/* Homedir Lock des Programms (needs not to be UnLock()-ed */

extern BPTR DefaultDir_Lock;	/* needs not to be UnLock()-ed */
extern char *Default_fnct_Name;	/* needs not to be free()-ed */


extern char Reset_fnct_Name[];	/* Name der Reset-Funktion */
extern char *_Filename;			/* Beim1. Aufruf von OpenAll() aus main() */
								/* steht hier evtl. der Pfad für  */
								/* Funktionen */

extern short ConsoleState;

short OpenAllHitCnt;	/* Zähler für OpenAll()-Durchläufe */

extern short ResWx,ResWy,PalWx,PalWy;

/* Gadgetzeiger für die Funktions-Laderoutine -------------------------- */

struct Gadget *_FuncG,*_xResG,*_yResG,*_xMinG,*_xMaxG,*_yMinG,*_yMaxG,
		*_zMinG,*_zMaxG,*_OPattG,*_OxSpaceG,*_OySpaceG,*_OBodyG,
		*_OxLightG,*_OyLightG,*_OzLightG,*_OPT_ACR_G,*_OPT_ZLimit_G,
		*_OPT_Time_G,*_OPT_Quad_G,*_OPT_WorkBench_G,*_OPT_DitherCol_G,
		*_OPT_LowPri_G,*_OPT_APS_G,*_OGLightStateG,*_dRfxG,*_pRfxG,
		*_OGPatColG,*_OLIntensG,*_OGBakFile,*_OPT_RG3d_G,*_APS_Int_G,
		*_OGBodyL,*_OGPatL;

/* Verbindung zum Disktask --------------------------------------------- */

extern struct Process *DiskTaskPtr;
extern volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */

struct Gadget *_DGPath;
struct Gadget *_DGPat;
struct Gadget *_DGFile;
struct Gadget *_DGList;

/* Fontdefinitionen ---------------------------------------------------- */

struct TextAttr font;
struct TextFont *textfont;

/* Gadgetprototypen ---------------------------------------------------- */

struct NewGadget GZFunct,GSKnob,GLText,GChoose,GStep,GRange1,
                 GRange2,GRText,GAct;

struct NewGadget OGChoose,OGLightChoose;
char *_OGLightChoose1[5];
char *_OGLightChoose2[4];
char *_OGChoose1[5];

struct NewGadget OGText,OGInput,OGSwitch,OGPrefs,OGMinIn,OGText2,
                 OGLightSel,OGBackup;

char *_OGLightChoose[3];

struct NewGadget DGList,DGText,DGLoad,DGSave;

struct NewGadget GHelpScroller;

/* importierte Gadgetprototypen ----------------------------------------- */

extern struct NewGadget PGMeasure,PGDArea,PGResol,PGAction,PGSpecial;

/* modulinterne Prototypen ---------------------------------------------- */

LONG __interrupt __RfxSliderConv(struct Gadget *gadget,WORD level);	/* s.u. */
extern void AdjustGadgetsToFont(void);
extern void PrepareNewGadgetStructs(void);

extern void __interrupt CloseWindowSafely(struct Window *);

/* Garfikstrukturen ----------------------------------------------------- */

WORD areaBuffer[25];		/* Areastrukturen */
struct AreaInfo areaInfo;
struct TmpRas tmpras;

char *Title[5];

/* ---------------------------------------------------------------------- *
 * Öffnungsroutine ------------------------------------------------------ *
 * ---------------------------------------------------------------------- */
 
int OpenAll(int notall)	/* (ERROR=) Öffnet den Bildschirm und initialisiert alles */
{
	static short NotFirstExecution=0;	/* OpenAll() vorher schon aufge- */
	char Buffer[80];					/* rufen ? */
	static UWORD Pens[]= { 0,1,1,1,2,1,3,0,7 };
	static WORD NullCols[] = { 0,0,0,0, 1,0,0,0, 2,0,0,0, 3,0,0,0, -1 };

	do { OpenAllHitCnt++; } while (!OpenAllHitCnt);

	ResWx=ResWy=PalWx=PalWy=-1;
	
	if (!NotFirstExecution) notall=0;
	if (!activePrefs) return(-1);

	/* Font öffnen ------------------------------------------------------- */
	
	freestr(font.ta_Name);
	font.ta_Name=getstr(activePrefs->FontName);
	font.ta_YSize=activePrefs->FontYSize;
	font.ta_Style=activePrefs->FontStyle;
	
	if (!(textfont=OpenDiskFont(&font))) { Error(l[115]); return(-1); }
	if (/* textfont->tf_Flags&FPF_PROPORTIONAL || */ W(8)<6 || H(8)<8) { Error(l[116]); return(-1); }
	
	/* Variable Strukturen initialisieren ----------------------------- */
	
	PrepareNewGadgetStructs();
			
	_OGChoose1[0]=l[15];
	_OGChoose1[1]=l[16];
	_OGChoose1[2]=l[17];
	_OGChoose1[3]=l[18];
	_OGChoose1[4]=0;
	
	_OGLightChoose1[0]=l[19];
	_OGLightChoose1[1]=l[20];
	_OGLightChoose1[2]=l[21];
	_OGLightChoose1[3]=l[22];
	_OGLightChoose1[4]=0;
	
	_OGLightChoose[0]=l[105];
	_OGLightChoose[1]=l[104];
	_OGLightChoose[2]=0;
	
	_OGLightChoose2[0]=l[108];
	_OGLightChoose2[1]=l[109];
	_OGLightChoose2[2]=l[110];
	_OGLightChoose2[3]=0;
		
	AdjustGadgetsToFont();

	Pens[8]=(UWORD)((double)(1<<activePrefs->DisplayDepth)*(14.0/16.0)+.5);
	Pens[8]&=~0x0001;
	Pens[8]|=0x0002;
	
	/* ---------------------------------------------------------------- */
	
	if (!notall)
	{
		if (!(TimeEstTH=OpenTime())||
		    !(APS_TH=OpenTime())||
		    !(Image_TH=OpenTime())||
		    !(Edit_TH=OpenTime())) { Error(l[76]); return(-1); }
		if (!(MainTask_MsgPort=CreateMsgPort())) { Error(l[118]); return(-1); }	
	}

	/* evtl. WorkBench schließen ----------------------------------------- */

	if (!NotFirstExecution) if (!(Options&OPT_WorkBench)) Options|=CloseWorkBench()?0:OPT_WorkBench;

	/* Screen ------------------------------------------------------------ */

	{	
		ULONG w1,w2,h1,h2;	/* Screendimensionen berechnen */
		
		GetDisplayInfoData(0,(UBYTE *)&OurS_DIM,sizeof(OurS_DIM),DTAG_DIMS,activePrefs->DisplayID);
		
		activePrefs->DisplayWidth=OurS_DIM.TxtOScan.MaxX-OurS_DIM.TxtOScan.MinX+1;
		activePrefs->DisplayHeight=OurS_DIM.TxtOScan.MaxY-OurS_DIM.TxtOScan.MinY+1;		
		activePrefs->XAspect=activePrefs->DisplayWidth/activePrefs->ScreenXMeasure;
		activePrefs->YAspect=activePrefs->DisplayHeight/activePrefs->ScreenYMeasure;
		
		w1=W(622)+(short)(activePrefs->XAspect);
		w2=activePrefs->DrawAreaWidth+8;
		h1=H(223)+4+(short)(activePrefs->YAspect);
		h2=activePrefs->DrawAreaHeight+H(64)+2;

		w1=max(w1,w2);
		h1=max(h1,h2);
		
		if (w1&1) w1++;	/* Breite durch 2 teilbar machen */
		
		sprintf(Buffer,"3dPlot V%hd.%02hd %s",PLOTVERSION,PLOTSUBVERSION,PROGTYPE);
		Title[0]=getstr(Buffer);
		if (!(OurS=OpenScreenTags(0,SA_DisplayID,activePrefs->DisplayID,
			SA_Depth,(ULONG)activePrefs->DisplayDepth,
			SA_Pens,(ULONG)Pens,
			SA_Title,Title[0],
			SA_AutoScroll,TRUE,
			SA_Overscan,OSCAN_TEXT,
			SA_Font,&font,
			SA_Width,w1,
			SA_Height,h1,
			SA_Behind,TRUE,
			SA_Colors,&NullCols,
			TAG_DONE))) { Error(l[77]); return(-1); }
	}
	
	activePrefs->DisplayDepth=OurS->BitMap.Depth;
	/* Mode-ID Feld ggf. korrigieren */
	activePrefs->DisplayID=GetVPModeID(&OurS->ViewPort);
	GetDisplayInfoData(0,(UBYTE *)&OurS_DIM,sizeof(OurS_DIM),DTAG_DIMS,activePrefs->DisplayID);
	
	if (TempS) { CloseScreen(TempS); TempS=0; }

												/* Farben setzen und 0-3 dunkel steuern */
	{
		struct PP_ColorFile cf;
		
		memcpy(&cf,&activePrefs->StdCols,sizeof(cf));
		memset(&cf.StdCol[0],0,sizeof(struct PP_CMEntry)*4);
		cf.StdCol[1].num=1;
		cf.StdCol[2].num=2;
		cf.StdCol[3].num=3;
		
		SetColors(&cf);
	}
	ScreenToFront(OurS);
	
	BorderCleared=1;

	COLm=4;				/* und Farbentabelle initialisieren */
	COLcnt=(1<<OurS->BitMap.Depth)-COLm;
		
	COL_CC1=.3;			/* Farben für die DrawCCross() */
	COL_CC2=.6;
	COL_CC3=.8;
		
	VisualInfo=GetVisualInfo(OurS,TAG_DONE);

	if (Options&OPT_ACR) ReduceColors(1); 	/* Auf Tiefe 2 schalten */
	
	/* Hauptwindow ------------------------------------------------------ */
	
	sprintf(Buffer,"3dPlot V%hd.%02hd %s - (P) %s %s S.Sichler - ®SSi\\93-97",PLOTVERSION,PLOTSUBVERSION,PROGTYPE,COMPILATIONDATE,l[27]);
	Title[1]=getstr(Buffer);
	if (!(OurW=OpenWindowTags(0,
		WA_Left,0, WA_Top,1, 
		WA_Width,OurS->Width, WA_Height, OurS->Height-1,
		WA_Flags,WFLG_ACTIVATE|WFLG_CLOSEGADGET/*|WFLG_BACKDROP*/,
		WA_Borderless,FALSE,
		WA_CustomScreen,(ULONG)OurS,
		WA_Title,Title[1],
		TAG_DONE))) { Error(l[78]); return(-1); }

	((struct Process *)FindTask(0))->pr_WindowPtr=OurW;

	/* Koordinatenbestimmung ------------------------------------------ */
		
	maxx=OurW->Width-1;
	maxy=OurW->Height-H(55);

	if (maxx<W(619) || maxy<H(149)) { Error(l[79]); return(-1); }
	
	BorderDrawx=4;
	BorderDrawy=H(8)+3;
	BorderDrawX=maxx-4;
	BorderDrawY=maxy;	

	{
		int Border;
		
		Border=BorderDrawX-BorderDrawx+1-activePrefs->DrawAreaWidth;
		Drawx=BorderDrawx+Border/2;
		DrawX=Drawx+activePrefs->DrawAreaWidth-1;
		Border=BorderDrawY-BorderDrawy+1-activePrefs->DrawAreaHeight;
		Drawy=BorderDrawy+Border/2;
		DrawY=Drawy+activePrefs->DrawAreaHeight-1;
	}

	_Midx=Midx=(double)(DrawX-Drawx)/2.0+(double)Drawx;
	_Midy=Midy=(double)(DrawY-Drawy)/2.0+(double)Drawy;

	Line(OurW->RPort,1,maxy+1,maxx-2,maxy+1,1);
/*	Line(OurW->RPort,1,maxy+2,maxx-2,maxy+2,2); */

	/* Optionen(2) ------------------------------------------------------- */
	
	#define OPTW_WIDTH W(300)
	#define OPTW_HEIGHT H(159)
	
	#define OPTW_LEFT (short)BorderDrawX-OPTW_WIDTH-(short)(activePrefs->ScreenXMeasure/50.0*activePrefs->XAspect)
	#define OPTW_TOP  (short)BorderDrawY-OPTW_HEIGHT-(short)(activePrefs->ScreenXMeasure/50.0*activePrefs->YAspect)+1
	
	sprintf(Buffer,"%s(2)",l[28]);
	Title[2]=getstr(Buffer);
	if (!(Opt1W=OpenWindowTags(0,WA_Left,OPTW_LEFT,WA_Top,OPTW_TOP,
		WA_Width,OPTW_WIDTH,WA_Height,OPTW_HEIGHT,
		WA_Flags,/*WFLG_BACKDROP|*/WFLG_CLOSEGADGET|WFLG_DRAGBAR,
		WA_CustomScreen,(ULONG)OurS,
		WA_Title,Title[2],
		TAG_DONE))) { Error(l[68]); return(-1); }
	
	WindowToBack(Opt1W);

	/* Optionen(1) ------------------------------------------------------- */
	
	sprintf(Buffer,"%s(1)",l[28]);
	Title[3]=getstr(Buffer);
	if (!(Opt2W=OpenWindowTags(0,WA_Left,OPTW_LEFT,WA_Top,OPTW_TOP,
		WA_Width,OPTW_WIDTH,WA_Height,OPTW_HEIGHT,
		WA_Flags,/*WFLG_BACKDROP|*/WFLG_CLOSEGADGET|WFLG_DRAGBAR,
		WA_CustomScreen,(ULONG)OurS,
		WA_Title,Title[3],
		TAG_DONE))) { Error(l[68]); return(-1); }
	
	WindowToBack(Opt2W);

	/* Disk-Window ------------------------------------------------------- */
	
	sprintf(Buffer,"%s",l[29]);
	Title[4]=getstr(Buffer);
	if (!(DiskW=OpenWindowTags(0,WA_Left,BorderDrawx+(short)(activePrefs->ScreenXMeasure/50.0*activePrefs->XAspect),
		WA_Top,(short)BorderDrawY-H(159)-(short)(activePrefs->ScreenXMeasure/50.0*activePrefs->YAspect)+1,
		WA_Width,W(300),WA_Height,H(159),
		WA_Flags,/*WFLG_BACKDROP|*/WFLG_CLOSEGADGET|WFLG_DRAGBAR,
		WA_CustomScreen,(ULONG)OurS,
		WA_Title,Title[4],
		TAG_DONE))) { Error(l[68]); return(-1); }

	WindowToBack(DiskW);

	/* Error-Window ----------------------------------------------------- */
	
	if (!(ErrW=OpenWindowTags(0,WA_Left,0,WA_Top,1,
		WA_Width,OurW->Width,WA_Height,H(11),
/*		WA_Flags,WFLG_BACKDROP, */
		WA_CustomScreen,(ULONG)OurS,
		WA_Title,Title[0],
		TAG_DONE))) { Error(l[68]); return(-1); }

	WindowToBack(ErrW);

	/* APS-Window ----------------------------------------------------- */
	
	if (!(ApsW=OpenWindowTags(0,WA_Left,BorderDrawx+(short)(activePrefs->ScreenXMeasure/50.0*activePrefs->XAspect),WA_Top,BorderDrawy+(short)(activePrefs->ScreenXMeasure/50.0*activePrefs->YAspect)+1,
		WA_Width,BorderDrawX-BorderDrawx-(short)(activePrefs->ScreenXMeasure/25.0*activePrefs->XAspect),WA_Height,H(8)+3,
/*		WA_Flags,WFLG_BACKDROP, */
		WA_CustomScreen,(ULONG)OurS,
		WA_Title,Title[0],
		TAG_DONE))) { Error(l[68]); return(-1); }

	WindowToBack(ApsW);

	/* 3dPlot Image einblenden ------------------------------------------ */

	if (!NotFirstExecution)
	{
		ReduceColors(0);	/* Mehr-Farben Modus */
	
		SetAPen(OurW->RPort,3);
		RectFill(OurW->RPort,BorderDrawx,BorderDrawy,BorderDrawX,BorderDrawY);
		BorderCleared=1;
		
		if (OurS->BitMap.Depth>=4) _3dPlotImage.PlanePick=0xf<<(OurS->BitMap.Depth-4);
		else
		{
			short width;
			width=_3dPlotImage.Width/16;
			width+=(width&0xf)?1:0;
			
			_3dPlotImage.PlanePick=0x7;
			_3dPlotImage.ImageData=&_3dPlotImage.ImageData[width*_3dPlotImage.Height];
			_3dPlotImage.Depth=3;
		}
		_3dPlotImage.LeftEdge=(BorderDrawX-BorderDrawx-_3dPlotImage.Width)/2+BorderDrawx;
		_3dPlotImage.TopEdge=(BorderDrawY-BorderDrawy-_3dPlotImage.Height)/2+BorderDrawy;
	
		DrawImage(OurW->RPort,&_3dPlotImage,0,0);
		
		_3dPlotImBorderBM.Planes[1]=_3dPlotImBorderBM.Planes[0];
		_3dPlotImBorderBM.Depth=2;
		
		BltBitMapRastPort(&_3dPlotImBorderBM,0,0,
		                  OurW->RPort,_3dPlotImage.LeftEdge,_3dPlotImage.TopEdge,
		                  _3dPlotImage.Width,_3dPlotImage.Height,ABC|NABC|ABNC|NABNC|ANBC|NANBC);
	
		StartTime(Image_TH,2);	/* Image soll min. 2 sec. eingeblendet bleiben. */
	}
	
	/* Vordefininitionen laden (Reset.fnct) ------------------------------ */

	if (!NotFirstExecution)
	{	
		char *name=0;
		BPTR oldLock=0;
		
		CompuSizeFact();/* Größenfaktor berechnen */	

		if (name=Default_fnct_Name)
		{
			oldLock=CurrentDir(DupLock(DefaultDir_Lock?DefaultDir_Lock:HomeDir_Lock));
		}
		if (!name && Reset_fnct_Name[0])
		{
			name=Reset_fnct_Name;
			oldLock=CurrentDir(DupLock(FnctDir_Lock?FnctDir_Lock:HomeDir_Lock));
		}
		
		if (name)
		{
			BPTR fp;
			
			if (fp=Open(name,MODE_OLDFILE))
			{
				struct FnctStruct *fs;
				
				fs=_LoadFnctStruct(fp);
				Close(fp);
				if (fs)
				{
					UseFnctStruct(fs,-1);
					FreeFnctStruct(fs);
				}
			}
			UnLock(CurrentDir(oldLock));
		}
	}

	/* Gadgets Hauptwindow ---------------------------------------------- */
	
	{
		struct Gadget *gad;
		
		gad=CreateContext(&FirstGad);
		
		GZFunct.ng_VisualInfo=VisualInfo;
		GZFunct.ng_TopEdge=maxy+H(7);
		GZFunct.ng_Width=maxx-W(150+60+110)-GZFunct.ng_LeftEdge;
		_FuncG=gad=CreateGadget(STRING_KIND,gad,&GZFunct,GTST_MaxChars,MAX_FNCTSTR_LEN-6,STRINGA_ExitHelp,TRUE,GTST_String,FunctionString,TAG_END);

		ModeDisplay_x=GZFunct.ng_LeftEdge+GZFunct.ng_Width+W(10);
		ModeDisplay_y=GZFunct.ng_TopEdge+textfont->tf_Baseline+2;
		
												/* Dreh-Funktionsgruppe */

		GSKnob.ng_VisualInfo=VisualInfo;
		GSKnob.ng_TopEdge=maxy+H(4);
		GSKnob.ng_LeftEdge=maxx-W(26);
		GSKnob.ng_GadgetText="A";
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_GadgetText="0";
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		GSKnob.ng_TopEdge=maxy+H(4);
		GSKnob.ng_LeftEdge-=W(22);
		GSKnob.ng_GadgetText="-";
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_TopEdge=maxy+H(4);
		GSKnob.ng_LeftEdge-=W(22);
		GSKnob.ng_GadgetText="+";
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GLText.ng_VisualInfo=VisualInfo;
		GLText.ng_TopEdge=maxy+H(4);
		GLText.ng_LeftEdge=GSKnob.ng_LeftEdge;
		GLText.ng_GadgetText=l[30];
		gad=CreateGadget(TEXT_KIND,gad,&GLText,TAG_END);
		GLText.ng_TopEdge+=H(12);
		GLText.ng_LeftEdge=GSKnob.ng_LeftEdge;
		GLText.ng_GadgetText=l[31];
		gad=CreateGadget(TEXT_KIND,gad,&GLText,TAG_END);
		GLText.ng_TopEdge+=H(12);
		GLText.ng_GadgetText=l[32];
		gad=CreateGadget(TEXT_KIND,gad,&GLText,TAG_END);

		GChoose.ng_VisualInfo=VisualInfo;
		GChoose.ng_LeftEdge=maxx-(GChoose.ng_Width)-W(8);
		GChoose.ng_TopEdge=maxy+H(40);
		GChoose.ng_GadgetText=l[88];
		gad=CreateGadget(BUTTON_KIND,gad,&GChoose,TAG_END);

		GChoose.ng_LeftEdge=maxx-2*(GChoose.ng_Width)-W(14);
		GChoose.ng_GadgetText=l[12];
		GChoose.ng_GadgetID=11;
		gad=CreateGadget(BUTTON_KIND,gad,&GChoose,TAG_END);

													/* Funktionsgruppe */

		GLText.ng_TopEdge=maxy+H(24);
		GLText.ng_LeftEdge=W(74);
		GLText.ng_Height=H(12);
		GLText.ng_GadgetText=l[33];
		gad=CreateGadget(TEXT_KIND,gad,&GLText,TAG_END);
		GLText.ng_TopEdge+=H(13);
		GLText.ng_GadgetText=l[34];
		gad=CreateGadget(TEXT_KIND,gad,&GLText,TAG_END);

		GStep.ng_VisualInfo=VisualInfo;
		GStep.ng_TopEdge=maxy+H(24);
		_xResG=gad=CreateGadget(INTEGER_KIND,gad,&GStep,GTIN_Number,(ULONG)xRes,GTIN_MaxChars,4,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		GRange1.ng_VisualInfo=VisualInfo;
		GRange1.ng_TopEdge=maxy+H(37);
		_xMinG=gad=CreateGadget(STRING_KIND,gad,&GRange1,GTST_String,ftoa(xMin),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		GRange2.ng_VisualInfo=VisualInfo;
		GRange2.ng_TopEdge=maxy+H(37);
		_xMaxG=gad=CreateGadget(STRING_KIND,gad,&GRange2,GTST_String,ftoa(xMax),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		GRText.ng_VisualInfo=VisualInfo;
		GRText.ng_TopEdge=maxy+H(37);
		gad=CreateGadget(TEXT_KIND,gad,&GRText,TAG_END);

		GStep.ng_LeftEdge+=W(90);
		GStep.ng_GadgetID++;
		GStep.ng_GadgetText="y ";
		GRange1.ng_LeftEdge+=W(90);
		GRange1.ng_GadgetID+=2;
		GRange2.ng_LeftEdge+=W(90);
		GRange2.ng_GadgetID+=2;
		GRText.ng_LeftEdge+=W(90);
		_yResG=gad=CreateGadget(INTEGER_KIND,gad,&GStep,GTIN_Number,(ULONG)yRes,GTIN_MaxChars,4,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		_yMinG=gad=CreateGadget(STRING_KIND,gad,&GRange1,GTST_String,ftoa(yMin),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		_yMaxG=gad=CreateGadget(STRING_KIND,gad,&GRange2,GTST_String,ftoa(yMax),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		gad=CreateGadget(TEXT_KIND,gad,&GRText,TAG_END);

		GStep.ng_LeftEdge+=W(90);
		GStep.ng_GadgetID++;
		GStep.ng_GadgetText="z ";
		GRange1.ng_LeftEdge+=W(90);
		GRange1.ng_GadgetID+=2;
		GRange2.ng_LeftEdge+=W(90);
		GRange2.ng_GadgetID+=2;
		GRText.ng_LeftEdge+=W(90);
		gad=CreateGadget(TEXT_KIND,gad,&GStep,TAG_END);
		_zMinG=gad=CreateGadget(STRING_KIND,gad,&GRange1,GTST_String,ftoa(zMin),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		_zMaxG=gad=CreateGadget(STRING_KIND,gad,&GRange2,GTST_String,ftoa(zMax),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		gad=CreateGadget(TEXT_KIND,gad,&GRText,TAG_END);
		
		GAct.ng_VisualInfo=VisualInfo;
		GAct.ng_GadgetID=21;
		GAct.ng_TopEdge=maxy+H(40);
		GAct.ng_LeftEdge=(maxx-W(214+340-12)-GAct.ng_Width)/2+W(340-12);
		GAct.ng_GadgetText=l[36];
		gad=CreateGadget(BUTTON_KIND,gad,&GAct,TAG_END);
		GAct.ng_GadgetID=22;
		GAct.ng_TopEdge=GChoose.ng_TopEdge;
		GAct.ng_LeftEdge=GChoose.ng_LeftEdge-GAct.ng_Width-9;
		GAct.ng_GadgetText=l[37];
		gad=CreateGadget(BUTTON_KIND,gad,&GAct,TAG_END);

											/* Verschiebe-Funktionsgruppe */

		GSKnob.ng_TopEdge=maxy+H(4);
		GSKnob.ng_LeftEdge=maxx-W(192);
		GSKnob.ng_GadgetText="^";
		GSKnob.ng_GadgetID=23;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_LeftEdge-=W(22);
		GSKnob.ng_GadgetText="<";
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_LeftEdge+=W(22);
		GSKnob.ng_GadgetID++;
		GSKnob.ng_GadgetText="·";
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		GSKnob.ng_LeftEdge+=W(22);
		GSKnob.ng_GadgetText=">";
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;
		GSKnob.ng_TopEdge+=H(12);
		GSKnob.ng_LeftEdge-=W(22);
		GSKnob.ng_GadgetText="v";
		GSKnob.ng_GadgetID++;
		gad=CreateGadget(BUTTON_KIND,gad,&GSKnob,TAG_END);
		if (gad) gad->Activation|=GACT_IMMEDIATE;

		if (!gad) { Error(l[80]); return(-1); }

		GHelpScroller.ng_VisualInfo=VisualInfo;
		GHelpScroller.ng_LeftEdge=BorderDrawX-10-GHelpScroller.ng_Width;
		GHelpScroller.ng_TopEdge=BorderDrawy+10;
		GHelpScroller.ng_Height=BorderDrawY-BorderDrawy-20;
	
		/* Gadgets Options(1)-window ----------------------------------- */

		gad=CreateContext(&FirstOpt1Gad);
		
		OGSwitch.ng_VisualInfo=VisualInfo;
		OGSwitch.ng_TopEdge=H(13);
		OGSwitch.ng_GadgetText=l[94];
		OGSwitch.ng_GadgetID=51;
		_OPT_APS_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_APS)?TRUE:FALSE,TAG_END);
		OGMinIn.ng_VisualInfo=VisualInfo;
		OGMinIn.ng_GadgetText=l[95];
		OGMinIn.ng_GadgetID=52;
		_APS_Int_G=gad=CreateGadget(INTEGER_KIND,gad,&OGMinIn,GTIN_MaxChars,6,GTIN_Number,APS_MinIntervall,STRINGA_ExitHelp,TRUE,GA_Disabled,(Options&OPT_APS)?FALSE:TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		OGMinIn.ng_GadgetText=l[96];
		OGMinIn.ng_Flags=PLACETEXT_RIGHT;
		gad=CreateGadget(TEXT_KIND,gad,&OGMinIn,TAG_END);

		Line(Opt1W->RPort,2,H(37),OPTW_WIDTH-3,H(37),1);
		Line(Opt1W->RPort,2,H(38),OPTW_WIDTH-3,H(38),2);

		OGText2.ng_VisualInfo=VisualInfo;
		OGText2.ng_GadgetText=l[97];
		gad=CreateGadget(TEXT_KIND,gad,&OGText2,TAG_END);

		OGMinIn.ng_TopEdge=OGText2.ng_TopEdge;
		OGMinIn.ng_LeftEdge=OPTW_WIDTH-W(108);
		OGMinIn.ng_Flags=PLACETEXT_RIGHT;
		OGMinIn.ng_GadgetText=l[117];
		OGMinIn.ng_GadgetID=53;
		gad=CreateGadget(INTEGER_KIND,gad,&OGMinIn,GTIN_MaxChars,6,GTIN_Number,TimeMinThreshold,STRINGA_ExitHelp,TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;

/*		Line(Opt1W->RPort,2,H(50),OPTW_WIDTH-3,H(50),1);
		Line(Opt1W->RPort,2,H(51),OPTW_WIDTH-3,H(51),2);
*/		
		OGSwitch.ng_TopEdge=H(54);
		OGSwitch.ng_GadgetText=l[40];
		OGSwitch.ng_GadgetID=36;
		_OPT_ACR_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_ACR)?TRUE:FALSE,TAG_END);
		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=38;
		OGSwitch.ng_GadgetText=l[42];
		_OPT_Time_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_Time)?TRUE:FALSE,TAG_END);
		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=46;
		OGSwitch.ng_GadgetText=l[44];
		_OPT_WorkBench_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_WorkBench)?TRUE:FALSE,TAG_END);		
		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=49;
		OGSwitch.ng_GadgetText=l[89];
		_OPT_LowPri_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_LowPri)?TRUE:FALSE,TAG_END);
		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=54;
		OGSwitch.ng_GadgetText=l[99];
		gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_AWH)?TRUE:FALSE,TAG_END);
		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=61;
		OGSwitch.ng_GadgetText=l[114];
		gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_Backup)?TRUE:FALSE,TAG_END);

		OGBackup.ng_TopEdge=OGSwitch.ng_TopEdge;
		OGBackup.ng_LeftEdge=OPTW_WIDTH-W(8)-OGBackup.ng_Width;
		OGBackup.ng_VisualInfo=VisualInfo;
		_OGBakFile=gad=CreateGadget(STRING_KIND,gad,&OGBackup,GTST_String,BackupFileName,GTST_MaxChars,559,STRINGA_ExitHelp,TRUE,GA_Disabled,(Options&OPT_Backup)?FALSE:TRUE,TAG_END);
		
		Line(Opt1W->RPort,2,OPTW_HEIGHT-H(20),OPTW_WIDTH-3,OPTW_HEIGHT-H(20),1);
		Line(Opt1W->RPort,2,OPTW_HEIGHT-H(19),OPTW_WIDTH-3,OPTW_HEIGHT-H(19),2);

		OGPrefs.ng_VisualInfo=VisualInfo;
		OGPrefs.ng_TopEdge=OPTW_HEIGHT-H(16);
		OGPrefs.ng_Width=OPTW_WIDTH-W(16);
		OGPrefs.ng_GadgetText=l[93];
		gad=CreateGadget(BUTTON_KIND,gad,&OGPrefs,TAG_END);
		
		if (!gad) { Error(l[80]); return(-1); }
		
		/* Gadgets Options(2)-window ----------------------------------- */

		gad=CreateContext(&FirstOpt2Gad);

		OGLightChoose.ng_VisualInfo=VisualInfo;
		OGLightChoose.ng_GadgetID=31;
		{
			short active=0;
			if (DrawMode&DMF_FL) { active=1; if (DrawMode&DMF_FLL) active=2; if (DrawMode&DMF_COL) active=3; }
			_OBodyG=gad=CreateGadget(CYCLE_KIND,gad,&OGLightChoose,GTCY_Labels,_OGLightChoose1,GTCY_Active,active,TAG_END);
		}
		OGLightChoose.ng_TopEdge=H(44);
		OGLightChoose.ng_GadgetID=59;
		{
			short active=0;
			if (DrawMode&DMF_PAT) active=(DrawMode&DMF_LCOL)?2:1; 
			_OGPatColG=gad=CreateGadget(CYCLE_KIND,gad,&OGLightChoose,GTCY_Labels,_OGLightChoose2,GTCY_Active,active,TAG_END);
		}
		OGChoose.ng_VisualInfo=VisualInfo;
		OGChoose.ng_GadgetText=l[13];
		{
			short active=0;
			if (DrawMode&DMF_LX) active=1;
			if (DrawMode&DMF_LY) active+=2;
			_OPattG=gad=CreateGadget(CYCLE_KIND,gad,&OGChoose,GTCY_Labels,_OGChoose1,GTCY_Active,active,GA_Disabled,(DrawMode&DMF_PAT)?FALSE:TRUE,TAG_END);
		}
		OGText.ng_VisualInfo=VisualInfo;
		OGText.ng_GadgetText=l[23];
		gad=CreateGadget(TEXT_KIND,gad,&OGText,TAG_END);
		OGInput.ng_VisualInfo=VisualInfo;
		OGInput.ng_GadgetText="x";
		_OxSpaceG=gad=CreateGadget(INTEGER_KIND,gad,&OGInput,GTIN_MaxChars,16,GTIN_Number,(long)SpacingX,STRINGA_ExitHelp,TRUE,GA_Disabled,(DrawMode&DMF_PAT)?FALSE:TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;
		OGInput.ng_GadgetID++;
		OGInput.ng_LeftEdge+=W(64);
		OGInput.ng_GadgetText="y";
		_OySpaceG=gad=CreateGadget(INTEGER_KIND,gad,&OGInput,GTIN_MaxChars,16,GTIN_Number,(long)SpacingY,STRINGA_ExitHelp,TRUE,GA_Disabled,(DrawMode&DMF_PAT)?FALSE:TRUE,TAG_END);
		if (gad) gad->Activation|=GACT_STRINGRIGHT;

		OGLightSel.ng_VisualInfo=VisualInfo;
		OGLightSel.ng_LeftEdge=W(210);
		OGLightSel.ng_TopEdge=H(15);
		OGLightSel.ng_GadgetID=92;
		_OGBodyL=gad=CreateGadget(SLIDER_KIND,gad,&OGLightSel,GTSL_Min,(UWORD)0,GTSL_Max,(UWORD)20,GTSL_LevelFormat,"%3ld%%",
			GTSL_MaxLevelLen,(UWORD)4,GTSL_DispFunc,&__RfxSliderConv,GTSL_Level,(UWORD)(BodyCol*20.0),GA_RelVerify,TRUE,GA_Disabled,((DrawMode&(DMF_FL|DMF_FLL|DMF_COL))==(DMF_FL|DMF_FLL))?FALSE:TRUE,TAG_END);

		OGLightSel.ng_TopEdge=H(46);
		OGLightSel.ng_GadgetID=93;
		_OGPatL=gad=CreateGadget(SLIDER_KIND,gad,&OGLightSel,GTSL_Min,(WORD)((DrawMode&DMF_LCOL)?-20:0),GTSL_Max,20,GTSL_LevelFormat,(DrawMode&DMF_LCOL)?"%4ld%%":" %3ld%%",
			GTSL_MaxLevelLen,(UWORD)5,GTSL_DispFunc,__RfxSliderConv,GTSL_Level,(WORD)(((DrawMode&DMF_LCOL)?PatternDeltaCol:PatternCol)*20.0),GA_RelVerify,TRUE,GA_Disabled,(DrawMode&DMF_PAT)?FALSE:TRUE,TAG_END);
		
		Line(Opt2W->RPort,2,H(39),OPTW_WIDTH-3,H(39),1);
		Line(Opt2W->RPort,2,H(40),OPTW_WIDTH-3,H(40),2);
	
		{
			BOOL LightNotUsed;
			
			LightNotUsed=(DrawMode&(DMF_COL|DMF_LCOL))?FALSE:TRUE;
	
			OGLightSel.ng_VisualInfo=VisualInfo;
			OGLightSel.ng_LeftEdge=W(65);
			OGLightSel.ng_TopEdge=H(27);
			OGLightSel.ng_GadgetID=57;
			OGLightSel.ng_GadgetText=l[106];
			_dRfxG=gad=CreateGadget(SLIDER_KIND,gad,&OGLightSel,GTSL_Min,(UWORD)0,GTSL_Max,(UWORD)40,GTSL_LevelFormat,"%3ld%%",
				GTSL_MaxLevelLen,(UWORD)4,GTSL_DispFunc,&__RfxSliderConv,GTSL_Level,(UWORD)(Diffuse_Rfx*20.0),GA_RelVerify,TRUE,GA_Disabled,LightNotUsed,TAG_END);
	
			OGLightSel.ng_LeftEdge=W(210);
			OGLightSel.ng_GadgetID=58;
			OGLightSel.ng_GadgetText=l[107];
			_pRfxG=gad=CreateGadget(SLIDER_KIND,gad,&OGLightSel,GTSL_Min,(UWORD)0,GTSL_Max,(UWORD)40,GTSL_LevelFormat,"%3ld%%",
				GTSL_MaxLevelLen,(UWORD)4,GTSL_DispFunc,&__RfxSliderConv,GTSL_Level,(UWORD)(Direct_Rfx*20.0),GA_RelVerify,TRUE,GA_Disabled,LightNotUsed,TAG_END);
	
			Line(Opt2W->RPort,2,H(72),OPTW_WIDTH-3,H(72),1);
			Line(Opt2W->RPort,2,H(73),OPTW_WIDTH-3,H(73),2);
	
			OGText.ng_TopEdge=H(76);
			OGText.ng_GadgetText=l[39];
			OGText.ng_LeftEdge=W(16)+strlen(l[39])*W(8);
			gad=CreateGadget(TEXT_KIND,gad,&OGText,TAG_END);
	
			OGLightSel.ng_TopEdge=H(76)+1;
			OGLightSel.ng_LeftEdge=W(210);
			OGLightSel.ng_GadgetID=60;
			OGLightSel.ng_GadgetText=0;
			_OLIntensG=gad=CreateGadget(SLIDER_KIND,gad,&OGLightSel,GTSL_Min,0,GTSL_Max,20,GTSL_LevelFormat,"%3ld%%",GTSL_MaxLevelLen,4,GTSL_DispFunc,&__RfxSliderConv,GTSL_Level,(short)(LightIntens[SLS]*20.0),GA_RelVerify,TRUE,
					GA_Disabled,(LightNotUsed||!(LightsOn&(1<<SLS)))?TRUE:FALSE,TAG_END);
	
			OGLightSel.ng_LeftEdge=W(119);
			OGLightSel.ng_TopEdge=H(77);
			OGLightSel.ng_Width=W(40);
			OGLightSel.ng_GadgetID=55;
			LightSelG=gad=CreateGadget(SLIDER_KIND,gad,&OGLightSel,GTSL_Min,1,GTSL_Max,MAXLIGHTS,GTSL_LevelFormat,"Nº%ld",GTSL_MaxLevelLen,3,GTSL_Level,SLS+1,GA_RelVerify,TRUE,GA_Disabled,LightNotUsed,TAG_END);
	
			OGLightChoose.ng_VisualInfo=VisualInfo;
			OGLightChoose.ng_LeftEdge=W(210);
			OGLightChoose.ng_TopEdge=H(89);
			OGLightChoose.ng_Width=W(80);
			OGLightChoose.ng_GadgetID=56;
			_OGLightStateG=gad=CreateGadget(CYCLE_KIND,gad,&OGLightChoose,GTCY_Labels,_OGLightChoose,GTCY_Active,(LightsOn&(1<<SLS))?1:0,GA_Disabled,LightNotUsed,TAG_END);
	
			OGInput.ng_GadgetID=33;
			OGInput.ng_TopEdge=H(89);
			OGInput.ng_LeftEdge=W(26);
			OGInput.ng_GadgetText="x";
			_OxLightG=gad=CreateGadget(STRING_KIND,gad,&OGInput,GTST_String,ftoa(LightVect[SLS].x),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,GA_Disabled,(LightNotUsed||!(LightsOn&(1<<SLS)))?TRUE:FALSE,TAG_END);
			if (gad) gad->Activation|=GACT_STRINGRIGHT;
			OGInput.ng_GadgetID++;
			OGInput.ng_LeftEdge+=W(64);
			OGInput.ng_GadgetText="y";
			_OyLightG=gad=CreateGadget(STRING_KIND,gad,&OGInput,GTST_String,ftoa(LightVect[SLS].y),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,GA_Disabled,(LightNotUsed||!(LightsOn&(1<<SLS)))?TRUE:FALSE,TAG_END);
			if (gad) gad->Activation|=GACT_STRINGRIGHT;
			OGInput.ng_GadgetID++;
			OGInput.ng_LeftEdge+=W(64);
			OGInput.ng_GadgetText="z";
			_OzLightG=gad=CreateGadget(STRING_KIND,gad,&OGInput,GTST_String,ftoa(LightVect[SLS].z),GTST_MaxChars,16,STRINGA_ExitHelp,TRUE,GA_Disabled,(LightNotUsed||!(LightsOn&(1<<SLS)))?TRUE:FALSE,TAG_END);
			if (gad) gad->Activation|=GACT_STRINGRIGHT;
		}
		
		Line(Opt2W->RPort,2,H(104),OPTW_WIDTH-3,H(104),1);
		Line(Opt2W->RPort,2,H(105),OPTW_WIDTH-3,H(105),2);

		OGSwitch.ng_TopEdge=H(108);
		OGSwitch.ng_GadgetID=37;
		OGSwitch.ng_GadgetText=l[41];
		_OPT_ZLimit_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_ZLimit)?TRUE:FALSE,TAG_END);
		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=39;
		OGSwitch.ng_GadgetText=l[43];
		_OPT_Quad_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_Quad)?TRUE:FALSE,TAG_END);
		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=47;
		OGSwitch.ng_GadgetText=l[45];
		_OPT_DitherCol_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_DitherCol)?TRUE:FALSE,TAG_END);

		OGSwitch.ng_TopEdge+=H(12);
		OGSwitch.ng_GadgetID=80;
		OGSwitch.ng_GadgetText=l[143];
		_OPT_RG3d_G=gad=CreateGadget(CHECKBOX_KIND,gad,&OGSwitch,GTCB_Checked,(Options&OPT_RG3d)?TRUE:FALSE,TAG_END);

		if (!gad) { Error(l[80]); return(-1); }

		/* Disk-window ----------------------------------------- */

		gad=CreateContext(&FirstDiskGad);

		DGList.ng_VisualInfo=VisualInfo;
		_DGList=gad=CreateGadget(LISTVIEW_KIND,gad,&DGList,TAG_END);

		DGText.ng_VisualInfo=VisualInfo;
		DGText.ng_GadgetText=l[24];
		_DGPath=gad=CreateGadget(STRING_KIND,gad,&DGText,GTST_MaxChars,512,TAG_END);
		DGText.ng_GadgetID=81;
		DGText.ng_TopEdge+=H(13);
		DGText.ng_GadgetText=l[119];
		_DGPat=gad=CreateGadget(STRING_KIND,gad,&DGText,GTST_MaxChars,39,TAG_END);
		DGText.ng_GadgetID=42;
		DGText.ng_TopEdge+=H(13);
		DGText.ng_GadgetText=l[46];
		_DGFile=gad=CreateGadget(STRING_KIND,gad,&DGText,GTST_MaxChars,35,TAG_END);

		DGLoad.ng_VisualInfo=VisualInfo;
		DGLoad.ng_GadgetText=l[25];
		gad=CreateGadget(BUTTON_KIND,gad,&DGLoad,TAG_END);

		Line(DiskW->RPort,2,H(139),W(300)-3,H(139),1);
		Line(DiskW->RPort,2,H(140),W(300)-3,H(140),2);

		DGSave.ng_VisualInfo=VisualInfo;
		DGSave.ng_GadgetText=l[26];
		gad=CreateGadget(BUTTON_KIND,gad,&DGSave,TAG_END);
		DGSave.ng_GadgetID++;
		DGSave.ng_LeftEdge+=W(146);
		DGSave.ng_GadgetText=l[47];
		gad=CreateGadget(BUTTON_KIND,gad,&DGSave,TAG_END);

		if (!gad) { Error(l[80]); return(-1); }

		/* Gadgets aktivieren ------------------------------------------- */
		
		if (ConsoleState!=CS_PREFS)
		{
			ConsoleState=CS_MAIN;
			AddGList(OurW,FirstGad,-1,-1,0);
			RefreshGadgets(FirstGad,OurW,0);
			GT_RefreshWindow(OurW,0);
			SetDrawGadget(0);
			
			RenderMainWindow();
		}
		
		AddGList(Opt1W,FirstOpt1Gad,-1,-1,0);
		RefreshGadgets(FirstOpt1Gad,Opt1W,0);
		GT_RefreshWindow(Opt1W,0);

		AddGList(Opt2W,FirstOpt2Gad,-1,-1,0);
		RefreshGadgets(FirstOpt2Gad,Opt2W,0);
		GT_RefreshWindow(Opt2W,0);

		AddGList(DiskW,FirstDiskGad,-1,-1,0);
		RefreshGadgets(FirstDiskGad,DiskW,0);
		GT_RefreshWindow(DiskW,0);
	}

	InitArea(&areaInfo,areaBuffer,10);	/* AreaInfo initialisieren */
	OurW->RPort->AreaInfo=&areaInfo;
		
	tmpras.Size=((maxx+16)>>3)*maxy;
	if (!(tmpras.RasPtr=AllocMem(tmpras.Size,MEMF_CLEAR|MEMF_CHIP))) { Error(l[68]); return(-1); }
	OurW->RPort->TmpRas=&tmpras;

	RefreshLightGadgets();	/* Licht-selektionsgad. initialisieren */
						
						/* Bildschirm einblenden ----------------------- */
	if (!NotFirstExecution)
	{
		WaitTime(Image_TH);	/* warten, bis Wartezeit für Image vorbei ist */
	}
	SetColors((Options&OPT_RG3d)?&activePrefs->RG3dCols:&activePrefs->StdCols);

	ScreenToFront(OurS);

	if (ConsoleState==CS_PREFS) { ConsoleState=CS_CLEAR; Edit_Prefs(); };

									/* Disktask starten -------------------- */
	if (!notall)
	{
		struct OwnMsg *msg;

		if (!(DiskTaskPtr=CreateNewProcTags(NP_Entry,&DiskTask,NP_Name,"3dPlot/Disktask",TAG_END))) { Error(l[81]); return(-1); }
		
		for (msg=0;;)		/* auf Startnachricht warten */
		{
			if (msg) ReplyMsg((struct Message *)msg);
			WaitPort(MainTask_MsgPort);
			if (!(msg=(struct OwnMsg *)GetMsg(MainTask_MsgPort))) continue;
			if (msg->Msg.mn_Length!=2) continue;
			if (msg->Type==MT_OK) break;
			if (msg->Type==MT_ENDED) { Error(l[81]); return(-1); }
		}
		ReplyOwnMsg(msg);	
	}
	else
	{
		if (DiskTaskPtr)
		{
			struct OwnMsg *msg;
			
			SendMainMsg(MT_ATTACH);
			for (msg=0;;)
			{
				if (msg) ReplyMsg((struct Message *)msg);
				WaitPort(MainTask_MsgPort);
				if (!(msg=(struct OwnMsg *)GetMsg(MainTask_MsgPort))) continue;
				if (msg->Msg.mn_Length!=2) continue;
				if (msg->Type==MT_OK) break;
			}
			ReplyOwnMsg(msg);
		}
	}
		
	UseErrorWindow=1;	/* Errorausgabe auf Errorwindow leiten */
	ClrError();
		
	if (!notall) if (Options&OPT_AWH) WindowToFront(DiskW);
	SetOurPri();
	ReduceColors(0);
	
	/* IDCMP System installieren ---------------------------------------- */
	
	UnlockIDCMP();

	if (!NotFirstExecution) AddCtrlBrkHandler();

	NotFirstExecution=1;
	return(0);
}

/* Gadgethilfsroutinen ------------------------------------------------------ */

LONG __interrupt __RfxSliderConv(struct Gadget *gadget,WORD level)
{
	return(level*5);	/* Conversion für Slidergadgets 57/58 u. 92/93 */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int __regargs W(int origWidth)	/* zur Fontanpassung */
{
	return(origWidth*textfont->tf_XSize/8);
}

int __regargs H(int origHeight)	/* zur Fontanpassung */
{
	return(origHeight*textfont->tf_YSize/8);
}

void RenderMainWindow(void)
{
	DrawBevelBox(OurW->RPort,maxx-W(146),maxy+H(3),W(142),H(36),GT_VisualInfo,VisualInfo,GTBB_Recessed,TAG_END);
	DrawBevelBox(OurW->RPort,W(6),maxy+H(22),W(328),H(29),GT_VisualInfo,VisualInfo,GTBB_Recessed,TAG_END);		
	DrawBevelBox(OurW->RPort,maxx-W(218),maxy+H(3),W(70),H(36),GT_VisualInfo,VisualInfo,GTBB_Recessed,TAG_END);
}

