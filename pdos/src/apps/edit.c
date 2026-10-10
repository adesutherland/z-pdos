/* SPDX-License-Identifier: MIT; EDIT, a small U31 C record line editor. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "twospace_ui.h"
#include "twospace_recordio.h"
#include "edit_core.h"
static unsigned int word(const unsigned char *p)
{return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|((unsigned int)p[2]<<8)|p[3];}
static void out(const char *p)
{TUILINE((const unsigned char *)p,(unsigned int)strlen(p));}
static void status(const EDCSTATE *s,int dirty)
{char b[100];sprintf(b,"EDIT: %u lines / current %u / %s",s->count,s->current,dirty?"unsaved changes":"saved");out(b);}
static void show(const EDCSTATE *s,unsigned int first,unsigned int last)
{
    unsigned int i,at,take;char b[257];
    for(i=first;i&&i<=last&&i<=s->count;++i){
        sprintf(b,"%3u (%u bytes)",i,s->line[i-1U].length);out(b);
        for(at=0U;at<s->line[i-1U].length;at+=take){take=s->line[i-1U].length-at;if(take>256U)take=256U;
            TUILINE(s->line[i-1U].text+at,take);}
        if(!s->line[i-1U].length)out("");
    }
}
static int number(char **p,unsigned int *n)
{
    unsigned int v=0U,got=0U;while(**p==' ')++*p;
    while(**p>='0'&&**p<='9'){if(v>EDC_LINES)return 0;v=v*10U+(unsigned int)(*(*p)++-'0');got=1U;}
    if(!got||v>EDC_LINES)return 0;*n=v;return 1;
}
int main(int argc,char **argv)
{
    EDCSTATE *s,*undo,*temp,*candidate,*saved;unsigned char *packed,info[TRI_BYTES],input[257];
    unsigned int rc,n,i,first,last,at,has_undo=0U,input_mode=0U,input_undo=0U,bytes;
    char *p,*end,*replacement,command[20],source[52],b[150];int result=0;
    if(argc>2){printf("EDIT dataset (optional) -- native PS FB/VB, 4096 lines, 256 bytes per line\n");return 8;}
    s=(EDCSTATE *)calloc(1,sizeof *s);undo=(EDCSTATE *)calloc(1,sizeof *undo);
    candidate=(EDCSTATE *)calloc(1,sizeof *candidate);saved=(EDCSTATE *)calloc(1,sizeof *saved);packed=(unsigned char *)malloc(TRI_LIMIT);
    if(!s||!undo||!candidate||!saved||!packed){result=4;goto cleanup;}
    source[0]=0;
    if(TUIOPEN((const unsigned char *)"EDIT / record line editor",sizeof "EDIT / record line editor"-1U)){result=8;goto cleanup;}
    if(argc==2){
        if(strlen(argv[1])>51U){result=8;goto cleanup;}strcpy(source,argv[1]);
        rc=TRIFILE(TRI_LOAD,source,packed,TRI_LIMIT,info);
        if(rc||EDCLOAD(s,packed,word(info+16U))){sprintf(b,"EDIT: open refused (%u); PS FB/VB first-track file required",rc?rc:20U);out(b);result=8;goto cleanup;}
    }
    *saved=*s;out("EDIT / HELP lists commands / SAVE AS writes a separate empty dataset");
    for(;;){
        if(input_mode)out("INPUT> . ends input; .. inserts a dot");else{status(s,!EDCEQUAL(s,saved));out("EDIT>");}
        rc=TUIREAD(input,256U,&n);if(rc){result=(int)rc;break;}input[n]=0;
        if(input_mode){
            if(n==1U&&input[0]=='.'){input_mode=0U;continue;}
            if(n==2U&&input[0]=='.'&&input[1]=='.')n=1U;
            *candidate=*s;rc=EDCINSERT(candidate,s->current,input,n);
            if(rc){out("EDIT: input full; line was not inserted");continue;}
            if(!input_undo){*undo=*s;has_undo=1U;input_undo=1U;}
            *s=*candidate;continue;
        }
        p=(char *)input;while(*p==' ')++p;i=0U;
        while(*p&&*p!=' '&&i<sizeof command-1U)command[i++]=(char)toupper((unsigned char)*p++);
        command[i]=0;if(*p==' ')++p;
        if(!strcmp(command,"HELP")){
            out("PRINT first last (optional) / TOP / BOTTOM / LOCATE literal");
            out("INPUT after (optional) / INSERT after text");
            out("DELETE first last (last optional)");
            out("CHANGE line /old/new/ (first literal occurrence) / UNDO");
            out("SAVE AS dataset / QUIT (refuses unsaved) / QUIT DISCARD");continue;
        }
        if(!strcmp(command,"QUIT")){
            if(*p&&strcmp(p,"DISCARD")&&strcmp(p,"discard")){out("EDIT: use QUIT or QUIT DISCARD");continue;}
            if(!EDCEQUAL(s,saved)&&!*p){out("EDIT: unsaved changes; SAVE AS or QUIT DISCARD");continue;}break;
        }
        if(!strcmp(command,"TOP")||!strcmp(command,"BOTTOM")){s->current=s->count?(!strcmp(command,"TOP")?1U:s->count):0U;continue;}
        if(!strcmp(command,"PRINT")){
            first=s->count?1U:0U;last=s->count;
            if(*p){if(!number(&p,&first)||!first){out("EDIT: invalid PRINT range");continue;}last=first;
                while(*p==' ')++p;if(*p&&!number(&p,&last)){out("EDIT: invalid PRINT range");continue;}}
            while(*p==' ')++p;if(*p||first>last||last>s->count){out("EDIT: invalid PRINT range");continue;}
            show(s,first,last);continue;
        }
        if(!strcmp(command,"LOCATE")){
            n=(unsigned int)strlen(p);rc=4U;
            for(i=s->current;i<s->count;++i)if(!EDCFIND(&s->line[i],(unsigned char *)p,n,&at)){s->current=i+1U;rc=0U;break;}
            if(rc)for(i=0U;i<s->current&&i<s->count;++i)if(!EDCFIND(&s->line[i],(unsigned char *)p,n,&at)){s->current=i+1U;rc=0U;break;}
            if(rc)out("EDIT: literal not found");else show(s,s->current,s->current);continue;
        }
        if(!strcmp(command,"INPUT")){
            first=s->current;if(*p&&!number(&p,&first)){out("EDIT: invalid insertion position");continue;}
            while(*p==' ')++p;if(*p||first>s->count){out("EDIT: invalid insertion position");continue;}
            s->current=first;input_mode=1U;input_undo=0U;continue;
        }
        if(!strcmp(command,"UNDO")){
            if(!has_undo){out("EDIT: no change to undo");continue;}temp=s;s=undo;undo=temp;continue;
        }
        if(!strcmp(command,"SAVE")){
            if((p[0]!='A'&&p[0]!='a')||(p[1]!='S'&&p[1]!='s')||p[2]!=' '){out("EDIT: SAVE AS dataset");continue;}
            p+=3;while(*p==' ')++p;n=(unsigned int)strlen(p);
            if(!n||n>51U||strchr(p,' ')){out("EDIT: invalid destination");continue;}
            for(i=0U;i<n;++i)p[i]=(char)toupper((unsigned char)p[i]);
            for(i=0U;source[i];++i)b[i]=(char)toupper((unsigned char)source[i]);b[i]=0;
            if(!strcmp(b,p)){out("EDIT: SAVE AS needs a separate dataset");continue;}
            rc=TRIFILE(TRI_PROBE,p,0,0U,info);
            if(!rc)rc=EDCPACK(s,packed,TRI_LIMIT,word(info+20U),word(info+24U),&bytes);
            if(!rc)rc=TRIFILE(TRI_SAVE_EMPTY,p,packed,bytes,info);
            if(rc){sprintf(b,"EDIT: save refused/failed (%u); changes retained",rc);out(b);continue;}
            *saved=*s;sprintf(b,"EDIT: saved %u records to %s",word(info+32U),p);out(b);continue;
        }
        *candidate=*s;rc=8U;
        if(!strcmp(command,"INSERT")){
            if(number(&p,&first)&&*p==' '){++p;rc=EDCINSERT(candidate,first,(unsigned char *)p,(unsigned int)strlen(p));}
        }else if(!strcmp(command,"DELETE")){
            if(number(&p,&first)){last=first;while(*p==' ')++p;
                if(!*p||number(&p,&last)){while(*p==' ')++p;if(!*p)rc=EDCDELETE(candidate,first,last);}}
        }else if(!strcmp(command,"CHANGE")){
            if(number(&p,&first)){while(*p==' ')++p;
                if(*p=='/'){++p;end=strchr(p,'/');if(end){*end=0;replacement=end+1;end=strchr(replacement,'/');
                    if(end&&!end[1]){*end=0;rc=EDCCHANGE(candidate,first,(unsigned char *)p,(unsigned int)strlen(p),
                        (unsigned char *)replacement,(unsigned int)strlen(replacement));}}}}
        }
        if(rc){out("EDIT: command refused; HELP lists syntax and limits");continue;}
        *undo=*s;has_undo=1U;*s=*candidate;
    }
cleanup:
    free(s);free(undo);free(candidate);free(saved);free(packed);return result;
}
