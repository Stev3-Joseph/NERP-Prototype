#ifndef NERP_JSON_H
#define NERP_JSON_H
#include <stddef.h>
/* Small, owned JSON tree. No network or external JSON dependency. */
typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } JType;
typedef struct J { JType type; char *s; double n; size_t len; struct J **v; char **keys; } J;
J *jnew(JType type);
J *js(const char *s);
J *jn(double n);
J *jb(int b);
void jadd(J *array, J *value);
void jset(J *object, const char *key, J *value);
J *jget(const J *object, const char *key);
const char *jstr(const J *object, const char *key);
int jint(const J *object, const char *key, int fallback);
J *jparse(const char *text);
J *jload(const char *path);
J *jcopy(const J *value);
char *jdump(const J *value);
void jsave(const char *path, const J *value);
void jfree(J *value);
void die(const char *message);
void *xmalloc(size_t n);
#endif
