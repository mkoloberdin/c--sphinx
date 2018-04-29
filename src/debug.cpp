#include <vector>
#include <string>
using namespace std::string_literals;
#include <cstdint>

#include "tok.h"

#ifdef _WIN32
#include <io.h>
#endif

#define _DEBUG_

#define MAXDBGS 1000
#define MAXNUMSYM 500
#define MAXLSTSTR 128	//максимальный размер строки листинга

void addNameToTable(const std::string &Name);
void AddSymbolList(struct idrec *ptr);
int createDosDebug(fs::ofstream &OFS);
int createW32Debug(fs::ofstream &OFS);
void GeneratLst();

std::vector<unsigned int> DbgLoc;            //адресс точки
std::vector<unsigned int> DbgLineNum;        //номер строки
std::vector<unsigned short> DbgModuleNum;    //номер файла
std::vector<std::string> Listing;            //строки исходного текста
std::vector<unsigned char> LstFlags;		 //флаги управления листингом
std::vector<unsigned int> LstEnd;

unsigned int pdbg=0;      /* number of post entrys */
unsigned int pdbgmax=MAXDBGS;      /* max number of post entrys */
static unsigned int oline=0,omodule=0xFFFFFFFF;
unsigned int ooutptr=0xffffffff;
char lsttypedata=0;	//тип данных для листинга, по умолчанию код
unsigned int outputcodestart=0;

struct D16START{
	unsigned long sign;      //0 Signify &	Version
	unsigned long pol_size;  //4 Name Pool Size (in bytes)
	unsigned long numname;   //8 Number of Names
	unsigned long numtentr;  //12 Number of Type Entries
	unsigned long nummentr;  //16 Number of Member Entries
	unsigned long numsymbl;  //20 Number of Symbols
	unsigned long numgsymb;  //24 Number of Global Symbols
	unsigned long numsours;  //28 Number of Source Modules
	unsigned long numlsumb;  //32 Number of Local Symbols
	unsigned long numscop;   //36 Number of Scopes
	unsigned long numline;   //40 Number of Line Number Entries
	unsigned long numincl;   //44 Number of Include Files
	unsigned long numseg;    //48 Number of Segments
	unsigned long numcorrel; //52 Number of Correlation Entries
	unsigned long imagesize; //56 Image Size
	unsigned short data[2];  //60 Basic RTL String Segment Offset & Data Count
	unsigned long casesensiv;//64 Case Sensivitive Link
	unsigned long ucnovn;
//только для 128-байтового заголовка
	unsigned long ucnovn1[6];
	unsigned long sizeblock;	//размер между началом имен и концом заголовка
	unsigned long ucnovn2[2];
	unsigned short fdebug;	//Debug Flags
	unsigned long reftsize;	//Reference Information Table Size (in bytes)
	unsigned long numnamesp;	//Number of Namespace Entries
	unsigned long numunamesp;	//Number of Namespace Using Entries
	unsigned long ucnovn3;
	unsigned short ucnovn4;
////////////////////////////////
};

struct MODULE{
	unsigned long name;
	unsigned char language;
	unsigned char memmodel;
	unsigned long symindex;
	unsigned short symcount;
	unsigned short sourindex;
	unsigned short sourcount;
	unsigned short corindex;
	unsigned short corcount;
};

typedef struct _SFT_
{
	unsigned long idx;
	struct ftime time;
}SFT;

typedef struct _LT_
{
	unsigned short line;
	unsigned short ofs;
}LT;

typedef struct _CT_
{
	unsigned short segidx;
	unsigned short filidx;
	unsigned long beg;
	unsigned short count;
}CT;

static struct _SMB_
{
	unsigned long idxname;
	unsigned long type;
	unsigned short ofs;
	unsigned short seg;
	unsigned short clas;
}*symbols;

static struct SEGMENT{
	unsigned short idx;
	unsigned short segm;
	unsigned short ofs;
	unsigned short size;
	unsigned short scopei;
	unsigned short scopec;
	unsigned short correli;
	unsigned short correlc;
}*segment;

unsigned char dbg=FALSE,dbgact;
static short numsymbols=0;
static int sizetabsym=MAXNUMSYM;
static unsigned short numcorrel;
static unsigned char *bufname;
static int lastofspul=0;
static int segcode;

static struct _COR_INFO_
{
	unsigned int ofs;	//начало блока
	unsigned int end; //конец блока
	unsigned int startline;	//номер первой строки информации
	unsigned short count;	//число строк
	unsigned short file;	//файл
}*corinfo=NULL;

void InitDbg()
{
	if(fobj)dbg&=0xFE;
	if (dbg && DbgLoc.empty()) {
		dbgact=1;	//запретить сбор информации
	}
}

void AddDataLine(char ssize/*,char typev*/)
{
/*	if(typev==pointer||typev==farpointer)ssize=(typev==pointer?(char)2:(char)4);
	else*/ //if(tok==tk_string)ssize=(char)3;
	lsttypedata=(unsigned char)(ssize<<1);
	addLine();
	lsttypedata=0;
}

void addDataNullLine(char ssize, const std::string &Name)
{
	oline--;

	lsttypedata=(unsigned char)(ssize<<1);
	addLine(TRUE);
	lsttypedata=0;
	DbgLineNum[pdbg-1]=0;	//очистить номер строки
//new !!!
	if (!Name.empty())
		Listing[pdbg - 1] = Name;
}

void addCodeNullLine(const std::string &Name)
{
	oline--;
	addLine(TRUE);
	DbgLineNum[pdbg-1]=0;	//очистить номер строки
	if (!Name.empty())
		Listing[pdbg - 1] = Name;
	oline--;
}

void addEndLine()
{
	if (pdbg/*&&lstend[pdbg-1]==0*/)
		LstEnd.emplace_back(outptr);
}

void addLine(int SkipLineInfo)
{
	if(/*ooutptr!=outptr&&*/dbgact==0&&(oline!=linenumber||omodule!=CurrentFileInfoNum)){
		while (ooutptr > outptr && pdbg != 0 && ooutptr != 0xffffffff)
			KillLastLine();
		ooutptr = outptr;
		DbgLoc.emplace_back(ooutptr);
		oline=linenumber;
		DbgLineNum.emplace_back(oline);
		DbgModuleNum.emplace_back((unsigned short)CurrentFileInfoNum);
		omodule=CurrentFileInfoNum;
		if (dbg & 2) {
			if (SkipLineInfo)Listing.emplace_back(""s);
			else {
				const char *ofs = startline;
				char c;
				int sizestring=0;
				char buf[MAXLSTSTR];
				for(;sizestring<(MAXLSTSTR-1);sizestring++){
					c=*ofs;
					ofs=ofs+1;
					if(c==13||ofs==endinput)break;
					buf[sizestring]=c;
				}
				buf[sizestring]=0;
				strbtrim(buf);
				if ((sizestring = strlen(buf)) == 0)Listing.emplace_back(""s);
				else {
					Listing.emplace_back(buf);
				}
			}
			LstFlags.emplace_back(am32 | lsttypedata);
			addEndLine();
//		printf("%s(%d) outptr=%d\n",(startfileinfo+currentfileinfo)->filename,linenumber,outptr);
		}
		pdbg++;
		FilesInfo[CurrentFileInfoNum].numdline++;
	}
}

void KillLastLine()
{
//	printf("dbgact=%d pdbg=%d outptr=%08X ooutptr=%08X\n",dbgact,pdbg,outptr,ooutptr);
	if(outptr==0&&ooutptr==0x100){
		ooutptr=0;
		return;
	}
	if(dbgact==0&&pdbg!=0){
//		printf("pdbg=%d dbgmod=%d currentfileinfo=%d\n",pdbg,dbgmod[0],currentfileinfo);
		if(pdbg==1&&DbgModuleNum[0]!=(unsigned short)CurrentFileInfoNum)return;
		pdbg--;
		if(pdbg==0){
			oline=0;
			omodule=ooutptr=0xFFFFFFFF;
		}
		else{
			oline=DbgLineNum[pdbg];
			omodule=DbgModuleNum[pdbg];
			ooutptr=DbgLoc[pdbg];
		}
//		printf("%s(%d) pdbg=%d oline=%d ooutptr=%d\n",(startfileinfo+currentfileinfo)->filename,linenumber,pdbg,oline,ooutptr);
		FilesInfo[DbgModuleNum[pdbg]].numdline--;
	}
}

//создание отладочного файла
void DoTDS()
{
int retcode;
unsigned int i,j;
//создать файл
	if(!LstFlags.empty())
		GeneratLst();
	if(dbg&1){
//убрать из списка файлов не используемые
		for(i=0;i<totalmodule;i++){
			if(FilesInfo[i].numdline==0){
				totalmodule--;
				if(totalmodule!=i){
					FilesInfo[i] = FilesInfo[totalmodule];
//корректировка таблиц строк
					for(j=0;j<pdbg;j++){
						if(DbgModuleNum[j]==(unsigned short)totalmodule)
							DbgModuleNum[j]=(unsigned short)i;
					}
					i--;
				}
			}
		}
//создать таблицу корреляций
		corinfo=(struct _COR_INFO_ *)MALLOC(sizeof(_COR_INFO_));
		corinfo->ofs=DbgLoc[0];
		corinfo->startline=0;
		omodule=corinfo->file=DbgModuleNum[0];
		numcorrel=0;
		for(j=1;j<pdbg;j++){
			if((unsigned short)omodule!=DbgModuleNum[j]){
				(corinfo+numcorrel)->count=(unsigned short)(j-(corinfo+numcorrel)->startline);
				(corinfo+numcorrel)->end=DbgLoc[j]-1;
				numcorrel++;
				corinfo=(struct _COR_INFO_ *)REALLOC(corinfo,sizeof(_COR_INFO_)*(numcorrel+1));
				(corinfo+numcorrel)->ofs=DbgLoc[j];
				(corinfo+numcorrel)->startline=j;
				omodule=(corinfo+numcorrel)->file=DbgModuleNum[j];
			}
		}
		(corinfo+numcorrel)->count=(unsigned short)(pdbg-(corinfo+numcorrel)->startline);
		(corinfo+numcorrel)->end=DbgLoc[j-1]+1;
		numcorrel++;
        fs::path Filename = makeOutputFilename("tds");
        fs::ofstream OFS(Filename, std::ios_base::binary);
		dieIfNotOpen(Filename, OFS);
		if (am32)
			retcode = createW32Debug(OFS);
		else retcode = createDosDebug(OFS);
		OFS.write((const char *)output, outptr);
        if (retcode == 0 && OFS.bad())retcode = -1;
		if(retcode!=0)ErrWrite();
		OFS.close();
	}
}

void addNameToTable(const std::string &Name)
{
	size_t i=0;
	do {
		char c;
		c=Name[i++];
		op(c);
	} while (i < Name.size());
}

void AddSymbolList(struct idrec *ptr)
{
	if(ptr!=NULL){
		AddSymbolList(ptr->right);
		if((ptr->rectok==tk_proc&&ptr->recsegm>=NOT_DYNAMIC)||
			 (ptr->rectok==tk_interruptproc&&ptr->recsegm>=NOT_DYNAMIC)||
			 (ptr->rectok>=tk_bits&&ptr->rectok<=tk_doublevar&&modelmem!=SMALL)||
			 (ptr->rectok==tk_structvar&&modelmem!=SMALL)){
			addNameToTable(ptr->recid);
			(symbols+numsymbols)->idxname=numsymbols+1;
			(symbols+numsymbols)->ofs=(unsigned short)(ptr->recpost==0?
				(unsigned short)ptr->recnumber:
				(unsigned short)ptr->recnumber+ooutptr);
 			if(modelmem==TINY&&comfile==file_exe)(symbols+numsymbols)->ofs-=(unsigned short)0x100;
//			if(ptr->rectok==tk_proc)(symbols+numsymbols)->clas=0x18;
			numsymbols++;
			if(numsymbols==sizetabsym){
				symbols=(_SMB_ *)REALLOC(symbols,(sizetabsym+MAXNUMSYM)*sizeof(_SMB_));
				memset(&(symbols+sizetabsym)->idxname,0,sizeof(_SMB_)*MAXNUMSYM);
				sizetabsym+=MAXNUMSYM;
			}
		}
		AddSymbolList(ptr->left);
	}
}

void addNameToPul(const std::string &Name)
{
	static size_t sizebuf = 0;
	size_t i = Name.size();
	if ((lastofspul + i + 2) >= sizebuf) {
		sizebuf+=STRLEN;
		if(sizebuf==STRLEN)bufname=(unsigned char *)MALLOC(sizebuf);
		else bufname=(unsigned char *)REALLOC(bufname,sizebuf);
	}
	bufname[lastofspul++]=(unsigned char)i;
	strcpy((char *) (bufname + lastofspul), Name.c_str());
	lastofspul+=++i;
}

void AddGlobalName(struct idrec *ptr)
{
	if(ptr!=NULL){
		AddGlobalName(ptr->right);
		if((ptr->rectok==tk_proc&&ptr->recsegm>=NOT_DYNAMIC)||
			 (ptr->rectok>=tk_bits&&ptr->rectok<=tk_doublevar)||
			 (ptr->rectok==tk_structvar)){
			addNameToPul(ptr->recid);
			numsymbols++;
			if(ptr->rectok==tk_proc){
				outword(0x1c);	//size
				outdword(0x20);	//type
			}
			else{
				outword(0x18);
				outdword(0x21);
			}
			outword(0);
			outdword(0);
			outdword(numsymbols);	//name
			outdword(0);
			if(ptr->recpost==0){
				outdword(ptr->recnumber);	//offset
				outword(segcode);		//segm
			}
			else{
				outdword(ptr->recnumber+(wbss!=FALSE?0:ooutptr));
				outword(1);
			}
			if(ptr->rectok==tk_proc)outdword(0);
		}
		AddGlobalName(ptr->left);
	}
}

int createW32Debug(fs::ofstream &OFS)
{
int sstNames,sstDirectory;
int	startcode=outptr;
int sstGlobalSym;
int sstsrc;
unsigned int i,j,jj,ofs;
	for (; numsymbols < (short) totalmodule; numsymbols++)
		addNameToPul(FilesInfo[numsymbols].Filename.string());
	segcode=(wbss==FALSE?1:2);
	outptr=0;
	outdword(0x41304246); 	 // TDS - signature
	outdword(0);		// offset of Subsection Directory (fill later)
//sstModule subsection
	outdword(0);		// OvlNum=0 & LibIndex=0
	outword(segcode); 			 // SegCount
	outword(0x5643); 			 // CV-style
	outdword(1);		// Name
	for(i=0;i<4;i++)outdword(0);
//	outdword(0);	//Time
//	outdword(0);
//	outdword(0);
//	outdword(0);
	if(wbss){
		outword(0x0001); 			 // SegNumber
		outword(0); 			 // flag
		outdword(0);		// start
		outdword(postsize); // len
	}
	outword(segcode); 			 // SegNumber
	outword(0x0001); 			 // flag
	outdword(startptr);		// start
	outdword(startcode); // len
	sstsrc=outptr;
//sstSrcModule subsection
	outword((short)numcorrel);  //cFile - количество SRC-файлов(сегментов)
	outword((short)numcorrel);  // SegCount (see SegCount in sstModule
	ofs=14*numcorrel+4;
	for(i=0,jj=0;i<(unsigned int)numcorrel;i++){
		if(i!=0)jj=jj+((corinfo+i-1)->count+1)*6+22;
		outdword(ofs+jj);
	}
	for(i=0;i<(unsigned int)numcorrel;i++){
		outdword((corinfo+i)->ofs);
		outdword((corinfo+i)->end);
	}
	for(i=0;i<(unsigned int)numcorrel;i++)outword(segcode); // массив индексов сегментов
	for(i=0;i<(unsigned int)numcorrel;i++){
		outword(1); 	 // Segm#
		outdword((corinfo+i)->file+1);// File#
		outdword(outptr-sstsrc+12);
		outdword((corinfo+i)->ofs);
		outdword((corinfo+i)->end);
		outword(segcode); 	 //Segm#
		jj=(corinfo+i)->count;
		outword(jj+1); 	 // Lines count
		ofs=(corinfo+i)->startline;
		for(j=0;j<jj;j++)outdword(DbgLoc[j+ofs]);
		outdword((corinfo+i)->end);
		for(j=0;j<jj;j++)outword(DbgLineNum[j+ofs]);
		outword(0);
	}
//таблица глобальных символов
	sstGlobalSym=outptr;
	for(i=0;i<8;i++)outdword(0);
//	outdword(0);	//modindex
//	outdword(0);	//size correct later
//	outdword(0);
//	outdword(0);
//	outdword(0);
//	outdword(0);	//num others correct later
//	outdword(0);	//total correct later
//	outdword(0);	//SymHash,  AddrHash
	outdword(0x02100008);	//S_ENTRY32
	outdword(EntryPoint());
	outword(segcode);
	AddGlobalName(treestart);
	sstNames=outptr;
	outdword(numsymbols);
	outptr=sstGlobalSym+4;
	outdword(sstNames-sstGlobalSym-32);
	outptr+=12;
	outdword(numsymbols-totalmodule);
	outdword(numsymbols-totalmodule);
	outptr=4;
	sstDirectory=sstNames+4+lastofspul;
	outdword(sstDirectory);
	OFS.write((const char *) output, sstNames + 4);
	if (OFS.bad())
		return -1;
	OFS.write((const char *) bufname, lastofspul);
	if (OFS.bad())
		return -1;
	free(bufname);
// Subsection Directory
	outptr=0;
	outdword(0x0C0010);
	outdword(4);		// cDir - number of subsections
	outdword(0);
	outdword(0);
//sstModule
	outdword(0x10120);
	outdword(8);	//start
	outdword(sstsrc-8);		// size
// sstSrcModule
	outdword(0x10127);
	outdword(sstsrc);	//start
	outdword(sstNames-sstsrc);//size
//sstGlobalSym
	outdword(0x129);
	outdword(sstGlobalSym);	//start
	outdword(sstNames-sstGlobalSym);	//size
// sstNames
	outdword(0x130);
	outdword(sstNames);
	outdword(sstDirectory-sstGlobalSym);
	outdword(0x41304246); 	 // TDS - signature
	outdword(sstDirectory+outptr+4); 	 // TDS-len
	return 0;
}

int createDosDebug(fs::ofstream &OFS)
{
unsigned int i,j,count;
D16START d16header;
MODULE *module;
SFT *sft;
LT *lt;
CT *ct;
int corrnum=0,ii;
unsigned short beg,end;
	outptr=0;
//16-бит заголовок
	memset(&d16header,0,sizeof(D16START));
//таблица глобальных имен
	symbols=(_SMB_ *)MALLOC(sizeof(_SMB_)*MAXNUMSYM);
	memset(symbols,0,sizeof(_SMB_)*MAXNUMSYM);
	AddSymbolList(treestart);
	d16header.numname=totalmodule*2+numsymbols;
	d16header.numsymbl=d16header.numgsymb=numsymbols;

	d16header.sign=0x040352FB;	//sign & version
	d16header.imagesize=runfilesize;	//image size
	d16header.numsours=totalmodule;	//1
	d16header.numline=pdbg; //lines
	d16header.numincl=totalmodule;	//include files
	d16header.numseg=totalmodule;//totalmodule;	//1
	d16header.numcorrel=numcorrel;//totalmodule;	//Correlation
	d16header.casesensiv=1;
//	d16header.numtentr=NUMTYPES;
//только для 128-байтового заголовка
//	d16header.fdebug=1;

	d16header.ucnovn=0x380000;
	d16header.sizeblock=sizeof(MODULE)*totalmodule+sizeof(SFT)*totalmodule+
			sizeof(LT)*pdbg+sizeof(SEGMENT)*totalmodule+sizeof(CT)*numcorrel+
			6*totalmodule+/*NUMTYPES*12+*/sizeof(_SMB_)*numsymbols/*+numsymbols*4*/;
	d16header.reftsize=numsymbols*4;

//module table
	module=(struct MODULE *)MALLOC(sizeof(MODULE)*totalmodule);
	memset(module,0,sizeof(MODULE)*totalmodule);
//sourse file table
	sft=(SFT *)MALLOC(sizeof(SFT)*totalmodule);
//segment table
	segment=(struct SEGMENT *)MALLOC(sizeof(SEGMENT)*totalmodule);
	memset(segment,0,sizeof(SEGMENT)*totalmodule);
//correlation table
	ct=(CT *)MALLOC(sizeof(CT)*numcorrel);
	for(i=0;i<totalmodule;i++){
//имена модулей
		addNameToTable((FilesInfo[i]).Filename.string());
		addNameToTable((FilesInfo[i]).Filename.filename().string());
//таблица модулей
		(module+i)->name=i*2+2+numsymbols;
		(module+i)->language=1;
		(module+i)->memmodel=8;//modelmem==SMALL&&comfile==file_exe?9:8;
		(module+i)->sourcount=1;//(unsigned short)totalmodule;
		(module+i)->sourindex=(unsigned short)(i+1);
//информация об исходных файлах
		(sft+i)->idx=i*2+1+numsymbols;
		(sft+i)->time=(FilesInfo[i]).time;
		count=0;	//число кореляций для данного модуля
		for(ii=0;ii<numcorrel;ii++){//обход таблицы корреляции
			if((corinfo+ii)->file==(unsigned short)i){	//корр для этого модуля
				if(count==0){	//первый блок
					(segment+i)->ofs=beg=(unsigned short)(corinfo+ii)->ofs;
					(segment+i)->correli=(unsigned short)(corrnum+1);	//correlation index
					(module+i)->corindex=(unsigned short)(corrnum+1);
				}
				(ct+corrnum)->beg=(corinfo+ii)->startline+1;
				(ct+corrnum)->segidx=(unsigned short)(1+i);	//segment idx
				(ct+corrnum)->filidx=(unsigned short)(i+1);	//file idx
				(ct+corrnum)->count=(corinfo+ii)->count;	//число линий
				end=(unsigned short)(corinfo+ii)->end;
				corrnum++;
				count++;
			}
		}
//таблица сегментов
		(segment+i)->idx=(unsigned short)(i+1);	//segment index
		(segment+i)->size=(unsigned short)(end-beg);//length
		(segment+i)->correlc=(unsigned short)count;//(unsigned short)totalmodule; //correlation count
		(module+i)->corcount=(unsigned short)count;
		if(modelmem==TINY&&comfile==file_exe)(segment+i)->segm=0xfff0;
	}
	d16header.pol_size=outptr;
	OFS.write((const char *) &d16header, sizeof(D16START));
	if (OFS.bad())
		return -1;
	OFS.write((const char *) symbols, sizeof(_SMB_) * numsymbols);
	if (OFS.bad())
		return -1;
	OFS.write((const char *) module, sizeof(MODULE) * totalmodule);
	if (OFS.bad())
		return -1;
	free(module);
	OFS.write((const char *) sft, sizeof(SFT) * totalmodule);
	if (OFS.bad())
		return -1;
	free(sft);
//line table
	lt=(LT *)MALLOC(sizeof(LT)*pdbg);
	for(j=0;(unsigned int)j<pdbg;j++){
//		printf("line %d loc %X\n",dbgnum[j],dbgloc[j]);
		(lt+j)->line=(unsigned short)DbgLineNum[j];
		(lt+j)->ofs=(unsigned short)DbgLoc[j];
	}
	OFS.write((const char *) lt, sizeof(LT) * pdbg);
	if (OFS.bad())
		return -1;
	free(lt);
	OFS.write((const char *) segment, sizeof(SEGMENT) * totalmodule);
	if (OFS.bad())
		return -1;
	free(segment);
	OFS.write((const char *) ct, sizeof(CT) * numcorrel);
	if (OFS.bad())
		return -1;
	free(ct);
//	if(fwrite(types,NUMTYPES*12,1,hout)!=1)return -1;
	memset(&string3,0,6*totalmodule);
	OFS.write((const char *) &string3, 6 * totalmodule);
	if (OFS.bad())
		return -1;
/*	if(numsymbols){
		memset(symbols,0,4*numsymbols);
		if(fwrite(symbols,4*numsymbols,1,hout)!=1)return -1;
	}*/
	free(symbols);
	return 0;
}

void KillDataLine(int line)
{
	FilesInfo[DbgModuleNum[line]].numdline--;
	for(unsigned int j=line;(j+1)<pdbg;j++){
		DbgLoc[j]=DbgLoc[j+1];
		DbgLineNum[j]=DbgLineNum[j+1];
		DbgModuleNum[j]=DbgModuleNum[j+1];
	}
	LstFlags.erase(LstFlags.begin());
	Listing.erase(Listing.begin());
	LstEnd.erase(LstEnd.begin());
	pdbg--;
}

void GeneratLst()
{
unsigned int j;
unsigned long startip;
unsigned int offs2,line;
unsigned char flag;
	fs::path Filename = makeOutputFilename("lst");
	fs::ofstream OFS(Filename);
	dieIfNotOpen(Filename, OFS);
	if(LstEnd[pdbg-1]==0)LstEnd[pdbg-1]=endinptr;
	startip=(comfile!=file_w32&&comfile!=file_bin?0:ImageBase);
	char ver2str[4];
	sprintf(ver2str, "%02d", ver2);
	OFS << "SPHINX/SHEKER C-- One Pass Disassembler. Version " << ver1 << "." << ver2str << betta << " " << __DATE__;
	for(j=0;j<pdbg;j++){
//printf("line %d loc %X\n",dbgnum[j],dbgloc[j]);
		if((int)LstFlags[j]!=-1){
			flag=LstFlags[j];
			offs2=LstEnd[j];
			outptr=DbgLoc[j];
			instruction_offset=outptr+startip;
			seg_size=(unsigned char)(16+16*(flag&1));
			line=DbgLineNum[j];
			if(offs2!=outptr){
/*
				if(line!=0)printf("%s %u:",(startfileinfo+dbgmod[j])->filename,line);
				if(lststring[j]!=NULL)printf(" %s\n",lststring[j]);
				else if(line!=0)printf("\n");
	*/
				OFS << std::endl;
				if (line != 0)
					OFS << FilesInfo[DbgModuleNum[j]].Filename << " " << line << ":";
				if (!Listing[j].empty())
					OFS << " " << Listing[j] << std::endl;
				else if (line != 0)
					OFS << std::endl;
  			while(outptr<offs2){
					if(flag&0x1e)undata(instruction_offset, offs2 - DbgLoc[j], (flag >> 1) & 15, OFS);
	  		  else unassemble(instruction_offset, OFS);
				}
			}
			if((dbg&1)!=0&&((flag&0xe)!=0||line==0)){
				KillDataLine(j);
				j--;
			}
		}
	}
	OFS.close();
}

#ifdef DEBUGMODE
void printdebuginfo()
{
static FILE *df=NULL;
	if(df==NULL){
		if((df=fopen("debug.tmp","w+t"))==NULL)df=stdout;
	}
	fprintf(df,"%s(%d)> %08X %08X tok=%d num=%08X flag=%08X scanmode=%d %s\n",startfileinfo==NULL?"":(startfileinfo+currentfileinfo)->filename,linenumber,input,inptr2,tok,itok.number,itok.flag,scanlexmode,itok.name);
	fflush(df);
}
#endif
