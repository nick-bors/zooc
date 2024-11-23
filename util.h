#ifndef ZOOC_UTIL_H
#define ZOOC_UTIL_H

#define MAX(A, B)        ((A) > (B) ? (A) : (B))
#define MIN(A, B)        ((A) < (B) ? (A) : (B))
#define BETWEEN(X, A, B) ((A) <= (X) && (X) <= (B))
#define LENGTH(X)        (sizeof (X) / sizeof(X)[0])

void die(const char *fmt, ...);

#endif
