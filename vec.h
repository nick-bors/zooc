#ifndef ZOOC_VEC_H
#define ZOOC_VEC_H

#include<GL/gl.h>

#define UNIT        (Vec2f){1.0f, 1.0f}
#define ZERO        (Vec2f){0.0f, 0.0f}

/* Vector operations */
#define MUL(v, u)   ((Vec2f){(v).x * (u).x, (v).y * (u).y})
#define DIV(v, u)   ((Vec2f){(v).x / (u).x, (v).y / (u).y})
#define ADD(v, u)   ((Vec2f){(v).x + (u).x, (v).y + (u).y})
#define SUB(v, u)   ((Vec2f){(v).x - (u).x, (v).y - (u).y})

#define EQ(v, u)    ((v).x == (u).x && (v).y == (u).y)
#define LEN(v)      sqrt((v).x * (v).x + (v).y * (v).y)
#define NORM(v)     LEN(V) == 0.0f ? ZERO : (DIV(v, LEN(v)))

/* Scalar operations */
#define MULS(v, s)  ((Vec2f){(v).x * (s), (v).y * (s)})
#define DIVS(v, s)  ((Vec2f){(v).x / (s), (v).y / (s)})
#define ADDS(v, s)  ((Vec2f){(v).x + (s), (v).y + (s)})
#define SUBS(v, s)  ((Vec2f){(v).x - (s), (v).y - (s)})

typedef union {
    struct {
        GLfloat x;
        GLfloat y;
    };
    GLfloat v[2];
} Vec2f;

#endif
