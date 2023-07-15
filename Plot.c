/* ---------------------------------------------------------------------- *
 * Plot - 25-12-93 SSi    -    Zeichenroutinen                            *
 * ---------------------------------------------------------------------- */

#include "Plot.h"

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <user/functions.h>
#include <graphics/gfxmacros.h>

/* globale Zeiger etc. ------------------------------------------------- */

extern struct Screen *OurS;
extern struct Window *OurW;
extern struct Window *ApsW;

extern short Drawx,DrawX,Drawy,DrawY;	/* min. und max. Koor. für die Funktion */
								/* auf dem Bildschirm */
extern short BorderDrawx,BorderDrawX,BorderDrawy,BorderDrawY;	/* min. und max. Koor. */

extern double Midx,Midy;					/* Koordinatenmitte (veränderlich) */
extern double _Midx,_Midy;				/*      -"-         (fest) */

extern short SpacingX,SpacingY; /* x/y - Musterabstand */

TIME *TimeEstTH;		/* für OPT_Time */

extern struct TextFont *textfont;

extern struct PlotPrefs *editPrefs;

extern struct RGColSupp *RGColSupp;	/* RG-3d ColorSupp - Struktur */

/* APS-System ------------------------------------------------------------- */

extern TIME *APS_TH;			/* für OPT_APS */
extern LONG APS_MinIntervall;
extern char APS_Filename[];

/* Daten der Funktion *****************************************************/

extern fnctcode *FunctionCode;		/* RAW-Code der Funktion */

/* extern int _FPERR;	*/	/* Errorstatus von EvalRaw() */

extern short Status;	/* Funktion zu berechnen 0 - ja / !=0 - nein */

extern double *X;					/* Zeiger auf x,y Variablen */
extern double *Y;

extern double xMin;
extern double xMax;
extern double yMin;
extern double yMax;			/* Bereich der Funktion */
extern double zMin;
extern double zMax;

extern vector FuncMid;	/* Funktionsmitte als Vector in Funktionskoordinaten */
vector xBase,yBase,zBase;	/* Basisvektoren der transformierten Funktion */

extern short xRes;			/* Berechnungsschritte */
extern short yRes;

extern double yAngle;			/* Kippwinkel der Funktion */
extern double zAngle;

extern double SizeFact;		/* Vergrößerungsfaktor der Funktion */

extern double Diffuse_Rfx;		/* Faktoren (0-2) für diffuse und */
extern double Direct_Rfx;		/* direkte Lichtreflexion */

extern double PatternCol,PatternDeltaCol;	/* Funktionsfarben */
extern double BodyCol;

/*************************************************************************/

/* Globale Daten ------------------------------------------------------- */

extern short mode;		/* derz. Arbeitsmodus des Hauptprogramms */

extern short submode;		/* Untermodus (interessant in Editmode */
                        /* und Drawmode) */

extern short ConsoleState;

extern short DrawMode;	/* Zeichenmodus */

extern short Options;		/* gewählte Optionen */

short COLm,COLcnt;			/* min. Farbe und Farbzahl für die Funktion */
double COL_CC1,COL_CC2,COL_CC3;	/* Farben für DrawCCross() (0-1) */

short ModeDisplay_x;		/* x/y-Koordinaten der Modus-Anzeige */
short ModeDisplay_y;

vector LightVect[MAXLIGHTS] =	/* Standard-Koordinaten der Lichtquellen */
{
	2.0,-5.0,5.0,
	0.0,0.0,10.0,
	-2.0,5.0,-5.0,
	0.0,0.0,-10.0
};
double LightIntens[MAXLIGHTS] = /* Standard-Intensitäten der Lichtquellen */
{ 1.0,0.6,1.0,0.6 };
	
short LightsOn=1;			/* welche Lichtquellen sind an ? (1 Bit / Lichtqu.) */

#define TIMEEST_SECONDS 2	/* Anzahl der Sekunden, nach denen eine neue Zeit- */
						/* schätzung für das Zeichnen gemacht wird */

struct DateStamp PicLastSave;	/* Datum+Zeit der letzten Bildspeicherung */
LONG PicSecWorth=-1;		/* Anzahl Sekunden, die das Bild "wert" ist. */
						/* (wird erst bei Beendigung des Zeichenvor- */
						/* gangs gesetzt */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Die von DrawFunction verwendete Datenstruktur DrawingInfo - - - - - - - - */

struct DrawingInfo DI;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Koordinatentransformation ----------------------------------------------- */

void ComputeBaseVects(void) // errechnet die FnctMid, xBase, yBase zBase
{                         	// Vektoren nach x/y/zDist, y/zAngle, SizeFact
	static vector xa = { 1,0,0 }, ya = { 0,1,0 }, za = { 0,0,1 };
	
	MakeBaseVect(&xBase,xa);
	MakeBaseVect(&yBase,ya);
	MakeBaseVect(&zBase,za);
}

void MakeBaseVect(vector *base, vector preset) // transformiert preset
{                                              // und legt ihn in base ab
	double dist;			/* Abstand des Punkt von der Drehachse */
	double w;

	if (dist=sqrt(preset.x*preset.x+preset.y*preset.y))	/* Drehung in xy-Ebene */
	{
		if (preset.x<0.0) w=pi-asin(preset.y/dist)+zAngle;
		else w=asin(preset.y/dist)+zAngle;
		preset.x=cos(w)*dist;
		preset.y=sin(w)*dist;
	} else preset.x=preset.y=0.0;

	if (dist=sqrt(preset.x*preset.x+preset.z*preset.z))	/* Drehung in xz-Ebene */
	{
		if (preset.x<0.0) w=pi-asin(preset.z/dist)+yAngle;
		else w=asin(preset.z/dist)+yAngle;
		preset.x=cos(w)*dist;
		preset.z=sin(w)*dist;
	} else preset.x=preset.z=0.0;

	base->x=preset.x*SizeFact;		/* auf Bildschirmgröße bringen */
	base->y=preset.y*SizeFact;
	base->z=preset.z*SizeFact;		/* -> Visualkoordinaten */
}

void PreTransform(v)	/* transformiert den Vector (dreht ihn und rechnet */
vector *v;						/* ihn in Bildschirmkoor. (ohne Fluchtpunktpers.) um) */
{
	vector buf=*v;
	
	VSub(&buf,&buf,&FuncMid);	// Funktion mitten
	v->x=buf.x*xBase.x+buf.y*yBase.x+buf.z*zBase.x;	// transformieren
	v->y=buf.x*xBase.y+buf.y*yBase.y+buf.z*zBase.y;
	v->z=buf.x*xBase.z+buf.y*yBase.z+buf.z*zBase.z;
}

point Transform(v,EyeY)	/* transformiert den Vector zu bx,by */
vector *v;							/* bei bx=by=ILL Punkt nicht zu berechnen */
double EyeY;							/* vom angeg. Augenpunkt aus */
{
	point P;
	
	if (activePrefs->EyeScreenDist-v->x<=0.0) P.bx=P.by=ILL;
	else
	{							/* und Fluchtpunktperspektive */
		int valx,valy;
		
		valx=Midx+activePrefs->XAspect*(EyeY+activePrefs->EyeScreenDist*(v->y-EyeY)/(activePrefs->EyeScreenDist-v->x));
		valy=Midy-activePrefs->EyeScreenDist*v->z*activePrefs->YAspect/(activePrefs->EyeScreenDist-v->x);

		if (abs(valx)>20000 || abs(valy)>20000) P.bx=P.by=ILL;
		else
		{
			P.bx=valx;
			P.by=valy;
		}
	}
	return(P);
}

/* ----------------------------------------------------------------------- */

void RetraceVect(Vect)		/* transformiert einen Vektor in Visualkoord. */
vector *Vect;					/* zurück in Funktions-Koordinaten */
{
	double w,dist;
	 
	VSDiv(Vect,SizeFact);		/* auf Funkt.Koord. Größe bringen */

	if (dist=sqrt(Vect->x*Vect->x+Vect->z*Vect->z))	/* Drehung in xz-Ebene */
	{
		if (Vect->x<0.0) w=pi-asin(Vect->z/dist)-yAngle;
		else w=asin(Vect->z/dist)-yAngle;
		Vect->x=cos(w)*dist;
		Vect->z=sin(w)*dist;
	} else Vect->x=Vect->z=0.0;

	if (dist=sqrt(Vect->x*Vect->x+Vect->y*Vect->y))	/* Drehung in xy-Ebene */
	{
		if (Vect->x<0.0) w=pi-asin(Vect->y/dist)-zAngle;
		else w=asin(Vect->y/dist)-zAngle;
		Vect->x=cos(w)*dist;
		Vect->y=sin(w)*dist;
	} else Vect->x=Vect->y=0.0;

	VAdd(Vect,Vect,&FuncMid);			/* und aus der Mitte holen */
}

/* ----------------------------------------------------------------------- */

void CompuSizeFact(void)		/* gleicht den Größenfaktor an die Range- */
{							/* definitionen an. */
	
	double Diagonal;			/* Länge der Diagonale des Koordinaten- */
							/* Würfels */
	
	Diagonal=sqrt((xMax-xMin)*(xMax-xMin)+
				  (yMax-yMin)*(yMax-yMin)+
				  (zMax-zMin)*(zMax-zMin));
	
	if (Diagonal==0.0) { Error(l[82]); }
	else
	{
		double maxxy;	/* durchschnittliche Maximalausbreitung in bx/by */
		
		maxxy=((double)(activePrefs->DrawAreaWidth)/activePrefs->XAspect+(double)(activePrefs->DrawAreaHeight)/activePrefs->YAspect)/2.0;
		
		SizeFact=maxxy/Diagonal;
	}
	ComputeBaseVects();
}

/* ----------------------------------------------------------------------- */

void DisplayMode(void)			/* druckt den Modus auf das Bild */
{
	char *str;

	if (ConsoleState==CS_PREFS) return;
	
	switch(mode)				/* Handlung entscheiden */
	{
		case MODE_START:
		case MODE_ENDREQ:
		case MODE_TEST:
			return;
		case MODE_CLEAR:
		case MODE_WAIT:
		case MODE_DISPL:
		case MODE_STARTEDIT:
			str=l[48];
			break;
		case MODE_DRAW:
			str=l[49];
			break;
		case MODE_EDIT:
			str=l[50];
			break;
		case MODE_LOAD:
			str=l[51];
			break;
		case MODE_SAVE:
			str=l[52];
			break;
		case MODE_HELP:
			str=l[55];
			break;
		default:
			str="  ???   ...";
	}
	SetDrMd(OurW->RPort,JAM2);
	SetAPen(OurW->RPort,(mode==MODE_SAVE || mode==MODE_LOAD)?3:1);
	SetBPen(OurW->RPort,0);
	
	Move(OurW->RPort,ModeDisplay_x,ModeDisplay_y);
	Text(OurW->RPort,str,11);
}

/* ----------------------------------------------------------------------- */

void PrintTime(Time)	/* druckt die in Minuten(gebrochen) angegeben Zeit */
double Time;			/* ins Modusdisplay */
{
	static char Buf[20];
	short Hs,Mins,Secs;

	if (Time>=0.0)
	{
		Hs=(short)(Time/3600.0); Time-=(double)Hs*3600.0;
		Mins=(short)(Time/60.0); Time-=(double)Mins*60.0;
		Secs=(short)(Time);
		
		if (Hs) sprintf(Buf,"  %02hdh %02hdm  ",Hs,Mins);
		else if (Mins) sprintf(Buf,"  %02hdm %02hds  ",Mins,Secs);
		else sprintf(Buf,"  %02hd secs  ",Secs);
		Buf[11]=0;
	}
	else strcpy(Buf,l[56]);

	SetDrMd(OurW->RPort,JAM2);
	SetAPen(OurW->RPort,1);
	SetBPen(OurW->RPort,0);
	
	Move(OurW->RPort,ModeDisplay_x,ModeDisplay_y);
	Text(OurW->RPort,Buf,strlen(Buf));
}

/* ----------------------------------------------------------------------- */

char CCSource[20*3+1] =		/* x/y/z - Koordinaten von 20 Punkten */
{
	-1,-1,-1, -1,-1,0, -1,-1,1, -1,0,-1, -1,0,1, -1,1,-1, -1,1,0, -1,1,1,
	0,-1,-1, 0,-1,1, 0,1,-1, 0,1,1,
	1,-1,-1, 1,-1,0, 1,-1,1, 1,0,-1, 1,0,1, 1,1,-1, 1,1,0, 1,1,1,
	 -2
};

point CCPoints[20];			/* bx/by - Koordinaten von 20 Punkten */
point _CCPoints[20];			/* -"- für OPT_RG3d */

char CCLines[24*2+1] =				/* 24 Linienverbindungen */
{
	 0,2, 3,4, 5,7, 8,9, 10,11, 12,14, 15,16, 17,19,
	 0,5, 1,6, 2,7, 8,10, 9,11, 12,17, 13,18, 14,19,
	 0,12, 1,13, 2,14, 3,15, 4,16, 5,17, 6,18, 7,19,
	-1
};

void DrawCCross(int m)			/* Koordinatenkreuz zeichnen */
{													/* bei m=0 normale Funktion */ 
	point p1,p2;						/*      =1 Zeichnung nach editPrefs (Testbild) */
	short a,b,dd;
	static double oxMin,oyMin,ozMin,oxMax,oyMax,ozMax,oSizeFact,
	             oMidx,oMidy,oXAspect,oYAspect;
	static short oDrawx,oDrawy,oDrawX,oDrawY;
	static vector oFuncMid;
	vector tv;
	double eo;
	
	struct PlotPrefs *oactivePrefs;
	
	if (m==1)
	{
		if (!editPrefs) return;
		
		oactivePrefs=activePrefs; activePrefs=editPrefs;		

		oXAspect=editPrefs->XAspect;
		oYAspect=editPrefs->YAspect;
	
		editPrefs->XAspect=oactivePrefs->DisplayWidth/editPrefs->ScreenXMeasure;
		editPrefs->YAspect=oactivePrefs->DisplayHeight/editPrefs->ScreenYMeasure;
		
		oxMin=xMin; xMin=-1.0;
		oyMin=yMin; yMin=-1.0;
		ozMin=zMin; zMin=-1.0;

		oxMax=xMax; xMax=1.0;
		oyMax=yMax; yMax=1.0;
		ozMax=zMax; zMax=1.0;
		
		oFuncMid=FuncMid; FuncMid.x=FuncMid.y=FuncMid.z=0.0;
		
		oMidx=Midx; Midx=_Midx;
		oMidy=Midy; Midy=_Midy;
		
		oSizeFact=SizeFact;
		CompuSizeFact();					

		oDrawx=Drawx; Drawx=BorderDrawx;
		oDrawy=Drawy; Drawy=BorderDrawy;
		oDrawX=DrawX; DrawX=BorderDrawX;
		oDrawY=DrawY; DrawY=BorderDrawY;
	}
	else if (Status&ST_RANGE) { Error(l[62]); return; }	/* bei fehlerhafter Range -> */
	
	for (a=b=0;CCSource[a]!=-2;b++)
	{
		switch (CCSource[a++])
		{ case -1: tv.x=xMin; break; case 0: tv.x=0.0; break; case 1: tv.x=xMax; break; }
		switch (CCSource[a++])
		{ case -1: tv.y=yMin; break; case 0: tv.y=0.0; break; case 1: tv.y=yMax; break; }
		switch (CCSource[a++])
		{ case -1: tv.z=zMin; break; case 0: tv.z=0.0; break; case 1: tv.z=zMax; break; }
		
		PreTransform(&tv);
		CCPoints[b]=Transform(&tv,(Options&OPT_RG3d)?-(activePrefs->EyeEyeDist/2.0):0.0);
		if (Options&OPT_RG3d) _CCPoints[b]=Transform(&tv,activePrefs->EyeEyeDist/2.0);
	}

	if (mode!=MODE_CLEAR && m==0) { short old; old=mode; Mode(MODE_DOCLEAR); mode=old; }

	SetColor(COL_CC1,(Options&OPT_RG3d)?1:0);
	
	for (a=0;CCLines[a]!=-1;)
	{
		DrawRawLine(CCPoints[CCLines[a++]],CCPoints[CCLines[a++]]);
	}
	
	if (Options&OPT_RG3d)
	{
		SetColor(COL_CC1,2);
		for (a=0;CCLines[a]!=-1;)
		{
			DrawRawLine(_CCPoints[CCLines[a++]],_CCPoints[CCLines[a++]]);
		}
	}

	for (dd=(Options&OPT_RG3d)?1:0;(Options&OPT_RG3d)?(dd<3):(dd<1);dd++)
	{
		switch (dd)
		{
			case 0: eo=0.0; break;
			case 1: eo=activePrefs->EyeEyeDist/-2.0; break;
			case 2: eo=activePrefs->EyeEyeDist/2.0; break;
		}
	
		SetColor(COL_CC2,dd);
	
		tv.x=xMin; tv.y=0.0; tv.z=0.0; PreTransform(&tv);
		p1=Transform(&tv,eo);
		tv.x=xMax; tv.y=0.0; tv.z=0.0; PreTransform(&tv);
		p2=Transform(&tv,eo);
		if (m==0 && p2.bx>Drawx && p2.bx<DrawX-textfont->tf_XSize && p2.by>Drawy+textfont->tf_Baseline && p2.by<DrawY-textfont->tf_YSize+textfont->tf_Baseline)
		{
			Move(OurW->RPort,p2.bx,p2.by);
			SetDrMd(OurW->RPort,JAM1);
			Text(OurW->RPort,"x",1);
		}
		SetColor(COL_CC3,dd);
		DrawRawLine(p1,p2);		/* x-Achse */
		
		SetColor(COL_CC2,dd);
		tv.x=0.0; tv.y=yMin; tv.z=0.0; PreTransform(&tv);
		p1=Transform(&tv,eo);
		tv.x=0.0; tv.y=yMax; tv.z=0.0; PreTransform(&tv);
		p2=Transform(&tv,eo);
		if (m==0 && p2.bx>Drawx && p2.bx<DrawX-textfont->tf_XSize && p2.by>Drawy+textfont->tf_Baseline && p2.by<DrawY-textfont->tf_YSize+textfont->tf_Baseline)
		{
			Move(OurW->RPort,p2.bx,p2.by);
			SetDrMd(OurW->RPort,JAM1);
			Text(OurW->RPort,"y",1);
		}
		SetColor(COL_CC3,dd);
		DrawRawLine(p1,p2);		/* y-Achse */

		SetColor(COL_CC2,dd);
		tv.x=0.0; tv.y=0.0; tv.z=zMin; PreTransform(&tv);
		p1=Transform(&tv,eo);
		tv.x=0.0; tv.y=0.0; tv.z=zMax; PreTransform(&tv);
		p2=Transform(&tv,eo);
		if (m==0 && p2.bx>Drawx && p2.bx<DrawX-textfont->tf_XSize && p2.by>Drawy+textfont->tf_Baseline && p2.by<DrawY-textfont->tf_YSize+textfont->tf_Baseline)
		{
			Move(OurW->RPort,p2.bx,p2.by);
			SetDrMd(OurW->RPort,JAM1);
			Text(OurW->RPort,"z",1);
		}
		SetColor(COL_CC3,dd);
		DrawRawLine(p1,p2);	/* z-Achse */
		
		/* Lichtquellen */
		if (m==0 && (DrawMode&(DMF_COL|DMF_LCOL)))
		{
			point lp;
			short a;
			
			for (a=0;a<MAXLIGHTS;a++)
			{
				if (LightsOn&(1<<a))
				{
					memcpy(&tv,&LightVect[a],sizeof(vector));
					PreTransform(&tv);
					lp=Transform(&tv,eo);
					if (lp.bx!=ILL || lp.by!=ILL)
					{
						p1.bx=lp.bx-(short)(activePrefs->ScreenXMeasure/100.0*activePrefs->XAspect);
						p2.bx=lp.bx+(short)(activePrefs->ScreenXMeasure/100.0*activePrefs->XAspect);
						p1.by=p2.by=lp.by;
						SetColor(COL_CC3,dd); DrawRawLine(p1,p2);
						p1.by=lp.by-(short)(activePrefs->ScreenXMeasure/100.0*activePrefs->YAspect);
						p2.by=lp.by+(short)(activePrefs->ScreenXMeasure/100.0*activePrefs->YAspect);
						p1.bx=p2.bx=lp.bx;
						SetColor(COL_CC3,dd); DrawRawLine(p1,p2);
	
						lp.bx+=4;
						lp.by-=2;
						if (lp.bx>Drawx && lp.bx<DrawX-textfont->tf_XSize && lp.by>Drawy+textfont->tf_Baseline && lp.by<DrawY-textfont->tf_YSize+textfont->tf_Baseline)
						{
							static char Buf[2]=" ";
							SetColor(COL_CC2,dd);
							Move(OurW->RPort,lp.bx,lp.by);
							Buf[0]=a+'1';
							SetDrMd(OurW->RPort,JAM1);
							Text(OurW->RPort,Buf,1);
						}
					}
				}
			}
		}	
	}
	NormalColor();
	/* Winkel aufs Bild drucken */
	
	if (m==0 && DrawX-Drawx>=W(20*8) && DrawY-Drawy>=H(5*8))
	{
		static char yAngBuf[10], zAngBuf[10];
	
		sprintf(yAngBuf,"y %#5.1f°",360.0-(yAngle?yAngle:2.0*pi)*(180.0/pi));
		sprintf(zAngBuf,"z %#5.1f°",zAngle*(180.0/pi));
		
		if (Options&OPT_RG3d) SetColor((double)RGColSupp->Colors[RGColSupp->ColNum/2],0);
		else SetAPen(OurW->RPort,1);
		SetBPen(OurW->RPort,(Options&OPT_RG3d)?RGColSupp->BGCol:3);
		SetDrMd(OurW->RPort,JAM2);
		Move(OurW->RPort,DrawX-(W(10*8)),DrawY-(H(2*8))); Text(OurW->RPort,yAngBuf,8);
		Move(OurW->RPort,DrawX-(W(10*8)),DrawY-(H(1*8))); Text(OurW->RPort,zAngBuf,8);
	}

	if (m==1)
	{
		
		activePrefs=oactivePrefs;

		xMin=oxMin;
		yMin=oyMin;
		zMin=ozMin;

		xMax=oxMax;
		yMax=oyMax;
		zMax=ozMax;
		
		FuncMid=oFuncMid;

		Midx=oMidx;
		Midy=oMidy;
		
		Drawx=oDrawx;
		Drawy=oDrawy;
		DrawX=oDrawX;
		DrawY=oDrawY;

		editPrefs->XAspect=oXAspect;
		editPrefs->YAspect=oYAspect;
		
		SizeFact=oSizeFact;
		ComputeBaseVects();
	}
	NormalColor();
}

/* --------------------------------------------------------------------- */

int CorrectLine(Line)			/* korrigiert die Linie so, daß sie auf */
line *Line;					/* dem Bildschirm liegt */
{							/* RETURN: 0 - ok / 1 - Linie nur teilw. auf */
							/* dem Schrim / -1 - Linie nicht auf dem Schirm */
	int state=0;	
	if (!Line) return(-1);
	if (Line->p1.bx==ILL && Line->p1.by==ILL || Line->p2.bx==ILL && Line->p2.by==ILL) goto __NotToDraw;
	
	if (Line->p1.bx<Drawx)
	{
		if (Line->p2.bx<Drawx) goto __NotToDraw;
		Line->p1.by=Line->p1.by+(Drawx-Line->p1.bx)*(Line->p2.by-Line->p1.by)/(Line->p2.bx-Line->p1.bx);
		Line->p1.bx=Drawx;
		state=1;
	}
	if (Line->p1.by<Drawy)
	{
		if (Line->p2.by<Drawy) goto __NotToDraw;
		Line->p1.bx=Line->p1.bx+(Drawy-Line->p1.by)*(Line->p2.bx-Line->p1.bx)/(Line->p2.by-Line->p1.by);
		Line->p1.by=Drawy;
		state=1;
	}
	if (Line->p1.bx>DrawX)
	{
		if (Line->p2.bx>DrawX) goto __NotToDraw;
		Line->p1.by=Line->p1.by+(DrawX-Line->p1.bx)*(Line->p2.by-Line->p1.by)/(Line->p2.bx-Line->p1.bx);
		Line->p1.bx=DrawX;
		state=1;
	}
	if (Line->p1.by>DrawY)
	{
		if (Line->p2.by>DrawY) goto __NotToDraw;
		Line->p1.bx=Line->p1.bx+(DrawY-Line->p1.by)*(Line->p2.bx-Line->p1.bx)/(Line->p2.by-Line->p1.by);
		Line->p1.by=DrawY;
		state=1;
	}
	if (Line->p2.bx<Drawx)
	{
		if (Line->p1.bx<Drawx) goto __NotToDraw;
		Line->p2.by=Line->p2.by+(Drawx-Line->p2.bx)*(Line->p1.by-Line->p2.by)/(Line->p1.bx-Line->p2.bx);
		Line->p2.bx=Drawx;
		state=1;
	}
	if (Line->p2.by<Drawy)
	{
		if (Line->p1.by<Drawy) goto __NotToDraw;
		Line->p2.bx=Line->p2.bx+(Drawy-Line->p2.by)*(Line->p1.bx-Line->p2.bx)/(Line->p1.by-Line->p2.by);
		Line->p2.by=Drawy;
		state=1;
	}
	if (Line->p2.bx>DrawX)
	{
		if (Line->p1.bx>DrawX) goto __NotToDraw;
		Line->p2.by=Line->p2.by+(DrawX-Line->p2.bx)*(Line->p1.by-Line->p2.by)/(Line->p1.bx-Line->p2.bx);
		Line->p2.bx=DrawX;
		state=1;
	}
	if (Line->p2.by>DrawY)
	{
		if (Line->p1.by>DrawY) goto __NotToDraw;
		Line->p2.bx=Line->p2.bx+(DrawY-Line->p2.by)*(Line->p1.bx-Line->p2.bx)/(Line->p1.by-Line->p2.by);
		Line->p2.by=DrawY;
		state=1;
	}
	return(state);
__NotToDraw:
	Line->p1.bx=Line->p1.by=Line->p2.bx=Line->p2.by=ILL;
	return(-1);
}

/* ----------------------------------------------------------------------- */

int DrawLine(p1,p2,c)			/* Linie in Farbe c zeichnen (mit Grenztest) */
point p1,p2;					/* RETURN: 0 - ok / 1 - Linie nur teilw. auf */
short c;						/* dem Schrim / -1 - Linie nicht auf dem Schirm */
{
	int state;
	line Line;
	
	Line.p1=p1;
	Line.p2=p2;
	state=CorrectLine(&Line);
	if (state>=0) FastDrawLine(&Line,c);
	return(state);
}	

int DrawRawLine(p1,p2)		/* Linie zeichnen (mit Grenztest) */
point p1,p2;					/* RETURN: 0 - ok / 1 - Linie nur teilw. auf */
							/* dem Schrim / -1 - Linie nicht auf dem Schirm */
{							/* es werden die akt. eingestellten Zeichen- */
	int state;				/* parameter verwender (APEN/BPEN/PATTERN) */
	line Line;
	
	Line.p1=p1;
	Line.p2=p2;
	state=CorrectLine(&Line);
	if (state>=0)
	{
		Move(OurW->RPort,Line.p1.bx,Line.p1.by);
		Draw(OurW->RPort,Line.p2.bx,Line.p2.by);
	}
	return(state);
}	

void FastDrawLine(Line,c)		/* Linie in Farbe c zeichnen (ohne Grenztest) */
line *Line;
short c;
{
	SetAPen(OurW->RPort,c);
	Move(OurW->RPort,Line->p1.bx,Line->p1.by);
	Draw(OurW->RPort,Line->p2.bx,Line->p2.by);
}

/* --------------------------------------------------------------------- */

void DrawTriangle(p1,p2,p3) /* zeichnet ein Dreieck */
point p1,p2,p3;             /* mit der derz. Farbe und Muster */
{
	short lawbreak=0;
	point zp;		/* Punkt-Zwischenspeicher */
	short a,count;
	line l[3];
	point p[3];		/* Punkte des Dreiecks */
	short EdgesInTri=0;/* Eckpunkte, die im Viereck liegen */
						/* Eckp. von l.o. gegen den Uhrzs. 0-2 nummeriert */
						
	l[2].p2=l[0].p1=p[0]=p1;	/* Linien übertragen */
	l[0].p2=l[1].p1=p[1]=p2;	
	l[1].p2=l[2].p1=p[2]=p3;

	if (CorrectLine(&l[0])) lawbreak++;
	if (CorrectLine(&l[1])) lawbreak++;
	if (CorrectLine(&l[2])) lawbreak++;
	
	if (lawbreak)	/* festst., welche Bildschirmeckp. im Dreieck liegen */
	{
		zp.bx=Drawx;
		zp.by=Drawy;
		if (IsPointInTri(zp,p)) EdgesInTri|=0x1;
		zp.by=DrawY;
		if (IsPointInTri(zp,p)) EdgesInTri|=0x2;
		zp.bx=DrawX;
		if (IsPointInTri(zp,p)) EdgesInTri|=0x4;
		zp.by=Drawy;
		if (IsPointInTri(zp,p)) EdgesInTri|=0x8;
	}
	
	if (EdgesInTri==0xf) RectFill(OurW->RPort,Drawx,Drawy,DrawX,DrawY);
	else
	{
		zp.bx=zp.by=ILL;
		
		for(a=0,count=0;a<3;a++)	/* Punkte an Graphics übergeben */
		{
			if ((l[a].p1.bx!=ILL || l[a].p1.by!=ILL) && l[a].p1!=zp)
			{
				zp=l[a].p1;
				if (count++) AreaDraw(OurW->RPort,zp.bx,zp.by);
				else AreaMove(OurW->RPort,zp.bx,zp.by);
			}
			if ((l[a].p2.bx!=ILL || l[a].p2.by!=ILL) && l[a].p2!=zp)
			{
				zp=l[a].p2;
				if (count++) AreaDraw(OurW->RPort,zp.bx,zp.by);
				else AreaMove(OurW->RPort,zp.bx,zp.by);
			}
			if (EdgesInTri)	/* gegf. Bilschirmecke in Zeichnung */
			{					/* einfügen */
				for (;;)
				{
					short is_to_draw;
					
					is_to_draw=0;
					
					if (zp.bx==Drawx)
					{
						if (!is_to_draw && EdgesInTri&0x1) { zp.by=Drawy; is_to_draw=1; EdgesInTri&=~0x1; }
						if (!is_to_draw && EdgesInTri&0x2) { zp.by=DrawY; is_to_draw=1; EdgesInTri&=~0x2; }
					}
					if (zp.bx==DrawX)
					{
						if (!is_to_draw && EdgesInTri&0x4) { zp.by=DrawY; is_to_draw=1; EdgesInTri&=~0x4; }
						if (!is_to_draw && EdgesInTri&0x8) { zp.by=Drawy; is_to_draw=1; EdgesInTri&=~0x8; }
					}
					if (zp.by==Drawy)
					{
						if (!is_to_draw && EdgesInTri&0x1) { zp.bx=Drawx; is_to_draw=1; EdgesInTri&=~0x1; }
						if (!is_to_draw && EdgesInTri&0x8) { zp.bx=DrawX; is_to_draw=1; EdgesInTri&=~0x8; }
					}
					if (zp.by==DrawY)
					{
						if (!is_to_draw && EdgesInTri&0x2) { zp.bx=Drawx; is_to_draw=1; EdgesInTri&=~0x2; }
						if (!is_to_draw && EdgesInTri&0x4) { zp.bx=DrawX; is_to_draw=1; EdgesInTri&=~0x4; }
					}
					if (is_to_draw)
					{
						if (count++) AreaDraw(OurW->RPort,zp.bx,zp.by);
						else AreaMove(OurW->RPort,zp.bx,zp.by);
					}
					else break;
				}
			}
		}
		AreaEnd(OurW->RPort);		/* und zeichnen */
	}
}

/* --------------------------------------------------------------------- */

void SetColor(double col,short RG)	/* setzt Farbe und Muster nach geg. Farbwert(0-1) */
{						/* oder nach direkter Farbnummer (Farbwert>1) */
						/* legt fest, ob normal (RG=0), rot (RG=1) oder grün (RG=2) gez. */
						/* wird */ 
	static USHORT __chip ditherdata[4][2] = { 0x0000, 0x0000,
		                                        0x2222, 0x8888,
																				    0x5555, 0xaaaa,
																				    0x7777, 0xdddd };
	UWORD colornum;
	UWORD maxcol;
	
	if (!RGColSupp) RG=0;
	
	if (RG)
	{
		SetWrMsk(OurW->RPort,(RG==1)?(RGColSupp->RedMask):(RGColSupp->GreenMask));
	}
	else SetWrMsk(OurW->RPort,0xff);
	
	if (col<=1.0)
	{
		maxcol=RG?RGColSupp->ColNum:COLcnt;
		
						/* Zwischenfarben erlaubt ? */
		if ((Options&OPT_DitherCol) && (!RG || RGColSupp->ColNum>1))
		{
			maxcol=maxcol*4-3;
			colornum=(UWORD)(col*(double)maxcol);
			if (colornum>=maxcol) colornum=maxcol-1;
			
			SetDrMd(OurW->RPort,JAM2);
			SetAfPt(OurW->RPort,ditherdata[colornum&3],1);
			SetDrPt(OurW->RPort,ditherdata[colornum&3][0]);
			colornum/=4;
			maxcol/=4;
			if (RG)	/* Rot-Grün-Modus ? */
			{
				SetBPen(OurW->RPort,RGColSupp->Colors[colornum++]);
				SetAPen(OurW->RPort,RGColSupp->Colors[min(maxcol,colornum)]);
			}
			else
			{
				colornum+=COLm;
				maxcol+=COLm;
				SetBPen(OurW->RPort,colornum++);
				SetAPen(OurW->RPort,min(maxcol,colornum));
			}
		}
		else	/* keine Zwischenfarben erlaubt */
		{
			colornum=(UWORD)(col*(double)maxcol);
			if (colornum>=maxcol) colornum=maxcol-1;
			
			SetAfPt(OurW->RPort,0,0);
			SetDrPt(OurW->RPort,0xffff);
			SetDrMd(OurW->RPort,JAM1);
			if (RG)	SetAPen(OurW->RPort,RGColSupp->Colors[colornum]);
			else SetAPen(OurW->RPort,colornum+COLm);
		}
	}
	else /* col Wert >1.0 */
	{
		if (RG && col==3.0)	/* RG Hintergrund */
		{
			SetAfPt(OurW->RPort,0,0);
			SetDrPt(OurW->RPort,0xffff);
			SetAPen(OurW->RPort,RGColSupp->BGCol);
			SetDrMd(OurW->RPort,JAM1);
		}
		else
		{
			NormalColor();
		  SetAPen(OurW->RPort,(UWORD)col);
		}
	}
}

void NormalColor(void)	/* setzt Farb u. Zeichenregister (Muster) zurück */
{
	SetAfPt(OurW->RPort,0,0);
	SetDrPt(OurW->RPort,0xffff);
	SetBPen(OurW->RPort,3);
	SetDrMd(OurW->RPort,JAM1);
	SetWrMsk(OurW->RPort,0xff);
}
	
/****************************************************************************/

double _stdcol;	/* Standardfarbe für DMF_FL ... */

void DrawFunction(void)		/* Zeichnet die Funktion */
{							/* bei submode=0 wird alles zuert initiali- */
							/* siert, dann wird bei jedem weiteren Aufruf */
							/* gezeichnet, bis DrawFunction() dann */
							/* mode=MODE_WAIT setzt, */
							/* submode wird autom. -1 -> Zws. freigeben */

							/* Datenstrukt DI unter starker Verwendung */
														
	complex _Z;			/* errechneter Z-Wert */
	double Z;				/* zu benutzender Z-Wert */
	point p1,_p1;		/* Punkt-Zws. */
	vector v1;			/* xyz-Zws. */
	vector tv;			/* Transform-Vector (für Zwischenergebnisse) */
	
	point *pp1,*pp2;	/* ^p1/p2 */	/* für OPT_RG3d */
	point *pP;				/* ^P[] */
	vector *eye;
	
	short dd;
	
	double c1,c2;		/* Farben für DMF_COL */
	double lc1,lc2;	/* Farben für DMF_LCOL */
		
	if (submode>=0 && (Status || !(DrawMode&(DMF_FL|DMF_PAT)))) /* Etwas nicht ok. ? */
	{
		Error(l[83]);
		Mode(MODE_WAIT);
		return;
	}
	if (!submode)	/* bei erstem Aufruf, testen, ob zeichenbar */
	{
		if (DrawMode&(DMF_LCOL|DMF_COL))
		{
			short light=0;
			short a;
			
			for (a=0;a<MAXLIGHTS;a++)
			{
				if (LightsOn&(1<<a)) if (LightIntens[a]) light++;
			}
			if (!(Diffuse_Rfx+Direct_Rfx)) light=0;

			if (!light)
			{
				Error(l[83]);
				Mode(MODE_WAIT);
				return;
			}
		}
	}
	if (submode<=0)			/* 1. Aufruf von DrawFunction() ? */
	{
		if (DI.P)			/* gegf. P-Zws. freigeben */
		{
			FreeMem(DI.P,DI.PSize*sizeof(point));
			DI.P=0;
		}
		if (DI._P)			/* gegf. P-Zws. freigeben */
		{
			FreeMem(DI._P,DI.PSize*sizeof(point));
			DI._P=0;
		}
		if (DI.V)				/* gegf. V-Zws. freigeben */
		{
			FreeMem(DI.V,DI.PSize*sizeof(vector));
			DI.V=0;
		}

		if (submode==-1)	/* bei Ende Zeichenzeit errechnen */
		{
			struct DateStamp Date;
			
			if (PicSecWorth>0)
			{
				DateStamp(&Date);
				
				PicSecWorth=(Date.ds_Days-PicLastSave.ds_Days)*(24*60*60);
				PicSecWorth+=(Date.ds_Minute-PicLastSave.ds_Minute)*60;
				PicSecWorth+=(Date.ds_Tick-PicLastSave.ds_Tick)/50;
				if (!PicSecWorth) PicSecWorth=1;
			}
							/* und APS schließen */
			WindowToBack(ApsW);
			return;
		}
		
		submode=1;

		MakeDrawingInfo();	/* DI-Struktur initialisieren */

		DateStamp(&PicLastSave);	/* letzte Bildspeicherung festlegen */
		PicSecWorth=1;
		
		GetAPSFilename();			/* APS ggf. einblenden */
		StartTime(APS_TH,APS_MinIntervall*60);
	}

	/* - - - - Schleifensteuerung 1 - - - - */
	
	if (++DI.PatX2Cnt>DI.SpacingX2) DI.PatX2Cnt=0;

	if (DI.SX2==DI.EndX2)
	{
		DI.P[DI.SX2+1]=DI.p2;
		if (DI._P) DI._P[DI.SX2+1]=DI._p2;
		if (DI.V) DI.V[DI.SX2+1]=DI.v2;
	
		if (DI.SX1==DI.EndX1) { submode=-1; Mode(MODE_WAIT); return; }
		DI.SX2=DI.BeginX2;
		DI.SX1+=DI.DirX1; 
		DI.p2.bx=DI.p2.by=ILL;
		DI._p2.bx=DI._p2.by=ILL;
		if (++DI.PatX1Cnt>DI.SpacingX1) DI.PatX1Cnt=0;
		DI.PatX2Cnt=-1;
	}
	DI.SX2+=DI.DirX2;

	if (Options&OPT_Time)
	{
		if (DI.LastSec || DI.LastMic)
		{
			if (CheckTime(TimeEstTH)) /* Zeit ist jetzt zu schätzen*/
			{
				ULONG NewSec,NewMic;
				double TimeNeeded;
				DI.PointsToDo-=DI.PointCnt;
				DI.PointsMeasured+=DI.PointCnt;
				CurrentTime(&NewSec,&NewMic);
				if (NewSec>DI.LastSec || NewMic>DI.LastMic)
				{
					if (DI.PointsToDo>=0 && DI.PointsMeasured>0)
					{
						TimeNeeded=((double)NewMic-(double)DI.LastMic)/1000000.0;
						TimeNeeded+=(double)NewSec-(double)DI.LastSec;
						TimeNeeded*=(double)DI.PointsToDo/(double)DI.PointsMeasured;
						PrintTime(TimeNeeded);
					}
					if (DI.PointsMeasured>=DI.MaxMeasure)
					{
						DI.PointsMeasured=0;
						CurrentTime(&DI.LastSec,&DI.LastMic);
					}
				}
				DI.PointCnt=0;
				StartTime(TimeEstTH,TIMEEST_SECONDS);
			}
		}
		else
		{
			if (DI.SX1!=(DI.BeginX1+DI.DirX1))
			{
				DI.PointsToDo-=DI.PointCnt;
				DI.PointCnt=0;
				CurrentTime(&DI.LastSec,&DI.LastMic);
				StartTime(TimeEstTH,TIMEEST_SECONDS);
			}
		}
	}
	else if (DI.LastSec || DI.LastMic) DI.LastSec=DI.LastMic=DI.PointsMeasured=0;
	
	/* - - - - - - Schleife - - - - - - - */ 
	
	if (!(DrawMode&DMF_LCOL) && (DI.SX1==DI.EndX1 || DI.SX2==DI.EndX2)) p1.bx=p1.by=ILL;
	else
	{
		*DI.X1=DI.x1Min+(double)DI.SX1*DI.dx1;	/* x/y berechnen und */
		*DI.X2=DI.x2Min+(double)DI.SX2*DI.dx2;	/* Variablen setzen */
		
		evalfnctcode(&_Z,FunctionCode);	/* zugehörigen Z-Wert berechnen */
		Z=_Z.re;
		DI.PointCnt++;
		
		v1.x=tv.x=*X; v1.y=tv.y=*Y; v1.z=tv.z=Z;		
		
		if (_FPERR) p1.bx=p1.by=_p1.bx=_p1.by=ILL; /* und verarbeiten */
		else
		{
			if (Options&OPT_ZLimit)
			{
				if (Z>zMax) Z=zMax;
				if (Z<zMin) Z=zMin;
				v1.z=tv.z=Z;
				PreTransform(&tv);
				if (Options&OPT_RG3d)
				{
					p1=Transform(&tv,-DI.EyeYOffset); _p1=Transform(&tv,DI.EyeYOffset);
				}
				else p1=_p1=Transform(&tv,0.0);
			}
			else
			{
				if (Z>=zMin && Z<=zMax)
				{
					PreTransform(&tv);
					if (Options&OPT_RG3d)
					{
						p1=Transform(&tv,-DI.EyeYOffset); _p1=Transform(&tv,DI.EyeYOffset);
					}
					else p1=_p1=Transform(&tv,0.0);
				}
				else p1.bx=p1.by=_p1.bx=_p1.by=ILL;
			}
		}
	}
	
	/* - - - - - - zeichnen mit p1/p2/P[DI.SX2-DI.DirX2+1]/P[DI.SX2+1] - - - - - - - */
	
	for (dd=(Options&OPT_RG3d)?1:0;(Options&OPT_RG3d)?(dd<3):(dd<1);dd++)
	{
		switch (dd)
		{
			case 0:
			case 1: pp1=(void*)&p1;  pp2=(void*)&DI.p2;  pP=DI.P;  eye=&DI.RetracedEyeVect; break;
			case 2: pp1=(void*)&_p1; pp2=(void*)&DI._p2; pP=DI._P; eye=&DI._RetracedEyeVect; break;
		}
		
		if ((DrawMode&DMF_FL)||(DrawMode&DMF_LCOL))
		{
			point p3,p4;    /* Lage: p3 p4           */
			vector v3,v4;   /*       *pp1 p2   <v vorn */
			
			short draw_tri=((1<<4)-1);	/* Bits für Dreieckzeichnung */
			
			p3=pP[DI.SX2+1];
			p4=pP[DI.SX2-DI.DirX2+1];	
	
			v3=DI.V[DI.SX2+1];
			v4=DI.V[DI.SX2-DI.DirX2+1];
				
			/* gezeichnet werden die Dreiecke 0) *pp1 p2 p3  u.  1) p2 p3 p4  *
			 *                          oder  2) *pp1 p2 p4  u.  3) *pp1 p3 p4  */
			
			if (pp1->bx==ILL && pp1->by==ILL) draw_tri&=(1<<1);
			if (pp2->bx==ILL && pp2->by==ILL) draw_tri&=(1<<3);
			if (p3.bx==ILL && p3.by==ILL) draw_tri&=(1<<2);
			if (p4.bx==ILL && p4.by==ILL) draw_tri&=(1<<0);
				
			if (draw_tri==((1<<4)-1)) /* (alle Dreiecke zeichenbar) */
			{
				/* an längerer Diagonalen in Dreiecke schneiden */
				
				if ((pp1->bx-p4.bx)*(pp1->bx-p4.bx)+(pp1->by-p4.by)*(pp1->by-p4.by)
				   <(pp2->bx-p3.bx)*(pp2->bx-p3.bx)+(pp2->by-p3.by)*(pp2->by-p3.by))
					   draw_tri&=(1<<0)|(1<<1);
				else draw_tri&=(1<<2)|(1<<3);
			}
			
			c1=c2=lc1=lc2=-1.0;
			if (DrawMode&(DMF_COL|DMF_LCOL))	/* Farben berechnen */
			{
				if (draw_tri&(1<<0)) c1=lc1=ComputeColor(eye,&v1,&DI.v2,&v3);
				if (draw_tri&(1<<1)) c2=lc2=ComputeColor(eye,&DI.v2,&v3,&v4);
				if (draw_tri&(1<<2)) c1=lc1=ComputeColor(eye,&v1,&DI.v2,&v4);
				if (draw_tri&(1<<3)) c2=lc2=ComputeColor(eye,&v1,&v3,&v4);
			}	
			lc1=(lc1>=0.0)?lc1:lc2;
			
			if (DI.SX1!=DI.EndX1 && DI.SX2!=DI.EndX2)
			{
				if (DrawMode&(DMF_COL))
				{
					if (Options&OPT_Quad)	/* Farben bestimmen */
					{
						if (c1>=0.0 && c2>=0.0) c1=c2=(c1+c2)/2.0;
						else c1=c2=(c1>=0.0)?c1:c2;
					}
				}
				else
				{
					c1=draw_tri&(1<<0|1<<2)?_stdcol:-1.0;
					c2=draw_tri&(1<<1|1<<3)?_stdcol:-1.0;
				}
				if (c1>=0.0) { SetColor(c1,dd); DrawTriangle(*pp1,*pp2,draw_tri&(1<<0)?p3:p4); }
				if (c2>=0.0) { SetColor(c2,dd); DrawTriangle(draw_tri&(1<<1)?*pp2:*pp1,p3,p4); }
			}
		}		
	
		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	
		if (DrawMode&(DMF_PAT))
		{
			if (DrawMode&DMF_LCOL)	/* Musterfarbe setzen */
			{
				if (lc1>=0.0)
				{
					lc1+=PatternDeltaCol;
					if (lc1<0.0) lc1=0.0;
					if (lc1>1.0) lc1=1.0;
					SetColor(lc1,dd);
				}
				else SetColor(PatternCol,dd);
			}
			else
			{
				SetColor(PatternCol,dd);
				lc1=0.0;
			}
			
			if (lc1>=0.0)
			{
				if (!(DrawMode&(DMF_LX|DMF_LY)) && !DI.PatX1Cnt && !DI.PatX2Cnt)
					if (pP[DI.SX2-DI.DirX2+1].bx>=Drawx && pP[DI.SX2-DI.DirX2+1].by>=Drawy && pP[DI.SX2-DI.DirX2+1].bx<=DrawX && pP[DI.SX2-DI.DirX2+1].by<=DrawY)
					{
						WritePixel(OurW->RPort,pP[DI.SX2-DI.DirX2+1].bx,pP[DI.SX2-DI.DirX2+1].by);
					};
				if (DrawMode&DI.dmf_LX1 && !DI.PatX2Cnt && DI.SX1!=DI.EndX1) DrawRawLine(*pp2,pP[DI.SX2-DI.DirX2+1]);
				if (DrawMode&DI.dmf_LX2 && !DI.PatX1Cnt && DI.SX2!=DI.EndX2) DrawRawLine(pP[DI.SX2+1],pP[DI.SX2-DI.DirX2+1]);
			}
		}
	}
	NormalColor();

	/* - - - - Schleifensteuerung 2 - - - - */

	DI.P[DI.SX2-DI.DirX2+1]=DI.p2;
	if (DI._P) DI._P[DI.SX2-DI.DirX2+1]=DI._p2;

	if (DI.V) DI.V[DI.SX2-DI.DirX2+1]=DI.v2;
	DI.p2=p1;
	DI._p2=_p1;
	DI.v2=v1;

	/* - - - - APS System - - - - - - - - - - - - - - - - - - - */
	
	if (Options&OPT_APS && (CheckTime(APS_TH) || DI.SX1==DI.EndX1 && DI.SX2==DI.EndX2))
	{
		if (APS_Filename[0]) // Break-Sicherung durchführen
		{
			if (Options&OPT_ACR) ReduceColors(1);
			if (Options&OPT_Backup) BackupFile(APS_Filename);
			if (SaveBreakFile(APS_Filename)) Error(l[66]);
			else
			{
				PicSecWorth=1;		/* Bild für "unwichtig" erklären */
				DateStamp(&PicLastSave);/* letzte Bildspeicherung festlegen */
			}
			DI.PointsMeasured=0;		/* Zeitschätzung neu starten */
			DI.LastSec=DI.LastMic=0;
		}	
		StartTime(APS_TH,APS_MinIntervall*60);
	}	
	return;
}

/* ------------------------------------------------------------------------ */

double ComputeColor(eyev,v1,v2,v3)	/* errechnet die Farbe für das Dreieck, */
vector *eyev,*v1,*v2,*v3;			/* das durch die Punkte der Ortsvektoren */
{							/* v1,v2,v3 begrenzt wird. */
	double a1,a2,a3;			/* RETURN: Farbe(0-1), oder -1.0 bei Fehler */
	vector Ort,Light,Eye,	/* (ein mittlerer Punkt(v4) ist der Bezugspunkt */
	       Reflex;
	vector v4;
	double lmax=0.0,lsum=0.0,l;
	short a;
	
	VAdd(&v4,VAdd(&v4,v1,v2),v3);	// v4 aus Punktedurchschnitt berechnen
	VSDiv(&v4,3.0);
	
	VSub(&Ort,v2,v1);				/* Ebenenvektoren aufstellen */
	VSub(&Light,v2,v3);				/* ->Vect1/Vect2 (Ort/Light) */

	VProd(&Ort,&Ort,&Light);		/* Ort ist orthogonal zur */
	VNorm(&Ort);					/* Dreiecksebene */

	VSub(&Eye,eyev,&v4);/* Eye ist der Sichtvektor */
	VNorm(&Eye);
	a1=VAngleN(&Ort,&Eye)/(pi/2);	/* a1 = 1 - Winkel der Ebene zum Auge */

	for (a=0;a<MAXLIGHTS;a++)
	{
		if (LightsOn&(1<<a) && LightIntens[a])
		{
			VSub(&Light,&LightVect[a],&v4);	/* Light ist der Lichtvekt. */
			VNorm(&Light);
			a2=VAngleN(&Ort,&Light)/(pi/2);	/* Winkel(Bogenmaß) zum einfallenden */
											/* -> a2 */
				
			if (a1<0.0 || a2<0.0) return(-1.0);
			
			if ((a1<=1.0)==(a2<=1.0))	/* Auge und Licht auf derselben */
			{							/* Seite der Ebene ? */
				/* diffuse Reflexion */
				if (a2>1.0) a2=a2-1.0;			/* 0<a2<1 */
				else a2=1.0-a2;
				a2*=Diffuse_Rfx;
				
				/* punktuelle Reflexion */
				if (Direct_Rfx)			/* Funktion glänzend ? */
				{
					Reflex=Ort;
					VSMul(&Reflex,2*VSProd(&Eye,&Ort)); /* Reflex ist Reflex.vect */
					VSub(&Reflex,&Reflex,&Eye);
					a3=VAngleN(&Reflex,&Light)/(pi/2);	/* a3 Winkel Licht,Reflex.vect */
		
					if (a3<=1.0) a3=1.0-a3;			/* 0<a3<1 ? */
					else a3=0.0;
					a3*=a3*a3*Direct_Rfx;
				}
				else a3=0.0;
				
				lsum+=(l=(a2+a3)*LightIntens[a]);
				lmax=max(lmax,l);
			}
		}
	}
	return(min(1.0,0.85*lmax+0.15*lsum));
}

/* ------------------------------------------------------------------------- */

void MakeDrawingInfo(void)	/* berechnet alle Einträge der DI-Struktur */
{
	if (DI.P)			/* gegf. P-Zws. freigeben */
	{
		FreeMem(DI.P,DI.PSize*sizeof(point));
		DI.P=0;
	}
	if (DI._P)			/* gegf. _P-Zws. freigeben */
	{
		FreeMem(DI._P,DI.PSize*sizeof(point));
		DI._P=0;
	}
	if (DI.V)				/* gegf. V-Zws. freigeben */
	{
		FreeMem(DI.V,DI.PSize*sizeof(vector));
		DI.V=0;
	}
						/* gegf. Achsentausch vornehmen */
	if (zAngle>.25*pi && zAngle<.75*pi || zAngle>1.25*pi && zAngle<1.75*pi)
	{
		DI.X1=Y; DI.X2=X;  DI.x1Res=yRes; DI.x2Res=xRes;
		DI.dmf_LX1=DMF_LY; DI.dmf_LX2=DMF_LX; 
		DI.x1Min=yMin; DI.x2Min=xMin;  DI.SpacingX1=SpacingY; DI.SpacingX2=SpacingX;
		DI.dx1=(yMax-yMin)/(double)DI.x1Res;	/* Schrittweiten berechnen */
		DI.dx2=(xMax-xMin)/(double)DI.x2Res;
	}
	else
	{
		DI.X1=X; DI.X2=Y;  DI.x1Res=xRes; DI.x2Res=yRes;
		DI.dmf_LX1=DMF_LX; DI.dmf_LX2=DMF_LY; 
		DI.x1Min=xMin; DI.x2Min=yMin;  DI.SpacingX1=SpacingX; DI.SpacingX2=SpacingY;
		DI.dx1=(xMax-xMin)/(double)DI.x1Res;	/* Schrittweiten berechnen */
		DI.dx2=(yMax-yMin)/(double)DI.x2Res;
	}
							/* Schleifenrichtung festlegen */
	if (zAngle>0.25*pi && zAngle<1.25*pi)
	{ DI.BeginX1=DI.x1Res+1; DI.EndX1=-1; DI.DirX1=-1; }
	else
	{ DI.BeginX1=-1; DI.EndX1=DI.x1Res+1; DI.DirX1=1; }
	if (zAngle>0 && zAngle<.25*pi || zAngle>.5*pi && zAngle<pi || zAngle>1.25*pi && zAngle<1.5*pi)
	{ DI.BeginX2=DI.x2Res+1; DI.EndX2=-1; DI.DirX2=-1; }
	else
	{ DI.BeginX2=-1; DI.EndX2=DI.x2Res+1; DI.DirX2=1; }
	if (yAngle>0.5*pi && yAngle<1.5*pi)
	{
		short z;
		z=DI.BeginX1; DI.BeginX1=DI.EndX1; DI.EndX1=z;
		z=DI.BeginX2; DI.BeginX2=DI.EndX2; DI.EndX2=z;
		DI.DirX1*=-1; DI.DirX2*=-1;
	}
		
	DI.SX1=DI.BeginX1; DI.SX2=DI.EndX2;			/* Schleifenzähler initialisieren */

	DI.PSize=DI.x2Res+3;				/* P-Zwischenspeicher vorbereiten */

	if (DI.P=AllocMem(DI.PSize*sizeof(point),0))
	{
		short a;
		for (a=0; a<DI.PSize;a++) DI.P[a].bx=DI.P[a].by=ILL;
	} else { Error(l[68]); submode=-1; Mode(MODE_WAIT); return; }
	if (DrawMode&(DMF_COL|DMF_LCOL)) if (!(DI.V=AllocMem(DI.PSize*sizeof(vector),0)))  { Error(l[68]); submode=-1; Mode(MODE_WAIT); return; }

	if (Options&OPT_RG3d)
	{
		if (DI._P=AllocMem(DI.PSize*sizeof(point),0))
		{
			short a;
			for (a=0; a<DI.PSize;a++) DI._P[a].bx=DI._P[a].by=ILL;
		} else { Error(l[68]); submode=-1; Mode(MODE_WAIT); return; }
	}		
	DI.EyeYOffset=activePrefs->EyeEyeDist/2.0;

	DI.RetracedEyeVect.x=DI._RetracedEyeVect.x=activePrefs->EyeScreenDist;
	DI.RetracedEyeVect.y=DI._RetracedEyeVect.y=0.0;
	DI.RetracedEyeVect.z=DI._RetracedEyeVect.z=0.0;
	
	if (Options&OPT_RG3d)	/* Auge-koor. zurückrechnen */
	{
		DI.RetracedEyeVect.y-=DI.EyeYOffset;
		DI._RetracedEyeVect.y+=DI.EyeYOffset;
		RetraceVect(&DI.RetracedEyeVect);
		RetraceVect(&DI._RetracedEyeVect);
	}
	else RetraceVect(&DI.RetracedEyeVect);
		
	DI.PatX1Cnt=-2;			/* Patternspacing initialisieren */
		
	DI.LastSec=DI.LastMic=0;		/* für OPT_Time */
	DI.PointCnt=0;
	DI.PointsToDo=(DrawMode&DMF_LCOL)?(DI.x1Res+2)*(DI.x2Res+2):(DI.x1Res+1)*(DI.x2Res+1);
	DI.PointsMeasured=0;
	DI.MaxMeasure=(DI.x2Res+1)*(DI.SpacingX1+1);

	/* Standardfarbe festlegen (evtl. bei DMF_FL gebraucht) */
	_stdcol=(DrawMode&DMF_FL)?((Options&OPT_RG3d)?(double)RGColSupp->BGCol:3.0):-1.0;
	_stdcol=(DrawMode&DMF_FLL)?BodyCol:_stdcol;
}
	
