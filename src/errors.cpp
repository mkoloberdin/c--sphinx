#define _ERRORS_
#include "tok.h"

#include <string>
using namespace std::string_literals;
#include <boost/algorithm/string.hpp>

void warningprint(char *str,int line,int file);
WARNACT wact[WARNCOUNT]={
	warningPrint,1,warningPrint,1,warningPrint,1,warningPrint,1,warningPrint,1,
	warningPrint,1,warningPrint,1,warningPrint,1,warningPrint,1,warningPrint,1,
	warningPrint,1,warningPrint,1,warningPrint,1,warningPrint,1,warningPrint,1};

int	maxerrors = 16; 				// number of errors to stop at

void warningprint(char *str,int line,int file);
unsigned char mapfile=FALSE;
FILE *hmap=NULL;

char shorterr[]="SHORT jump distance too large";
char buferr[128];

/* ================== error messages start =========================== */

void FindStopTok()
{
//	while(tok!=tk_eof&&itok2.type!=tp_stopper/*tok2!=tk_semicolon&&tok2!=tk_camma*/){
	do{
		nexttok();
	}while(tok!=tk_eof&&itok2.type!=tp_stopper&&itok.type!=tp_stopper/*tok2!=tk_semicolon&&tok2!=tk_camma*/);
//	}
}

void SkipBlock2()
{
	cha=cha2;
	inptr=inptr2;
	SkipBlock();
	cha2=cha;
	inptr=inptr2;
	linenum2=linenumber;
	FindStopTok();
}

void FindEndLex()
{
	while(tok2!=tk_semicolon&&tok!=tk_eof)nexttok();
}

void prError(const std::string &Str) /* error on currentline with line number and file name */
{
	prError3(Str, linenumber);
}

void prError3(const std::string &Str, unsigned int line, unsigned int file)//error message at a different than current line
{
	if(error<maxerrors){
		error++;
		sprintf((char *) string3, "%s(%d)#%d> %s.\n", FilesInfo.empty() ? "" : FilesInfo[file].Filename.c_str(),
				line, error, Str.c_str());
		printf((char *)string3);
		if(errfile.file==NULL)errfile.file=fopen(errfile.Name.c_str(),"w+t");
		if(errfile.file!=NULL)fprintf(errfile.file,(char *)string3);
	}
	else exit(e_toomanyerrors);
}

void internalError(const char *Str)// serious internal compiler error message
{
char buf[200];
	sprintf(buf,"*** SERIOUS COMPILER INTERNAL ERROR ***\n>%s",Str);
	prError(buf);
	printf("STRING:%s\nIDNAME:%s\n",String,itok.name);
	printf("TOK:%d SEGM:%d POST:%d RM:%d number:%ld\n",tok,itok.segm,itok.post,itok.rm,itok.number);
	printf("STRING2:%s\nIDNAME2:%s\n",String2,itok2.name);
	printf("TOK2:%d SEGM2:%d POST2:%d RM2:%d number2:%ld\n",tok2,itok2.segm,itok2.post,itok2.rm,itok2.number);
	printf("Out position:%04X\n",outptr);
	exit(e_internalerror);
}

const char * getnumoperand(int type, const char *Name)
{
	switch(type){
		case 0:
			return "";
		case 1:
			strcpy(buferr,"1-st ");
			break;
		case 2:
			strcpy(buferr,"2-nd ");
			break;
		case 3:
			strcpy(buferr,"3-rd ");
			break;
		default:
			sprintf(buferr,"%d-th ",type%256);
			break;
	}
	strcat(buferr,Name);
	return buferr;
}

void  expected (char ch)
{
char holdstr[80];
	sprintf(holdstr,"'%c' expected",ch);
	prError(holdstr);
}

void numexpected(int type)
{
char buf[40];
	sprintf(buf,"%snumber expected",getnumoperand(type,"operand "));
	prError(buf);
}

void varexpected(int type)
{
char buf[45];
	sprintf(buf,"%svariable expected",getnumoperand(type,"operand "));
	prError(buf);
}

void stringexpected()
{
	prError("string expected");
}

void valueexpected()
{
	prError("value expected");
}

void wordvalexpected()
{
	prError("word value expected");
}

void dwordvalexpected()
{
	prError("dword value expected");
}

void qwordvalexpected()
{
	prError("qword value expected");
}

void codeexpected()
{
	prError("assembly opcode expected");
}

void operatorexpected()
{
	prError("operator identifier expected");
}

void unexpectedeof()
{
	prError("unexpected END OF FILE");
}

void swaperror()
{
	prError("invalid or incompatable swap item");
}

void notexternfun()
{
	prError("Do not insert extern function");
}

void idalreadydefined()
{
char holdstr[80];
	sprintf(holdstr,"identifier '%s' already defined",itok.name);
	prError(holdstr);
	FindStopTok();
//	nexttok();
}

void jumperror(unsigned int Line, const char *Type)
{
	std::string SmallType = boost::algorithm::to_lower_copy(std::string(Type));
	prError3("'"s + Type + "' jump distance too large, use '"s + SmallType + "'"s, Line);
}

void unknowncompop()
{
	prError("unknown comparison operator");
}

void maxoutputerror()
{
	prError("maximum output code size exceeded");
	exit( e_outputtoobig );
}

void unableToOpenFile(const fs::path &Filename)
{
	prError("unable to open file '"s + Filename.string() + "'"s);
}

void shortjumptoolarge()
{
	prError(shorterr);
}

void thisUndefined(const char *Str, int next)
{
char holdstr[80];
	sprintf(holdstr,"'%s' undefined",Str);
	prError(holdstr);

	if(next)FindStopTok();
}

void datatype_expected(int type)
{
char buf[45];
	sprintf(buf,"%smemory variable expected",getnumoperand(type,"operand "));
	prError(buf);
	FindStopTok();
}

void illegalfloat()
{
	prError("illegal use of float point");
}

void tobigpost()
{
	prError("maximum size of post variables exceeded");
	postsize=0xFFFF;
}

void unuseableinput()
{
	prError("unuseable input");
	FindStopTok();
}

void ManyLogicCompare()
{
	prError("to many use logic compare");
}

void ZeroMassiv()
{
	prError("size massiv unknown or zero");
}

void maxdataerror()
{
	prError("maximum output data size exceeded");
	exit( e_outputtoobig );
}

void errorReadingFile(const fs::path &Filename)
{
	prError("error reading from file '"s + Filename.string() + "'"s);
}

void badInputFile(const fs::path &Filename)
{
	prError("bad input file '"s + Filename.string() + "'"s);
}

void edpip(int num)
{
char buf[64];
//	preerror("error declare parameters in function");
	sprintf(buf,"error declare %sparameters in function",getnumoperand(num,""));
	prError(buf);
}

void CompareOr()
{
	prError("compare logic OR or AND to big distance");
}

void dynamiclabelerror()
{
	prError("global labels illegal within dynamic functions");
}

void OnlyComFile()
{
	prError("this option only for COM output files");
}

void redeclare(char *name)
{
char buf[120];
	sprintf(buf,"error redeclare function \"%s\"",name);
	prError(buf);
}

void retvoid()
{
	prError("function has return type of void");
}

void extraparam(const char *Name)
{
char buf[120];
	sprintf(buf,"extra parameter in function %s",Name);
	prError(buf);
}

void blockerror()
{
	prError("illegal syntax within [ ]");
}

void block16_32error()
{
	prError("only one of 16 or 32 bit allowed within [ ]");
}

void notstructname()
{
	prError("unique struct name expected");
}

void badtoken()
{
char buf[80];
	if(displaytokerrors){
		sprintf(buf,"tokenizer: bad character value - '%c'",cha);
		prError(buf);
	}
}

void expectedError(const std::string &Str)
{
	if(displaytokerrors){
		prError(Str + " expected"s);
	}
}

void declareanonim()
{
	prError("Error declare anonimus union");
}

void declareunion()
{
	prError("Error declare union");
}
/*
void not_union_static()
{
	preerror("'static' not use in 'union'");
} */

void segoperror()
{
	prError("only '=' or '><' operands valid with segment register");
}

void segbyteerror()
{
	prError("segment registers can not be used in byte or char math");
}

void regmathoperror()
{
	prError("invalid operation for non-AX register math");
}

void begmathoperror()
{
	prError("invalid operation for non-AL register math");
}

void negregerror()
{
	prError("negative non-constant invalid for non-AX register math");
}

void regbyteerror()
{
	prError("byte or char operands invalid for non-AX register math");
}

void begworderror()
{
	prError("specified 16 bit operand invalid for non-AL register math");
}

void regshifterror()
{
	prError("only CL or 1 valid for non AX or AL register bit shifting");
}

void regmatherror()
{
	prError("invalid operand for non-AX register math");
}

void DevideZero()
{
	prError("impossible to divide into ZERO");
}

void wordnotoper()
{
	prError("word or int operands invalid for non-EAX register math");
}

void regexpected(int type)
{
char buf[50];
	sprintf(buf,"%sword register expected",getnumoperand(type,"operand "));
	prError(buf);
}

void bytevalexpected(int type)
{
char buf[50];
	sprintf(buf,"%sbyte value expected",getnumoperand(type,"operand "));
	prError(buf);
}

void shortjumperror()
{
	prError("invalid operand for SHORT jump");
}

void invalidfarjumpitem()
{
	prError("invalid operand for FAR jump");
}

void invalidfarcallitem()
{
	prError("invalid operand for FAR call");
}

void begexpected(int type)
{
char buf[50];
	sprintf(buf,"%sbyte register expected",getnumoperand(type,"operand "));
	prError(buf);
}

void reg32expected(int type)
{
char buf[50];
	sprintf(buf,"%s32 bit register expected",getnumoperand(type,"operand "));
	prError(buf);
}

void reg32regexpected(int type)
{
char buf[50];
	sprintf(buf,"%s16 or 32 bit register expected",getnumoperand(type,"operand "));
	prError(buf);
}

void regBXDISIBPexpected()
{
	prError("use only one of BX, DI, SI or BP register");
}

void bytedxexpected()
{
	prError("byte constant or DX expected");
}

void axalexpected()
{
	prError("EAX, AX or AL expected");
}

void invalidoperand(int type)
{
char buf[25];
	sprintf(buf,"%sinvalid",getnumoperand(type,"operand "));
	prError(buf);
}

void mmxregexpected(int type)
{
char buf[50];
	sprintf(buf,"%sMMX register expected",getnumoperand(type,"operand "));
	prError(buf);
}

void xmmregexpected(int type)
{
char buf[50];
	sprintf(buf,"%sXMM register expected",getnumoperand(type,"operand "));
	prError(buf);
}

void xmmregorvarexpected(int type)
{
char buf[60];
	sprintf(buf,"%sXMM register or memory varible expected",getnumoperand(type,"operand "));
	prError(buf);
}

void mmxregordwordexpected(int type)
{
char buf[60];
	sprintf(buf,"%sMMX register or memory varible expected",getnumoperand(type,"operand "));
	prError(buf);
}

void clornumberexpected()
{
	prError("CL or constant expected");
}

void fpuvarexpected(int type)
{
char buf[70];
	sprintf(buf,"%sexpected FPU register|long|dword|float var",getnumoperand(type,"operand "));
	prError(buf);
}

void fpustakexpected(int type)
{
char buf[40];
	sprintf(buf,"%sexpected FPU register",getnumoperand(type,"operand "));
	prError(buf);
}

void fpu0expected()
{
	prError("2-nd expected only st(0) fpu register");
}

void fpustdestroed()
{
	prError("FPU register more 6 destroed");
}

void errstruct()
{
	prError("illegal use of struct");
}

void tegnotfound()
{
	prError("tag struct not found");
}

void ErrWrite()
{
	fprintf(stderr,"unable to write output file.\n");
}

void ErrReadStub()
{
	fprintf(stderr,"unable to read stubfile\n");
}

void InvOperComp()
{
	prError("invalid operation for compare");
}

void mmxormem(int type)
{
char buf[60];
	sprintf(buf,"%sexpected mmx register or memory variable",getnumoperand(type,"operand "));
	prError(buf);
}

void reg32orword(int type)
{
char buf[60];
	sprintf(buf,"%s32 bit register or word variable expected",getnumoperand(type,"operand "));
	prError(buf);
}

void undefclass(char *name)
{
char buf[30+IDLENGTH];
	sprintf(buf,"Base class '%s' not defined",name);
	prError(buf);
}

void unknowntype()
{
	prError("unknown type");
}

void destrdestreg()
{
	prError("destroyed destination register");
}

void unknownstruct (char *name,char *sname)
{
char buf[IDLENGTH*2+30];
	sprintf(buf,"unknown member '%s' in struct '%s'",name,sname);
	prError(buf);
}

void unknowntagstruct(char *name)
{
char buf[IDLENGTH+16];
	sprintf(buf,"unknown tag '%s'",name);
	prError(buf);
}

void unknownobj(char *name)
{
char buf[IDLENGTH+32];
	sprintf(buf,"unknown object for member '%s'",name);
	prError(buf);
}

void unknownPragma(const char *Name)
{
char buf[IDLENGTH+32];
	sprintf(buf,"unknown parametr for #pragma %s",Name);
	prError(buf);
}

/*-----------------08.08.00 22:49-------------------
 Предупреждения
	--------------------------------------------------*/

void warningoptnum()
{
	if(wact[0].usewarn)wact[0].fwarn("Optimize numerical expressions",linenumber,CurrentFileInfoNum);
}

void warningreg(const char *str2)
{
char buf[50];
	if(wact[1].usewarn){
		sprintf(buf,"%s has been used by compiler",str2);
		wact[1].fwarn(buf,linenumber,CurrentFileInfoNum);
	}
}

void warningJmp(const std::string &Str2, unsigned int Line, unsigned int File)
{
	if(wact[2].usewarn){
		wact[2].fwarn("Short operator '"s + Str2 + "' may be used"s, Line, File);
	}
}

void warningstring()
{
	if(wact[3].usewarn){
		sprintf((char *)String2,"String \"%s\" repeated",string3);
		wact[3].fwarn((char *)String2,linenumber,CurrentFileInfoNum);
	}
}

void warningexpand()
{
	if(wact[4].usewarn)wact[4].fwarn("Expansion variable",linenumber,CurrentFileInfoNum);
}

void warningretsign()
{
	if(wact[5].usewarn)wact[5].fwarn("Signed value returned",linenumber,CurrentFileInfoNum);
}

void warningPrint(const std::string &Str, unsigned int Line, unsigned int File)
{
	if(warning==TRUE){
		if (!wartype.Name.empty() && wartype.file == stdout) {
			if ((wartype.file = fopen(wartype.Name.c_str(), "w+t")) == NULL)wartype.file = stdout;
		}
		fprintf(wartype.file, "%s(%d)> Warning! %s.\n",
				FilesInfo.empty() ? "" : FilesInfo[File].Filename.c_str(), Line, Str.c_str());
	}
}

void  warningdefined(char *name)
{
char buf[IDLENGTH+30];
	if(wact[6].usewarn){
		sprintf(buf,"'%s' defined above, therefore skipped",name);
		wact[6].fwarn(buf,linenumber,CurrentFileInfoNum);
	}
}

void warningnotused(char *name,int type)
{
char buf[IDLENGTH+40];
	const char *typenames[] = {"Variable", "Structure", "Function", "Local variable",
							   "Parameter variable", "Local structure"};
	if(wact[7].usewarn){
		sprintf(buf,"%s '%s' possible not used",(char *)typenames[type],name);
		wact[7].fwarn(buf,linenumber,CurrentFileInfoNum);
	}
}

void warningusenotintvar(char *name)
{
char buf[IDLENGTH+50];
	if(wact[8].usewarn){
		sprintf(buf,"Non-initialized variable '%s' may have been used",name);
		wact[8].fwarn(buf,linenumber,CurrentFileInfoNum);
	}
}

void warningdestroyflags()
{
	if(wact[9].usewarn)wact[9].fwarn("Return flag was destroyed",linenumber,CurrentFileInfoNum);
}

void warningunreach()
{
	if(wact[10].usewarn)wact[10].fwarn("Code may not be executable",linenumber,CurrentFileInfoNum);
}

void warninline()
{
	if(wact[11].usewarn)wact[11].fwarn("Don't use local/parametric values in inline functions",linenumber,CurrentFileInfoNum);
}

void warnsize()
{
	if(wact[12].usewarn)wact[12].fwarn("Sources size exceed destination size",linenumber,CurrentFileInfoNum);
}

void waralreadinit(char *reg)
{
#ifdef BETTA
	warningPrint(("Register "s + reg + " already initialized"s).c_str(), linenumber, CurrentFileInfoNum);
#endif
}

void waralreadinitreg(char *reg, char *reg2)
{
#ifdef BETTA
	warningPrint(("Register "s + reg + " same as "s + reg2).c_str(), linenumber, CurrentFileInfoNum);
#endif
}

void warpragmapackpop()
{
	if(wact[13].usewarn)wact[13].fwarn("Pragma pack pop with no matching pack push",linenumber,CurrentFileInfoNum);
}

void missingpar(const char *Name)
{
char buf[120];
	if(wact[14].usewarn){
		sprintf(buf,"Missing parameter in function %s",Name);
		wact[14].fwarn(buf,linenumber,CurrentFileInfoNum);
	}
//	preerror(buf);
}

void warreplasevar(char *name)
{
//	if(usewarn[14]){
	if(displaytokerrors){
		warningPrint(("Variable "s + name + " is replased on constant"s).c_str(), linenumber, CurrentFileInfoNum);
	}
//	}
//	preerror(buf);
}

void waralreadinitvar(char *name,unsigned int num)
{
//	if(usewarn[14]){
	if(displaytokerrors){
		warningPrint(("Variable "s + name + " already initialized by constant "s + std::to_string(num)).c_str(),
					 linenumber, CurrentFileInfoNum);
	}
//	}
//	preerror(buf);
}

void warcompneqconst()
{
	warningPrint("Comparison not equal constant. Skipped code", linenumber, CurrentFileInfoNum);
}

void warcompeqconst()
{
	warningPrint("Comparison equal constant. Skipped compare", linenumber, CurrentFileInfoNum);
}

void warpointerstruct()
{
	warningPrint("Compiler not support pointers on structure", linenumber, CurrentFileInfoNum);
}

void warESP()
{
	warningPrint("ESP has ambiguous value", linenumber, CurrentFileInfoNum);
}

/* *****************   map file *************** */

void OpenMapFile()
{
char buf[256];
	sprintf(buf,"%s.map",RawFileName.c_str());
	hmap=fopen(buf,"w+t");
}

const char * getRetType(int type, int flag)
{
	const char *retcode;
	if(flag&f_interrupt)return "";
	switch(type){
		case tk_bytevar:
		case tk_byte:
			retcode="byte ";
			break;
		case tk_charvar:
		case tk_char:
			retcode="char ";
			break;
		case tk_wordvar:
		case tk_word:
			retcode="word ";
			break;
		case tk_longvar:
		case tk_long:
			retcode="long ";
			break;
		case tk_dwordvar:
		case tk_dword:
			retcode="dword ";
			break;
		case tk_doublevar:
		case tk_double:
			retcode="double ";
			break;
		case tk_fpust:
			retcode="fpust ";
			break;
		case tk_floatvar:
		case tk_float:
			retcode="float ";
			break;
		case tk_qwordvar:
		case tk_qword:
			retcode="qword ";
			break;
		case tk_void:
			retcode="void ";
			break;
/*		case tk_intvar:
		case tk_int:
			retcode="int ";
			break;*/
		case tk_structvar:
			retcode="struct";
			break;
		default:
			retcode="int ";
			break;
	}
	return retcode;;
}

const char * getTypeProc(int flag)
{
	const char *t;
	String2[0]=0;
	if(flag&f_interrupt)return "interrupt";
	if(flag&f_far)strcat((char *)String2,"far ");
	if(flag&f_extern)strcat((char *)String2,"extern ");
	if(flag&f_export)strcat((char *)String2,"_export ");
	if(flag&f_inline)strcat((char *)String2,"inline ");
	if(flag&f_static)strcat((char *)String2,"static ");
	if(flag&f_retproc){
		t="";
		switch(((flag&f_retproc)>>8)+tk_overflowflag-1){
			case tk_overflowflag:
				t="OVERFLOW ";
				break;
			case tk_notoverflowflag:
				t="NOTOVERFLOW ";
				break;
			case tk_carryflag:
				t="CARRYFLAG ";
				break;
			case tk_notcarryflag:
				t="NOTCARRYFLAG ";
				break;
			case tk_zeroflag:
				t="ZEROFLAG ";
				break;
			case tk_notzeroflag:
				t="NOTZEROFLAG ";
				break;
			case tk_minusflag:
				t="MINUSFLAG ";
				break;
			case tk_plusflag:
				t="PLUSFLAG ";
				break;
		}
		strcat((char *)String2,t);
	}
	switch(flag&f_typeproc){
		case tp_pascal:
			t="pascal ";
			break;
		case tp_cdecl:
			t="cdecl ";
			break;
		case tp_stdcall:
			t="stdcall ";
			break;
		case tp_fastcall:
			t="fastcall ";
			break;
	}
	strcat((char *)String2,t);
	return (char *)String2;
}

const char * getFunParam(unsigned char c, unsigned char c2, unsigned char c3)
{
	switch(c){
		case 'B':
			if(c2>=0x30&&c2<=0x37)return begs[c2-0x30];
			return "byte";
		case 'W':
			if(c2>=0x30&&c2<=0x37)return regs[0][c2-0x30];
			return "word";
		case 'D':
			if(c2>=0x30&&c2<=0x37)return regs[1][c2-0x30];
			return "dword";
		case 'C': return "char";
		case 'I': return "int";
		case 'L': return "long";
		case 'F': return "float";
		case 'A': return "...";
		case 'Q':
			if(c2>=0x30&&c2<=0x37){
				sprintf((char *)String2,"%s:%s",regs[1][c2-0x30],regs[1][c3-0x30]);
				return (char *)String2;
			}
			return "qword";
		case 'E': return "double";
		case 'S':
			if(c2>=0x30&&c2<=0x37){
				sprintf((char *)String2,"st(%c)",c2);
				return (char *)String2;
			}
			return "fpust";
		case 'T': return "struct";
		case 'U': return "void";
	}
	return "";;
}

char *GetName(char *name,int flag)
{
char *tn;
char iname[IDLENGTH];
	strcpy(iname,name);
	if((tn=strchr(iname,'@'))!=NULL)*tn=0;
	if(flag&fs_constructor)sprintf((char *)string3,"%s::%s",iname,iname);
	else if(flag&fs_destructor)sprintf((char *)string3,"%s::~%s",iname,iname);
	else if(flag&f_classproc)sprintf((char *)string3,"%s::%s",searchteg->name,iname);
	else strcpy((char *)string3,iname);
	return (char *)string3;
}

const char * getSizeVar(int type, int adr)
{
	const char *reg;
	const char *sign;
	if(type==tp_postvar||type==tp_gvar)return "DS:???";
	if(am32){
		if(ESPloc)reg="ESP";
		else reg="EBP";
	}
	else reg="BP";
	if(adr<0)sign="";
	else sign="+";
	sprintf((char *)String2,"%s%s%d",reg,sign,adr);
	return (char *)String2;
}

void GetRangeUsed(char *buf,localinfo *ptr)
{
	if(ptr->count==0)buf[0]=0;
	else if(ptr->count==1)sprintf(buf,"%d",ptr->usedfirst);
	else sprintf(buf,"%d-%d",ptr->usedfirst,ptr->usedlast);
}

void mapfun(int line)
{
treelocalrec *ftlr;
struct localrec *ptr;
int i,fheader;
char buf[32];
	if(hmap==NULL)OpenMapFile();
	if(hmap){
		fprintf(hmap,"\n%s%s%s(", getTypeProc(itok.flag), getRetType(itok.rm, itok.flag),GetName(itok.name,itok.flag));
		for(i=0;;i++){
			unsigned char c=String[i];
			if(c==0)break;
			if(c>=0x30&&c<=0x37)continue;
			if(i)fputc(',',hmap);
			unsigned char c2=String[i+1];
			unsigned char c3=String[i+2];
			fputs(getFunParam(c, c2, c3),hmap);
		}
		fputc(')',hmap);
		fprintf(hmap, "\nlocation: line %d-%d file %s", line, linenumber,
				FilesInfo.empty() ? "" : FilesInfo[CurrentFileInfoNum].Filename.c_str());
		fprintf(hmap,"\noffset=0x%X(%d)\tsize=0x%X(%d)",itok.number,itok.number,itok.size,itok.size);
	}
	fheader=0;
	for(ftlr=btlr;ftlr!=NULL;ftlr=ftlr->next){
		for(ptr=ftlr->lrec;ptr!=NULL;ptr=ptr->rec.next){
			i=ptr->rec.type;
			if(i==tp_postvar||i==tp_gvar||i==tp_localvar||i==tp_paramvar){
				if(!fheader){
					fputs("\nType    Address   Used     Count  Size    Name",hmap);
					fputs("\n----------------------------------------------",hmap);
					fheader=TRUE;
				}
				GetRangeUsed(buf,&ptr->li);
				fprintf(hmap,"\n%-8s%-10s%-12s%-4d%-8d", getRetType(ptr->rec.rectok, 0),
						getSizeVar(ptr->rec.type, ptr->rec.recnumber),buf,ptr->li.count,ptr->rec.recsize);
				if(ptr->rec.npointr)for(i=ptr->rec.npointr;i>0;i--)fputc('*',hmap);
				fputs(ptr->rec.recid,hmap);
			}
		}
	}
	fputs("\n",hmap);
}

