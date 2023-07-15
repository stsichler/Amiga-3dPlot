/* PlotV2.0 - Disk-Modul (eigenständiger Task) */

#include "Plot.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <intuition/intuition.h>
#include <proto/gadtools.h>
#include <exec/lists.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <string.h>

/* Verbindung zum Haupttask ------------------------------------------------- */

char *_Filename=0;			/* gewählter Filename od. 0 bei ungültig */

extern BPTR HomeDir_Lock;	/* Homedir Lock des Programms (needs not to be UnLock()-ed */

extern BPTR DefaultDir_Lock;	/* needs not to be UnLock()-ed */
extern char *Default_fnct_Name;	/* needs not to be free()-ed */

extern BPTR FnctDir_Lock;		/* gets UnLock()-ed in CloseAll() */

extern volatile short IDCMP_locked;

/* Taskkommunikation -------------------------------------------------------- */

#define SendDiskMsg(type) { while (DiskMsg.Type); DiskMsg.Msg.mn_Length=2; DiskMsg.Type=type; if (MainTask_MsgPort) PutMsg(MainTask_MsgPort,(struct Message *)&DiskMsg); else DiskMsg.Type=0; }
#define ReplyOwnMsg(msg) ((struct OwnMsg *)msg)->Type=0;
#define ClearMsgPort(port) { struct Message *msg; if (port) while (msg=GetMsg(port)) ReplyMsg(msg); }

extern struct MsgPort *MainTask_MsgPort;
extern struct MsgPort *DiskTask_MsgPort;

extern volatile struct OwnMsg MainMsg;		/* Nachricht von Main->Disk */
       volatile struct OwnMsg DiskMsg;		/* Nachricht von Disk->Main */
 
/* Gadgets ------------------------------------------------------------------ */

extern struct Screen *OurS;
extern struct Window *OurW;
extern struct Window *DiskW;

extern struct Gadget *_DGPath;
extern struct Gadget *_DGPat;
extern struct Gadget *_DGFile;
extern struct Gadget *_DGList;

/* Taskeigene Daten --------------------------------------------------------- */

char Buffer[560];			/* Puffer für Stringoperationen */

struct List Entries; 		/* Liste für das Listview-Gadget */

short Ready=0;			/* Programm hat die Dir ganz eingelesen */

short End=0;				/* Programm soll beendet werden */
short IsToRead=1;			/* Dir muß frisch gelesen werden */

BPTR DirLock=0;			/* Lock des derz. Dir */
BPTR NewLock=0;			/* Lock des anzuzeigenden Dirs */

struct FileInfoBlock __aligned FIB;

short Window_Detached=0;	/* Window ist geschlossen und soll nicht benutzt */
												/* werden. */

char FilePat[40]="#?.((fnct)|(brk))";	/* Lese-Filepattern */
char ParsedFilePat[40];

/* Prototypen --------------------------------------------------------------- */

void __interrupt DClose(void);
void __interrupt DError(void);
void __interrupt DFreeList(void);
void __interrupt DEnqueueEntry(struct Node *);

void __interrupt __asm _ReqTFIntCode(register __a1 void *data);

/* TaskEntry ---------------------------------------------------------------- */

int __saveds __interrupt DiskTask(void)	/* Beim Start übernimmt der Task */
{										/* gegf. einen Pfad aus _Filename */
	struct IntuiMessage *msg,*gtmsg;
	struct Process *OurProc;
	short ID;
	char *dt_Pfad,*dt_File;	/* Puffer bei Windowdetach */
	static char StdPath[560];
	
	if (!(DiskTask_MsgPort=CreateMsgPort())) { Error(l[118]); SendDiskMsg(MT_ENDED); return(0); }
	
	DiskW->UserPort=DiskTask_MsgPort;
	ModifyIDCMP(DiskW,IDCMP_DISKREMOVED|IDCMP_DISKINSERTED|IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|LISTVIEWIDCMP|IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_CHANGEWINDOW);

	GT_SetGadgetAttrs(_DGPat,DiskW,0,GTST_String,FilePat,TAG_END);
	ParsePatternNoCase(FilePat,ParsedFilePat,39);

	OurProc=(struct Process *)FindTask(0);
	OurProc->pr_WindowPtr=DiskW;
	
	if (FnctDir_Lock) NameFromLock(FnctDir_Lock,StdPath,559);
	else StdPath[0]=0;

	if (DefaultDir_Lock) NameFromLock(DefaultDir_Lock,Buffer,559);
	else if (FnctDir_Lock) NameFromLock(FnctDir_Lock,Buffer,559);
	else if (HomeDir_Lock) NameFromLock(HomeDir_Lock,Buffer,559);
	
	NewLock=Lock(Buffer,ACCESS_READ);
		
	Entries.lh_Head=(struct Node *)&Entries.lh_Tail;
	Entries.lh_Tail=0;
	Entries.lh_TailPred=(struct Node *)&Entries.lh_Head;

	Forbid();
	
	if (DefaultDir_Lock) GT_SetGadgetAttrs(_DGFile,DiskW,0,GTST_String,Default_fnct_Name,TAG_END);
	
	strmfp(Buffer,Buffer,((struct StringInfo *)(_DGFile->SpecialInfo))->Buffer);
	_Filename=Buffer;
	
	Permit();

	SendDiskMsg(MT_OK);	/* dem MainTask erfolgreichen Start signalisieren */
	
	while (End==0)
	{
		if (Ready || Window_Detached) WaitPort(DiskTask_MsgPort);
		
		if (msg=(struct IntuiMessage *)GetMsg(DiskTask_MsgPort))
		{
			if (msg->ExecMessage.mn_Length==2)/* eigene Msg. */
			{
				if (((struct OwnMsg *)msg)->Type==MT_ENDREQ) End=-1;
				if (((struct OwnMsg *)msg)->Type==MT_DISKREFRESHREQ)
				{	/* Liste soll aufgefrischt werden. */
					if (DirLock)
					{
						NewLock=DupLock(DirLock);
						IsToRead=1;
					}
					else
					{
						NewLock=0;
						IsToRead=2;
					}
					Ready=0;
				}
				if (((struct OwnMsg *)msg)->Type==MT_DETACH)
				{
					if (!Window_Detached)
					{
						Window_Detached=1;
						Forbid();
						StripIntuiMessages(DiskTask_MsgPort,DiskW);

						dt_Pfad=strdup(((struct StringInfo *)(_DGPath->SpecialInfo))->Buffer);
						dt_File=strdup(((struct StringInfo *)(_DGFile->SpecialInfo))->Buffer);

						if (DiskW)
						{
							DiskW->UserPort=0;
							ModifyIDCMP(DiskW,0);
						}
						OurProc->pr_WindowPtr=0;
						Permit();
					}
					SendDiskMsg(MT_OK);
				}
				if (((struct OwnMsg *)msg)->Type==MT_ATTACH)
				{
					Window_Detached=0;
					DiskW->UserPort=DiskTask_MsgPort;
					ModifyIDCMP(DiskW,IDCMP_DISKREMOVED|IDCMP_DISKINSERTED|IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|LISTVIEWIDCMP|IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY);
					OurProc->pr_WindowPtr=DiskW;
					GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,&Entries,TAG_END);
					GT_SetGadgetAttrs(_DGPath,DiskW,0,GTST_String,dt_Pfad,TAG_END);
					((struct StringInfo *)(_DGPath->SpecialInfo))->BufferPos=strlen(dt_Pfad)+1;
					GT_SetGadgetAttrs(_DGPat,DiskW,0,GTST_String,FilePat,TAG_END);
					GT_SetGadgetAttrs(_DGFile,DiskW,0,GTST_String,dt_File,TAG_END);
					free(dt_File); dt_File=0;
					free(dt_Pfad); dt_Pfad=0;
					SendDiskMsg(MT_OK);
				}
				
				ReplyOwnMsg(msg);
				msg=0;
			}
			else	/* Intuition-Message */
			{
				if (!Window_Detached)
				{
					if (msg->Class&(IDCMP_GADGETUP|IDCMP_GADGETDOWN)) ID=((struct Gadget *)(msg->IAddress))->GadgetID;
					if (!(msg->Class&(IDCMP_CLOSEWINDOW|IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY) || msg->Class&IDCMP_GADGETUP && ID>=43 && ID<=45))
					{
						if (gtmsg=GT_FilterIMsg(msg))
						{
							/* eigenes Gadget gedrückt ----------------------- */
							
							if (gtmsg->Class&(IDCMP_DISKREMOVED|IDCMP_DISKINSERTED) && IsToRead==2)
							{
								NewLock=0;	/* Devs neu einlesen */
								IsToRead=2;
								Ready=0;
							}							
							if (gtmsg->Class&IDCMP_GADGETUP)
							{
								switch (((struct Gadget *)(gtmsg->IAddress))->GadgetID)
								{
									case 40:	/* List-select - - - - - - - - - - */
									{
										struct Node *sel;
										short i=0;			/* Eintrag suchen */
										
										sel=(struct Node *)&Entries;
										while (i++<=gtmsg->Code) sel=sel->ln_Succ;
											
										switch (sel->ln_Pri)	/* Eintragstyp ? */
										{
											case 0: 			/* Datei */
												GT_SetGadgetAttrs(_DGFile,DiskW,0,GTST_String,sel->ln_Name,TAG_END);
												strcpy(Buffer,((struct StringInfo *)(_DGPath->SpecialInfo))->Buffer);
												strmfp(Buffer,Buffer,sel->ln_Name);
												_Filename=Buffer;
												break;
											case 1:				/* Directory */
											case 2:				/* Device */
											case 3:				/* Assign */
											case 4:				/* Volume */
												NewLock=Lock(&sel->ln_Name[6],ACCESS_READ);
												IsToRead=1;
												Ready=0;
												break;
											case 11:				/* Parent Dir */
												NewLock=ParentDir(DirLock);
												IsToRead=1;
												Ready=0;
												if (NewLock) break;
											case 10:				/* Devs */
												NewLock=0;
												IsToRead=2;
												Ready=0;
												break;
										}
									}
									break;
									case 41:	/* Pfad eingegeben - - - - - - - - */
										{
											char *name;
											name=((struct StringInfo *)(_DGPath->SpecialInfo))->Buffer;
											if (!name[0] && StdPath[0]) name=StdPath;
											NewLock=Lock(name,ACCESS_READ);
											IsToRead=1;
											Ready=0;
										}
										break;
									case 81:	/* Filemuster eingegeben - - - - - - - - */
										{
											strcpy(FilePat,((struct StringInfo *)(_DGPat->SpecialInfo))->Buffer);
											ParsePatternNoCase(FilePat,ParsedFilePat,39);
											if (DirLock)
											{
												NewLock=DupLock(DirLock);
												IsToRead=1;
												Ready=0;
											}
										}
										break;
									case 42:	/* File eingegeben - - - - - - - - */
										if (DirLock)
										{
											strcpy(Buffer,((struct StringInfo *)(_DGPath->SpecialInfo))->Buffer);
											strmfp(Buffer,Buffer,((struct StringInfo *)(_DGFile->SpecialInfo))->Buffer);
											_Filename=Buffer;
										}
										break;
									default : break;
								}	
							}
							/* ------------------------------------------------ */
							GT_PostFilterIMsg(gtmsg);
						}
					}
					else
					{
						if (!IDCMP_locked)	/* sonst weitersenden */
						{
							PutMsg(MainTask_MsgPort,(struct Message *)msg);
							msg=0;
						}
					}
				}
			}
			if (msg) ReplyMsg((struct Message *)msg);	
		}
		/* Directory einlesen ------------------------------------------- */
					/* (Newlock) */
		
		if (!Ready && !Window_Detached)
		{
			if (NewLock || IsToRead)
			{
				struct Node *node;	/* neue Dir ist anzuzeigen */
				_Filename=0;
				if (IsToRead==1) IsToRead=0;
				UnLock(CurrentDir(DupLock(NewLock)));
				if (DirLock) UnLock(DirLock);
				DirLock=NewLock;
				NewLock=0;
				GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,0,TAG_END);
				
				if (DirLock) NameFromLock(DirLock,Buffer,520);
				else Buffer[0]=0;
				GT_SetGadgetAttrs(_DGPath,DiskW,0,GTST_String,Buffer,TAG_END);
				((struct StringInfo *)(_DGPath->SpecialInfo))->BufferPos=strlen(Buffer)+1;
				ActivateGadget(_DGPath,DiskW,0);
				strmfp(Buffer,Buffer,((struct StringInfo *)(_DGFile->SpecialInfo))->Buffer);
				if (DirLock) _Filename=Buffer;
				else _Filename=0;

				DFreeList();
				
				if (DirLock)
				{
					if (!(node=AllocMem(sizeof(struct Node),MEMF_CLEAR))) { Error(l[68]); DError(); }
					else { node->ln_Name=getstr(l[0]); node->ln_Pri=11; AddTail(&Entries,node); }
				}
				if (IsToRead!=2)
				{
					if (!(node=AllocMem(sizeof(struct Node),MEMF_CLEAR))) { Error(l[68]); DError(); }
					else { node->ln_Name=getstr(l[1]); node->ln_Pri=10; AddTail(&Entries,node); }
				}
				GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,&Entries,TAG_END);
				
				if (IsToRead==2)		/* Devices angewählt */
				{
					struct DeviceList *dl;
					struct Node *node;
					
					GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,-1,TAG_END);
					
					dl=(struct DeviceList *)BADDR(((struct DosInfo *)BADDR(((struct RootNode *)(DOSBase->dl_Root))->rn_Info))->di_DevInfo);
				
					Forbid();
					while (dl)
					{
						if (node=(struct Node *)AllocMem(sizeof(struct Node),MEMF_CLEAR))
						{
							if (node->ln_Name=AllocMem(((char*)BADDR(dl->dl_Name))[0]+8,0))
							{
								strcpy(node->ln_Name,l[2]);
								node->ln_Pri=1;
								if (dl->dl_Type==DLT_DEVICE) { strcpy(node->ln_Name,l[3]); node->ln_Pri=2; }
								if (dl->dl_Type==DLT_DIRECTORY) { strcpy(node->ln_Name,l[4]); node->ln_Pri=3; }
								if (dl->dl_Type==DLT_VOLUME) { strcpy(node->ln_Name,l[5]); node->ln_Pri=4; }
								
								strncat(node->ln_Name,&((char*)BADDR(dl->dl_Name))[1],((char*)BADDR(dl->dl_Name))[0]);
								strcat(node->ln_Name,":");
								
								DEnqueueEntry(node);
							} else { Error(l[68]); DError(); }
						} else { Error(l[68]); DError(); }
						dl=(struct DeviceList *)BADDR(dl->dl_Next); /* nächstes Device holen */
					}
					Permit();
					GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,&Entries,TAG_END);
					Ready=1;
				}
				else
				{
					if (Examine(DirLock,&FIB)==FALSE) DError();
					if (FIB.fib_DirEntryType<=0)	/* wirklich dir ? */
					{
						DError();
						UnLock(DirLock);
						DirLock=0;
					}
				}
			}
			if (DirLock) /* Verzeichnis kann eingelesen werden */
			{
				if (ExNext(DirLock,&FIB))
				{
					struct Node *node;	/* neue Node anlegen */
					
					if (FIB.fib_DirEntryType>0 || (FilePat[0]?MatchPatternNoCase(ParsedFilePat,FIB.fib_FileName):1))
					{
						if (node=(struct Node *)AllocMem(sizeof(struct Node),MEMF_CLEAR))
						{
							if (FIB.fib_DirEntryType>0)	/* Verz. */
							{
								node->ln_Name=AllocMem(strlen(FIB.fib_FileName)+7,0);
								strcpy(node->ln_Name,l[6]);
								strcat(node->ln_Name,FIB.fib_FileName);
								node->ln_Pri=1;
							}
							else						/* Datei */
							{
								node->ln_Name=getstr(FIB.fib_FileName);
								node->ln_Pri=0;
							}
							
							GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,-1,TAG_END);
	
							DEnqueueEntry(node);
							
							GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,&Entries,TAG_END);
						}
						else { Error(l[68]); DError(); }
					}
				}
				else Ready=1;
			} else if (!Ready) { Ready=1; Error(l[69]); DError(); }
		}
	}

	if (!Window_Detached) GT_SetGadgetAttrs(_DGList,DiskW,0,GTLV_Labels,0,TAG_END);
	
	DFreeList();

	if (dt_Pfad) free(dt_Pfad);
	if (dt_File) free(dt_File);	

	DClose();
	
	SendDiskMsg(MT_ENDED);
	return(0);
}

void __interrupt DClose(void)	/* schließt alles */
{
	_Filename=0;
	if (DirLock) UnLock(DirLock);
	DirLock=0;

	Forbid();
	if (!Window_Detached && DiskW)
	{
		DiskW->UserPort=0;
		ModifyIDCMP(DiskW,0);
	}
	ClearMsgPort(DiskTask_MsgPort);
	DeleteMsgPort(DiskTask_MsgPort);
	DiskTask_MsgPort=0;	
	Permit();
}

void __interrupt DError(void)	/* reagiert auf einen Fehler */
{
	Ready=1; _Filename=0;
}

void __interrupt DFreeList(void)	/* löscht alle Einträge der Liste */
{
	struct Node *node;
	
	while (node=RemHead(&Entries))
	{
		freestr(node->ln_Name);
		FreeMem(node,sizeof(struct Node));
	}
}

void __interrupt DEnqueueEntry(struct Node *node)	/* Eintrag in die Liste einsortieren */
{
	struct Node *nd;	/* File einsortieren */
	nd=(struct Node *)Entries.lh_Head;
	
	while (nd->ln_Succ)
	{
		if (stricmp(nd->ln_Name,node->ln_Name)>0 && nd->ln_Pri==node->ln_Pri || nd->ln_Pri<node->ln_Pri) break;
		nd=nd->ln_Succ;
	}
	Insert(&Entries,node,nd->ln_Pred);
}
