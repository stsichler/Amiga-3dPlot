/* Plot.h - 25-12-93 SSi */

#ifndef PLOT_H
#define PLOT_H
#define INTUI_V37_NAMES_ONLY

extern short PLOTVERSION,PLOTSUBVERSION;
extern char *COMPILATIONDATE;

#include <user/user.h>
#include <user/functions.h>
#include <exec/ports.h>
#include <proto/dos.h>

/* Definitionen zur Mathematikartbestimmung --------------------------- */

#define STD_FLOATTYPE 0
#define FFP_FLOATTYPE 1
#define IEEE_FLOATTYPE 2
#define FPU_FLOATTYPE 3
#define REAL_FLOATTYPE 4

#ifdef _FFP
#define FLOATTYPE FFP_FLOATTYPE
#endif
#ifdef _IEEE
#define FLOATTYPE IEEE_FLOATTYPE
#endif
#ifdef _M68881
#define FLOATTYPE FPU_FLOATTYPE
#endif

#ifndef FLOATTYPE
#define FLOATTYPE STD_FLOATTYPE
#endif

/* -------------------------------------------------------------------- */

extern struct VarList Vars;

/* Operatorprioritäten ------------------------------------------------ */

/* siehe user/functions.h */

/* -------------------------------------------------------------------- */

typedef struct
{
	short bx;
	short by;
} point;

typedef struct		/* ACHTUNG: darf nicht als Funktionsargument */
{					/* übergeben werden (Compiler kann das nicht) */
	point p1;
	point p2;
} line;

/* Nachrichten zwischen den beiden Tasks ------------------------------ */

struct OwnMsg
{
	struct Message Msg;	/* eigentliche Nachrichtenstruktur, Length=2 */
	short Type;			/* Nachrichtentype (MT_) */
};

/* ------------------------------------------------------------------------- */

struct PP_CMEntry	/* (s.u.) */
{
	ULONG num, red, green, blue;
};

struct PP_ColorFile /* (s.u.) */
{
	struct PP_CMEntry StdCol[4];

	short	Flags;		/* s.u. PPCF_... */
	char	Red[80];
	char	Green[80];
	char	Blue[80];
};

#define PPCF_RG3d 1	/* Red/Green/Blue im RG3d Modus */
#define PPCF_ERR	2	/* Formeln sind fehlerhaft */

/* Prototypen --------------------------------------------------------- */

void goto_exit(int);

int OpenAll(int);
void RenderMainWindow(void);

void CloseAll(int);
void __interrupt StripIntuiMessages(struct MsgPort *,struct Window *);
void LockIDCMP(void);
void UnlockIDCMP(void);

void PrepNewFunction(void);
int Mode(short);
void DoMode(void);
void ReduceColors(short);
int AskWriteIfExists(void);
int AskForExit(void);
int AskForClear(void);
void SetOurPri(void);
void GetAPSFilename(void);
int _TestRange(short);
void SetDrawGadget(short);

short CompileFunction(fnctcode **);
void ClearFunctionCode(fnctcode **);

void PreTransform(vector *);
point Transform(vector *,double);
void DrawCCross(int);
int DrawLine(point,point,short);
int DrawRawLine(point,point);
void FastDrawLine(line *,short);
int CorrectLine(line *);
void DrawFunction(void);
void DrawTriangle(point,point,point);
void CompuSizeFact(void);
void DisplayMode(void);
void RetraceVect(vector *);
double ComputeColor(vector *,vector *,vector *,vector *);
void SetColor(double,short);
void NormalColor(void);
void PrintTime(double);
void MakeDrawingInfo(void);

void ComputeBaseVects(void);
void MakeBaseVect(vector *,vector);

int DiskTask(void);

int SavePicture(void);
int _SavePicture(BPTR);

int _LoadPicture(BPTR,short);
int _ReadIFF_BMHD(BPTR);

struct FnctStruct *LoadFnctStruct(void);		/* FnctStruct= */
struct FnctStruct *_LoadFnctStruct(BPTR);		/* FnctStruct= */
int SaveFnctStruct(struct FnctStruct *);		/* Error= */
int _SaveFnctStruct(BPTR,struct FnctStruct *);/* Error= */
struct FnctStruct *BuildFnctStruct(void);		/* FnctStruct= */
int UseFnctStruct(struct FnctStruct *,short);	/* Error= */
void FreeFnctStruct(struct FnctStruct *);
ULONG GetFirstLWofFile(char *);

char *ftoa(double);

void ComputeWBMsg(int,char **);
void CreateIcon(char *);

void DisplayHelp(short);
void EndDisplayHelp(void);
void RemoveHelpScroller(void);
void PrintLines(short);

void LoadLanguage(void);

void __interrupt Error(char *);
void __interrupt Message(char *);
void __interrupt ClrError(void);

void AddCtrlBrkHandler(void);
void RemCtrlBrkHandler(void);

void RefreshLightGadgets(void);

int SaveBreakFile(char *);
int _SaveBreakStruct(BPTR);

int LoadBreakFile(char *);
int _LoadBreakStruct(BPTR);

void BackupFile(char *);

int IsPointInTri(point,point []);
int DoLinesIntersect(line *,line *);

int __regargs W(int);	/* Fontanpassung */
int __regargs H(int);

struct PlotPrefs *Load_default_Prefs(void);
struct PlotPrefs *Load_Prefs(void);
struct PlotPrefs *_Load_Prefs(BPTR);
int Save_Prefs(struct PlotPrefs *);
int _Save_Prefs(BPTR,struct PlotPrefs *);
int Use_Prefs(struct PlotPrefs *);
void Free_Prefs(struct PlotPrefs *);
struct PlotPrefs *Copy_Prefs(struct PlotPrefs *);
void Edit_Prefs(void);
void End_Prefs(void);
void DrawTestScreen(void);
void OpenResolutionWindow(void);
void CloseResolutionWindow(short);

void SetRGB(struct ViewPort *vp,unsigned long n,unsigned long r,unsigned long g,unsigned long b);
void SetRGBCM(struct ColorMap *cm,unsigned long n,unsigned long r,unsigned long g,unsigned long b);
void GetRGB(struct ColorMap *cm, unsigned long n, ULONG *table);
int SetColors(struct PP_ColorFile *);
UWORD MakeDIList(ULONG);
void FreeDIList(void);

void __interrupt GadDisable(struct Gadget *,struct Window *,BOOL state);
int GadgetCount(struct Gadget *);
void FreeGadgetList(struct Gadget *,int);

void OpenPaletteWindow(void);
void ClosePaletteWindow(short mode);

/* Globale Daten --------------------------------------------------------- */

extern const short GADGETNUM;	/* siehe GList.c */

extern char **l;	/* Zeiger auf aktiven Wortschatz */

extern char *Language;/* Name des akt. Languagefiles (0=English stdrd.) */

extern UWORD OSVERSION;

extern struct PlotPrefs *activePrefs;

extern unsigned long _IterationBrk;

/* Statusflags / Modi ---------------------------------------------------- */

#define ST_NO_FUNC 1		/* Funktion ungültig */
#define ST_RANGE 2		/* Range nicht gültig */
#define ST_RES 4			/* Resolution nicht gültig */ 
#define ST_SPACE 8		/* Spacingwerte nicht gültig */

#define MODE_START 0	/* Programm wurde gerade gestartet */
#define MODE_WAIT	1	/* in Wartestellung (Bildschirm: leer/Funktion) */
#define MODE_DRAW	2	/* gerade beim Zeichnen (Bildschirm: Funktion) */
#define MODE_EDIT	3	/* gerade beim Editieren (Bildschirm: KoorKreuz) */
#define MODE_DISPL 4	/* in Wartestellung (Bildschirm: KoorKreuz) */
#define MODE_CLEAR 5	/* in Wartestellung (Bildschirm: leer) */
#define MODE_STARTEDIT 7	/* Start des Edit-Modus */
#define MODE_DOCLEAR 8	/* Bildschirm wird gelöscht, dann ->MODE_CLEAR */
#define MODE_LOAD 9		/* laden */
#define MODE_SAVE 10		/* speichern */
#define MODE_OLD	11		/* alten Modus (vor MODE_LOAD / MODE_SAVE) holen */
#define MODE_HELP 14		/* Hilfemodus (derz. Bilschrim: Hilfetext) */
#define MODE_TEST 15		/* Abfragen, ob Bild gelöscht werden kann */
#define MODE_ENDREQ 16	/* Programm wird beendet */

#define CS_CLEAR 0			/* Konsole ist leer (Var: short ConsoleState) */
#define CS_MAIN 1				/* Konsole zeigt Hauptkonsole */
#define CS_PREFS 2			/*      -"-      Prefs-Konsole */

#define DMF_PAT (1<<0)	/* Punkte zeichnen (min.) */	/* Zeichenmodusflags */
#define DMF_LX (1<<1)		/* Linien x */
#define DMF_LY (1<<2)		/* Linien y */
#define DMF_LCOL (1<<4)	/* Muster farbig (an den Fnktkörper angepaßt) */
// #define DMF_LZN (1<<5)	/* Muster dunkler als Fnkt. [OBSOLETE] */
#define DMF_FL (1<<8)		/* Funktion gefüllt */
#define DMF_FLL (1<<9)	/* Funktion farbig gefüllt */
#define DMF_COL (1<<10)	/* Funktion beleuchtet gefüllt (nach dRfx/pRfx) */

#define DMF_ALL (DMF_PAT|DMF_LX|DMF_LY|DMF_LCOL|DMF_FL|DMF_FLL|DMF_COL)

#define OPT_ACR (1<<0)	/* AutoColorRed */		/* Optionen */
#define OPT_ZLimit (1<<1)	/* z-Wert Begrenzung */
#define OPT_Time (1<<2)	/* benötigte Zeit schätzen */
#define OPT_Quad (1<<3)	/* nur Vierecke zeichnen */
#define OPT_WorkBench (1<<4) /* Workbench ist offen */
#define OPT_INTERLACE (1<<5) /* nur noch zur Kompatibilität */
#define OPT_DitherCol (1<<6) /* Color-Dithering verwenden ? */
#define OPT_LowPri (1<<7)	/* niedere Task Priorität */
#define OPT_APS (1<<8)	/* autom. Bildsicherung */
#define OPT_AWH (1<<9)	/* autom. WindowHandling */
#define OPT_Backup (1<<10) /* Backupfiles verwenden */
#define OPT_RG3d (1<<11)	/* Rot-Grün-3d Modus */

#define MT_ENDREQ 1		/* Anfrage auf Beendigung des Disktasks (mit MT_ENDED quittiert) */
#define MT_ENDED 2		/* Disktask hat beendet */
#define MT_DISKREFRESHREQ 3 /* Anfrage auf Auffrischung der angez. Dir */
#define MT_DETACH 4		/* Window freigeben (mit MT_OK quittiert) */
#define MT_ATTACH 5		/* Window wieder ankoppeln und benutzen (mit MT_OK quittiert) */
#define MT_OK 6			/* allgem. Bestätigung */

/* sonstige Konstanten ------------------------------------------------------ */

#define pi 3.1415927

#define MAXXRES 5000	/* max. erlaubte Auflösung beim Zeichnen */
#define MAXYRES 5000

#define MAXLIGHTS 4	/* max. Anzahl der Lichtquellen */

#define ILL -10000 /* "Illegal"-Kennzeichnung für Punkte (point: bx=by=ILL) */

#define MAX_FNCTSTR_LEN 400	/* max. Länge für gesamten Funktionsstring */

/* Funktion-Lade/Speicherstruktur für V2.1 ---------------------------------- */

#define IDSTRING "3dPlot"
#define OPTMASK (OPT_ZLimit|OPT_Quad|OPT_DitherCol)

struct old_FnctStruct	/* OBSOLETE !!!!!!!! Nicht benutzen ------------- */
{
	char IDString[8];			/* Erkennung: "3dPlot" */
	short Version;				/* PLOTVERSION */
	short Subversion;			/* PLOTSUBVERSION */
	ULONG StructLen;			/* Gesamtlänge der FnctStruct ohne den */
								/* Funktionsstring */
	short FnctStrLen;			/* Länge des Funktionsstring */
	short FloatLen;				/* Länge eines floats in Byte (derz. 4) */
	short FloatType;			/* Typ der Floatwerte (FLOATTYPE) */
	/* - - - - - - - */
	char DataBegin[0];			/* nur zur Längenbestimmung */
	short Resx;					/* x/y Auflösung */		/* ***** V2 ***** */
	short Resy;
	short DrawMode;				/* gewählter Zeichenmodus */
	short Options;				/* gewählte Optionen (durch die OPTMASK) */

	short PttnSpacingx;			/* x/y-Musterabstand */
	short PttnSpacingy;
	
	ULONG/*~float*/ Minx;
	ULONG/*~float*/ Maxx;
	ULONG/*~float*/ Miny;					/* Min/Max Werte des Fnkt. Bereichs */
	ULONG/*~float*/ Maxy;
	ULONG/*~float*/ Minz;
	ULONG/*~float*/ Maxz;
	
	ULONG/*~float*/ Lightx;
	ULONG/*~float*/ Lighty;				/* Koordinaten der Lichtquelle(0) */
	ULONG/*~float*/ Lightz;	

	ULONG/*~float*/ Angley;				/* Drehwinkel um die y-Achse */
	ULONG/*~float*/ Anglez;				/*         -"-       z-Achse */

														/* ***** V3 ***** */
	ULONG/*~float*/ SizeQuot;				/* Quotienten zur Größe der Funktion */
								/* = SizeFact/((DrawX-Drawx)/BX_FACT+(DrawY-Drawy)/BY_FACT) */
	ULONG/*~float*/ MidxQuot;				/* = Midx/(DrawX-Drawx+1) */
	ULONG/*~float*/ MidyQuot;				/* = Midy/(DrawY-Drawy+1) */
	
	ULONG/*~float*/ Diffuse_Rfx;			/* Index für diffuse Lichtreflexion */
	ULONG/*~float*/ Direct_Rfx;			/* Index für direkte      -"-       */
	
	ULONG/*~float*/ BodyFillColor;		/* Farbe (0-1) zum Füllen der Funktion */
	ULONG/*~float*/ PatternColor;			/* Farbe (0-1) zum Zeichnen des Musters */
	
	short ExtraLightCnt;		/* Anzahl folgender zusätzlicher Lichtqu. */
	short LightsOn;				/* eingeschaltete Lichtquellen */

	ULONG/*~float*/ Light0Intens;			/* Intens. der Lichtquelle #0 */
		
	struct						/* zusätzliche Lichtquellen (norm. 3) */
	{
		ULONG/*~float*/ x,y,z;
		ULONG/*~float*/ Intens;
	} ExLight[MAXLIGHTS-1];
	
	char FnctString[0];			/* nur für die Adresse */
};
/* !!!!! es schließt sich direkt der Funktionsstring(nullterm.) an !!!!! */

/* ------------------------------------------------------------------------- */

struct FnctStruct
{
	char IDString[8];			/* Erkennung: "3dPlot" */
	short Version;				/* PLOTVERSION */
	short Subversion;			/* PLOTSUBVERSION */
	ULONG StructLen;			/* Gesamtlänge der FnctStruct ohne den */
								/* Funktionsstring */
	short FnctStrLen;			/* Länge des Funktionsstring */
	short FloatLen;				/* Länge eines floats in Byte (derz. 12) */
	short FloatType;			/* Typ der Floatwerte (FLOATTYPE) */
	/* - - - - - - - */
	char DataBegin[0];			/* nur zur Längenbestimmung */
	short Resx;					/* x/y Auflösung */		/* ***** V2 ***** */
	short Resy;
	short DrawMode;				/* gewählter Zeichenmodus */
	short Options;				/* gewählte Optionen (durch die OPTMASK) */

	short PttnSpacingx;			/* x/y-Musterabstand */
	short PttnSpacingy;
	
	real Minx;
	real Maxx;
	real Miny;					/* Min/Max Werte des Fnkt. Bereichs */
	real Maxy;
	real Minz;
	real Maxz;
	
	real Lightx;
	real Lighty;				/* Koordinaten der Lichtquelle(0) */
	real Lightz;	

	real Angley;				/* Drehwinkel um die y-Achse */
	real Anglez;				/*         -"-       z-Achse */

														/* ***** V3 ***** */
	real SizeQuot;				/* Quotienten zur Größe der Funktion */
								/* = SizeFact/((DrawX-Drawx)/BX_FACT+(DrawY-Drawy)/BY_FACT) */
	real MidxQuot;				/* = Midx/(DrawX-Drawx+1) */
	real MidyQuot;				/* = Midy/(DrawY-Drawy+1) */
	
	real Diffuse_Rfx;			/* Index für diffuse Lichtreflexion */
	real Direct_Rfx;			/* Index für direkte      -"-       */
	
	short LightCnt;				/* Anzahl der Lichtqu. */
	short LightsOn;				/* eingeschaltete Lichtquellen */

	struct						/* Lichtquellen (norm. 4) */
	{
		real x,y,z;
		real Intens;
	} Light[MAXLIGHTS];
															/******* V5 *******/
	real	BodyColor;					/* Farben von Funktionskörper */
	real	PatternColor;				/* und Muster */
	real	PatternDeltaColor;

	char FnctString[0];			/* nur für die Adresse */
};
/* !!!!! es schließt sich direkt der Funktionsstring(nullterm.) an !!!!! */

/* ------------------------------------------------------------------------- */

/* DrawingInfo-Strukt. für DrawFuntion ab V3.0 ----------------------------- */

struct DrawingInfo		/* !! '_'-Einträge sind für RG-3d nötig */
{
	short BeginX1,BeginX2;	/* Anfangs- u. */
	short EndX1,EndX2;		/* Endwerte der Zeichenschleife */

	point *P,*_P;			/* Zeiger auf Punkt-Zwischenspeicher */
	vector *V;				/* Zeiger auf xyz-Speicher */
	short PSize;			/* max. Anzahl der Punkte im Z-Zws. */

	short DirX1,DirX2;		/* Zählrichtung in X/Y */
	short x1Res,x2Res;		/* x1 äußere / x2 innere Schleife */
	double x1Min,x2Min;

	short SX1,SX2;			/* Schleifenvariablen */
	double dx1,dx2;			/* Schrittweite */
	
	double *X1,*X2;		/* Zeiger auf die zwei Schleifenvar. */
	short dmf_LX1;		/* Flag, das DrawMode->DMF_LX entspricht */
	short dmf_LX2;		/* Flag, das DrawMode->DMF_LY entspricht */

	point p2,_p2;			/* Punkt-Zws. */
	vector v2;				/* xyz-Zws. */
	
	short PatX1Cnt,PatX2Cnt;		/* x/y - Musterzähler */
	short SpacingX1,SpacingX2; 

	long PointsToDo;		/* Anzahl der Punkte, die noch zu zeichnen */
							/* sind. (für OPT_Time) */
	long PointCnt;		/* Anzahl der gez. Punkte (für OPT_Time) */
	long PointsMeasured;	/* Anzahl der P. für die die Zeitmess. gilt */
	long MaxMeasure;		/* Max.anzahl der P., über die gemessen */
							/* werden darf */
	ULONG LastSec,LastMic;/* letzte Zeitmessung (für OPT_Time) */

	vector RetracedEyeVect;	/* Koordinate des Auges (in Fnkt.-Koors.) */
	vector _RetracedEyeVect;

	double EyeYOffset;	/* Y-Offset (virtual) bei RG-3d-Modus */ 
};

/* ------------------------------------------------------------------------- */
 
/* BreakFile - Struktur (ab V3.0) ------------------------------------------ */

struct Break			/* Beschreibungen in der DrawingInfo Struktur */
{
	short	FloatLen;
	short FloatType;
	short SX1,SX2;
	point p2,_p2;
	vector v2;
	short PatX1Cnt,PatX2Cnt;
	long PointsToDo;
	long PointCnt;
	short PSize;
	short Flags;	/* BRK_... */
	short Options;  /* Options, unter denen die Fnkt. gezeichnet wird, */
	                /* bisher nur OPT_RG3d von Belang */
	/* - - */		/* es schließen sich die P und V Felder an */
};

#define BRK_PFIELD (1<<0) // P-Feld schließt sich an
#define BRK__PFIELD (1<<2)// _P-Feld schließt sich an
#define BRK_VFIELD (1<<1) // V-Feld schließt sich an

/* PlotPrefs - Struktur (ab V4.0) ------------------------------------------ */

#define PP_IDSTRING "3dPPref"

struct PlotPrefs
{
	char  IDString[8];			/* Erkennung: "3dPPref" */
	short Version;				/* PLOTVERSION */
	short Subversion;			/* PLOTSUBVERSION */
	ULONG StructLen;			/* Gesamtlänge der gespeicherten Struct */
	short FloatLen;				/* Länge eines floats in Byte (derz. 4/8) */
	short FloatType;			/* Typ der Floatwerte (FLOATTYPE) */
	/* - - - - - - - */
	short Flags;				/* PPF_... (s.u.) */
	
	double EyeScreenDist;			/* Augenentfernung zum Bildschirm [cm] */
	double EyeEyeDist;					/* Augenabstand [cm] */
	double ScreenXMeasure;			/* Breite und Höhe des Displays [cm] */
	double ScreenYMeasure;

	double XAspect,YAspect;		/* Verh. Pixels / Maß[cm] auf dem Bild */
	
	short DrawAreaWidth;		/* Breite des Zeichenbereichs */
	short DrawAreaHeight;		/* Höhe        -"-	*/
	
	ULONG DisplayID;				/* verwendete DisplayID */
	UWORD DisplayDepth;			/* Anzahl der Bitplanes */

	short DisplayWidth;			/* Breite und Höhe des Displays (in Pixel)  */
	short DisplayHeight;		/* (vom Computer gesetzt) */
	
	char  FontName[40];			/* Bildschirmfont */
	UWORD FontYSize;			/* Höhe des Fonts */
	UWORD FontStyle;			/* Stilflags des Fonts */
	
	short Options;				/* NUR im Zsmhng. mit brk-Files gebraucht */

	struct PP_ColorFile StdCols;	/* Farbtabellen */
	struct PP_ColorFile RG3dCols;
};

/* ---------------------------------------------------------------------- */

struct RGColSupp	/* Hilfs-Struktur zum Zeichen im Rot-Grün-3d Modus */
{
	UBYTE	RedMask;		/* Bitplane-Write-Masken */
	UBYTE GreenMask;
	
	UBYTE BGCol;			/* Hintergrundfarbe */
	UBYTE ColNum;			/* verwendbare Farbzahl (=Anzahl Einträge in Colors[]) */
	
	UBYTE *Colors;		/* ^Zeiger auf eine Color-Liste */
};

/* ---------------------------------------------------------------------- */

struct DINode				/* Eintrag in der DI-Liste (s. AGASupp.c) */
{
	struct Node node;
	ULONG DisplayID;
};

/* ---------------------------------------------------------------------- */

#endif
