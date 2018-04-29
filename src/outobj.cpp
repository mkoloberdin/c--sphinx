#include "tok.h"

void obj_outrecord(int recordtype, unsigned int recordlength, unsigned char *data, fs::ofstream &OFS);
void outeachPUBDEF(struct idrec *ptr, fs::ofstream &OFS);
void obj_outLEDATA(unsigned int segm, unsigned int offset, unsigned int recordlength, unsigned char *data,
				   fs::ofstream &OFS);

#define MAXNUMEXTNAME 1024;

int *numextname;
int maxnumextname=MAXNUMEXTNAME;
unsigned int lenextstr=0;	//длина строки с внешними именами
int numextern=0;

int postseg,stackseg;

unsigned int findextname(int extnum)
{
	for(unsigned int i=0;i<externnum;i++){
		if(numextname[i]==extnum)return i+1;
	}
	return 0;
}

int makeObj()
{
	size_t i;
	unsigned int count, sizeblock;
	fs::path Filename = makeOutputFilename("obj");
	fs::ofstream OFS(Filename, std::ios_base::binary);
	dieIfNotOpen(Filename, OFS);
    i = FilesInfo[0].Filename.size();
	String2[0]=(unsigned char)i;
	strcpy((char *)&String2[1],FilesInfo[0].Filename.string().c_str()); // FIXME: Convert to proper codepage on Windows [?]
	obj_outrecord(0x80, i + 1, &String2[0], OFS);// output the LNAMES
	sprintf((char *)&String2[3],"%s %s",compilerstr,__DATE__);
	i=strlen((char *)&String2[3]);
	*(short *)&String2[0]=0;
	String2[2]=(unsigned char)i;
	obj_outrecord(0x88, i + 3, &String2[0], OFS);// output the LNAMES
	for(count=0;count<totalmodule;count++){	//имена включаемых файлов
		*(struct ftime *) &String2[2] = FilesInfo[count].time;
		strcpy((char *) &String2[7], FilesInfo[count].Filename.string().c_str()); // FIXME: Convert to proper codepage on Windows [?]
		i = FilesInfo[count].Filename.size();
		*(short *)&String2[0]=0xE940;
		String2[6]=(unsigned char)i;
		obj_outrecord(0x88, i + 7, &String2[0], OFS);// output the LNAMES
	}
	count=outptr-startptr;	//размер кода
	unsigned char *data=output+startptr;	//начало данных
	*(short *)&String2[0]=0xE940;
	obj_outrecord(0x88, 2, &String2[0], OFS);//конец коментарий
	if(!am32){
		*(short *)&String2[0]=0xEA00;
		String2[2]=1;
		String2[3]=(unsigned char)(modelmem==SMALL?9:8);
		obj_outrecord(0x88, 4, &String2[0], OFS);
	}
	else{
		*(short *)&String2[0]=0xA140;
		obj_outrecord(0x88, 2, &String2[0], OFS);
	}
	obj_outrecord(0x96, 39, (unsigned char *) "\000\005_TEXT\004CODE\004_BSS\003BSS\006DGROUP\005_DATA\004DATA",
				  OFS);
// output the SEGDEF
	if(!am32){
		String2[0]=(unsigned char)0x28;
		*(short *)&String2[1]=(short)outptr;//count;// Set the length of the segment of DATA or CODE
		String2[3]=0x02;	//имя сегмента _TEXT
		*(short *)&String2[4]=0x0103;	//класс CODE Overlay NONE 1
		obj_outrecord(0x98, 6, String2, OFS);
		i=2;

		if(comfile==file_exe&&modelmem==SMALL){
			String2[0]=(unsigned char)0x48;
			*(short *)&String2[1]=outptrdata;// Set the length of the segment DATA
			String2[3]=0x07;	//имя сегмента _DATA
			*(short *)&String2[4]=0x0108;	//класс DATA Overlay NONE
			obj_outrecord(0x98, 6, String2, OFS);
			i++;
		}

		postseg=i;
		String2[0]=(unsigned char)0x48;
		*(short *)&String2[1]=(short)postsize;// Set the length of the segment BSS
		String2[3]=0x04;	//имя сегмента _BSS
		*(short *)&String2[4]=0x0105;	//класс BSS Overlay NONE
		obj_outrecord(0x98, 6, String2, OFS);
		i++;

		if(comfile==file_exe&&modelmem==SMALL){
			obj_outrecord(0x96, 6, (unsigned char *) "\005STACK", OFS);
			String2[0]=0x74;
			*(short *)&String2[1]=(short)stacksize;// Set the length of the segment STACK
			String2[3]=0x09;	//имя сегмента STACK
			*(short *)&String2[4]=0x0109;	//класс STACK Overlay NONE
			obj_outrecord(0x98, 6, String2, OFS);
			stackseg=i;
		}
		String2[0]=6;	//имя DGROUP
		if(comfile==file_exe&&modelmem==SMALL){
			*(short *)&String2[1]=0x2FF;
			*(short *)&String2[3]=0x3FF;//postseg*256+255;//0x3FF;
			*(short *)&String2[5]=0x4ff;//stackseg*256+255;//0x4FF;
			i=7;
		}
		else{
			*(short *)&String2[1]=0x1FF;
//			*(short *)&string2[3]=0x2FF;
			*(short *)&String2[3]=0x2ff;//postseg*256+255;//0x3FF;
			i=5;
		}
		obj_outrecord(0x9A, i, String2, OFS);
	}
	else{
		String2[0]=(unsigned char)0xA9;
		*(long *)&String2[1]=(long)outptr;//count;// Set the length of the segment of DATA or CODE
		String2[5]=0x02;	//имя сегмента _TEXT
		*(short *)&String2[6]=0x0103;	//класс CODE Overlay NONE
		obj_outrecord(0x99, 8, String2, OFS);
		i=2;
/*
		string2[0]=(unsigned char)0xA9;
		*(long *)&string2[1]=0;// Set the length of the segment DATA
		string2[5]=0x07;	//имя сегмента _DATA
		*(short *)&string2[6]=0x0108;	//класс DATA Overlay NONE
		obj_outrecord(0x99,8,string2);
		i++;*/

		postseg=i;
		String2[0]=(unsigned char)0xA9;
		*(long *)&String2[1]=(long)postsize;// Set the length of the segment BSS
		String2[5]=0x04;	//имя сегмента _BSS
		*(short *)&String2[6]=0x0105;	//класс BSS Overlay NONE
		obj_outrecord(0x99, 8, String2, OFS);
		i++;

		obj_outrecord(0x96, 11, (unsigned char *) "\005STACK\004FLAT", OFS);//9,10

		if(comfile!=file_w32){
			String2[0]=0x75;
			*(long *)&String2[1]=(long)stacksize;// Set the length of the segment STACK
			String2[5]=0x09;	//имя сегмента STACK
			*(short *)&String2[6]=0x0109;	//класс STACK Overlay NONE
			obj_outrecord(0x99, 8, String2, OFS);
			stackseg=i;
		}
		String2[0]=10;

		obj_outrecord(0x9A, 1, String2, OFS);	//GRPDEF Group: FLAT
		String2[0]=6;	//имя DGROUP
		i=1;
//		*(short *)&string2[i]=0x2FF;	//DATA
//		i+=2;
		*(short *)&String2[i]=postseg*256+255;//0x3FF;	//BSS
		i+=2;
		if(comfile!=file_w32){
			*(short *)&String2[i]=stackseg*256+255;//0x4FF
			i+=2;
		}
		obj_outrecord(0x9A, i, String2, OFS);
	}
// вывод EXTDEF
	while(externnum>maxnumextname)maxnumextname+=MAXNUMEXTNAME;
	numextname=(int *)MALLOC(maxnumextname*sizeof(int));
// output the PUBDEF records for each exteral procedures (all procedures)
    outeachPUBDEF(treestart, OFS);
	if(lenextstr!=0)obj_outrecord(0x8c, lenextstr, &String[0], OFS);
// output the data (LEDATA) in 1K chunks as required!
	i=0;
	char *bufobj=(char *)MALLOC(512*5);
	while(i<count){
		unsigned int j;
		sizeblock=1024;
restart:
		for(j=0;j<posts;j++){
			if((postbuf+j)->type>=CALL_EXT&&(postbuf+j)->type<=FIX_CODE32&&
					(postbuf+j)->loc>=(i+startptr)&&(postbuf+j)->loc<(i+sizeblock+startptr)){
				if((postbuf+j)->loc>(i+startptr+sizeblock-(am32==FALSE?2:4))){
					sizeblock=(postbuf+j)->loc-i-startptr;//изменить размер блока
					goto restart;
				}
			}
		}
		if((i+sizeblock)>count)sizeblock=count-i;
		obj_outLEDATA(1, i + startptr, sizeblock, data + i, OFS);
		int ofsfix=0;
		for(j=0;j<posts;j++){
			if((postbuf+j)->loc>=(i+startptr)&&(postbuf+j)->loc<(i+sizeblock+startptr)){
				int hold=(postbuf+j)->loc-i-startptr;
				if((postbuf+j)->type>=CALL_32I/*POST_VAR*/&&(postbuf+j)->type<=FIX_CODE32){
					bufobj[ofsfix++]=(unsigned char)((am32==FALSE?0xC4:0xE4)|(hold/256));
					bufobj[ofsfix++]=(unsigned char)(hold%256);
					bufobj[ofsfix++]=0x14;
					bufobj[ofsfix++]=1;
					switch((postbuf+j)->type){
						case POST_VAR:
						case POST_VAR32:
							bufobj[ofsfix++]=postseg;
							break;
						case FIX_VAR:
						case FIX_VAR32:
							bufobj[ofsfix++]=(unsigned char)((comfile==file_exe&&modelmem==SMALL)?2:1);
							break;
						case FIX_CODE:
						case FIX_CODE32:
							bufobj[ofsfix++]=1;
							break;
					}
				}
				if((postbuf+j)->type>=POST_VAR32&&(postbuf+j)->type<=FIX_CODE32){

				}
				else if((postbuf+j)->type==CALL_EXT){
					int numext=findextname((postbuf+j)->num);
					if(numext!=0){
						bufobj[ofsfix++]=(unsigned char)((am32==FALSE?0x84:0xA4)|(hold/256));
						bufobj[ofsfix++]=(unsigned char)(hold%256);
						bufobj[ofsfix++]=0x56;
						bufobj[ofsfix++]=(unsigned char)numext;
					}
				}
				else if((postbuf+j)->type==EXT_VAR){
					int numext=findextname((postbuf+j)->num);
					if(numext!=0){
						bufobj[ofsfix++]=(unsigned char)((am32==FALSE?0xC4:0xE4)|(hold/256));
						bufobj[ofsfix++]=(unsigned char)(hold%256);
						bufobj[ofsfix++]=0x16;
						bufobj[ofsfix++]=1;
						bufobj[ofsfix++]=(unsigned char)numext;
					}
				}
			}
		}
		if(ofsfix!=0)obj_outrecord(0x9C + am32, ofsfix, (unsigned char *) bufobj, OFS);
		i+=sizeblock;
	}
	free(bufobj);
	if(comfile==file_exe&&modelmem==SMALL){
		i=0;
		while(i<outptrdata){
			if((i+1024)>outptrdata)obj_outLEDATA(2, i, outptrdata - i, outputdata + i, OFS);
			else obj_outLEDATA(2, i, 1024, outputdata + i, OFS);
			i+=1024;
		}
	}
	if(sobj!=FALSE){
// output end of OBJ notifier
		String2[0]=0;
		i=1;
	}
	else{
		count=EntryPoint();
		*(short *)&String2[0]=0xC1;	//главный модуль имеет стартовый адрес котор нельзя менять.
		*(short *)&String2[2]=0x101;
		if(count<65536){
			*(short *)&String2[4]=(short)count;
			i=6;
		}
		else{
			*(long *)&String2[4]=(long)count;
			i=8;
		}
	}
	obj_outrecord(i == 8 ? 0x8B : 0x8A, i, String2, OFS);
	free(numextname);
	runfilesize = OFS.tellp();
	OFS.close();
	return 0;
}

void obj_outLEDATA(unsigned int segm, unsigned int offset, unsigned int recordlength, unsigned char *data,
				   fs::ofstream &OFS)
{
int checksum=0;
int i;
unsigned char buf[8];
	buf[3]=(unsigned char)segm;
	if(offset>(65536-1024)){
		*(short *)&buf[1]=(short)(recordlength+6);
		buf[0]=0xA1;
		*(long *)&buf[4]=(long)offset;
		i=8;
	}
	else{
		*(short *)&buf[1]=(short)(recordlength+4);
		buf[0]=0xA0;
		*(short *)&buf[4]=(short)offset;
		i=6;
	}
	OFS.write((const char *) buf, i);
	for(i--;i>=0;i--)checksum+=buf[i];
	for(i=0;(unsigned int)i<recordlength;i++)checksum+=data[i];
	OFS.write((const char *) data, recordlength);
	checksum=-checksum;
	OFS.write((const char *) &checksum, 1);
}

void obj_outrecord(int recordtype, unsigned int recordlength, unsigned char *data, fs::ofstream &OFS)
// Outputs an OBJ record.
{
int checksum;
unsigned int i;
	recordlength++;
	checksum=recordtype;
	OFS.write((const char *) &recordtype, 1);
	checksum+=(recordlength/256);
	checksum+=(recordlength&255);
	OFS.write((const char *) &recordlength, 2);
	recordlength--;
	for(i=0;i<recordlength;i++)checksum+=data[i];
	OFS.write((const char *) data, recordlength);
	checksum=-checksum;
	OFS.write((const char *)&checksum, 1);
}

void outeachPUBDEF(struct idrec *ptr, fs::ofstream &OFS)
{
unsigned int i;
	if(ptr!=NULL){
        outeachPUBDEF(ptr->right, OFS);
		if(ptr->rectok==tk_apiproc){
			i=0;
			for(unsigned int j=0;j<posts;j++){	//поиск использования процедуры
				if((postbuf+j)->num==(unsigned long)ptr->recnumber&&((postbuf+j)->type==CALL_32I||(postbuf+j)->type==CALL_32)){
					i++;
					(postbuf+j)->type=CALL_EXT;
					externnum++;
					if(externnum>=maxnumextname){
						maxnumextname+=MAXNUMEXTNAME;
						numextname=(int *)REALLOC(numextname,maxnumextname*sizeof(int));
					}
				}
			}
			if(i)goto nameext;
			goto endp;
		}
		if(externnum!=0&&ptr->rectok==tk_undefproc&&(ptr->flag&f_extern)!=0){
nameext:
			numextname[numextern++]=ptr->recnumber;
			i=strlen(ptr->recid);
			if((lenextstr+i+2)>=STRLEN){
				obj_outrecord(0x8c, lenextstr, &String[0], OFS);
				lenextstr=0;
			}
			String[lenextstr++]=(unsigned char)i;
			strcpy((char *)&String[lenextstr],ptr->recid);
			lenextstr+=i+1;
		}
		else{
			if((ptr->rectok==tk_proc||ptr->rectok==tk_interruptproc)&&ptr->recsegm>=NOT_DYNAMIC){
				String2[0]=(unsigned char)(am32==0?0:1);
				String2[1]=0x01;
			}
			else if((ptr->rectok>=tk_bits&&ptr->rectok<=tk_doublevar)||ptr->rectok==tk_structvar){
				if((ptr->flag&f_extern))goto nameext;
				if(am32){
					String2[0]=1;
					String2[1]=(unsigned char)(ptr->recpost==0?1:postseg);
				}
				else if(comfile==file_exe&&modelmem==SMALL){
					String2[0]=1;
					String2[1]=(unsigned char)(ptr->recpost==0?2:postseg);
				}
				else{
					String2[0]=(unsigned char)(ptr->recpost==0?0:1);
					String2[1]=(unsigned char)(ptr->recpost==0?1:postseg);
				}
			}
			else goto endp;
			for(i=0;ptr->recid[i]!=0;i++)String2[i+3]=ptr->recid[i];
			String2[2]=(unsigned char)i;
			if(ptr->recnumber<65536){
				*(short *)&String2[i+3]=(short)ptr->recnumber;
				String2[i+5]=0x00;
				obj_outrecord(0x90, i + 6, &String2[0], OFS);
			}
			else{
				*(long *)&String2[i+3]=(long)ptr->recnumber;
				String2[i+7]=0x00;
				obj_outrecord(0x91, i + 8, &String2[0], OFS);
			}
		}
endp:
        outeachPUBDEF(ptr->left, OFS);
	}
}

