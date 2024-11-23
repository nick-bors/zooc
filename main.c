#include <X11/Xutil.h>
#include <math.h>
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
#include <sys/types.h>

#include "util.h"
#include "config.h"
#include "vec.h"

#define MIN_GLX_MAJOR   1
#define MIN_GLX_MINOR   3

#define VELOCITY_THRESHOLD  15.0f
#define DELTA_RADIUS_DECELERATION 10.0f

typedef struct {
    bool isEnabled;
    GLfloat shadowPercentage;
    GLfloat radius;
    GLfloat deltaRadius;
} Flashlight;

typedef struct {
    Vec2f position;
    Vec2f velocity;
    GLfloat scale;
    GLfloat deltaScale;
    GLfloat scalePivot;
} Camera;

void expose(XEvent *);
void keypress(XEvent *);
void checkGlxVersion(Display *);

static Display *dpy = NULL;
static int screen = 0;
static XWindowAttributes wa;
static Window w;
static bool running = true;

static Flashlight flashlight = {
    .isEnabled = false,
    .shadowPercentage = 0.0f,
    .radius = 200.0f,
    .deltaRadius = 0.0f,
};

static Camera camera = {
    .position = ZERO,
    .velocity = ZERO,
    .scale = 1.0f,
    .deltaScale = 0.0f,
    .scalePivot = 0.0f
};

static void (*handler[LASTEvent]) (XEvent *) = {
    [Expose] = expose,
    [KeyPress] = keypress,
};

GLuint
loadShader(const char *name, GLenum type)
{
    FILE *fp = fopen(name, "r");
    GLchar *shaderSrc = NULL;

    if (fp == NULL)
        die("Unable to open shader file at '%s':", name);

    size_t len;
    ssize_t bytes_read = getdelim(&shaderSrc, &len, '\0', fp);

    if (bytes_read < 0)
        die("Unable to read shader file at '%s'.", name);
    fclose(fp);

    GLuint shader;

    shader = glCreateShader(type);

    glShaderSource(shader, 1, (const GLchar **)&shaderSrc, NULL);
    glCompileShader(shader);

    int sucess = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &sucess);
    if (!sucess) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        die("Error comiling shader:\n%s", infoLog);
    }

    return shader;
}

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

// TODO: implement support for the MIT shared memory extension. (MIT-SHM)
XImage*
getScreenshot()
{
    return XGetImage(
        dpy, w,
        0, 0,
        wa.width,
        wa.height,
        AllPlanes,
        ZPixmap);
}

void
destroyScreenshot(XImage *screenshot)
{
    XDestroyImage(screenshot);
}

void
drawImage(XImage *img, Camera camera, GLuint shader, GLuint vao, GLuint texture, Vec2f windowSize, /*Mouse mouse,*/ Flashlight flashlight)
{
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);

    glUniform2f(glGetUniformLocation(shader, "cameraPos"), camera.position.x, camera.position.y);
    glUniform1f(glGetUniformLocation(shader, "cameraScale"), camera.scale);
    glUniform2f(glGetUniformLocation(shader, "screenshotSize"), (float)img->width, (float)img->height);
    glUniform2f(glGetUniformLocation(shader, "windowSize"), windowSize.x, windowSize.y);
    glUniform2f(glGetUniformLocation(shader, "cursorPos"), 1920.0f - 200.0f, 100.0f);
    glUniform1f(glGetUniformLocation(shader, "flShadow"), flashlight.shadowPercentage);
    glUniform1f(glGetUniformLocation(shader, "flRadius"), flashlight.radius);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void
updateFlashlight(GLfloat dt)
{
    if (fabsf(flashlight.deltaRadius) > 1.0f) {
        flashlight.radius = MAX(0.0f, flashlight.radius + flashlight.deltaRadius * dt);
        flashlight.deltaRadius -= flashlight.deltaRadius * DELTA_RADIUS_DECELERATION;
    }

    /* Smoothly interpolate between on/off */
    if (flashlight.isEnabled) {
        flashlight.shadowPercentage = MIN(flashlight.shadowPercentage + 6.0 * dt, 0.8f);
    } else {
        flashlight.shadowPercentage = MAX(flashlight.shadowPercentage - 6.0 * dt, 0.0f);
    }
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

    if (GLEW_OK != glewInit())
        die("Couldnt initialize glew!");
    
    glViewport(0, 0, wa.width, wa.height);

    // TODO: figure out what loadExtensions means
    XSelectInput(dpy, w, ExposureMask | KeyPressMask);

    int revertToParent;
    Window origin_win;

    XGetInputFocus(dpy, &origin_win, &revertToParent);

    // TODO: make shader source a configurable dir
    // GLchar shaderDir[] = "./shaders/";

    GLuint vertexShader;
    GLchar vertexSrc[] = "./shaders/vertex.glsl";

    GLuint fragmentShader;
    GLchar fragmentSrc[] = "./shaders/fragment.glsl";

    /* Load and compile shaders */
    vertexShader   = loadShader(vertexSrc, GL_VERTEX_SHADER);
    fragmentShader = loadShader(fragmentSrc, GL_FRAGMENT_SHADER);

    /* Link shaders and create a program */
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int link_success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &link_success);

    if(!link_success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        die("Error whilst linking program:\n%s", infoLog);
    }

    XImage *screenshot = getScreenshot();
    float ww = screenshot->width;
    float hh = screenshot->height;

    GLuint vbo, vao, ebo;
    GLfloat vertices[] = {
         1.0,  -1.0, 0.0, 1.0, 1.0, // Top right
         1.0,  1.0,  0.0, 1.0, 0.0, // Bottom right
        -1.0,  1.0,  0.0, 0.0, 0.0, // Bottom left
        -1.0,  -1.0, 0.0, 0.0, 1.0  // Top left
        /*
        //x   y     z       UV coords
        ww,   0.0f, 0.0f,   1.0f, 1.0f, //Top right
        ww,   hh,   0.0f,   1.0f, 0.0f, //Bot right
        0.0f, 0.0f, 0.0f,   0.0f, 1.0f, //Bot left 
        0.0f, hh,   0.0f,   0.0f, 0.0f, //Top left 
        */
    };
    /* Indecies of the triangles. 
     * We want to fill a screen rect so we create two triangles:
     * 3_____0
     * |\    |
     * |  \  |
     * 2____\1
     *
     * Therefore we have two triangles, 0-1-3 and 1-2-3.
     */
    GLuint indices[] = {0, 1, 3, 1, 2, 3};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

    GLsizei stride = 5 * sizeof(GLfloat);

    /* Pos attribute = vec3(x, y, z) */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    /* UV attribute = vec2(x, y) */
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);

    GLint texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D, 
        0, 
        GL_RGB, 
        screenshot->width,
        screenshot->height,
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        screenshot->data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    /* bind tex in the glsl code to be the loaded texture */
    glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);

    glEnable(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    XEvent e;
    while (running) {
        // HACK: setting this every time is probably inefficient. Is there a 
        // better way to maintain fullscreen input focus?
        if (!conf.windowed)
            XSetInputFocus(dpy, w, RevertToParent, CurrentTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        updateFlashlight(1.0/60.0f);
        drawImage(screenshot, camera, shaderProgram, vao, texture, (Vec2f){wa.width, wa.height}, flashlight);

        glXSwapBuffers(dpy, w);
        glFinish();

        XSync(dpy, 0);

        XNextEvent(dpy, &e);
        switch (e.type) {
        case ClientMessage:
            if ((Atom)e.xclient.data.l[0] == wmDeleteAtom)
                running = false;
            break;
        case KeyPress:
            handler[e.type](&e);
            break;
        default:
            break;
        }
    }

    XSetInputFocus(dpy, origin_win, RevertToParent, CurrentTime);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, glc);

    XCloseDisplay(dpy);
    return 0;
}


void 
keypress(XEvent *e) 
{
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    printf("event: keypress\n");
    ev = &e->xkey;
    keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, e->xkey.state & ShiftMask ? 1 : 0);
    switch (keysym) {
    case XK_q:
    case XK_Escape:
        running = false;
        break;
    case XK_f:
        flashlight.isEnabled = !flashlight.isEnabled;
        break;
    }
}

void
expose(XEvent *e)
{
}
