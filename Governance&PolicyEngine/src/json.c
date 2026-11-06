#define _POSIX_C_SOURCE 200809L
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <sys/stat.h>

void die(const char *s) { fprintf(stderr,"ERROR: %s\n",s); exit(2); }
void *xmalloc(size_t n) { void *p=calloc(1,n?n:1); if(!p)die("out of memory"); return p; }
static void *grow(void *p,size_t n) { void *q=realloc(p,n); if(!q)die("out of memory"); return q; }
J *jnew(JType t) { J *j=xmalloc(sizeof *j); j->type=t; return j; }
J *js(const char *s) { J *j=jnew(J_STR); j->s=strdup(s?s:""); if(!j->s)die("out of memory"); return j; }
J *jn(double n) { if(!isfinite(n))die("nonfinite JSON number"); J *j=jnew(J_NUM);j->n=n;return j; }
J *jb(int b) { J *j=jnew(J_BOOL); j->n=!!b;return j; }
void jadd(J *a,J *v) { if(!a||a->type!=J_ARR)die("expected array"); a->v=grow(a->v,(a->len+1)*sizeof *a->v);a->v[a->len++]=v; }
void jset(J *o,const char *k,J *v) {
    if(!o||o->type!=J_OBJ)die("expected object");
    for(size_t i=0;i<o->len;i++)if(!strcmp(o->keys[i],k)){jfree(o->v[i]);o->v[i]=v;return;}
    o->v=grow(o->v,(o->len+1)*sizeof *o->v);o->keys=grow(o->keys,(o->len+1)*sizeof *o->keys);
    o->keys[o->len]=strdup(k);if(!o->keys[o->len])die("out of memory");o->v[o->len++]=v;
}
J *jget(const J *o,const char *k){if(o&&o->type==J_OBJ)for(size_t i=0;i<o->len;i++)if(!strcmp(k,o->keys[i]))return o->v[i];return NULL;}
const char *jstr(const J *o,const char *k){J *v=jget(o,k);return v&&v->type==J_STR?v->s:"";}
int jint(const J *o,const char *k,int fallback){J *v=jget(o,k);return v&&v->type==J_NUM&&v->n>=INT_MIN&&v->n<=INT_MAX&&floor(v->n)==v->n?(int)v->n:fallback;}
void jfree(J *j){if(!j)return;for(size_t i=0;i<j->len;i++){jfree(j->v[i]);if(j->keys)free(j->keys[i]);}free(j->keys);free(j->v);free(j->s);free(j);}

typedef struct { const unsigned char *p; int depth; } Parser;
static void ws(Parser *p){while(*p->p&&strchr(" \r\n\t",*p->p))p->p++;}
static unsigned hex4(Parser *p){unsigned x=0;for(int i=0;i<4;i++){unsigned c=*p->p++;if(c>='0'&&c<='9')c-='0';else if(c>='a'&&c<='f')c=c-'a'+10;else if(c>='A'&&c<='F')c=c-'A'+10;else die("invalid JSON unicode escape");x=16*x+c;}return x;}
static char *string(Parser *p){
    if(*p->p++!='"')die("expected JSON string");
    size_t cap=strlen((const char*)p->p)+1,len=0;char *s=xmalloc(cap);
    while(*p->p&&*p->p!='"'){
        unsigned c=*p->p++;if(c<32)die("control character in JSON string");
        if(c=='\\'){
            c=*p->p++;if(!c)die("truncated JSON escape");
            switch(c){case '"':case '\\':case '/':break;case 'b':c='\b';break;case 'f':c='\f';break;case 'n':c='\n';break;case 'r':c='\r';break;case 't':c='\t';break;
            case 'u':{
                if(strlen((const char*)p->p)<4)die("truncated unicode escape");
                c=hex4(p);
                if(c>=0xd800&&c<=0xdbff){if(strlen((const char*)p->p)<6||p->p[0]!='\\'||p->p[1]!='u')die("unpaired surrogate");p->p+=2;unsigned lo=hex4(p);if(lo<0xdc00||lo>0xdfff)die("bad surrogate");c=0x10000+((c-0xd800)<<10)+lo-0xdc00;}
                else if(c>=0xdc00&&c<=0xdfff)die("unpaired surrogate");
                if(!c)die("embedded NUL unsupported");
                if(c<0x80)s[len++]=(char)c;
                else if(c<0x800){s[len++]=(char)(0xc0|(c>>6));s[len++]=(char)(0x80|(c&63));}
                else if(c<0x10000){s[len++]=(char)(0xe0|(c>>12));s[len++]=(char)(0x80|((c>>6)&63));s[len++]=(char)(0x80|(c&63));}
                else{s[len++]=(char)(0xf0|(c>>18));s[len++]=(char)(0x80|((c>>12)&63));s[len++]=(char)(0x80|((c>>6)&63));s[len++]=(char)(0x80|(c&63));}
                continue;
            }default:die("invalid JSON escape");}
        }
        s[len++]=(char)c;
    }
    if(*p->p!='"')die("unterminated JSON string");
    p->p++;s[len]=0;return s;
}
static J *value(Parser *p){
    ws(p);if(++p->depth>32)die("JSON nesting too deep");J *j=NULL;
    unsigned c=*p->p;
    if(c=='{'||c=='['){
        p->p++;j=jnew(c=='{'?J_OBJ:J_ARR);ws(p);unsigned end=c=='{'?'}':']';
        if(*p->p!=end)for(;;){
            char *k=NULL;if(c=='{'){if(*p->p!='"')die("object key expected");k=string(p);if(jget(j,k))die("duplicate JSON key");ws(p);if(*p->p++!=':')die("colon expected");}
            J *v=value(p);if(k){jset(j,k,v);free(k);}else jadd(j,v);ws(p);
            if(*p->p!=',')break;
            p->p++;ws(p);
        }
        if(*p->p!=end)die("JSON closing bracket expected");
        p->p++;
    }else if(c=='"'){j=jnew(J_STR);j->s=string(p);}
    else if(!strncmp((const char*)p->p,"null",4)){p->p+=4;j=jnew(J_NULL);}
    else if(!strncmp((const char*)p->p,"true",4)){p->p+=4;j=jb(1);}
    else if(!strncmp((const char*)p->p,"false",5)){p->p+=5;j=jb(0);}
    else{
        const unsigned char *start=p->p;if(*p->p=='-')p->p++;
        if(*p->p=='0')p->p++;else{if(!isdigit(*p->p))die("JSON value expected");while(isdigit(*p->p))p->p++;}
        if(*p->p=='.'){p->p++;if(!isdigit(*p->p))die("invalid JSON fraction");while(isdigit(*p->p))p->p++;}
        if(*p->p=='e'||*p->p=='E'){p->p++;if(*p->p=='+'||*p->p=='-')p->p++;if(!isdigit(*p->p))die("invalid JSON exponent");while(isdigit(*p->p))p->p++;}
        char *end=NULL;double n=strtod((const char*)start,&end);if(end!=(const char*)p->p)die("invalid JSON number");j=jn(n);
    }
    p->depth--;return j;
}
J *jparse(const char *s){Parser p={(const unsigned char*)s,0};J *j=value(&p);ws(&p);if(*p.p)die("trailing JSON data");return j;}
J *jload(const char *path){FILE *f=fopen(path,"rb");if(!f){perror(path);die("cannot open JSON file");}if(fseek(f,0,SEEK_END))die("seek failed");long n=ftell(f);if(n<0||n>1024*1024)die("JSON file exceeds 1 MiB limit");rewind(f);char *s=xmalloc((size_t)n+1);if(fread(s,1,(size_t)n,f)!=(size_t)n)die("read failed");fclose(f);if(memchr(s,0,(size_t)n))die("NUL in input");J *j=jparse(s);free(s);return j;}

typedef struct {char *s;size_t n,cap;} Buf;
static void put(Buf *b,const char *s){size_t n=strlen(s);if(b->n+n+1>b->cap){b->cap=(b->n+n+1)*2;b->s=grow(b->s,b->cap);}memcpy(b->s+b->n,s,n+1);b->n+=n;}
static void quote(Buf *b,const char *s){put(b,"\"");for(const unsigned char *p=(const unsigned char*)s;*p;p++){char t[8];if(*p=='"'||*p=='\\'){t[0]='\\';t[1]=(char)*p;t[2]=0;}else if(*p<32)snprintf(t,sizeof t,"\\u%04x",*p);else{t[0]=(char)*p;t[1]=0;}put(b,t);}put(b,"\"");}
static void dump(Buf *b,const J *j){char num[40];switch(j->type){case J_NULL:put(b,"null");break;case J_BOOL:put(b,j->n?"true":"false");break;case J_NUM:snprintf(num,sizeof num,"%.17g",j->n);put(b,num);break;case J_STR:quote(b,j->s);break;case J_ARR:case J_OBJ:put(b,j->type==J_OBJ?"{":"[");for(size_t i=0;i<j->len;i++){if(i)put(b,",");if(j->type==J_OBJ){quote(b,j->keys[i]);put(b,":");}dump(b,j->v[i]);}put(b,j->type==J_OBJ?"}":"]");break;}}
char *jdump(const J *j){Buf b={0};dump(&b,j);return b.s;}
J *jcopy(const J *j){char *s=jdump(j);J *v=jparse(s);free(s);return v;}
void jsave(const char *p,const J *j){char *s=jdump(j);FILE *f=fopen(p,"wb");if(!f){perror(p);die("cannot write JSON file");}if(chmod(p,0600))die("cannot restrict file permissions");if(fputs(s,f)==EOF||fputc('\n',f)==EOF||fclose(f))die("JSON write failed");free(s);}
