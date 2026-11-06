#define _GNU_SOURCE
#include "nerp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

static void random_bytes(unsigned char *p,int n){if(RAND_bytes(p,n)!=1)die("cryptographic RNG failed");}
static char *hex(const unsigned char *p,size_t n){static const char d[]="0123456789abcdef";char *s=xmalloc(2*n+1);for(size_t i=0;i<n;i++){s[2*i]=d[p[i]>>4];s[2*i+1]=d[p[i]&15];}return s;}
static unsigned char *unhex(const char *s,size_t *n){*n=strlen(s)/2;if(strlen(s)%2)die("odd hex length");unsigned char *p=xmalloc(*n);for(size_t i=0;i<*n;i++){unsigned v=0;for(int j=0;j<2;j++){char c=s[2*i+j];if(c>='0'&&c<='9')v=v*16+c-'0';else if(c>='a'&&c<='f')v=v*16+c-'a'+10;else die("invalid hexadecimal");}p[i]=(unsigned char)v;}return p;}
static char *mac(const unsigned char *key,const char *s){unsigned char md[32];unsigned int len=0;if(!HMAC(EVP_sha256(),key,32,(const unsigned char*)s,strlen(s),md,&len)||len!=32)die("HMAC failed");return hex(md,32);}
static void key_write(const char *p,unsigned char key[32]){int fd=open(p,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)die("cannot create new secret key");if(write(fd,key,32)!=32||fsync(fd)||close(fd))die("key persistence failed");}
static int key_read(const char *p,unsigned char key[32]){int fd=open(p,O_RDONLY|O_NOFOLLOW);if(fd<0)return 0;struct stat st;if(fstat(fd,&st)||!S_ISREG(st.st_mode)||(st.st_mode&077)||st.st_size!=32){close(fd);die("key file must be regular, private and 32 bytes");}int ok=read(fd,key,32)==32;close(fd);return ok;}
static void key_dispose(const char *p){
    int fd=open(p,O_WRONLY|O_NOFOLLOW);
    if(fd<0)die("cannot open key for disposal");
    unsigned char zeros[32]={0};struct stat st;
    if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size!=32)die("unsafe key disposal target");
    if(write(fd,zeros,32)!=32||fsync(fd)||close(fd))die("key overwrite failed");
    if(unlink(p))die("key unlink failed");
    /* This only disposes of the live managed key; it makes no SSD/COW/backup guarantee. */
}

static J *encrypt(const unsigned char key[32],const char *plain,const char *aad){
    unsigned char iv[12],tag[16];random_bytes(iv,12);size_t n=strlen(plain);unsigned char *cipher=xmalloc(n+16);int z=0,total=0;
    EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();if(!ctx)die("cipher allocation failed");
    if(EVP_EncryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,key,iv)!=1||EVP_EncryptUpdate(ctx,NULL,&z,(const unsigned char*)aad,(int)strlen(aad))!=1||EVP_EncryptUpdate(ctx,cipher,&z,(const unsigned char*)plain,(int)n)!=1)die("encryption failed");
    total=z;
    if(EVP_EncryptFinal_ex(ctx,cipher+total,&z)!=1||EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_GET_TAG,16,tag)!=1)die("encryption finalisation failed");
    total+=z;EVP_CIPHER_CTX_free(ctx);
    char *a=hex(iv,12),*b=hex(tag,16),*c=hex(cipher,(size_t)total);J *o=jnew(J_OBJ);jset(o,"algorithm",js("AES-256-GCM"));jset(o,"iv",js(a));jset(o,"tag",js(b));jset(o,"ciphertext",js(c));free(a);free(b);free(c);free(cipher);return o;
}
static char *decrypt(const unsigned char key[32],J *o,const char *aad){
    size_t ni,nt,nc;unsigned char *iv=unhex(jstr(o,"iv"),&ni),*tag=unhex(jstr(o,"tag"),&nt),*cipher=unhex(jstr(o,"ciphertext"),&nc);
    if(ni!=12||nt!=16||nc>1024*1024)die("invalid encrypted envelope");
    char *p=xmalloc(nc+1);EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();int z=0,total=0,ok=ctx!=NULL;
    if(ok)ok=EVP_DecryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,key,iv)==1&&EVP_DecryptUpdate(ctx,NULL,&z,(const unsigned char*)aad,(int)strlen(aad))==1&&EVP_DecryptUpdate(ctx,(unsigned char*)p,&z,cipher,(int)nc)==1;
    total=z;if(ok)ok=EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,16,tag)==1&&EVP_DecryptFinal_ex(ctx,(unsigned char*)p+total,&z)==1;
    EVP_CIPHER_CTX_free(ctx);free(iv);free(tag);free(cipher);if(!ok){OPENSSL_cleanse(p,nc+1);free(p);return NULL;}p[total+z]=0;return p;
}
long day_number(const char *date){
    int y,m,d;char tail;if(strlen(date)!=10||sscanf(date,"%4d-%2d-%2d%c",&y,&m,&d,&tail)!=3||y<1970||y>2200||date[4]!='-'||date[7]!='-')die("date must be YYYY-MM-DD, 1970..2200");
    struct tm t={0};t.tm_year=y-1900;t.tm_mon=m-1;t.tm_mday=d;time_t v=timegm(&t);
    if(t.tm_year!=y-1900||t.tm_mon!=m-1||t.tm_mday!=d)die("invalid calendar date");
    return (long)(v/86400);
}
static void pseudonym(J *out,const char *field,const char *text,unsigned char key[32]){char *s=mac(key,text);jset(out,field,js(s));free(s);}
static void field(J *o,J *in,const char *key){J *v=jget(in,key);if(v)jset(o,key,jcopy(v));}
static const char *casualty_band(int n){return n==0?"0":n<=2?"1-2":"3+";}
static const char *date_of(Nerp *n,Record *r,int *assumed){
    *assumed=0;const char *s=jstr(r->body,"timestamp");
    if(!*s&&r->kind==3){Record *i=find_record(n,"incidents",jstr(r->body,"incidentId"));if(i)s=jstr(i->body,"timestamp");}
    if(!*s){*assumed=1;s=jstr(n->config,"fixture_ingestion_date");}return s;
}
static int minutes(const char *s){int h,m;char c;if(sscanf(s,"%d:%d%c",&h,&m,&c)!=2||h<0||h>23||m<0||m>59)return -1;return h*60+m;}
void generate_views(Nerp *n,const char *out,Subject s){
    unsigned char key[32];random_bytes(key,32);char p[1024];path_join(p,sizeof p,out,"analytics.key");key_write(p,key);
    J *op=jnew(J_OBJ),*an=jnew(J_OBJ),*research=jnew(J_OBJ),*access=jnew(J_ARR),*flow=jnew(J_OBJ);
    J *inc=jnew(J_ARR),*dispatch=jnew(J_ARR),*roles=jnew(J_OBJ),*actions=jnew(J_OBJ);
    for(int k=0;k<KINDS;k++)jset(op,kinds[k],jnew(J_ARR));
    int group_n[3]={0},group_c[3]={0};const char *agencies[3]={"Police","Ambulance","Fire"};
    for(size_t i=0;i<n->count;i++){
        Record *r=&n->records[i];J *d=decision(n,s,r,"read");int permit=allowed(d);jadd(access,d);
        if(permit){
            J *v=jnew(J_OBJ);
            if(r->kind==0){const char *f[]={"incidentId","timestamp","location","incidentType","casualties","priority","assignedAgency"};for(size_t a=0;a<7;a++)field(v,r->body,f[a]);}
            else if(r->kind==1){const char *f[]={"responderId","role","station","currentAssignment"};for(size_t a=0;a<4;a++)field(v,r->body,f[a]);}
            else if(r->kind==2){const char *f[]={"reportId","region","threatLevel","summary"};for(size_t a=0;a<4;a++)field(v,r->body,f[a]);}
            else if(r->kind==3){const char *f[]={"dispatchId","incidentId","unit","departureTime","arrivalTime","status"};for(size_t a=0;a<6;a++)field(v,r->body,f[a]);}
            else{const char *f[]={"eventId","actor","action","resource","timestamp"};for(size_t a=0;a<5;a++)field(v,r->body,f[a]);}
            jset(v,"classification",js(conf_names[r->conf]));jadd(jget(op,kinds[r->kind]),v);
        }
        /* Explicit allowlists, not a fragile list of fields to remove. */
        if(r->review||r->conf>1)continue;
        if(r->kind==0){
            J *v=jnew(J_OBJ);pseudonym(v,"incidentToken",record_id(r),key);field(v,r->body,"assignedAgency");field(v,r->body,"priority");jset(v,"casualtyBand",js(casualty_band(jint(r->body,"casualties",0))));
            char month[8]={0};strncpy(month,jstr(r->body,"timestamp"),7);jset(v,"month",js(month));jadd(inc,v);
            for(int a=0;a<3;a++)if(!strcmp(agencies[a],jstr(r->body,"assignedAgency"))){group_n[a]++;group_c[a]+=jint(r->body,"casualties",0);}
        }else if(r->kind==3){
            J *v=jnew(J_OBJ);pseudonym(v,"incidentToken",jstr(r->body,"incidentId"),key);field(v,r->body,"status");
            int start=minutes(jstr(r->body,"departureTime")),end=minutes(jstr(r->body,"arrivalTime"));
            /* Missing/overnight ambiguous time never becomes a fabricated duration. */
            if(start>=0&&end>=start)jset(v,"responseMinutes",jn(end-start));else jset(v,"responseMinutes",jnew(J_NULL));jadd(dispatch,v);
        }else if(r->kind==1){const char *role=jstr(r->body,"role");jset(roles,role,jn(jint(roles,role,0)+1));}
        else if(r->kind==4){const char *action=jstr(r->body,"action");jset(actions,action,jn(jint(actions,action,0)+1));}
    }
    jset(an,"classification",js("OFFICIAL"));jset(an,"incidents",inc);jset(an,"dispatch",dispatch);jset(an,"responderRoleCounts",roles);jset(an,"auditActionCounts",actions);
    jset(an,"intelligence",jnew(J_ARR));jset(an,"note",js("All raw intelligence excluded. Tokens are pseudonyms, not anonymity. Release requires trusted governance approval."));
    int k=jint(n->config,"research_k",3);if(k<3)die("research_k must be at least 3");J *groups=jnew(J_ARR);
    for(int a=0;a<3;a++)if(group_n[a]>=k){J *g=jnew(J_OBJ);jset(g,"agency",js(agencies[a]));jset(g,"incidentCount",jn(group_n[a]));jset(g,"casualtiesRoundedTo5",jn(((group_c[a]+2)/5)*5));jadd(groups,g);}
    jset(research,"classification",js("OFFICIAL"));jset(research,"minimumCellSize",jn(k));jset(research,"groups",groups);jset(research,"releaseStatus",js("RESTRICTED_REVIEW_REQUIRED"));jset(research,"note",js("Aggregate anonymisation candidate; no row IDs, times, places or free text. Not proof against linkage or differencing."));
    jset(flow,"ordinaryOperationalAccess",js("ACM AND assignment/agency scope AND BLP AND Biba AND not quarantined"));jset(flow,"analyticsAndResearch",js("Explicit trusted sanitisation/declassification path; never an ordinary BLP write-down exception. Restricted OFFICIAL outputs pending human approval."));
    jset(flow,"sourceFilter",js("Only OFFICIAL records enter analytics/research. SECRET/TOP SECRET and CONFIDENTIAL inputs excluded. All raw intelligence excluded."));
    jset(flow,"integrity",js("Transformation never endorses data: output integrity UNVERIFIED; a separate validated endorsement is required for operational use."));
    jset(an,"integrity",js("UNVERIFIED"));jset(research,"integrity",js("UNVERIFIED"));
    write_at(out,"operational_view.json",op);write_at(out,"operational_decisions.json",access);write_at(out,"analytics_view.json",an);write_at(out,"research_view.json",research);write_at(out,"information_flow.json",flow);
    jfree(op);jfree(access);jfree(an);jfree(research);jfree(flow);OPENSSL_cleanse(key,32);
}

static void stem_for(char *s,size_t z,Record *r){if(snprintf(s,z,"%s--%s",kinds[r->kind],record_id(r))>=(int)z)die("record name too long");}
static void key_path(char *p,size_t z,const char *vault,const char *stem){char q[1024],file[200];path_join(q,sizeof q,vault,"keys");snprintf(file,sizeof file,"%s.key",stem);path_join(p,z,q,file);}
static void cipher_path(char *p,size_t z,const char *vault,const char *stem){char q[1024],file[200];path_join(q,sizeof q,vault,"ciphertext");snprintf(file,sizeof file,"%s.json",stem);path_join(p,z,q,file);}
static char *aad_for(J *meta){J *a=jnew(J_OBJ);field(a,meta,"stem");field(a,meta,"classification");field(a,meta,"integrity");field(a,meta,"date");char *s=jdump(a);jfree(a);return s;}
static void inventory_save(const char *vault,J *a){
    char p[1024],tmp[1024];unsigned char key[32];path_join(p,sizeof p,vault,"inventory.key");if(!key_read(p,key))die("missing inventory key");
    char *s=jdump(a),*h=mac(key,s);J *o=jnew(J_OBJ);jset(o,"records",jcopy(a));jset(o,"hmacSHA256",js(h));
    path_join(tmp,sizeof tmp,vault,"inventory.new.json");jsave(tmp,o);
    int fd=open(tmp,O_RDONLY|O_NOFOLLOW);if(fd<0||fsync(fd)||close(fd))die("inventory fsync failed");
    path_join(p,sizeof p,vault,"inventory.json");if(rename(tmp,p))die("inventory atomic replacement failed");
    fd=open(vault,O_RDONLY|O_DIRECTORY);if(fd<0||fsync(fd)||close(fd))die("inventory directory fsync failed");
    free(s);free(h);jfree(o);OPENSSL_cleanse(key,32);
}
static J *inventory_load(const char *vault){char p[1024];unsigned char key[32];path_join(p,sizeof p,vault,"inventory.key");if(!key_read(p,key))die("missing inventory authentication key");path_join(p,sizeof p,vault,"inventory.json");J *o=jload(p),*a=jget(o,"records");if(!a||a->type!=J_ARR)die("bad inventory");char *s=jdump(a),*h=mac(key,s);const char *expected=jstr(o,"hmacSHA256");if(strlen(expected)!=64||CRYPTO_memcmp(expected,h,64))die("inventory authentication failed");free(s);free(h);OPENSSL_cleanse(key,32);J *copy=jcopy(a);jfree(o);return copy;}
void vault_init(Nerp *n,const char *vault){
    make_dir(vault);char p[1024];path_join(p,sizeof p,vault,"inventory.json");if(access(p,F_OK)==0)die("vault already initialised; choose a fresh directory");
    path_join(p,sizeof p,vault,"keys");make_dir(p);path_join(p,sizeof p,vault,"ciphertext");make_dir(p);unsigned char key[32];random_bytes(key,32);path_join(p,sizeof p,vault,"inventory.key");key_write(p,key);OPENSSL_cleanse(key,32);
    J *a=jnew(J_ARR);
    for(size_t i=0;i<n->count;i++){
        Record *r=&n->records[i];char stem[160],date[11]={0};stem_for(stem,sizeof stem,r);int assumed=0;const char *stamp=date_of(n,r,&assumed);if(strlen(stamp)>=10){memcpy(date,stamp,10);day_number(date);}
        J *m=jnew(J_OBJ);jset(m,"stem",js(stem));jset(m,"id",js(record_id(r)));jset(m,"dataset",js(kinds[r->kind]));jset(m,"classification",js(conf_names[r->conf]));jset(m,"integrity",js(integrity_names[r->integrity]));jset(m,"date",js(date));jset(m,"assumedDate",jb(assumed));jset(m,"state",js("ACTIVE"));
        char *aad=aad_for(m),*plain=jdump(r->body);random_bytes(key,32);key_path(p,sizeof p,vault,stem);key_write(p,key);J *enc=encrypt(key,plain,aad);cipher_path(p,sizeof p,vault,stem);jsave(p,enc);OPENSSL_cleanse(plain,strlen(plain));free(plain);free(aad);jfree(enc);OPENSSL_cleanse(key,32);jadd(a,m);
    }
    inventory_save(vault,a);jfree(a);
}
J *vault_read(const char *vault,const char *stem){
    for(const char *p=stem;*p;p++)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'))die("invalid vault name");
    J *a=inventory_load(vault),*meta=NULL;for(size_t i=0;i<a->len;i++)if(!strcmp(stem,jstr(a->v[i],"stem")))meta=a->v[i];if(!meta||!strcmp(jstr(meta,"state"),"ERASED")){jfree(a);return NULL;}
    char p[1024];unsigned char key[32];key_path(p,sizeof p,vault,stem);if(!key_read(p,key)){jfree(a);return NULL;}cipher_path(p,sizeof p,vault,stem);J *enc=jload(p);char *aad=aad_for(meta),*plain=decrypt(key,enc,aad);free(aad);jfree(enc);OPENSSL_cleanse(key,32);jfree(a);if(!plain)return NULL;J *r=jparse(plain);OPENSSL_cleanse(plain,strlen(plain));free(plain);return r;
}
static int held(J *config,const char *kind,const char *id){J *a=jget(config,"legal_holds");if(!a||a->type!=J_ARR)return 0;char ref[160];snprintf(ref,sizeof ref,"%s/%s",kind,id);for(size_t i=0;i<a->len;i++)if(a->v[i]->type==J_STR&&!strcmp(a->v[i]->s,ref))return 1;return 0;}
void retention(const char *config,const char *vault,const char *date){
    J *cfg=jload(config),*a=inventory_load(vault),*events=jnew(J_ARR);long today=day_number(date);
    /* A simulation clock cannot roll back a previously applied lifecycle. */
    for(size_t i=0;i<a->len;i++){const char *last=jstr(a->v[i],"lastEvaluated");if(*last&&today<day_number(last))die("retention clock cannot move backwards");}
    for(size_t i=0;i<a->len;i++){
        J *m=a->v[i],*e=jnew(J_OBJ);const char *kind=jstr(m,"dataset"),*id=jstr(m,"id"),*stamp=jstr(m,"date");field(e,m,"stem");field(e,m,"state");
        const char *next="ACTIVE";long age=0;int active=0,total=0;
        J *rule=jget(jget(cfg,"retention"),jstr(m,"classification"));active=jint(rule,"active_days",-1);total=jint(rule,"total_days",-1);
        if(!strcmp(kind,"audit")){int min=jint(cfg,"audit_minimum_days",2555);if(total<min)total=min;}
        if(active<0||total<active)die("invalid retention policy");
        if(!strcmp(jstr(m,"state"),"ERASED"))next="ERASED";
        else if(!*stamp)next="UNKNOWN_DATE_HOLD";
        else{
            age=today-day_number(stamp);
            if(age<0)next="FUTURE_DATE_HOLD";
            else if(held(cfg,kind,id))next="LEGAL_HOLD";
            else if(age>=total){
                char p[1024];key_path(p,sizeof p,vault,jstr(m,"stem"));
                /* Demonstration key disposal. SSD/snapshot/backups require a real KMS sanitisation process. */
                key_dispose(p);
                next="ERASED";
            }else if(age>=active)next="ARCHIVED";
        }
        jset(m,"state",js(next));jset(m,"lastEvaluated",js(date));jset(e,"nextState",js(next));jset(e,"ageDays",jn(age));jset(e,"activeDays",jn(active));jset(e,"totalDays",jn(total));jset(e,"assumedDate",jcopy(jget(m,"assumedDate")));jadd(events,e);
    }
    inventory_save(vault,a);write_at(vault,"retention_events.json",events);jfree(events);jfree(a);jfree(cfg);
}
void crypto_selftest(const char *out){
    J *a=jnew(J_ARR);unsigned char key[32];random_bytes(key,32);J *e=encrypt(key,"{\"value\":42}","label-bound AAD");char *p=decrypt(key,e,"label-bound AAD");
    J *r=jnew(J_OBJ);jset(r,"test",js("authenticated round trip"));jset(r,"passed",jb(p&&!strcmp(p,"{\"value\":42}")));jadd(a,r);if(!p)die("crypto roundtrip test failed");free(p);
    char *wrong=decrypt(key,e,"altered label");r=jnew(J_OBJ);jset(r,"test",js("modified label/AAD rejected"));jset(r,"passed",jb(wrong==NULL));jadd(a,r);if(wrong)die("AAD integrity test failed");
    const char *c=jstr(e,"ciphertext");char *mut=strdup(c);mut[0]=mut[0]=='0'?'1':'0';jset(e,"ciphertext",js(mut));free(mut);p=decrypt(key,e,"label-bound AAD");r=jnew(J_OBJ);jset(r,"test",js("ciphertext tampering rejected"));jset(r,"passed",jb(p==NULL));jadd(a,r);if(p)die("tamper test failed");
    write_at(out,"crypto_tests.json",a);jfree(a);jfree(e);OPENSSL_cleanse(key,32);
}
