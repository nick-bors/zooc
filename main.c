#include <X11/Xutil.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <string.h>

#include "util.h"
#include "config.h"

#define MIN_GLX_MAJOR 1
#define MIN_GLX_MINOR 3

void expose(XEvent *);
void keypress(XEvent *);
void checkGlxVersion(Display *);

static Display *dpy = NULL;
static int screen = 0;
static XWindowAttributes wa;
static Window w;
static bool running = true;

static void (*handler[LASTEvent]) (XEvent *) = {
    [Expose] = expose,
    [KeyPress] = keypress,
};

void 
checkGlxVersion(Display *dpy)
{
    int glx_major, glx_minor;
    // Check shit if necessary
    if (!glXQueryVersion(dpy, &glx_major, &glx_minor)
        || ((glx_minor == MIN_GLX_MAJOR) && (glx_minor < MIN_GLX_MINOR))
        || (MIN_GLX_MAJOR < 1))
        die("Invalid GLX version %d.%d. Requires GLX >= %d.%d", glx_major, glx_minor, MIN_GLX_MAJOR, MIN_GLX_MINOR);
}

int
main(int argc, char *argv[])
{
    Config conf = loadConf("config.conf");

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printf("Cannot connect to the X display server\n");
        exit(EXIT_FAILURE);
    }
    checkGlxVersion(dpy);

    screen = DefaultScreen(dpy);

    GLint attrs[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };

    XVisualInfo *vi = glXChooseVisual(dpy, screen, attrs);
    if (vi == NULL)
        die("No appropriate visual found");

    XSetWindowAttributes swa;
    memset(&swa,0,sizeof(XSetWindowAttributes));

    swa.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), vi->visual, AllocNone);
    swa.event_mask = ButtonPressMask 
        | ButtonReleaseMask 
        | KeyPressMask 
        | KeyReleaseMask 
        | PointerMotionMask 
        | ExposureMask 
        | ClientMessage;

    if (!conf.windowed) {
        swa.override_redirect = 1;
        swa.save_under = 1;
    }

    XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &wa);

     w = XCreateWindow(
        dpy, DefaultRootWindow(dpy), 
        0, 0, wa.width, wa.height, 0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder, &swa
    );

    XMapWindow(dpy, w);

    char *wmName  = "coomer";
    char *wmClass = "coomer";
    XClassHint hints = {.res_name = wmName, .res_class = wmClass};

    XStoreName(dpy, w, wmName);
    XSetClassHint(dpy, w, &hints);

    Atom wmDeleteAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(dpy, w, &wmDeleteAtom, 1);

    GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, w, glc);

    // TODO: figure out what loadExtensions means
    XSelectInput(dpy, w, ExposureMask | KeyPressMask );

    int revertToParent;
    Window origin_win;

    XGetInputFocus(dpy, &origin_win, &revertToParent);

    XEvent e;
    while (running) {
        // HACK: setting this every time is probably inefficient. Is there a 
        // better way to maintain fullscreen input focus?
        if (!conf.windowed)
            XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
        XNextEvent(dpy, &e);
        switch (e.type) {
        case KeyPress:
            handler[e.type](&e);
            break;
        default:
            break;
        }
    }

    XSetInputFocus(dpy, origin_win, RevertToParent, CurrentTime);

    XDestroyWindow(dpy, w);
    XCloseDisplay(dpy);
    return 0;
}


void 
keypress(XEvent *e) 
{
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

            printf("event: kp");
    ev = &e->xkey;
    keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, e->xkey.state & ShiftMask ? 1 : 0);
    if (keysym == XK_q || keysym == XK_Escape) {
        running = false; 
    }
}

void
expose(XEvent *e)
{
}
