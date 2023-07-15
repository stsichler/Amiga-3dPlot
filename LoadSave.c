/* Plot V2.1 ® SSi 93/94 ---------------------------------------------------- */
/* Lade-/Speicherroutinen */

#include "Plot.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <exec/memory.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <user/functions.h>
#include <intuition/gadgetclass.h>

/* Makrodefinition für Kompatibilität (FnctStruct/old_FnctStruct) ---------- */

#define from_ffp(target,source) { real __buf; toreal_ffp(&__buf,source,4); byreal(target,&__buf); }
	
/* Übergabeparameter ------------------------------------------------------- */

extern char FunctionString[];	/* Funktion im ASCII-Format */
extern fnctcode *FunctionCode;		/* RAW-Code der Funktion */

extern double xMin;
extern double xMax;
extern double yMin;
extern double yMax;			/* Bereich der Funktion */
extern double zMin;
extern double zMax;
	
extern short xRes;			/* Berechnungsschritte */
extern short yRes;

extern double yAngle;			/* Kippwinkel der Funktion */
extern double zAngle;

extern short DrawMode;		/* Zeichenmodus */
extern short Options;			/* gewählte Optionen */

extern vector LightVect[MAXLIGHTS];	/* Lichtquellenkoordinaten */
extern double LightIntens[MAXLIGHTS];	/* Lichtquellenintensitäten (0-1) */
extern short LightsOn;
extern short SLS;
extern struct Gadget *LightSelG;	/* Lichtselektionsgadgets (ID 55) */

extern short SpacingX,SpacingY;	/* x/y - Musterabstand */
extern double xResPac,yResPac;		/* Resol./Spacing - Quotient */

extern double SizeFact;

extern double Diffuse_Rfx,Direct_Rfx;
extern double PatternCol,PatternDeltaCol;	/* Funktionsfarben */
extern double BodyCol;

/* Globale Daten ----------------------------------------------------------- */

extern struct Screen *OurS;
extern struct Window *OurW;
extern struct Window *Opt1W,*Opt2W;

extern short Status;
extern short mode;

extern char Filename[];	/* Filename für alle Routinen */

extern short Drawx,DrawX,Drawy,DrawY;
extern double Midx,Midy;				/* Bildschirmmitte (veränderlich) */
extern double _Midx,_Midy;				/* Bildschirmmitte (fest) */

/* Gadgetzeiger von OpenAll() ---------------------------------------------- */

extern struct Gadget *_FuncG,*_xResG,*_yResG,*_xMinG,*_xMaxG,*_yMinG,*_yMaxG,
		*_zMinG,*_zMaxG,*_OPattG,*_OxSpaceG,*_OySpaceG,*_OBodyG,
		*_OxLightG,*_OyLightG,*_OzLightG,*_OPT_ACR_G,*_OPT_ZLimit_G,
		*_OPT_Time_G,*_OPT_Quad_G,*_OPT_WorkBench_G,*_OPT_DitherCol_G,
		*_dRfxG,*_pRfxG,*_OGPatColG,*_OGLightStateG,*_OLIntensG,*_OGBodyL,
		*_OGPatL;
		
/* Programm ---------------------------------------------------------------- */

struct FnctStruct *BuildFnctStruct(void)		/* baut eine FnctStruct auf */
{
	struct FnctStruct *fs;
	ULONG StruLen;
	ULONG StriLen;
	short a;
	double buf;
	
	if (Status) { Error(l[63]); return(0); }
	StruLen=sizeof(struct FnctStruct);
	StriLen=strlen(FunctionString);
	
	if (!(fs=(struct FnctStruct *)AllocMem(StruLen+StriLen+1,MEMF_CLEAR))) { Error(l[68]); return(0); }
	
	strcpy(fs->FnctString,FunctionString);
	
	strcpy(fs->IDString,IDSTRING);
	fs->Version=PLOTVERSION;	
	fs->Subversion=PLOTSUBVERSION;	
	fs->StructLen=StruLen;
	fs->FnctStrLen=StriLen;
	
	fs->FloatLen=sizeof(real);			/* ******** ACHTUNG ******** */
	fs->FloatType=REAL_FLOATTYPE;
	
	fs->Resx=xRes;
	fs->Resy=yRes;
	fs->DrawMode=DrawMode;
	fs->Options=Options&OPTMASK;
	
	fs->PttnSpacingx=SpacingX;
	fs->PttnSpacingy=SpacingY;
	toreal(&fs->Minx,&xMin);
	toreal(&fs->Maxx,&xMax);
	toreal(&fs->Miny,&yMin);
	toreal(&fs->Maxy,&yMax);
	toreal(&fs->Minz,&zMin);
	toreal(&fs->Maxz,&zMax);
	toreal(&fs->Angley,&yAngle);
	toreal(&fs->Anglez,&zAngle);
	buf=SizeFact/((double)(DrawX-Drawx)/activePrefs->XAspect+(double)(DrawY-Drawy)/activePrefs->YAspect);
	toreal(&fs->SizeQuot,&buf);
	buf=(Midx-(double)Drawx)/(double)(DrawX-Drawx+1);
	toreal(&fs->MidxQuot,&buf);
	buf=(Midy-(double)Drawy)/(double)(DrawY-Drawy+1);
	toreal(&fs->MidyQuot,&buf);
	
	toreal(&fs->Diffuse_Rfx,&Diffuse_Rfx);
	toreal(&fs->Direct_Rfx,&Direct_Rfx);
	
	fs->LightCnt=MAXLIGHTS;
	fs->LightsOn=LightsOn;
	
	for (a=0; a<MAXLIGHTS; a++)
	{
		toreal(&fs->Light[a].x,&LightVect[a].x);
		toreal(&fs->Light[a].y,&LightVect[a].y);
		toreal(&fs->Light[a].z,&LightVect[a].z);
		toreal(&fs->Light[a].Intens,&LightIntens[a]);
	}
	
	toreal(&fs->BodyColor,&BodyCol);
	toreal(&fs->PatternColor,&PatternCol);
	toreal(&fs->PatternDeltaColor,&PatternDeltaCol);
	
	return(fs);
}

/* ------------------------------------------------------------------------- */

void FreeFnctStruct(struct FnctStruct *fs)	/* gibt den Speicher wieder */
{											/* frei */
	if (!fs) return;
	FreeMem(fs,sizeof(struct FnctStruct)+fs->FnctStrLen+1);
}

/* ------------------------------------------------------------------------- */

struct FnctStruct *LoadFnctStruct(void)	/* lädt eine Struktur (Filename) */
{										/* RETURN: Strukt oder 0 bei Fehler */
	struct FnctStruct *fs;
	BPTR File;

	if (!(File=Open(Filename,MODE_OLDFILE))) { Error(l[69]); return(0); }
	
	fs=_LoadFnctStruct(File);

	Close(File);
	return(fs);
}
	
struct FnctStruct *_LoadFnctStruct(BPTR File)/* lädt eine Struktur (File) */
{										/* RETURN: Strukt oder 0 bei Fehler */
	char SH[offsetof(struct FnctStruct,DataBegin)];
	struct FnctStruct *fs=0;

	long FileBegin;

	FileBegin=Seek(File,0,OFFSET_CURRENT);
	
	/* Strukturkopf einlesen */
	
	if (Read(File,SH,offsetof(struct FnctStruct,DataBegin))!=-1)
	{
		ULONG Len;
		struct FnctStruct *fsh;	/* Zeiger auf Headerpuffer */
		
		fsh=(struct FnctStruct *)SH;
									/* Legalität überprüfen */
		if (!strcmp(fsh->IDString,IDSTRING)) /* && fsh->Version<=VERSION) */
		{
			Len=sizeof(struct FnctStruct)+fsh->FnctStrLen+1;
			
			if (fs=(struct FnctStruct *)AllocMem(Len,MEMF_CLEAR))
			{
				ULONG ReadStrLen;
				Seek(File,FileBegin,OFFSET_BEGINNING);
				
				ReadStrLen=min(fsh->StructLen,sizeof(struct FnctStruct));

				if (Read(File,fs,ReadStrLen)==-1)	/* Struktur einlesen */
				{
					Error(l[74]);
					FreeMem(fs,Len);
					fs=0;
				}
				Seek(File,fsh->StructLen+FileBegin,OFFSET_BEGINNING); /* String einl. */
				if (Read(File,fs->FnctString,fsh->FnctStrLen+1)==-1)
				{
					Error(l[47]);
					FreeMem(fs,Len);
					fs=0;
				}
			} else Error(l[68]);
		} else Error(FileBegin?l[112]:l[73]);
	}
	return(fs);
}

/* ------------------------------------------------------------------------- */

int SaveFnctStruct(struct FnctStruct *fs)		/* speichert die Struktur */
{											/* (Filename) / RETURN: Error */
	int error;
	BPTR File;
	
	if (!fs) return(-1);
	
	if (!(File=Open(Filename,MODE_NEWFILE))) { Error(l[70]); return(-1); }

	error=_SaveFnctStruct(File,fs);

	Close(File);

	CreateIcon(Filename);

	return(error);
}

int _SaveFnctStruct(BPTR File,struct FnctStruct *fs)/* speichert die Struktur */
{											/* (Filename) / RETURN: Error */
	if (!fs) return(-1);
	
	if (Write(File,fs,fs->StructLen+fs->FnctStrLen+1)<0)/* und schreiben */
	{
		Error(l[71]);
		return(-1);
	}
	
	return(0);
}

/* ------------------------------------------------------------------------- */

int UseFnctStruct(struct FnctStruct *fs,short NoRF) /* NoRF = NoGadgetRefresh*/
										/* überträt die Parameter ins Prg. */
{										/* RETURN: Error */	
	if (!fs) return(-1);	/* überhaupt eine Struktur da ? */
	
	PrepNewFunction();		/* *** Funktion einlesen */
	strcpy(FunctionString,fs->FnctString);
	if (CompileFunction(&FunctionCode))
	{
		ClearFunctionCode(&FunctionCode);	/* Funktion nicht in Ordnung */
		Status|=ST_NO_FUNC;
		Error(l[63]);
	}
	else Status&=~ST_NO_FUNC;

							/* ab Version 2 - - - - - - - - */

									/* *** Auflösung */
	xRes=fs->Resx;
	yRes=fs->Resy;
	Status&=~ST_RES;
	
	Options&=~OPTMASK;				/* *** Optionen */
	Options|=fs->Options&OPTMASK;
	DrawMode=fs->DrawMode&DMF_ALL;			/* *** DrawMode */
	
	if (fs->Version<5) DrawMode|=(fs->DrawMode&(DMF_LX|DMF_LY))?DMF_PAT:0;
	
	if (!NoRF)
	{
		short active=0;
		if (DrawMode&DMF_LX) active=1;
		if (DrawMode&DMF_LY) active+=2;
		GT_SetGadgetAttrs(_OPattG,Opt2W,0,GTCY_Active,active,GA_Disabled,(DrawMode&DMF_PAT)?FALSE:TRUE,TAG_END);
		active=0;
		if (DrawMode&DMF_PAT) active=(DrawMode&DMF_LCOL)?2:1; 
		GT_SetGadgetAttrs(_OGPatColG,Opt2W,0,GTCY_Active,active,TAG_END);
		active=0;
		if (DrawMode&DMF_FL) { active=1; if (DrawMode&DMF_FLL) active=2; if (DrawMode&DMF_COL) active=3; }
		GT_SetGadgetAttrs(_OBodyG,Opt2W,0,GTCY_Active,active,TAG_END);
	}			
								/* *** x/y Spacing */
	SpacingX=fs->PttnSpacingx;
	xResPac=(double)xRes/(double)(SpacingX+1);
	SpacingY=fs->PttnSpacingy;
	yResPac=(double)yRes/(double)(SpacingY+1);
	Status&=~ST_SPACE;
								/* !!!!! ACHTUNG !!!!! */
		
	if (fs->FloatLen==4 && fs->FloatType==FFP_FLOATTYPE)
	{
		struct old_FnctStruct *fso=(struct old_FnctStruct *)fs;

		from_ffp(&xMin,&fso->Minx);
		from_ffp(&xMax,&fso->Maxx);					/* Altes Fließkommaformat */
		from_ffp(&yMin,&fso->Miny);
		from_ffp(&yMax,&fso->Maxy);
		from_ffp(&zMin,&fso->Minz);
		from_ffp(&zMax,&fso->Maxz);
		from_ffp(&LightVect[0].x,&fso->Lightx);
		from_ffp(&LightVect[0].y,&fso->Lighty);
		from_ffp(&LightVect[0].z,&fso->Lightz);
		
		from_ffp(&yAngle,&fso->Angley);
		from_ffp(&zAngle,&fso->Anglez);
	
		if (fso->Version>=3)			/* ab Version 3 - - - - - - - - - - - - */
		{
			short a,lread=1;			/* *** Position/Größe */
			double buf=0;
			
			from_ffp(&buf,&fso->SizeQuot);
			SizeFact=buf*((double)(DrawX-Drawx)/activePrefs->XAspect+(double)(DrawY-Drawy)/activePrefs->YAspect);
			ComputeBaseVects();
			from_ffp(&buf,&fso->MidxQuot);
			Midx=buf*(double)(DrawX-Drawx+1)+(double)Drawx;
			from_ffp(&buf,&fso->MidyQuot);
			Midy=buf*(double)(DrawY-Drawy+1)+(double)Drawy;
		
			from_ffp(&Diffuse_Rfx,&fso->Diffuse_Rfx);
			from_ffp(&Direct_Rfx,&fso->Direct_Rfx);
	
			LightsOn=fso->LightsOn;	/* zusätzliche Lichtquellen einlesen */
			
			from_ffp(&LightIntens[0],&fso->Light0Intens);
			
			for (a=0;a<min(MAXLIGHTS-1,fso->ExtraLightCnt);a++)
			{
				from_ffp(&LightVect[a+1].x,&fso->ExLight[a].x);
				from_ffp(&LightVect[a+1].y,&fso->ExLight[a].y);
				from_ffp(&LightVect[a+1].z,&fso->ExLight[a].z);
				from_ffp(&LightIntens[a+1],&fso->ExLight[a].Intens);
				lread|=1<<(a+1);
			}
			LightsOn&=lread;
		}
		else
		{
			CompuSizeFact();
			Midx=_Midx;
			Midy=_Midy;
			LightsOn=1;
			LightIntens[0]=1.0;
			SLS=0;
			if (!NoRF) GT_SetGadgetAttrs(LightSelG,Opt2W,0,GTSL_Level,SLS+1,TAG_END);
			Diffuse_Rfx=1.0;
			Direct_Rfx=0.0;
			BodyCol=0.3;
			PatternCol=0.5;
			PatternDeltaCol=(fs->DrawMode&(1<<5)/*DMF_LZN*/)?-0.15:0.15;
		}
	}
	else
	{		/* neues real-Fließkommaformat */

		if (fs->FloatLen!=sizeof(real) ||  /* richtiges Format ? */
		    fs->FloatType!=REAL_FLOATTYPE) { Error(l[75]); return(-1); }
	
		byreal(&xMin,&fs->Minx);
		byreal(&xMax,&fs->Maxx);
		byreal(&yMin,&fs->Miny);
		byreal(&yMax,&fs->Maxy);
		byreal(&zMin,&fs->Minz);
		byreal(&zMax,&fs->Maxz);
		
		byreal(&yAngle,&fs->Angley);
		byreal(&zAngle,&fs->Anglez);
	
		if (fs->Version>=3)			/* ab Version 3 - - - - - - - - - - - - */
		{
			short a,lread=1;			/* *** Position/Größe */
			double buf;
			
			byreal(&buf,&fs->SizeQuot);
			SizeFact=buf*((double)(DrawX-Drawx)/activePrefs->XAspect+(double)(DrawY-Drawy)/activePrefs->YAspect);
			ComputeBaseVects();
			byreal(&buf,&fs->MidxQuot);
			Midx=buf*(double)(DrawX-Drawx+1)+(double)Drawx;
			byreal(&buf,&fs->MidyQuot);
			Midy=buf*(double)(DrawY-Drawy+1)+(double)Drawy;
		
			byreal(&Diffuse_Rfx,&fs->Diffuse_Rfx);
			byreal(&Direct_Rfx,&fs->Direct_Rfx);
	
			LightsOn=fs->LightsOn;	/* zusätzliche Lichtquellen einlesen */
						
			for (a=0;a<min(MAXLIGHTS,fs->LightCnt);a++)
			{
				byreal(&LightVect[a].x,&fs->Light[a].x);
				byreal(&LightVect[a].y,&fs->Light[a].y);
				byreal(&LightVect[a].z,&fs->Light[a].z);
				byreal(&LightIntens[a],&fs->Light[a].Intens);
				lread|=1<<a;
			}
			LightsOn&=lread;
		
			if (fs->Version>=5)		/* Ab Version 5 - - - - - - - - - - - - - - */
			{
				byreal(&BodyCol,&fs->BodyColor);
				byreal(&PatternCol,&fs->PatternColor);
				byreal(&PatternDeltaCol,&fs->PatternDeltaColor);
			}
			else
			{
				BodyCol=0.3;
				PatternCol=0.5;
				PatternDeltaCol=(fs->DrawMode&(1<<5)/*DMF_LZN*/)?-0.15:0.15;
			}
		}
		else
		{
			CompuSizeFact();
			Midx=_Midx;
			Midy=_Midy;
			LightsOn=1;
			LightIntens[0]=1.0;
			SLS=0;
			if (!NoRF) GT_SetGadgetAttrs(LightSelG,Opt2W,0,GTSL_Level,SLS+1,TAG_END);
			Diffuse_Rfx=1.0;
			Direct_Rfx=0.0;
			BodyCol=0.3;
			PatternCol=0.5;
			PatternDeltaCol=(fs->DrawMode&(1<<5)/*DMF_LZN*/)?-0.15:0.15;
		}
	}
	
	if (!NoRF)
	{
	/* GADGETS AUFFRISCHEN +*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*+ */

		GT_SetGadgetAttrs(_xResG,OurW,0,GTIN_Number,(ULONG)xRes,TAG_END);
		GT_SetGadgetAttrs(_yResG,OurW,0,GTIN_Number,(ULONG)yRes,TAG_END);

		GT_SetGadgetAttrs(_FuncG,OurW,0,GTST_String,FunctionString,TAG_END);

		GT_SetGadgetAttrs(_OPT_ACR_G,Opt1W,0,GTCB_Checked,(Options&OPT_ACR)?TRUE:FALSE,TAG_END);
		GT_SetGadgetAttrs(_OPT_ZLimit_G,Opt2W,0,GTCB_Checked,(Options&OPT_ZLimit)?TRUE:FALSE,TAG_END);
		GT_SetGadgetAttrs(_OPT_Time_G,Opt1W,0,GTCB_Checked,(Options&OPT_Time)?TRUE:FALSE,TAG_END);
		GT_SetGadgetAttrs(_OPT_Quad_G,Opt2W,0,GTCB_Checked,(Options&OPT_Quad)?TRUE:FALSE,TAG_END);
		GT_SetGadgetAttrs(_OPT_WorkBench_G,Opt1W,0,GTCB_Checked,(Options&OPT_WorkBench)?TRUE:FALSE,TAG_END);
		GT_SetGadgetAttrs(_OPT_DitherCol_G,Opt2W,0,GTCB_Checked,(Options&OPT_DitherCol)?TRUE:FALSE,TAG_END);
		
		GT_SetGadgetAttrs(_OxSpaceG,Opt2W,0,GTIN_Number,(ULONG)SpacingX,TAG_END);
		GT_SetGadgetAttrs(_OySpaceG,Opt2W,0,GTIN_Number,(ULONG)SpacingY,TAG_END);
		
		GT_SetGadgetAttrs(_xMinG,OurW,0,GTST_String,ftoa(xMin),TAG_END);
		GT_SetGadgetAttrs(_xMaxG,OurW,0,GTST_String,ftoa(xMax),TAG_END);
		GT_SetGadgetAttrs(_yMinG,OurW,0,GTST_String,ftoa(yMin),TAG_END);
		GT_SetGadgetAttrs(_yMaxG,OurW,0,GTST_String,ftoa(yMax),TAG_END);
		GT_SetGadgetAttrs(_zMinG,OurW,0,GTST_String,ftoa(zMin),TAG_END);
		GT_SetGadgetAttrs(_zMaxG,OurW,0,GTST_String,ftoa(zMax),TAG_END);

		{
			BOOL dis;	/* Gadgets enable/disable Status anpassen */

			dis=(DrawMode&DMF_PAT)?FALSE:TRUE;
			GadDisable(_OxSpaceG,Opt2W,dis);
			GadDisable(_OySpaceG,Opt2W,dis);
			dis=(DrawMode&(DMF_COL|DMF_LCOL))?FALSE:TRUE,
			GadDisable(_dRfxG,Opt2W,dis);	
			GadDisable(_pRfxG,Opt2W,dis);	
			GadDisable(LightSelG,Opt2W,dis);	
			GadDisable(_OGLightStateG,Opt2W,dis);	
			GadDisable(_OGBodyL,Opt2W,((DrawMode&(DMF_FL|DMF_FLL|DMF_COL))==(DMF_FL|DMF_FLL))?FALSE:TRUE);
			GT_SetGadgetAttrs(_OGPatL,Opt2W,0,GA_Disabled,(DrawMode&DMF_PAT)?FALSE:TRUE,GTSL_Min,(DrawMode&DMF_LCOL)?-20:0,GTSL_Level,(WORD)(((DrawMode&DMF_LCOL)?PatternDeltaCol:PatternCol)*20.0),TAG_END);
	}							
	
	/* REFLEXIONSANZEIGEN AUFFRISCHEN +*+*+*+*+*+*+*+*+*+*+*+*+*+* */
	
		GT_SetGadgetAttrs(_dRfxG,Opt2W,0,GTSL_Level,(short)(Diffuse_Rfx*20.0),TAG_END);
		GT_SetGadgetAttrs(_pRfxG,Opt2W,0,GTSL_Level,(short)(Direct_Rfx*20.0),TAG_END);

	/* LICHTQUELLENANZEIGE AUFFRISCHEN +*+*+*+*+*+*+*+*+*+*+*+* */

		RefreshLightGadgets();
	}
	return(0);
}

/* Hilfsroutinen -------------------------------------------------------- */

char *ftoa(double val)		/* konvertiert die double-Zahl in einen String */
{
	static char Buffer[18];
	
	gcvt(val,5,Buffer);
	return(Buffer);
}

ULONG GetFirstLWofFile(char *Filename)	/* liest das 1. LW */
{
	BPTR fp;
	ULONG lw=0;
	
	if (fp=Open(Filename,MODE_OLDFILE))
	{
		Read(fp,&lw,sizeof(ULONG));
		Close(fp);
	}
	return(lw);
}
