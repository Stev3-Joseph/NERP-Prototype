#define _POSIX_C_SOURCE 200809L
#include "nerp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

const char *kinds[KINDS]={"incidents","responders","intelligence","dispatch","audit"};
const char *conf_names[5]={"PUBLIC","OFFICIAL","CONFIDENTIAL","SECRET","TOP SECRET"};
const char *integrity_names[3]={"UNVERIFIED","VERIFIED","AUTHORITATIVE"};
static const char *ids[KINDS]={"incidentId","responderId","reportId","dispatchId","eventId"};
int conf_level(const char *name){for(int i=0;i<5;i++)if(!strcmp(name,conf_names[i]))return i;return -1;}
void path_join(char *o,size_t z,const char *a,const char *b){if(snprintf(o,z,"%s/%s",a,b)>=(int)z)die("path too long");}
void make_dir(const char *p){if(mkdir(p,0700)&&errno!=EEXIST)die("cannot create directory");struct stat st;if(lstat(p,&st)||!S_ISDIR(st.st_mode))die("directory path is not a real directory");if(chmod(p,0700))die("directory permissions failed");}
void write_at(const char *dir,const char *file,J *j){char p[1024];path_join(p,sizeof p,dir,file);jsave(p,j);}
const char *record_id(const Record *r){return jstr(r->body,ids[r->kind]);}
Record *find_record(Nerp *n,const char *kind,const char *id){for(size_t i=0;i<n->count;i++){Record *r=&n->records[i];if(!strcmp(kinds[r->kind],kind)&&!strcmp(record_id(r),id))return r;}return NULL;}
static int contains(const char *s,const char *word){size_t n=strlen(s),m=strlen(word);for(size_t i=0;i+m<=n;i++){size_t k=0;while(k<m&&tolower((unsigned char)s[i+k])==tolower((unsigned char)word[k]))k++;if(k==m)return 1;}return 0;}
static void reason(Record *r,const char *s){if(*r->reason)strncat(r->reason,"; ",sizeof r->reason-strlen(r->reason)-1);strncat(r->reason,s,sizeof r->reason-strlen(r->reason)-1);}
static void classify(Nerp *n,Record *r){
    J *defs=jget(n->config,"dataset_integrity");r->conf=1;r->integrity=jint(defs,kinds[r->kind],-1);
    J *over=jget(jget(n->config,"integrity_overrides"),record_id(r));if(over)r->integrity=jint(over,"level",-1);
    if(r->integrity<0||r->integrity>2)die("missing or invalid configured integrity level");
    if(r->kind==0){
        reason(r,"personal contact and operational details: OFFICIAL with restricted handling");
        if(contains(jstr(r->body,"incidentType"),"terror")){r->conf=3;reason(r,"terrorism response: SECRET");}
        if(contains(jstr(r->body,"incidentType"),"medical")||jint(r->body,"casualties",0)>0)reason(r,"medical/casualty handling tag");
    }else if(r->kind==1){reason(r,"named personnel and assignments: OFFICIAL; clearance is not record sensitivity");}
    else if(r->kind==2){
        const char *hint = jstr(r->body, "classificationHint");
        const char *threat = jstr(r->body, "threatLevel");
        const char *suspect = jstr(r->body, "suspectIdentifier");
        const char *summary = jstr(r->body, "summary");
        int hint_level = conf_level(hint);
        int content_level = 1;

        /* Start with the threat level. */
        if (strcmp(threat, "CRITICAL") == 0) {
            content_level = 4;
        } else if (strcmp(threat, "HIGH") == 0 || strcmp(threat, "SEVERE") == 0) {
            content_level = 3;
        } else if (strcmp(threat, "MEDIUM") == 0) {
            content_level = 2;
        }

        /* Content can raise the label. */
        if (*suspect && strcmp(suspect, "N/A") != 0 && content_level < 2) {
            content_level = 2;
        }
        int sensitive_summary = contains(summary, "attack")
            || contains(summary, "sabotage")
            || contains(summary, "critical infrastructure");
        if (sensitive_summary && content_level < 3) {
            content_level = 3;
        }

        if (hint_level < 0) {
            r->conf = 4;
            r->review = 1;
            reason(r, "missing/unknown hint: quarantine at TOP SECRET");
        } else if (content_level > hint_level) {
            r->conf = content_level;
            r->review = 1;
            reason(r, "content exceeds hint: escalate and review");
        } else {
            r->conf = hint_level;
            reason(r, "maximum of hint, threat level, suspect and content indicators");
        }
    }else if(r->kind==3){
        Record *i=find_record(n,"incidents",jstr(r->body,"incidentId"));
        if(!i){r->conf=4;r->review=1;reason(r,"missing incident link: quarantine");}
        else{r->conf=i->conf;r->review=i->review;reason(r,"inherits linked incident sensitivity");}
    }else{
        const char *res=jstr(r->body,"resource");Record *ref=NULL;
        for(int k=0;k<4&&!ref;k++)ref=find_record(n,kinds[k],res);
        if(ref){r->conf=ref->conf;r->review=ref->review;reason(r,"inherits referenced resource sensitivity");}
        else if(!strcmp(res,"AUDIT-SYSTEM")){r->conf=3;reason(r,"audit-system oversight: SECRET");}
        else{r->conf=4;r->review=1;reason(r,"unresolved audit reference: quarantine");}
    }
    reason(r,over?"integrity from explicit review fixture":"integrity from configured dataset provenance assumption");
}
static int safe_id(const char *id){if(!*id||strlen(id)>60)return 0;for(;*id;id++)if(!isalnum((unsigned char)*id)&&*id!='-'&&*id!='_')return 0;return 1;}
void nerp_load(Nerp *n,const char *data,const char *config){
    memset(n,0,sizeof *n);n->config=jload(config);
    if(jint(n->config,"schema_version",-1)!=1)die("unsupported config schema");
    for(int k=0;k<KINDS;k++){
        char name[80],p[1024];snprintf(name,sizeof name,"%s.json",kinds[k]);path_join(p,sizeof p,data,name);
        J *a=n->datasets[k]=jload(p);if(a->type!=J_ARR)die("dataset must be array");
        for(size_t i=0;i<a->len;i++){
            if(n->count==MAX_RECORDS)die("too many records");
            J *row=a->v[i];if(row->type!=J_OBJ||!safe_id(jstr(row,ids[k])))die("invalid record or ID");
            if(find_record(n,kinds[k],jstr(row,ids[k])))die("duplicate record ID");
            Record *r=&n->records[n->count++];r->body=row;r->kind=k;
        }
    }
    /* Dependency order matters: dispatch and audit inherit already-classified labels. */
    for(size_t i=0;i<n->count;i++)classify(n,&n->records[i]);
}
void nerp_free(Nerp *n){for(int k=0;k<KINDS;k++)jfree(n->datasets[k]);jfree(n->config);}
Subject subject(Nerp *n,const char *id,const char *session){
    Subject s={id,"",-1,-1,-1,NULL};Record *r=find_record(n,"responders",id);
    s.body=r?r->body:jget(jget(n->config,"service_subjects"),id);
    if(!s.body)return s;
    s.role=jstr(s.body,"role");s.clearance=conf_level(jstr(s.body,"clearanceLevel"));
    J *role=jget(jget(n->config,"roles"),s.role);s.integrity=jint(role,"integrity",-1);
    s.level=session&&*session?conf_level(session):s.clearance;
    /* Clearance is a maximum; the fresh session's current label controls writes. */
    if(s.level>s.clearance||s.level<0||s.integrity<0||s.integrity>2||!role)s.body=NULL;
    return s;
}
static int scope_ok(Nerp *n,Subject s,Record *r){
    J *role=jget(jget(n->config,"roles"),s.role);const char *scope=jstr(role,"scope");
    if(!strcmp(scope,"global"))return 1;
    if(strcmp(scope,"assigned"))return 0;
    if(r->kind==1)return !strcmp(record_id(r),s.id);
    if(r->kind==2)return 1; /* Coarse intelligence read remains gated by BLP+Biba. No compartments are supplied. */
    if(r->kind==4)return 0;
    Record *inc=r->kind==0?r:find_record(n,"incidents",jstr(r->body,"incidentId"));
    if(!inc||strcmp(record_id(inc),jstr(s.body,"currentAssignment")))return 0;
    return !strcmp(jstr(inc->body,"assignedAgency"),jstr(role,"agency"));
}
J *decision(Nerp *n, Subject s, Record *r, const char *op) {
    int reading = strcmp(op, "read") == 0;
    int writing = strcmp(op, "write") == 0;
    int valid = s.body && r && (reading || writing);

    int acm = 0;
    int scope = 0;
    int blp = 0;
    int biba = 0;

    /* Record each check separately. */
    if (valid) {
        J *roles = jget(n->config, "roles");
        J *role = jget(roles, s.role);
        J *grants = jget(role, "grants");
        const char *permissions = jstr(grants, kinds[r->kind]);
        scope = scope_ok(n, s, r);

        if (reading) {
            acm = strchr(permissions, 'r') != NULL;
            blp = s.level >= r->conf;
            biba = s.integrity <= r->integrity;
        } else {
            acm = strchr(permissions, 'w') != NULL;
            blp = s.level <= r->conf;
            biba = s.integrity >= r->integrity;
        }
    }

    int quarantine = r ? r->review : 1;
    int permit = valid && acm && scope && blp && biba && !quarantine;

    /* Report the first denial reason. */
    const char *why = "all controls permit";
    if (!valid) {
        why = "invalid subject/session/resource/action";
    } else if (!acm) {
        why = "role matrix denies";
    } else if (!scope) {
        why = "assignment/agency scope denies";
    } else if (quarantine) {
        why = "classification requires review";
    } else if (!blp) {
        why = reading ? "BLP: no read up" : "BLP: no write down";
    } else if (!biba) {
        why = reading ? "Biba: no read down" : "Biba: no write up";
    }

    J *d = jnew(J_OBJ);
    jset(d, "subject", js(s.id));
    jset(d, "role", js(s.role));
    jset(d, "operation", js(op));
    jset(d, "resource", js(r ? record_id(r) : "UNKNOWN"));
    jset(d, "dataset", js(r ? kinds[r->kind] : "UNKNOWN"));
    jset(d, "session", js(s.level >= 0 ? conf_names[s.level] : "INVALID"));
    jset(d, "valid", jb(valid));
    jset(d, "ACM", jb(acm));
    jset(d, "scope", jb(scope));
    jset(d, "BLP", jb(blp));
    jset(d, "Biba", jb(biba));
    jset(d, "quarantine", jb(quarantine));
    jset(d, "allowed", jb(permit));
    jset(d, "reason", js(why));
    return d;
}
int allowed(J *d){J *a=jget(d,"allowed");return a&&a->type==J_BOOL&&a->n!=0;}
void classify_output(Nerp *n,const char *out){
    J *a=jnew(J_ARR),*counts=jnew(J_OBJ);
    for(int c=0;c<5;c++)jset(counts,conf_names[c],jn(0));
    for(size_t i=0;i<n->count;i++){
        Record *r=&n->records[i];J *v=jnew(J_OBJ);jset(v,"id",js(record_id(r)));jset(v,"dataset",js(kinds[r->kind]));jset(v,"classification",js(conf_names[r->conf]));jset(v,"integrity",js(integrity_names[r->integrity]));jset(v,"reviewRequired",jb(r->review));jset(v,"rationale",js(r->reason));jadd(a,v);
        jset(counts,conf_names[r->conf],jn(jint(counts,conf_names[r->conf],0)+1));
    }
    write_at(out,"classifications.json",a);write_at(out,"classification_counts.json",counts);write_at(out,"access_matrix.json",jget(n->config,"roles"));jfree(a);jfree(counts);
}
