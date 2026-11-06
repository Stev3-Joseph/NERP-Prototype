#ifndef NERP_H
#define NERP_H
#include "json.h"
#define KINDS 5
#define MAX_RECORDS 512
typedef struct { J *body; int kind,conf,integrity,review; char reason[320]; } Record;
typedef struct { J *config,*datasets[KINDS]; Record records[MAX_RECORDS]; size_t count; } Nerp;
typedef struct { const char *id,*role; int clearance,level,integrity; J *body; } Subject;
extern const char *kinds[KINDS];
extern const char *conf_names[5];
extern const char *integrity_names[3];
int conf_level(const char *name);
const char *record_id(const Record *r);
Record *find_record(Nerp *n,const char *kind,const char *id);
void nerp_load(Nerp *n,const char *data,const char *config);
void nerp_free(Nerp *n);
Subject subject(Nerp *n,const char *id,const char *session);
J *decision(Nerp *n,Subject s,Record *r,const char *operation);
int allowed(J *decision);
void classify_output(Nerp *n,const char *out);
void generate_views(Nerp *n,const char *out,Subject s);
void scenario_output(Nerp *n,const char *out);
void vault_init(Nerp *n,const char *out);
void retention(const char *config,const char *vault,const char *date);
J *vault_read(const char *vault,const char *stem);
void crypto_selftest(const char *out);
void make_dir(const char *path);
void path_join(char *out,size_t size,const char *a,const char *b);
long day_number(const char *date);
void write_at(const char *dir,const char *file,J *value);
#endif
