/* SPDX-License-Identifier: MIT; cataloged procedure policy in the U converter.
 * Included once by twospace_jcl.c to share its checked lexical primitives. */
static int symbol_value(TJCWORK *w,const char *key,const char *value,TJCERROR *e)
{
 unsigned int i;if(!ident(key,8U,1)||strlen(value)>100U)return fail(w,e,TJC_SYMBOL,0U);
 for(i=0U;i<w->symbols&&strcmp(key,w->symbol[i].name);++i){}
 if(i==TJC_SYMBOLS)return fail(w,e,TJC_LIMIT,0U);if(i==w->symbols)++w->symbols;
 strcpy(w->symbol[i].name,key);strcpy(w->symbol[i].value,value);return 0;
}
static void outer_symbols(TJCWORK *w)
{
 unsigned int i,n=w->symbols;TJCSYM tmp;
 for(i=0U;i<TJC_SYMBOLS;++i){tmp=w->symbol[i];w->symbol[i]=w->proc.outer[i];w->proc.outer[i]=tmp;}
 w->symbols=w->proc.outer_count;w->proc.outer_count=n;
}
static int execution_kind(TJCWORK *w,unsigned int *is_program,TJCERROR *e)
{
 char key[18],value[TJC_TEXT];unsigned int at=0U,where=0U,pgm=0U,proc=0U;int rc;
 while((rc=operand(w,&at,key,value,&where,e))==0){if(!strcmp(key,"PGM"))++pgm;
  else if(!strcmp(key,"PROC")||(!key[0]&&!where))++proc;}
 if(rc!=1)return rc;if(pgm>1U||proc>1U||(pgm&&proc))return fail(w,e,TJC_SYNTAX,0U);
 *is_program=pgm;return 0;
}
static int procedure_libraries(TJCWORK *w,TJCERROR *e)
{
 char key[18],value[TJC_TEXT],*p,*comma;unsigned int at=0U,where=0U,n;TJCDECL d;int rc;
 if(w->result.steps||w->proclibs)return fail(w,e,TJC_SYNTAX,0U);
 rc=operand(w,&at,key,value,&where,e);if(rc||strcmp(key,"ORDER")||w->expanded[at])return fail(w,e,TJC_SYNTAX,0U);
 n=(unsigned int)strlen(value);p=value;
 if(n&&value[0]=='('){if(n<3U||value[n-1U]!=')')return fail(w,e,TJC_SYNTAX,where);value[n-1U]=0;++p;}
 while(*p){comma=strchr(p,',');if(comma)*comma=0;
  memset(&d,0,sizeof d);if(!dataset(p,&d)||d.kind!=TJD_DATASET||d.member[0])return fail(w,e,TJC_SYNTAX,where);
  if(w->proclibs==TJC_LIBS)return fail(w,e,TJC_LIMIT,where);strcpy(w->proclib[w->proclibs++].dsn,d.dsn);
  if(!comma)break;p=comma+1;if(!*p)return fail(w,e,TJC_SYNTAX,where);
 }return w->proclibs?0:fail(w,e,TJC_SYNTAX,where);
}
/* Capture exact caller DD spans, including complete explicitly delimited data.
 * Nothing is bound or executed; the declarations are merged later in PROC order. */
static int collect_overrides(TJCWORK *w,const unsigned char *deck,unsigned int cards,
                              unsigned int *next,TJCERROR *e)
{
 unsigned int first,pos,at,where,data,i,found;char key[18],value[TJC_TEXT],dlm[3],*dot;
 unsigned char delimiter[2];TJCOVR *o,*previous=0;int rc;
 while(*next<cards){first=*next;pos=first;rc=statement(w,deck,cards,&pos,e);if(rc)return rc;
  if(w->ended||strcmp(w->operation,"DD")){w->ended=0U;return 0;}
  if(w->proc.overrides==TJD_MAX_COUNT)return fail(w,e,TJC_LIMIT,0U);
  o=&w->proc.override[w->proc.overrides];o->first=first;o->next=pos;
  if(w->name[0]){dot=strchr(w->name,'.');if(!dot)return fail(w,e,TJC_SYNTAX,0U);*dot=0;
   if(!ident(w->name,8U,1)||!ident(dot+1,8U,1))return fail(w,e,TJC_SYNTAX,0U);
   strcpy(o->step,w->name);strcpy(o->name,dot+1);
   for(i=0U;i<w->proc.overrides;++i)if(!strcmp(o->step,w->proc.override[i].step)&&!strcmp(o->name,w->proc.override[i].name))return fail(w,e,TJC_SYNTAX,0U);
  }else{if(!previous||previous->ordinal==3U)return fail(w,e,TJC_SYNTAX,0U);
   strcpy(o->step,previous->step);strcpy(o->name,previous->name);o->ordinal=previous->ordinal+1U;}
  rc=substitute(w,e);if(rc)return rc;at=0U;data=0U;strcpy(dlm,"/*");
  while((rc=operand(w,&at,key,value,&where,e))==0){
   if(!key[0]&&(!strcmp(value,"*")||!strcmp(value,"DATA")))data=!strcmp(value,"*")?1U:2U;
   if(!strcmp(key,"DLM")){if(!unquote(value)||strlen(value)!=2U||strchr(value,'&'))return fail(w,e,TJC_FEATURE,where);strcpy(dlm,value);}}
  if(rc!=1)return rc;
  if(data){TJCENC(dlm,2U,delimiter);found=0U;
   while(pos<cards){const unsigned char *p=deck+80U*pos;++pos;
    if(p[0]==delimiter[0]&&p[1]==delimiter[1]){for(i=2U;i<80U&&p[i]==0x40U;++i){}if(i==80U){found=1U;break;}}
    if(data==1U&&p[0]==0x61U&&p[1]==0x61U)return fail(w,e,TJC_END,0U);}
   if(!found)return fail(w,e,TJC_END,0U);o->next=pos;
  }
  previous=o;++w->proc.overrides;*next=pos;
 }return fail(w,e,TJC_END,0U);
}
static int field_put(TJCWORK *w,const char *key,const char *value,unsigned int phase,
                       unsigned int card,unsigned int column,TJCERROR *e)
{
 unsigned int i;if(strlen(key)>17U||strlen(value)>255U)return fail(w,e,TJC_LIMIT,0U);
 for(i=0U;i<w->fields&&strcmp(w->field[i].key,key);++i){}
 if(i==16U)return fail(w,e,TJC_LIMIT,0U);
 if(i==w->fields)++w->fields;else if(w->field[i].phase&phase)return fail(w,e,TJC_SYNTAX,0U);
 strcpy(w->field[i].key,key);strcpy(w->field[i].value,value);w->field[i].phase|=phase;
 w->field[i].card=card;w->field[i].column=column;return 0;
}
static void remove_field(TJCWORK *w,const char *key)
{unsigned int i;for(i=0U;i<w->fields;++i)if(!strcmp(key,w->field[i].key)){w->field[i].key[0]=0;return;}}
static int merge_fields(TJCWORK *w,unsigned int phase,TJCERROR *e)
{
 char key[18],value[TJC_TEXT],part[18],*p,*end,*eq;
 unsigned int at=0U,where=0U,n,card,column;int rc;
 while((rc=operand(w,&at,key,value,&where,e))==0){
  card=w->expanded_card[where];column=w->expanded_column[where];
  if(!strcmp(key,"DSNAME"))strcpy(key,"DSN");if(!strcmp(key,"VOLUME"))strcpy(key,"VOL");
  if(!key[0])strcpy(key,"@");
  if(phase==2U){
   if(!strcmp(key,"DSN")){remove_field(w,"@");remove_field(w,"SYSOUT");}
   else if(!strcmp(key,"@")||!strcmp(key,"SYSOUT")){
    if(!strcmp(key,"@"))remove_field(w,"SYSOUT");else remove_field(w,"@");
    remove_field(w,"DSN");remove_field(w,"DISP");remove_field(w,"UNIT");remove_field(w,"VOL");remove_field(w,"SPACE");}}
  if(!strcmp(key,"DCB")){
   n=(unsigned int)strlen(value);if(n<3U||value[0]!='('||value[n-1U]!=')')return fail(w,e,TJC_SYNTAX,where);
   value[n-1U]=0;p=value+1;
   while(*p){end=strchr(p,',');if(end)*end=0;eq=strchr(p,'=');if(!eq)return fail(w,e,TJC_SYNTAX,where);*eq++=0;
    if(strlen(p)>8U)return fail(w,e,TJC_FEATURE,where);strcpy(part,"DCB.");strcat(part,p);
    rc=field_put(w,part,eq,phase,card,column,e);if(rc)return rc;
    if(!end)break;p=end+1;if(!*p)return fail(w,e,TJC_SYNTAX,where);}
  }else {rc=field_put(w,key,value,phase,card,column,e);if(rc)return rc;}
 }
 return rc==1?0:rc;
}
static int emit_text(TJCWORK *w,unsigned int *used,const char *p,const TJCFIELD *f,TJCERROR *e)
{
 unsigned int i;for(i=0U;p[i];++i){if(*used==TJC_TEXT-1U)return fail(w,e,TJC_LIMIT,0U);
  w->expanded[*used]=p[i];w->expanded_card[*used]=(unsigned short)f->card;
  w->expanded_column[(*used)++]=(unsigned short)f->column;}return 0;
}
static int merged_text(TJCWORK *w,TJCERROR *e)
{
 unsigned int i,n=0U,dcb_started=0U;int rc;
 memset(w->expanded_card,0,sizeof w->expanded_card);
 for(i=0U;i<w->fields;++i){TJCFIELD *f=&w->field[i];if(!f->key[0]||!strncmp(f->key,"DCB.",4U))continue;
  if(n&&(rc=emit_text(w,&n,",",f,e)))return rc;
  if(strcmp(f->key,"@")){if((rc=emit_text(w,&n,f->key,f,e))||(rc=emit_text(w,&n,"=",f,e)))return rc;}
  if((rc=emit_text(w,&n,f->value,f,e)))return rc;
 }
 for(i=0U;i<w->fields;++i){TJCFIELD *f=&w->field[i];if(strncmp(f->key,"DCB.",4U))continue;
  if(!dcb_started){if(n&&(rc=emit_text(w,&n,",",f,e)))return rc;if((rc=emit_text(w,&n,"DCB=(",f,e)))return rc;dcb_started=1U;}
  else if((rc=emit_text(w,&n,",",f,e)))return rc;
  if((rc=emit_text(w,&n,f->key+4U,f,e))||(rc=emit_text(w,&n,"=",f,e))||(rc=emit_text(w,&n,f->value,f,e)))return rc;
 }
 if(dcb_started&&(rc=emit_text(w,&n,")",&w->field[0],e)))return rc;w->expanded[n]=0;return 0;
}
static int override_group(TJCWORK *w,const unsigned char *jobdeck,unsigned int jobcards,
                           unsigned int base[4],unsigned int bases,const char *name,
                           unsigned int override,unsigned int overrides,TJCERROR *e)
{
 unsigned int i,n=bases>overrides?bases:overrides,pos=0U,source_cards=0U;const unsigned char *source=0;int rc;
 if(overrides>1U&&overrides<bases)return fail(w,e,TJC_FEATURE,0U);
 if(overrides){if(override<w->proc.last_override)return fail(w,e,TJC_SYNTAX,0U);w->proc.last_override=override+overrides;}
 for(i=0U;i<n;++i){w->fields=0U;memset(w->field,0,sizeof w->field);
  if(i<bases){pos=base[i];source=w->proc.deck;source_cards=w->proc.cards;
   rc=statement(w,source,source_cards,&pos,e);if(rc)return rc;rc=substitute(w,e);if(rc)return rc;
   rc=merge_fields(w,1U,e);if(rc)return rc;
   {unsigned int f;for(f=0U;f<w->fields;++f)if(!strcmp(w->field[f].key,"@")&&
     (!strcmp(w->field[f].value,"*")||!strcmp(w->field[f].value,"DATA")))return fail(w,e,TJC_FEATURE,0U);}
  }
  if(i<overrides){TJCOVR *o=&w->proc.override[override+i];source=jobdeck;source_cards=jobcards;pos=o->first;
   rc=statement(w,source,source_cards,&pos,e);if(rc)return rc;outer_symbols(w);rc=substitute(w,e);outer_symbols(w);if(rc)return rc;
   rc=merge_fields(w,2U,e);if(rc)return rc;o->used=1U;
  }
  if(!w->fields)return fail(w,e,TJC_SYNTAX,0U);
  rc=merged_text(w,e);if(rc)return rc;if(i)w->name[0]=0;else strcpy(w->name,name);
  rc=dd(w,source,source_cards,&pos,e);if(rc)return rc;
  if(i<overrides&&pos!=w->proc.override[override+i].next)return fail(w,e,TJC_END,0U);
 }
 return 0;
}
static int proc_dds(TJCWORK *w,const unsigned char *jobdeck,unsigned int jobcards,
                     unsigned int first,TJCERROR *e)
{
 unsigned int base[4],count=1U,pos,first_next,i,over=0U,overs=0U,at=0U,where=0U;char name[9],step[9],key[18],value[TJC_TEXT];int rc;
 if(!ident(w->name,8U,1)||!w->result.steps)return fail(w,e,TJC_SYNTAX,0U);
 strcpy(name,w->name);strcpy(step,w->result.step[w->result.steps-1U].local);base[0]=first;
 while((rc=operand(w,&at,key,value,&where,e))==0)if(!key[0]&&(!strcmp(value,"*")||!strcmp(value,"DATA")))return fail(w,e,TJC_FEATURE,where);
 if(rc!=1)return rc;
 while(w->proc.next<w->proc.cards){first_next=w->proc.next;pos=first_next;
  rc=statement(w,w->proc.deck,w->proc.cards,&pos,e);if(rc)return rc;
  if(w->ended)return fail(w,e,TJC_END,0U);
  if(strcmp(w->operation,"DD")||w->name[0])break;
  if(count==4U)return fail(w,e,TJC_LIMIT,0U);base[count++]=first_next;w->proc.next=pos;
 }
 for(i=0U;i<w->proc.overrides;++i){TJCOVR *o=&w->proc.override[i];
  if(!o->ordinal&&!strcmp(o->step,step)&&!strcmp(o->name,name)){over=i;overs=1U;
   while(i+overs<w->proc.overrides&&w->proc.override[i+overs].ordinal)++overs;break;}}
 return override_group(w,jobdeck,jobcards,base,count,name,over,overs,e);
}
static int added_dds(TJCWORK *w,const unsigned char *deck,unsigned int cards,TJCERROR *e)
{
 unsigned int i,n,base[4];int rc;const char *step;
 if(w->result.steps==w->proc.first_step)return 0;step=w->result.step[w->result.steps-1U].local;
 for(i=0U;i<w->proc.overrides;++i){TJCOVR *o=&w->proc.override[i];
  if(o->used||o->ordinal||strcmp(o->step,step))continue;n=1U;
  while(i+n<w->proc.overrides&&w->proc.override[i+n].ordinal)++n;
  rc=override_group(w,deck,cards,base,0U,o->name,i,n,e);if(rc)return rc;
 }return 0;
}
static int proc_exec(TJCWORK *w,const unsigned char *deck,unsigned int cards,
                      unsigned int *next,const TJCSITE *site,TJCERROR *e)
{
 char key[18],value[TJC_TEXT],defaults[TJC_TEXT],default_name[TJC_SYMBOLS][9];
 unsigned short dc[TJC_TEXT],dx[TJC_TEXT];
 unsigned int at=0U,where=0U,i,j,n,bytes=0U,pos,pgm,declared=0U,seen=0U,default_count=0U,caller=w->card;
 unsigned char hash[32],*m=w->result.job+TJB_DESC_BYTES;int rc;
 TJCPROC *p=&w->proc;
 if(p->active||!ident(w->name,8U,1))return fail(w,e,TJC_FEATURE,0U);
 memset(p,0,sizeof *p);strcpy(p->call,w->name);p->first_step=w->result.steps;
 memcpy(p->outer,w->symbol,sizeof p->outer);p->outer_count=w->symbols;
 while((rc=operand(w,&at,key,value,&where,e))==0){
  if(!strcmp(key,"PROC")||(!key[0]&&!where)){
   if(p->procedure[0]||!ident(value,8U,1))return fail(w,e,TJC_SYNTAX,where);strcpy(p->procedure,value);
  }else if(!strcmp(key,"PARM")||!strncmp(key,"PARM.",5U)){
   if(!parameter(value)||strlen(value)>100U)return fail(w,e,TJC_LIMIT,where);
   if(!strcmp(key,"PARM")){if(p->parameter_set||p->parms)return fail(w,e,TJC_FEATURE,where);p->parameter_set=1U;strcpy(p->parameter,value);}
   else {if(p->parameter_set||p->parms==TJB_STEPS||!ident(key+5U,8U,1))return fail(w,e,TJC_FEATURE,where);
    for(i=0U;i<p->parms;++i)if(!strcmp(key+5U,p->parm_step[i]))return fail(w,e,TJC_SYNTAX,where);
    strcpy(p->parm_step[p->parms],key+5U);strcpy(p->parm_value[p->parms++],value);}
  }else if(!strcmp(key,"REGION")){if(!region(value))return fail(w,e,TJC_FEATURE,where);}
  else {
   if(!ident(key,8U,1)||!unquote(value)||strlen(value)>100U)return fail(w,e,TJC_FEATURE,where);
   if(p->arguments==TJC_SYMBOLS)return fail(w,e,TJC_LIMIT,where);
   for(i=0U;i<p->arguments;++i)if(!strcmp(key,p->argument[i].name))return fail(w,e,TJC_SYNTAX,where);
   strcpy(p->argument[p->arguments].name,key);strcpy(p->argument[p->arguments++].value,value);
  }
 }
 if(rc!=1)return rc;if(!p->procedure[0])return fail(w,e,TJC_SYNTAX,0U);
 if(!site->procedure||site->procedure(site->context,p->procedure,w->proclib,w->proclibs,&p->deck,&bytes)||
    !p->deck||!bytes||bytes%80U)return fail(w,e,TJC_RESOLVE,0U);
 p->cards=bytes/80U;if(p->cards>TJC_CARDS)return fail(w,e,TJC_LIMIT,0U);
 rc=TJJHASH(p->deck,bytes,hash);if(rc)return fail(w,e,TJC_LIMIT,0U);
 n=TJBWORD(m+224U);for(i=0U;i<n;++i){unsigned char encoded[8];native_name(encoded,8U,p->procedure);
  if(!memcmp(encoded,m+232U+8U*i,8U)){if(memcmp(hash,m+264U+32U*i,32U))return fail(w,e,TJC_RESOLVE,0U);break;}}
 if(i==n){if(n==TJC_PROCS)return fail(w,e,TJC_LIMIT,0U);native_name(m+232U+8U*n,8U,p->procedure);memcpy(m+264U+32U*n,hash,32U);TJBPUT(m+224U,n+1U);}p->index=i+1U;
 rc=collect_overrides(w,deck,cards,next,e);if(rc)return rc;
 for(i=0U;i<p->arguments;++i){rc=symbol_value(w,p->argument[i].name,p->argument[i].value,e);if(rc)return rc;}
 pos=0U;rc=statement(w,p->deck,p->cards,&pos,e);if(rc)return rc;
 if(strcmp(w->operation,"PROC")||(w->name[0]&&strcmp(w->name,p->procedure)))return fail(w,e,TJC_FEATURE,0U);
 p->next=pos;strcpy(defaults,w->text);memcpy(dc,w->source_card,sizeof dc);memcpy(dx,w->source_column,sizeof dx);at=0U;
 while(1){strcpy(w->expanded,defaults);memcpy(w->expanded_card,dc,sizeof dc);memcpy(w->expanded_column,dx,sizeof dx);
  rc=operand(w,&at,key,value,&where,e);if(rc==1)break;if(rc)return rc;
  if(!ident(key,8U,1)||!unquote(value))return fail(w,e,TJC_SYMBOL,where);
  for(i=0U;i<default_count;++i)if(!strcmp(key,default_name[i]))return fail(w,e,TJC_FEATURE,where);
  if(default_count==TJC_SYMBOLS)return fail(w,e,TJC_LIMIT,where);strcpy(default_name[default_count++],key);
  for(i=0U;i<p->arguments;++i)if(!strcmp(key,p->argument[i].name))break;
  if(i<p->arguments){declared|=1U<<i;continue;}
  /* A PROC default precedes an outer SET value; EXEC symbols precede both. */
  strcpy(w->text,value);for(j=0U;j<TJC_TEXT;++j){w->source_card[j]=dc[where];w->source_column[j]=dx[where];}
  rc=substitute(w,e);if(rc)return rc;rc=symbol_value(w,key,w->expanded,e);if(rc)return rc;
 }
 if(declared!=(p->arguments==32U?0xffffffffU:(1U<<p->arguments)-1U))return fail(w,e,TJC_SYMBOL,0U);
 p->active=1U;
 while(p->next<p->cards){pos=p->next;rc=statement(w,p->deck,p->cards,&p->next,e);if(rc)return rc;
  if(w->ended)return fail(w,e,TJC_END,0U);rc=substitute(w,e);if(rc)return rc;
  if(!strcmp(w->operation,"EXEC")){
   rc=added_dds(w,deck,cards,e);if(rc)return rc;
   /* added_dds reuses the lexical buffer; reread this EXEC. */
   j=pos;rc=statement(w,p->deck,p->cards,&j,e);if(rc)return rc;rc=substitute(w,e);if(rc)return rc;
   w->last_library=0U;rc=execution_kind(w,&pgm,e);if(rc)return rc;if(!pgm)return fail(w,e,TJC_FEATURE,0U);rc=exec_step(w,e);
  }else if(!strcmp(w->operation,"DD"))rc=proc_dds(w,deck,cards,pos,e);
  else if(!strcmp(w->operation,"PEND")){
   if(w->name[0]||w->expanded[0]||p->next!=p->cards||w->result.steps==p->first_step)return fail(w,e,TJC_SYNTAX,0U);
   rc=added_dds(w,deck,cards,e);if(rc)return rc;seen=1U;break;
  }else return fail(w,e,TJC_FEATURE,0U);
  if(rc)return rc;
 }
 if(!seen)return fail(w,e,TJC_END,0U);
 for(i=0U;i<p->overrides;++i)if(!p->override[i].used)return fail(w,e,TJC_SYNTAX,0U);
 if((p->parameter_set&&!p->parameter_used)||p->parm_used!=((1U<<p->parms)-1U))return fail(w,e,TJC_SYNTAX,0U);
 memcpy(w->symbol,p->outer,sizeof w->symbol);w->symbols=p->outer_count;p->active=0U;w->last_library=0U;w->card=caller;
 return 0;
}
