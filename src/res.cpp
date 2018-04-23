#define _RES_

#include <fcntl.h>	 /* O_ constant definitions */

#ifdef _WIN32
#include <io.h>
#pragma option -w-pin
#ifdef __CONSOLE__
#include <windows.h>
#else
#include <wchar.h>
//#include <mbctype.h>
#endif
#else
#include <cwchar>
using namespace std;
#endif

#include "tok.h"
#include "res.h"

#include <string>
using namespace std::string_literals;
#include <boost/algorithm/string.hpp>

RES *listres;	//таблица ресурсов
int numres=0;	//текущее число ресурсов
int maxres=0;	//максимальное число ресурсов

unsigned short *sortidx;	//масив отсортированых индексов ресурсов
RES *curtres;	//текущая таблица ресурсов

unsigned char *resbuf;
unsigned int cursizeresbuf;
unsigned int curposbuf=0;
unsigned int iconcount=0;	//число иконок
unsigned int cursorcount=0;	//число курсоров
unsigned int numidres;	//число ресурсов с разным id
unsigned int numlangres=0;	//число ресурсов с языками
unsigned int numhlangres=0;	//число узлов с языками
unsigned short langdef=0;
unsigned short numzerotype=0;

_STRINGS_ *strinfo=NULL;
int numstrtbl=0;

struct TUSE{
	unsigned short id;
	unsigned short count;
	std::string TName;	//имя типа
}*tuse=NULL;

unsigned int numtyperes=0;	//число типов ресурсов

void addType(unsigned short Type, const string &TName = ""s)
{
	if(tuse==NULL){
		tuse=(TUSE *)MALLOC(sizeof(TUSE));
		tuse->id=Type;
		tuse->count=1;
		if (Type == 0)
			tuse->TName = TName;
		numtyperes=1;
		return;
	}
	for(unsigned int i=0;i<numtyperes;i++){
		if(Type==(tuse+i)->id){
			if (Type == 0 && boost::iequals(TName, (tuse + i)->TName))
				continue;
			(tuse+i)->count++;
			return;
		}
	}
	tuse=(TUSE *)REALLOC(tuse,sizeof(TUSE)*(numtyperes+1));
	(tuse+numtyperes)->id=Type;
	(tuse+numtyperes)->count=1;
	if(Type==0)(tuse+numtyperes)->TName=TName;
	numtyperes++;
}

static unsigned int idnum;
static std::string IDName;
std::string ResName;
static int restok;

void InitBufRes();	//инициализировать буфер для ресурса
void CheckResBuf(unsigned int size);	//проверить и если надо увеличить буфер
void addWString(const char *Name); //добавить строку в ресурс
void addNumOrd(unsigned char *Name);	//добавить ординал/строку
void r_Accelerators();
void r_Dialog();
void r_Icon();
void r_Bitmap();
void r_Menu();
void r_Cursor();
void r_Accelerators();
void r_Font();
void r_Stringtable();
void r_Rcdata();
void GetFileName(char *name);
unsigned char *loadFileBin(const fs::path &Filename);
unsigned short GetFlag(_STRINGS_ *type,int num);
void r_Language();
void r_Version();
void newResource();


void badFormat(const char *name)
{
char buf[80];
	sprintf(buf,"bad format in '%s'",name);
	prError(buf);
	while(tok!=tk_endline&&tok!=tk_eof)nexttok();
}

void expectedrescommand()
{
	prError("expected resourse command");
	while(tok!=tk_endline&&tok!=tk_eof)nexttok();
}

void equalres()
{
	prError("two resource with equal 'id'");
}

void badico()
{
	prError("not load binare image");
	while(tok!=tk_endline&&tok!=tk_eof)nexttok();
}

extern int checkResName(const char *Name);
/*
void SaveFile(char *name)
{
FILE *inih;
	if((inih=fopen(name,"wb"))!=NULL){
		fwrite(resbuf,1,curposbuf,inih);
		fclose(inih);
	}
} */

void input_res()
{
	scanlexmode=RESLEX;
	nexttok();
	while(tok!=tk_eof){
		while(tok==tk_semicolon||tok==tk_endline)nexttok();
		while(tok==tk_question){
			directive();//обработка директив
			while(tok==tk_semicolon||tok==tk_endline)nexttok();
		}
		if(scanlexmode!=RESLEX||tok==tk_eof)break;
		IDName.clear();
		ResName.clear();
		idnum=0;
		restok=-1;
		switch(tok){
			case tk_number:
				idnum=itok.number;
				nexttok();
				break;
			case tk_id:
			case tk_ID:
				IDName = boost::algorithm::to_upper_copy(std::string(itok.name));
				nexttok();
				break;
			case tk_rescommand:
				restok=itok.number;
				nexttok();
				break;
			default:
//				printf("line %u: tok=%u\n",linenumber,tok);
				unuseableinput(); break;
		}
		if(restok==-1){
			if(tok==tk_number)restok=CRT_NEWRESOURCE;
			else if(tok!=tk_rescommand){
				if(strlen(itok.name)/*&&tok2==tk_string*/){
					restok=0;
					ResName = itok.name;
					nexttok();
					newResource();
					continue;
				}
				expectedrescommand();
			}
			else{
				restok=itok.number;
				nexttok();
			}
		}
		switch(restok){
			case rc_accelerators:
				r_Accelerators();
				break;
			case rc_dialogex:
			case rc_dialog:
				r_Dialog();
				break;
			case rc_icon:
				r_Icon();
				break;
			case rc_bitmap:
				r_Bitmap();
				break;
			case rc_menuex:
			case rc_menu:
				r_Menu();
				break;
			case rc_cursor:
				r_Cursor();
				break;
			case rc_font:
				r_Font();
				break;
			case rc_stringtable:
				r_Stringtable();
				break;
			case rc_rcdata:
				r_Rcdata();
				break;
			case rc_language:
				r_Language();
				break;
			case rc_versioninfo:
				r_Version();
				break;
			case CRT_NEWRESOURCE:
				restok=itok.number;
				nexttok();
				newResource();
				break;
			default:
				expectedrescommand();
				nexttok();
				break;
		}
	}
}

void GetResBlock()
{
	if(numres==0){
		maxres=DRESNUM;
		listres=(RES *)MALLOC(DRESNUM*sizeof(RES));
		memset(listres,0,DRESNUM*sizeof(RES));//очистить таблицу
	}
	else{
		if((numres+1)==maxres){
			listres=(RES *)REALLOC(listres,sizeof(RES)*(maxres+DRESNUM));
			memset(listres+maxres,0,DRESNUM*sizeof(RES));//очистить таблицу
			maxres+=DRESNUM;
		}
	}
	curtres=listres+numres;
	curtres->lang=langdef;
	numres++;
}

int OpenBlock()
{
	while(tok==tk_endline)nexttok();
	if(tok==tk_openbrace||(tok==tk_rescommand&&itok.number==rc_begin)){
		nexttok();
		while(tok==tk_endline)nexttok();
		return TRUE;
	}
	return FALSE;
}

int CloseBlock()
{
	while(tok==tk_endline)nexttok();
	if(tok==tk_closebrace||(tok==tk_rescommand&&itok.number==rc_end)){
		nexttok();
		while(tok==tk_endline)nexttok();
		return TRUE;
	}
	return FALSE;
}

unsigned int GetNumber(int par)
{
unsigned int num=0;
	if(tok==tk_number||(tok==tk_minus&&tok2==tk_number))num=doconstdwordmath();
	else{
		numexpected(par);
		nexttok();
	}
	return num;
}

void GetOrdinal(unsigned char **name,int par)
{
NameOrdinal Temp;
	if(tok==tk_number){
		Temp.ordinal[0]=0xffff;
		Temp.ordinal[1]=(unsigned short)doconstdwordmath();
	}
	else if(tok==tk_string){
        Temp.name = (unsigned char *) strdup((char *) String);
		nexttok();
	}
	else numexpected(par);
	*name=Temp.name;
}

void GetRectangle(unsigned char *buf,int par)
{
int i;
	for(i=0;i<4;){
		*(unsigned short *)&buf[i*2]=(unsigned short)GetNumber(i+par);
		i++;
		if(tok==tk_endline)break;
		expecting(tk_camma);
	}
	if(i!=4)prError("expecting window rectangle (4 signed integers)");
}

void addWString(const char *Name)
{
unsigned int pos;
//unsigned char c;
	if(Name==NULL){
		CheckResBuf(2);
		curposbuf+=2;
	}
	else{
		pos=(strlen((char *)Name)+1)*3;
		CheckResBuf(pos);
#ifdef __CONSOLE__
		pos=MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,(char *)Name,-1,(wchar_t *)&resbuf[curposbuf],pos);
#else
		pos=mbsrtowcs((wchar_t *)&resbuf[curposbuf],(char const **)&Name,pos,NULL)+1;
#endif
		curposbuf+=pos*2;
	}
}

void addNumOrd(unsigned char *Name)
{
NameOrdinal Temp;
	Temp.name=Name;
	if(Temp.ordinal[0]==0xFFFF){
		CheckResBuf(4);
		*(unsigned short *)&resbuf[curposbuf]=Temp.ordinal[0];
		*(unsigned short *)&resbuf[curposbuf+2]=Temp.ordinal[1];
		curposbuf+=4;
	}
	else addWString((const char *)Name);
}

void InitBufRes()
{
	resbuf=(unsigned char *)MALLOC(SIZERESBUF);
	memset(resbuf,0,SIZERESBUF);//очистить таблицу
	curposbuf=0;
	cursizeresbuf=SIZERESBUF;
}

void CheckResBuf(unsigned int size)
{
	while((size+curposbuf)>=cursizeresbuf){
		resbuf=(unsigned char *)REALLOC(resbuf,cursizeresbuf+SIZERESBUF);
		memset(resbuf+cursizeresbuf,0,SIZERESBUF);//очистить таблицу
		cursizeresbuf+=SIZERESBUF;
	}
}

void FreeOrdinal(unsigned char *name)
{
NameOrdinal Temp;
	if(name){
		Temp.name=name;
		if(Temp.ordinal[0]!=0xFFFF)free(name);
	}
}

unsigned short GetFlag(_STRINGS_ *type,int num)
{
	for(int i=0;i<num;i++){
		if(boost::iequals(itok.name,(type+i)->id)){
			return (type+i)->val;
		}
	}
	return 0;
}

void r_Language()
{
unsigned short langsec;
	langdef=(unsigned short)GetNumber(1);
	expecting(tk_camma);
	langsec=(unsigned short)GetNumber(2)*langdef*256;
	langdef|=langsec;
	while(tok!=tk_endline&&tok!=tk_eof)nexttok();
}

void r_Stringtable()
{
#define MAXSTRTABINFO 64
static int maxstrinf=MAXSTRTABINFO;
int num;
int j;
	if(strinfo==NULL)strinfo=(_STRINGS_ *)MALLOC(sizeof(_STRINGS_)*MAXSTRTABINFO);
	while(tok!=tk_endline&&tok!=tk_eof)nexttok();
	if(!OpenBlock())badFormat("STRINGTABLE");	//добавить новую
	do{
		num=GetNumber(1);
		for(j=0;j<numstrtbl;j++){
			if(num==(strinfo+j)->val)prError("String ID is already used");
		}
		(strinfo+numstrtbl)->val=(short)num;
		if(tok==tk_camma)nexttok();
		if(tok!=tk_string)badFormat("STRINGTABLE");
        (strinfo + numstrtbl)->id = strdup((char *) String);
		nexttok();
		numstrtbl++;
		if(numstrtbl>=maxstrinf){
			maxstrinf+=MAXSTRTABINFO;
			strinfo=(_STRINGS_ *)REALLOC(strinfo,maxstrinf*sizeof(_STRINGS_));
		}
	}while(!CloseBlock()&&tok!=tk_eof);
}

void CreatStrTabRes()
{
unsigned int i,num,j;
int idnum=0;
int usesec;
	for(i=0;i<65536;){
		idnum++;
		usesec=FALSE;
		for(int ii=0;ii<16;ii++,i++){
			for(j=0;j<numstrtbl;j++){
				if(i==(strinfo+j)->val){
					if(usesec==FALSE){
						usesec=TRUE;
						InitBufRes();
						curposbuf=ii*2;
					}
					const char *name=(strinfo+j)->id;
					*(unsigned short *)&resbuf[curposbuf]=(unsigned short)strlen(name);
					curposbuf+=2;
					addWString(name);
					free((char *)name);
					curposbuf-=4;
					break;
				}
			}
			curposbuf+=2;
		}
		if(usesec){
			GetResBlock();
			curtres->type=CRT_STRING;
			addType(CRT_STRING);
			curtres->res=(unsigned char *)REALLOC(resbuf,curposbuf);
			curtres->size=curposbuf;
			curtres->id=idnum;
		}
	}
	free(strinfo);
}

void domenu(unsigned int exts)
{
	std::string Name;
unsigned int lastpos;
unsigned long help;
	if(OpenBlock()){
		do{
			if(tok!=tk_rescommand)badFormat("MENU");
			lastpos=(exts==TRUE?curposbuf+12:curposbuf);
			CheckResBuf(18);
			restok=itok.number;
			nexttok();
			switch(restok){
				case rc_menuitem:
					if(tok==tk_string){
						Name = std::string((char *) String);
						nexttok();
						if(tok==tk_camma){
							nexttok();
							if(exts){
								if(tok!=tk_camma)*(unsigned long *)&resbuf[curposbuf+8]=GetNumber(2);
								if(tok==tk_camma){
									nexttok();
									if(tok!=tk_camma)*(unsigned long *)&resbuf[curposbuf]=GetNumber(3);
									if(tok==tk_camma){
										do{
											nexttok();
											if(tok==tk_number)*(unsigned long *)&resbuf[curposbuf+4]|=itok.number;
											else *(unsigned long *)&resbuf[curposbuf+4]|=GetFlag((_STRINGS_ *)&typemenu,NUMMENUPOPUP);
											nexttok();
										}while(tok==tk_or);
									}
								}
							}
							else{
								if(tok!=tk_camma)*(unsigned short *)&resbuf[curposbuf+2]=(unsigned short)GetNumber(2);
								if(tok==tk_camma){
									do{
										nexttok();
										if(tok==tk_number)*(unsigned short *)&resbuf[curposbuf]|=(unsigned short)itok.number;
										else *(unsigned short *)&resbuf[curposbuf]|=GetFlag((_STRINGS_ *)&typemenu,NUMMENUPOPUP);
										nexttok();
									}while(tok==tk_or||tok==tk_camma);
								}
							}
						}
						curposbuf+=(exts==FALSE?4:14);
						addWString(Name.c_str());
						if(exts){
							CheckResBuf(2);
							curposbuf=Align(curposbuf,4);
						}
					} else if (boost::iequals(itok.name, "SEPARATOR"s)) {
						if(exts){
							*(unsigned long *)&resbuf[curposbuf]=0x800;
							curposbuf+=16;
						}
						else curposbuf+=6;
						nexttok();
					}
					else badFormat("MENU");
					break;
				case rc_popup:
					help=0;
					if(tok!=tk_string)badFormat("MENU");
					Name = std::string((char *) String);
					*(unsigned short *)&resbuf[lastpos]=(exts==FALSE?0x10:1);
					nexttok();
					if(tok==tk_camma){
						if(exts){
							nexttok();
							if(tok!=tk_camma)*(unsigned long *)&resbuf[curposbuf+8]=GetNumber(2);
							if(tok==tk_camma){
								nexttok();
								if(tok!=tk_camma)*(unsigned long *)&resbuf[curposbuf]=GetNumber(3);
								if(tok==tk_camma){
									do{
										nexttok();
										if(tok==tk_number)*(unsigned long *)&resbuf[curposbuf+4]|=itok.number;
										else *(unsigned long *)&resbuf[curposbuf+4]|=GetFlag((_STRINGS_ *)&typemenu,NUMMENUPOPUP);
										nexttok();
									}while(tok==tk_or);
								}
								if(tok==tk_camma){
									nexttok();
									help=GetNumber(5);
								}
							}
						}
						else{
							do{
								nexttok();
								if(tok==tk_number)*(unsigned short *)&resbuf[curposbuf]|=(unsigned short)itok.number;
								else *(unsigned short *)&resbuf[curposbuf]|=GetFlag((_STRINGS_ *)&typemenu,NUMMENUPOPUP);
								nexttok();
							}while(tok==tk_or);
						}
					}
					curposbuf+=(exts==FALSE?2:14);
					addWString(Name.c_str());
					if(exts){
						CheckResBuf(6);
						curposbuf=Align(curposbuf,4);
						*(unsigned long *)&resbuf[curposbuf]=help;
						curposbuf+=4;
					}
					domenu(exts);
					break;
				default:
					badFormat("MENU");
					break;
			}
		}while(!CloseBlock()&&tok!=tk_eof);
		resbuf[lastpos]|=0x80;
	}
	else badFormat("MENU");
}

void r_Menu()
{
unsigned int exts=FALSE;
	if(restok==rc_menuex)exts=TRUE;
	GetResBlock();
	curtres->type=CRT_MENU;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	addType(CRT_MENU);
	InitBufRes();
	while(tok!=tk_endline)nexttok();
	if(tok==tk_rescommand&&itok.number==rc_language){
		unsigned short olang;
		olang=langdef;
		nexttok();
		r_Language();
		curtres->lang=langdef;
		langdef=olang;
	}
	if(exts){
		*(unsigned long *)&resbuf[0]=0x00040001;
		curposbuf=8;
	}
	else curposbuf=4;
	domenu(exts);
	curtres->res=(unsigned char *)REALLOC(resbuf,curposbuf);
	curtres->size=curposbuf;
}

void r_Rcdata()
{
	GetResBlock();
	curtres->type=CRT_RCDATA;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	addType(CRT_RCDATA);
char name[256];
	name[0]=0;
	GetFileName(name);
	if(name[0]!=0)resbuf= loadFileBin(name);
	else if(tok==tk_string)resbuf= loadFileBin((char *) String);
	else{
		InitBufRes();
		if(!OpenBlock())badFormat("RCDATA");
		do{
			if(tok==tk_number||(tok==tk_minus&&tok2==tk_number)){
				CheckResBuf(2);
				*(unsigned short *)&resbuf[curposbuf]=(unsigned short)GetNumber(0);
				curposbuf+=2;
			}
			else if(tok==tk_string){
				CheckResBuf(itok.number);
				for(int i=0;i<itok.number;i++)resbuf[curposbuf++]=String[i];
				nexttok();
			}
			else if(tok==tk_id||tok==tk_ID){
				CheckResBuf(strlen(itok.name));
				unsigned char c;
				int j=0;
				for(;;){
					c=itok.name[j++];
					if(c==0)break;
					resbuf[curposbuf++]=c;
				}
				nexttok();
			}
			if(tok==tk_rescommand&&itok.number==rc_characteristics){
				nexttok();
				CheckResBuf(4);
				*(unsigned long *)&resbuf[curposbuf]=GetNumber(0);
				curposbuf+=4;
			}
			else badFormat("RCDATA");
			if(tok==tk_camma||tok==tk_semicolon)nexttok();
		}while(!CloseBlock()&&tok!=tk_eof);
	}
	curtres->res=(unsigned char *)REALLOC(resbuf,curposbuf);
	curtres->size=curposbuf;
}

void GetVersPar(unsigned char *buf)
{
	nexttok();
	*(unsigned short *)&buf[2]=(unsigned short)GetNumber(1);
	expecting(tk_camma);
	*(unsigned short *)&buf[0]=(unsigned short)GetNumber(2);
	expecting(tk_camma);
	*(unsigned short *)&buf[6]=(unsigned short)GetNumber(3);
	expecting(tk_camma);
	*(unsigned short *)&buf[4]=(unsigned short)GetNumber(4);
}

void GetBlockInfo()
{
int startpos;
	do{
		startpos=curposbuf;
		CheckResBuf(6);
		curposbuf+=6;
		if (boost::iequals("BLOCK"s, itok.name)) {
			nexttok();
			if(tok!=tk_string)stringexpected();
			CheckResBuf((itok.number+1)*2+2);
			addWString((const char *) String);
			curposbuf=Align(curposbuf,4);
			nexttok();
			if(OpenBlock())GetBlockInfo();
			else badFormat("VERSIONINFO");
		} else if (boost::iequals("VALUE"s, itok.name)) {
			nexttok();
			if(tok!=tk_string)stringexpected();
			CheckResBuf((itok.number+1)*2+2);
			addWString((const char *) String);
			curposbuf=Align(curposbuf,4);
			nexttok();
			if(tok2==tk_string){
				expecting(tk_camma);
				CheckResBuf((itok.number+1)*2+2);
				addWString((const char *) String);
				*(unsigned short *)&resbuf[startpos+4]=1;
				*(unsigned short *)&resbuf[startpos+2]=(unsigned short)itok.number;
				nexttok();
			}
			else{
				do{
					nexttok();
					CheckResBuf(4);
					*(unsigned short *)&resbuf[curposbuf]=(unsigned short)GetNumber(0);
					curposbuf+=2;
					*(unsigned short *)&resbuf[startpos+2]+=2;
				}while(tok==tk_camma);
//				nexttok();
			}
		}
		else badFormat("VERSIONINFO");
		*(unsigned short *)&resbuf[startpos]=(unsigned short)(curposbuf-startpos);
		curposbuf=Align(curposbuf,4);
	}while(CloseBlock()==FALSE);
}

void r_Version()
{
	GetResBlock();
	InitBufRes();
	curtres->type=CRT_VERSION;
	curtres->id=1;
	addType(CRT_VERSION);
	*(unsigned short *)&resbuf[2]=0x34;
//	if(idname[0]==0)stringexpected();
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	curposbuf=6;
	addWString("VS_VERSION_INFO");
	curposbuf=Align(curposbuf,4);
	*(unsigned long *)&resbuf[curposbuf]=0xfeef04bd;
	*(unsigned long *)&resbuf[curposbuf+4]=0x00010000;
	curposbuf+=8;
	while(!OpenBlock()&&tok!=tk_eof){
		switch(GetFlag((_STRINGS_ *)&typeversion,7)){
			case v_fv:
				GetVersPar(resbuf+curposbuf);
				break;
			case v_pv:
				GetVersPar(resbuf+curposbuf+8);
				break;
			case v_ffm:
				nexttok();
				*(unsigned long *)&resbuf[curposbuf+16]=(unsigned long)GetNumber(0);
				break;
			case v_ff:
				nexttok();
				*(unsigned long *)&resbuf[curposbuf+20]=(unsigned long)GetNumber(0);
				break;
			case v_fo:
				nexttok();
				*(unsigned long *)&resbuf[curposbuf+24]=(unsigned long)GetNumber(0);
				break;
			case v_ft:
				nexttok();
				*(unsigned long *)&resbuf[curposbuf+28]=(unsigned long)GetNumber(0);
				break;
			case v_fs:
				nexttok();
				*(unsigned long *)&resbuf[curposbuf+32]=(unsigned long)GetNumber(0);
				break;
			default:
				badFormat("VERSIONINFO");
				break;
		}
	}
	curposbuf+=44;
	GetBlockInfo();
	*(unsigned short *)&resbuf[0]=(unsigned short)curposbuf;
	curtres->res=(unsigned char *)REALLOC(resbuf,curposbuf);
	curtres->size=curposbuf;
}

void r_Dialog()
{
	std::string Name;
	std::string Font;
int sizefont=0,i;
NameOrdinal Menu;
NameOrdinal Class;
unsigned int poscount;	//позиция счетчика элементов
unsigned int exts=FALSE;
//unsigned short id;
	Menu.name=NULL;
	Class.name=NULL;
	if(restok==rc_dialogex)exts=TRUE;
	GetResBlock();
	curtres->type=CRT_DIALOG;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	addType(CRT_DIALOG);
	InitBufRes();
	if(exts){
		*(unsigned long *)&resbuf[0]=0xFFFF0001;
		curposbuf=8;
		poscount=16;
		*(unsigned long *)&resbuf[12]=0x80880000;//WS_POPUP|WS_BORDER|WS_SYSMENU;	//установки по умолчанию
	}
	else{
		*(unsigned long *)&resbuf[0]=0x80880000;//WS_POPUP|WS_BORDER|WS_SYSMENU;	//установки по умолчанию
		curposbuf=0;
		poscount=8;
	}
	if(tok2!=tk_camma){	//пропускаем возможные первые два параметра
		nexttok();
		if(tok2!=tk_camma){
			nexttok();
			if(tok2!=tk_camma)badFormat("DIALOG");
		}
	}
	GetRectangle(&resbuf[curposbuf+10],1);
	//определить место для IDHelp
	if(tok!=tk_endline&&exts)/**(unsigned long *)&resbuf[curposbuf]=*/GetNumber(0);
	while(!OpenBlock()&&tok!=tk_eof){
		if(tok!=tk_rescommand)expectedrescommand();
		switch(itok.number){
			case rc_style:
				nexttok();
				*(unsigned long *)&resbuf[exts==TRUE?12:0]=GetNumber(1);
				break;
			case rc_caption:
				nexttok();
				if(tok!=tk_string)stringexpected();
				Name = std::string((char *) String);
				nexttok();
				break;
			case rc_font:
				nexttok();
				sizefont=GetNumber(1);
				expecting(tk_camma);
				if(tok!=tk_string)stringexpected();
				Font = std::string((char *) String);
				nexttok();
				break;
			case rc_class:
				nexttok();
				GetOrdinal(&Class.name,1);
//				if(tok!=tk_string)stringexpected();
//				Class.name=(unsigned char *)BackString((char *)string);
//				nexttok();
				break;
			case rc_exstyle:
				nexttok();
				*(unsigned long *)&resbuf[exts==TRUE?8:4]=GetNumber(1);
				break;
			case rc_language:
				unsigned short olang;
				olang=langdef;
				nexttok();
				r_Language();
				curtres->lang=langdef;
				langdef=olang;
				break;
			default:
				if(exts&&itok.number==rc_menu){
					nexttok();
					GetOrdinal(&Menu.name,1);
				}
				else badFormat("DIALOG");
				break;
		}
		while(tok==tk_endline)nexttok();
	}
//доформировываем диалог
	curposbuf=exts==TRUE?26:18;
	addNumOrd(Menu.name);
	FreeOrdinal(Menu.name);
	addNumOrd(Class.name);
	FreeOrdinal(Class.name);
	addWString(Name.c_str());
	if(sizefont){
		resbuf[exts==TRUE?12:0]|=0x40;
		*(unsigned short *)&resbuf[curposbuf]=(unsigned short)sizefont;
		curposbuf+=2;
		if(exts){
				*(unsigned int *)&resbuf[curposbuf]=0x01000000;
				curposbuf+=4;
		}
		addWString(Font.c_str());
	}
	while(!CloseBlock()&&tok!=tk_eof){
//	do{
		if(tok!=tk_rescommand)expectedrescommand();
		restok=itok.number;
		nexttok();
		curposbuf=Align(curposbuf,4);
		CheckResBuf(34);
		Name.clear();
		Menu.name=NULL;
		i=1;
		if(exts)curposbuf+=8;
		*(unsigned short *)&resbuf[curposbuf+(exts==TRUE?16:18)]=0XFFFF;
		*(unsigned short *)&resbuf[curposbuf+(exts==TRUE?18:20)]=defdialog[restok].dclass;
		*(unsigned long *)&resbuf[curposbuf]=defdialog[restok].style|0x50000000;
		switch(restok){
			case rc_control:
				GetOrdinal(&Menu.name,1);
				expecting(tk_camma);
				while(tok==tk_endline)nexttok();
				if(exts)*(unsigned int *)&resbuf[curposbuf+12]=(unsigned int)GetNumber(2);
				else *(unsigned short *)&resbuf[curposbuf+16]=(unsigned short)GetNumber(2);
				expecting(tk_camma);
				while(tok==tk_endline)nexttok();
				if(tok==tk_number)*(unsigned short *)&resbuf[curposbuf+(exts==TRUE?18:20)]=(unsigned short)itok.number;
				else{
					if(tok==tk_string)strncpy(itok.name,(char *)String,IDLENGTH);
					i=GetFlag((_STRINGS_ *)&typeclass,6);
					if (!i)
						Name = std::string((char *) String);
					else *(unsigned short *)&resbuf[curposbuf+(exts==TRUE?18:20)]=(unsigned short)i;
				}
				nextexpecting2(tk_camma);
				while(tok==tk_endline)nexttok();
				*(unsigned long *)&resbuf[curposbuf]|=GetNumber(4);
				expecting(tk_camma);
				while(tok==tk_endline)nexttok();
				GetRectangle(&resbuf[curposbuf+(exts==TRUE?4:8)],5);
				if(exts&&tok==tk_number)*(unsigned long *)&resbuf[curposbuf-4]=GetNumber(9);
				while(tok!=tk_endline&&tok!=tk_eof&&tok!=tk_rescommand)nexttok();	//пропуск излишних параметров
				break;
			case rc_auto3state:
			case rc_autocheckbox:
			case rc_autoradiobutton:
			case rc_checkbox:
			case rc_ctext:
			case rc_defpushbutton:
			case rc_groupbox:
			case rc_ltext:
			case rc_pushbox:
			case rc_pushbutton:
			case rc_radiobutton:
			case rc_rtext:
			case rc_state3:
				if(tok!=tk_string)stringexpected();
				Menu.name=(unsigned char *) strdup((char *) String);
				nexttok();
				if(tok==tk_camma)nexttok();
				while(tok==tk_endline&&tok!=tk_eof)nexttok();
				i++;
			case rc_combobox:
			case rc_edittext:
			case rc_listbox:
			case rc_scrollbar:
				if(exts)*(unsigned int *)&resbuf[curposbuf+12]=(unsigned int)GetNumber(i);
				else *(unsigned short *)&resbuf[curposbuf+16]=(unsigned short)GetNumber(i);
				expecting(tk_camma);
				while(tok==tk_endline)nexttok();
				GetRectangle(&resbuf[curposbuf+(exts==TRUE?4:8)],i+1);
				if(tok!=tk_endline){
					*(unsigned long *)&resbuf[curposbuf]|=(unsigned long)GetNumber(i+5);
					if(tok==tk_camma){
						nexttok();
						*(unsigned long *)&resbuf[curposbuf+(exts==TRUE?-4:4)]=(unsigned long)GetNumber(i+6);
					}
				}
				if(exts&&tok==tk_number)*(unsigned long *)&resbuf[curposbuf-4]=GetNumber(i+9);
				while(tok!=tk_endline&&tok!=tk_eof&&tok!=tk_rescommand)nexttok();	//пропуск излишних параметров
				break;
			case rc_icon:
				GetOrdinal(&Menu.name,1);
				expecting(tk_camma);
				while(tok==tk_endline)nexttok();
				if(exts)*(unsigned int *)&resbuf[curposbuf+12]=(unsigned int)GetNumber(2);
				else *(unsigned short *)&resbuf[curposbuf+16]=(unsigned short)GetNumber(2);
				expecting(tk_camma);
				while(tok==tk_endline)nexttok();
				*(unsigned short *)&resbuf[curposbuf+(exts==TRUE?4:8)]=(unsigned short)GetNumber(3);
				expecting(tk_camma);
				while(tok==tk_endline)nexttok();
				*(unsigned short *)&resbuf[curposbuf+(exts==TRUE?6:10)]=(unsigned short)GetNumber(4);
				if(tok==tk_camma){
					nexttok();
					*(unsigned short *)&resbuf[curposbuf+(exts==TRUE?8:12)]=(unsigned short)GetNumber(5);
					if(tok==tk_camma){
						nexttok();
						*(unsigned short *)&resbuf[curposbuf+(exts==TRUE?10:14)]=(unsigned short)GetNumber(6);
						if(tok==tk_camma){
							nexttok();
							*(unsigned long *)&resbuf[curposbuf]|=GetNumber(7);
						}
					}
				}
				break;
			default:
				badFormat("DIALOG");
				break;
		}
		while(tok==tk_endline)nexttok();
		*(unsigned short *)&resbuf[poscount]=(unsigned short)(*(unsigned short *)&resbuf[poscount]+1);
		curposbuf+=(exts==TRUE?20:22);
		if(!Name.empty()){
			curposbuf-=4;
			addWString(Name.c_str());
		}
		if(Menu.name){
			addNumOrd(Menu.name);
			FreeOrdinal(Menu.name);
		}
		else curposbuf+=2;
		CheckResBuf(6);
		curposbuf+=2;
//	}while(!CloseBlock()&&tok!=tk_eof);
	}
	curtres->res=(unsigned char *)REALLOC(resbuf,curposbuf);
	curtres->size=curposbuf;
}

void GetFileName(char *name)
{
	while(tok!=tk_string&&tok!=tk_endline&&tok!=tk_eof){
		int i;
		for(i=0;i<7;i++){
			if (boost::iequals(itok.name, typemem[i].id))
				break;
		}
		if(i==7){
			strcpy(name,itok.name);
			i=strlen(itok.name);
			name[i++]=cha2;
			for(;;){
				cha2=input[inptr2++];
				if(cha2<=0x20)break;
				name[i++]=cha2;
			}
			name[i]=0;
			break;
		}
		nexttok();
	}
}

unsigned char *loadFileBin(const fs::path &Filename)
{
int inico;
unsigned char *bitobr;
	if((inico=open(Filename.c_str(),O_BINARY|O_RDONLY))==-1){
		badInputFile(Filename);
		return NULL;
	}
	boost::system::error_code ec;
	curposbuf = fs::file_size(Filename, ec);
	if (curposbuf == 0 || ec) {
		badInputFile(Filename);
		close(inico);
		return NULL;
	}
	bitobr=(unsigned char *)MALLOC(curposbuf);
	if((unsigned int)read(inico,bitobr,curposbuf)!=curposbuf){
		errorReadingFile(Filename);
		close(inico);
		free(bitobr);
		return NULL;
	}
	close(inico);
	nexttok();
	return bitobr;
}

unsigned char *LoadBitmap()
{
//загрузить
unsigned char *bitobr=NULL;
char name[80];
	curposbuf=0;
	name[0]=0;
	GetFileName(name);
	if(name[0]!=0)bitobr= loadFileBin(name);
	else if(tok==tk_endline){	//нет имени файла
		InitBufRes();
		if(!OpenBlock()){
			badico();
			return NULL;
		}
		do{
			inptr=inptr2;
			cha=cha2;
			if(tok!=tk_singlquote)badico();
			whitespace(); //пропуск незначащих символов
			CheckResBuf(16);
			displaytokerrors=1;
			do{
				unsigned char hold=0;
				for(int i=0;i<2;i++){
					hold*=(unsigned char)16;
					if(isdigit(cha))hold+=(unsigned char)(cha-'0');
					else if(isxdigit(cha))hold+=(unsigned char)((cha&0x5f)-'7');
					else expectedError("hexadecimal digit"s);
					nextchar();
				}
				resbuf[curposbuf++]=hold;
				whitespace(); //пропуск незначащих символов
			}while(cha!='\''&&cha!=26);
			inptr2=inptr;
			cha2=cha;
			linenum2=linenumber;
			nexttok();
			nexttok();
		}while(!CloseBlock()&&tok!=tk_eof);
		bitobr=(unsigned char *)REALLOC(resbuf,curposbuf);
	}
	else if(tok==tk_string)bitobr= loadFileBin((char *) String);
	return bitobr;
}

void r_Bitmap()
{
unsigned int size;
unsigned char *bitobr;
	if((bitobr=LoadBitmap())==NULL)return;
	size=curposbuf-14;
	GetResBlock();	//битмар
	curtres->type=CRT_BITMAP;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	curtres->size=size;
	curtres->res=(unsigned char *)MALLOC(size);
	addType(CRT_BITMAP);
	memcpy(curtres->res,bitobr+14,size);
	free(bitobr);
}

void newResource()
{
unsigned char *bitobr=NULL;
char name[256];
	GetResBlock();
	if (ResName.empty())
		curtres->type = restok;
	else
		curtres->TName = ResName;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	addType(restok, curtres->TName);
	name[0]=0;
	GetFileName(name);
	if(name[0]!=0)bitobr= loadFileBin(name);
	else if(tok==tk_string)bitobr= loadFileBin((char *) String);
	else stringexpected();
	if(bitobr!=NULL){
		curtres->res=bitobr;
		curtres->size=curposbuf;
	}
	nexttok();
}

void r_Font()
{
unsigned char *fontobr;
	if((fontobr=LoadBitmap())==NULL)return;
	if((unsigned short)curposbuf==*(unsigned short *)&fontobr[2]){
		GetResBlock();	//фонт
		curtres->type=CRT_FONT;
		if (IDName.empty())
			curtres->id = idnum;
		else
			curtres->Name = IDName;
		curtres->size=curposbuf;
		curtres->res=fontobr;
		addType(CRT_FONT);
	}
}

void r_Icon()
{
unsigned char *icoobr;
unsigned long size;
//загрузить иконку
	if((icoobr=LoadBitmap())==NULL)return;
	GetResBlock();	//группа икон
	curtres->type=CRT_GROUP_ICON;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	addType(CRT_GROUP_ICON);
unsigned int countico=*(unsigned short *)&icoobr[4];	//число иконок
int sizeicohead=sizeof(_ICOHEAD_)+(sizeof(_RESDIR_)*countico);
	curtres->size=sizeicohead;
	curtres->res=(unsigned char *)MALLOC(sizeicohead);
unsigned char *icohead=curtres->res;
unsigned int i;
	for(i=0;i<6;i++)icohead[i]=icoobr[i];	//заголовок
unsigned int ofs=6;
unsigned int ofs2=6;
	for(i=0;i<countico;i++){
		int j;
		for(j=0;j<12;j++)icohead[j+ofs]=icoobr[j+ofs2];	//описание иконки
		iconcount++;
		*(unsigned short *)&icohead[ofs+12]=(unsigned short)iconcount;	//ее номер
		GetResBlock();	//образ иконки
		curtres->type=CRT_ICON;
		curtres->id=iconcount;
		curtres->size=size=*(unsigned long *)&icohead[ofs+8];
		curtres->res=(unsigned char *)MALLOC(size);
		addType(CRT_ICON);
		unsigned int ofs3=*(unsigned int *)&icoobr[ofs2+12];
		for(j=0;(unsigned int)j<size;j++)curtres->res[j]=icoobr[j+ofs3];
		ofs+=sizeof(_RESDIR_);
		ofs2+=sizeof(_RESDIR_)+2;
	}
	free(icoobr);
}

void r_Cursor()
{
unsigned char *curobr;
unsigned long size;
//загрузить курсор
	if((curobr=LoadBitmap())==NULL)return;
	GetResBlock();	//группа курсоров
	curtres->type=CRT_GROUP_CURSOR;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	addType(CRT_GROUP_CURSOR);
unsigned int countcur=*(unsigned short *)&curobr[4];	//число курсоров в файле
int sizecurhead=sizeof(_ICOHEAD_)+(sizeof(_CURDIR_)*countcur);
	curtres->size=sizecurhead;
	curtres->res=(unsigned char *)MALLOC(sizecurhead);
unsigned char *curhead=curtres->res;
unsigned int i;
	for(i=0;i<6;i++)curhead[i]=curobr[i];	//заголовок
unsigned int ofs=6;
unsigned int ofs2=6;
	for(i=0;i<countcur;i++){
		cursorcount++;
		*(unsigned short *)&curhead[ofs]=curobr[ofs2];
		*(unsigned short *)&curhead[ofs+2]=curobr[ofs2+1];
		*(unsigned long *)&curhead[ofs+4]=0x10001;
		*(unsigned short *)&curhead[ofs+12]=(unsigned short)cursorcount;	//ее номер
		GetResBlock();	//образ курсора
		curtres->type=CRT_CURSOR;
		curtres->id=cursorcount;
		curtres->size=size=*(unsigned long *)&curhead[ofs+8]=*(unsigned long *)&curobr[ofs2+8]+4;
		curtres->res=(unsigned char *)MALLOC(size);
		addType(CRT_CURSOR);
		unsigned int ofs3=*(unsigned int *)&curobr[ofs2+12];
		*(unsigned short *)&curtres->res[0]=*(unsigned short *)&curobr[ofs2+4];
		*(unsigned short *)&curtres->res[2]=*(unsigned short *)&curobr[ofs2+6];
		size-=4;
		for(int j=0;(unsigned int)j<size;j++)curtres->res[j+4]=curobr[j+ofs3];
		ofs+=sizeof(_CURDIR_);
		ofs2+=sizeof(_CURDIR_)+2;
	}
	free(curobr);
}

void r_Accelerators()
{
	GetResBlock();
	curtres->type=CRT_ACCELERATOR;
	if (IDName.empty())
		curtres->id = idnum;
	else
		curtres->Name = IDName;
	addType(CRT_ACCELERATOR);
	InitBufRes();
	if(OpenBlock()){
		do{
			CheckResBuf(8);
			if(tok==tk_string){
				unsigned char c;
				c=String[0];
				if(c=='^'){
					c=String[1];
					if(c<0x40)prError("Unsolved symbol for Contrl");//error
					c-=0x40;
				}
				*(unsigned short *)&resbuf[curposbuf+2]=(unsigned short)c;
				nexttok();
			}
			else{
				*(unsigned short *)&resbuf[curposbuf+2]=(unsigned short)GetNumber(1);
			}
			expecting(tk_camma);
			*(unsigned short *)&resbuf[curposbuf+4]=(unsigned short)GetNumber(2);
			if(tok==tk_camma){
				nexttok();
				if(tok==tk_number)*(unsigned short *)&resbuf[curposbuf]|=(unsigned short)itok.number;
				*(unsigned short *)&resbuf[curposbuf]|=GetFlag((_STRINGS_ *)&typeacceler,5);//GetAccerFlag();
				nexttok();
				while(tok!=tk_endline){
					if(tok==tk_camma)nexttok();
					if(tok==tk_number)*(unsigned short *)&resbuf[curposbuf]|=(unsigned short)itok.number;
					*(unsigned short *)&resbuf[curposbuf]|=GetFlag((_STRINGS_ *)&typeacceler,5);//GetAccerFlag();
					nexttok();
				}
			}
			curposbuf+=8;
		}while(!CloseBlock()&&tok!=tk_eof);
		resbuf[curposbuf-8]|=0x80;
	}
	else badFormat("ACCELERATORS");
	curtres->res=(unsigned char *)REALLOC(resbuf,curposbuf);
	curtres->size=curposbuf;
}

void SortRes()
{
int i,j,k;
int sortpos=0;	//позиция в списке сортировки
	for(i=0;i<numtyperes;i++){
		for(j=i+1;j<numtyperes;j++){
			TUSE buf;
			if((tuse+i)->id>(tuse+j)->id){
				buf.id=(tuse+i)->id;
				buf.count=(tuse+i)->count;
				buf.TName=(tuse+i)->TName;
				(tuse+i)->id=(tuse+j)->id;
				(tuse+i)->count=(tuse+j)->count;
				(tuse+i)->TName=(tuse+j)->TName;
				(tuse+j)->count=buf.count;
				(tuse+j)->id=buf.id;
				(tuse+j)->TName=buf.TName;
			}
			if((tuse+i)->id==(tuse+j)->id){
				if (boost::algorithm::to_lower_copy((tuse + i)->TName) >
					boost::algorithm::to_lower_copy((tuse + j)->TName)) {
					buf.count=(tuse+i)->count;
					buf.TName=(tuse+i)->TName;
					(tuse+i)->count=(tuse+j)->count;
					(tuse+i)->TName=(tuse+j)->TName;
					(tuse+j)->count=buf.count;
					(tuse+j)->TName=buf.TName;
				}
			}
		}
		if((tuse+i)->id==0)numzerotype++;
	}

	sortidx=(unsigned short *)MALLOC(sizeof(unsigned short)*numres);
	for(i=0;i<numtyperes;i++){
		for(k=0,j=0;j<numres;j++){
			if((tuse+i)->id==(listres+j)->type){
				if((listres+j)->lang){
					numlangres++;
					numhlangres++;
				}
				if((tuse+i)->count==1){
					sortidx[sortpos++]=(unsigned short)j;
					break;
				}
				else{
					if(k==0)sortidx[sortpos++]=(unsigned short)j;
					else{
						int m=k;	//число элементов с данным типом
						int n=sortpos-k;
						do{
							m--;
							if ((listres + j)->Name.empty() &&
								(listres + sortidx[m + n])->Name.empty()) {
								if((listres+j)->id>=(listres+sortidx[m+n])->id){//новый больше
									if((listres+j)->id==(listres+sortidx[m+n])->id){	//равны
										numidres--;
										numlangres++;
										if((listres+j)->lang==0){
											numlangres++;
											numhlangres++;
										}
										if((listres+j)->lang==(listres+sortidx[m+n])->lang)equalres();
										if((listres+j)->lang>(listres+sortidx[m+n])->lang){	//у нового язык старше
											sortidx[n+m+1]=(unsigned short)j;	//добавить в конец
											break;
										}
									}
									else{
										sortidx[n+m+1]=(unsigned short)j;	//добавить в конец
										break;
									}
								}
								sortidx[n+m+1]=sortidx[n+m];	//сдвинуть
							} else if (!((listres + j)->Name.empty()) &&
									   !((listres + sortidx[m + n])->Name.empty())) {
								sortidx[n+m+1]=(unsigned short)j;
								break;
							} else if (!((listres + j)->Name.empty())
									   && (listres + sortidx[m + n])->Name.empty()) {
								sortidx[n+m+1]=sortidx[n+m];
							}
							else{
								int cmp;
								if ((cmp = strcmp((listres + j)->Name.c_str(),
												  (listres + sortidx[m + n])->Name.c_str())) >= 0) {
									if(cmp==0){
										numidres--;
										numlangres++;
										if((listres+j)->lang==0){
											numlangres++;
											numhlangres++;
										}
//										numhlangres--;
										if((listres+j)->lang==(listres+sortidx[m+n])->lang)equalres();
										if((listres+j)->lang>(listres+sortidx[m+n])->lang){
											sortidx[n+m+1]=(unsigned short)j;
											break;
										}
									}
									else{
										sortidx[n+m+1]=(unsigned short)j;
										break;
									}
								}
								sortidx[n+m+1]=sortidx[n+m];
							}
							if(m==0)sortidx[sortpos-k]=(unsigned short)j;
						}while(m>0);
						sortpos++;
					}
				}
				k++;
			}
		}
	}
/*
	for(i=0;i<numres;i++){
		printf("type=%-5uid=%-10u type=%-5uid=%u\n",(listres+sortidx[i])->type,(listres+sortidx[i])->id,
			(listres+i)->type,(listres+i)->id);
	}
	*/
}


int MakeRes(unsigned long ofsres,LISTRELOC **listrel)
{
int i,j;
unsigned long nextofs;
unsigned int startlang,ofsback;
unsigned int startofsdata,startdata;
int numrel=0;
RES *curres;
LISTRELOC *listr=NULL;
	linenumber=0;
	InitBufRes();
	numidres=numres;
	SortRes();
//создать корневой уровень
	*(unsigned short *)&resbuf[12]=(unsigned short)numzerotype;
	*(unsigned short *)&resbuf[14]=(unsigned short)(numtyperes-numzerotype);
	curposbuf=16;
	nextofs=numtyperes*8+16;
//расчет размеров уровней
	startlang=curposbuf+numtyperes*24+numidres*8;
	startofsdata=startlang+numhlangres*16+numlangres*8;
	startdata=startofsdata+numres*16;
	CheckResBuf(startdata-curposbuf);
	for(i=0;i<numtyperes;i++){
		if((tuse+i)->id==0){
			*(unsigned long *)&resbuf[curposbuf]=startdata|0x80000000;
			curres=listres+sortidx[i];
			int len = curres->TName.size();
			CheckResBuf(startdata-curposbuf+len*2+4);
			*(unsigned short *)&resbuf[startdata]=(unsigned short)len;
			startdata+=2;
			for (auto c : curres->TName) {
				resbuf[startdata]=c;
				startdata+=2;
			}
		}
		else *(unsigned long *)&resbuf[curposbuf]=(tuse+i)->id;
		*(unsigned long *)&resbuf[curposbuf+4]=nextofs|0x80000000;
		nextofs+=(tuse+i)->count*8+16;
		curposbuf+=8;
	}
//расчет размеров уровней
/*	startlang=curposbuf+numtyperes*16+numidres*8;
	startofsdata=startlang+numhlangres*16+numlangres*8;
	startdata=startofsdata+numres*16;
	CheckResBuf(startdata-curposbuf);*/
//создать уровень имен и языка
	curres=listres+sortidx[0];
	for(j=0;j<numres;){
		ofsback=curposbuf+12;
		curposbuf+=16;
		unsigned int type=curres->type;
		while((unsigned int)curres->type==type){
			int k=j;
			if (!curres->Name.empty()) {    //добавить имя
				if (j < (numres - 1) && type == (unsigned int) (listres + sortidx[j + 1])->type &&
					(!(listres + sortidx[j + 1])->Name.empty()))
					while (curres->Name == (listres + sortidx[j + 1])->Name)
						j++;
				*(unsigned short *)&resbuf[ofsback]=(unsigned short)(*(unsigned short *)&resbuf[ofsback]+1);
				*(unsigned long *)&resbuf[curposbuf]=startdata|0x80000000;
				int len = curres->Name.size();
				CheckResBuf(startdata-curposbuf+len*2+4);
				*(unsigned short *)&resbuf[startdata]=(unsigned short)len;
				len=0;
				startdata+=2;
				for(auto c : curres->Name){
					resbuf[startdata]=c;
					startdata+=2;
				}
//				startdata=Align(startdata,4);
			}
			else{	//добавить id
				if(j<(numres-1)&&type==(unsigned int)(listres+sortidx[j+1])->type)
					while(curres->id==(listres+sortidx[j+1])->id)j++;
				*(unsigned short *)&resbuf[ofsback+2]=(unsigned short)(*(unsigned short *)&resbuf[ofsback+2]+1);
				*(unsigned long *)&resbuf[curposbuf]=curres->id;
			}
			curposbuf+=4;
			if(j!=k){	//несколько имен с разными языками
				*(unsigned long *)&resbuf[curposbuf]=startlang|0x80000000;
				*(unsigned long *)&resbuf[startlang+12]=j-k+1;
				startlang+=16;
				for(;k<=j;k++){
					*(unsigned long *)&resbuf[startlang]=(listres+sortidx[k])->lang;
					*(unsigned long *)&resbuf[startlang+4]=startofsdata;
					startlang+=8;
					startofsdata+=16;
				}
			}
			else{
				if(curres->lang){//указан язык
					*(unsigned long *)&resbuf[curposbuf]=startlang|0x80000000;
					resbuf[startlang+14]=1;
					startlang+=16;
					*(unsigned long *)&resbuf[startlang]=curres->lang;
					*(unsigned long *)&resbuf[startlang+4]=startofsdata;
					startlang+=8;
				}
				else *(unsigned long *)&resbuf[curposbuf]=startofsdata;
				startofsdata+=16;
			}
			curposbuf+=4;
			j++;
			if(j==numres)break;
			curres=listres+sortidx[j];
		}
	}
	curposbuf=startlang;
//создать уровень смещений данных
	for(i=0;i<numres;i++){
		startdata=Align(startdata,4);
		curres=listres+sortidx[i];
		*(unsigned long *)&resbuf[curposbuf]=startdata+ofsres;
		if(!numrel)listr=(LISTRELOC *)MALLOC(sizeof(LISTRELOC));
		else listr=(LISTRELOC *)REALLOC(listr,(numrel+1)*sizeof(LISTRELOC));
		(listr+numrel)->val=curposbuf;
		numrel++;
		curposbuf+=4;
		*(unsigned long *)&resbuf[curposbuf]=curres->size;
		curposbuf+=12;
		CheckResBuf(startdata-curposbuf+curres->size);
		memcpy(resbuf+startdata,curres->res,curres->size);
		free(curres->res);
		startdata+=curres->size;
	}
	curposbuf=startdata;
	nextofs=Align(curposbuf,FILEALIGN);
	CheckResBuf(nextofs-curposbuf);
//	SaveFile("resorse");
	*listrel=listr;
	return numrel;
}
