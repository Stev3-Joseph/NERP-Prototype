#include "nerp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void usage(void){
    fputs("NERP C/D prototype (trusted administrator simulation)\n"
          "  nerp demo DATA CONFIG NEW_OUTPUT_DIR\n"
          "  nerp access DATA CONFIG SUBJECT DATASET ID read|write [SESSION_LABEL]\n"
          "  nerp vault-init DATA CONFIG NEW_VAULT_DIR\n"
          "  nerp retention CONFIG VAULT_DIR YYYY-MM-DD\n"
          "  nerp vault-read VAULT_DIR DATASET--ID\n"
          "read returns data only after all controls pass; write evaluates a blind-write permission, not CRUD.\n",stderr);
    exit(2);
}
int main(int argc,char **argv){
    if(argc<2)usage();
    if(!strcmp(argv[1],"retention")){if(argc!=5)usage();retention(argv[2],argv[3],argv[4]);puts("Retention applied to managed encrypted copies only.");return 0;}
    if(!strcmp(argv[1],"vault-read")){if(argc!=4)usage();J *r=vault_read(argv[2],argv[3]);if(!r){fputs("No decryptable managed record (erased, missing or authentication failure).\n",stderr);return 3;}char *s=jdump(r);puts(s);free(s);jfree(r);return 0;}
    if(argc<4)usage();
    Nerp n;nerp_load(&n,argv[2],argv[3]);int exitcode=0;
    if(!strcmp(argv[1],"access")){
        if(argc!=8&&argc!=9)usage();
        Subject s=subject(&n,argv[4],argc==9?argv[8]:NULL);Record *r=find_record(&n,argv[5],argv[6]);
        J *d=decision(&n,s,r,argv[7]);int permit=allowed(d);
        if(permit&&!strcmp(argv[7],"read"))jset(d,"data",jcopy(r->body));
        if(!strcmp(argv[7],"write"))jset(d,"mode",js("blind-write decision only; no data mutation performed"));
        char *text=jdump(d);puts(text);free(text);jfree(d);exitcode=permit?0:3;
    }else if(!strcmp(argv[1],"demo")){
        if(argc!=5)usage();
        const char *out=argv[4];struct stat st;if(!stat(out,&st))die("output already exists; choose a new output directory");make_dir(out);
        classify_output(&n,out);generate_views(&n,out,subject(&n,"R002",NULL));scenario_output(&n,out);crypto_selftest(out);
        char v[1024];path_join(v,sizeof v,out,"vault_demo");vault_init(&n,v);
        J *before=vault_read(v,"incidents--INC-002");if(!before)die("vault roundtrip failed");jfree(before);
        retention(argv[3],v,"2027-01-03");J *arch=vault_read(v,"incidents--INC-002");if(!arch)die("archived record unreadable");jfree(arch);
        /* Preserve first checkpoint; the later run updates the live vault. */
        char p[1024];path_join(p,sizeof p,v,"retention_events.json");J *snapshot=jload(p);write_at(out,"retention_archive_checkpoint.json",snapshot);jfree(snapshot);
        retention(argv[3],v,"2028-11-05");J *after=vault_read(v,"incidents--INC-002"),*hold=vault_read(v,"incidents--INC-006");
        int passed=after==NULL&&hold!=NULL;
        J *res=jnew(J_OBJ);jset(res,"normalRecordDecryptsBeforeExpiry",jb(1));jset(res,"archivedRecordRemainsDecryptable",jb(1));jset(res,"expiredRecordUnavailable",jb(after==NULL));jset(res,"legalHoldStillDecryptable",jb(hold!=NULL));jset(res,"scope",js("Managed encrypted copies only. Input fixtures and prior exports are outside this demonstration; deleting a key file is not certified physical sanitisation."));write_at(out,"retention_tests.json",res);jfree(res);jfree(after);jfree(hold);if(!passed)die("retention test failed");
        printf("Loaded %zu records. 16 policy scenarios, 3 crypto tests and 4 lifecycle checks passed.\n",n.count);
        printf("Results: %s\n",out);
    }else if(!strcmp(argv[1],"vault-init")){if(argc!=5)usage();vault_init(&n,argv[4]);puts("Encrypted managed vault created.");}
    else usage();
    nerp_free(&n);return exitcode;
}
