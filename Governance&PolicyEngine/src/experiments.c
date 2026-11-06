#include "nerp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {const char *name,*who,*kind,*id,*op,*level;int expect;} Case;
void scenario_output(Nerp *n,const char *out){
    const Case cases[]={
      {"D4.1 ACM permits; BLP prevents read up","R002","intelligence","INT-001","read",NULL,0},
      {"D4.2 BLP permits; Biba prevents read down","R009","intelligence","INT-003","read",NULL,0},
      {"D4.3 All controls permit assigned incident read","R002","incidents","INC-002","read",NULL,1},
      {"BLP prevents write down","R001","incidents","INC-001","write",NULL,0},
      {"Biba prevents write up","R014","intelligence","INT-014","write",NULL,0},
      {"Unassigned incident denied despite clearance","R001","incidents","INC-002","read",NULL,0},
      {"Audit modification denied by ACM","R001","audit","AUD-002","write",NULL,0},
      {"Assigned incident write decision permitted","R002","incidents","INC-002","write",NULL,1},
      {"Fresh lower-labelled session avoids write down","R001","incidents","INC-001","write","OFFICIAL",1},
      {"Session cannot exceed clearance","R002","incidents","INC-002","read","TOP SECRET",0},
      {"Unknown subject denied","UNKNOWN","incidents","INC-002","read",NULL,0},
      {"Unknown action denied","R002","incidents","INC-002","delete",NULL,0},
      {"Agency-assignment inconsistency denied","R007","incidents","INC-007","read",NULL,0},
      {"Auditor reads authoritative audit event","R019","audit","AUD-002","read",NULL,1},
      {"Analyst reads peer-integrity intelligence","R014","intelligence","INT-001","read",NULL,1},
      {"Unknown resource denied","R002","incidents","MISSING","read",NULL,0}
    };
    J *a=jnew(J_ARR);int failures=0;
    for(size_t i=0;i<sizeof cases/sizeof cases[0];i++){
        const Case *c=&cases[i];J *d=decision(n,subject(n,c->who,c->level),find_record(n,c->kind,c->id),c->op);
        int pass=allowed(d)==c->expect;failures+=!pass;jset(d,"scenario",js(c->name));jset(d,"expectedAllowed",jb(c->expect));jset(d,"passed",jb(pass));jadd(a,d);
    }
    write_at(out,"policy_scenarios.json",a);jfree(a);if(failures)die("policy experiment expectations failed");
    J *warnings=jnew(J_ARR);
    for(size_t i=0;i<n->count;i++){
        Record *r=&n->records[i];if(r->kind!=1)continue;const char *id=jstr(r->body,"currentAssignment");if(!*id)continue;
        Record *inc=find_record(n,"incidents",id);J *role=jget(jget(n->config,"roles"),jstr(r->body,"role"));
        if(!inc||(*jstr(role,"agency")&&strcmp(jstr(role,"agency"),jstr(inc->body,"assignedAgency")))){
            J *w=jnew(J_OBJ);jset(w,"responder",js(record_id(r)));jset(w,"assignment",js(id));jset(w,"finding",js("assignment/agency mismatch; deny pending incident-command review"));jadd(warnings,w);
        }
    }
    write_at(out,"data_quality_warnings.json",warnings);jfree(warnings);
}
