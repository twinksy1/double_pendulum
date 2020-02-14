#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdbe.h>

struct global {
	int xres;
	int yres;
};

struct X11_wrapper {
	Display *dpy;
	Window win;
	GC gc;
	XdbeBackBuffer back_buffer;
	XdbeSwapInfo swap_info;
};

struct pendulum {
};

//GLOBAL OBJECTS///////
struct X11_wrapper x11;
struct global g;
///////////////////////

//X11 Function Definitions/////////////////////////////////////////////////////////
void set_window_title()
{
	char title[255];
	sprintf(title, "Double Pendulum Simulation %i x %i", g.xres, g.yres);
	XStoreName(x11.dpy, x11.win, title);
}

void setup()
{
	g.xres = 1280;
	g.yres = 1080;

	XSetWindowAttributes attributes;
	XdbeBackBufferAttributes* back_attr;
	x11.dpy = XOpenDisplay(NULL);
	
	attributes.event_mask = ExposureMask | StructureNotifyMask |
		PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
		KeyPressMask | KeyReleaseMask;
	attributes.backing_store = Always;
	attributes.save_under = True;
	attributes.override_redirect = False;
	attributes.background_pixel = 0xffffffff;
	
	Window root = DefaultRootWindow(x11.dpy);
	x11.win = XCreateWindow(x11.dpy, root, 0, 0, g.xres, g.yres, 0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWBackingStore | CWOverrideRedirect | CWEventMask |
			CWSaveUnder | CWBackPixel, & attributes);
	x11.gc = XCreateGC(x11.dpy, x11.win, 0, NULL);
	
	x11.back_buffer = XdbeAllocateBackBufferName(x11.dpy, x11.win, XdbeUndefined);
	back_attr = XdbeGetBackBufferAttributes(x11.dpy, x11.back_buffer);
	
	x11.swap_info.swap_window = back_attr->window;
	x11.swap_info.swap_action = XdbeUndefined;
	
	XFree(back_attr);
	set_window_title();
	XMapWindow(x11.dpy, x11.win);
	XRaiseWindow(x11.dpy, x11.win);
}

void swap_buffers()
{
	XdbeSwapBuffers(x11.dpy, &x11.swap_info, 1);
	usleep(4000);
}

int get_pending()
{
	return XPending(x11.dpy);
}

void get_next_event(XEvent* e)
{
	XNextEvent(x11.dpy, e);
}

void check_resize(XEvent *e)
{
        //ConfigureNotify is sent when the window is resized.
        if (e->type != ConfigureNotify)
                return;
        XConfigureEvent xce = e->xconfigure;
        g.xres = xce.width;
        g.yres = xce.height;
        set_window_title();
}

int check_keys(XEvent *e)
{
        static int shift=0;
        int key = XLookupKeysym(&e->xkey, 0);
        if (e->type == KeyRelease) {
                if (key == XK_Shift_L || key == XK_Shift_R)
                        shift=0;
                return 0;
        }
        if (e->type == KeyPress) {
                if (key == XK_Shift_L || key == XK_Shift_R) {
                        shift=1;
                        return 0;
                }
        } else {
                return 0;
        }
        //a key was pressed
        switch (key) {
		case XK_Escape:
			return(1);
			break;
	}

	return(0);
}

void check_mouse(XEvent *e)
{
        //Did the mouse move?
        //Was a mouse button clicked?
        static int savex = 0;
        static int savey = 0;
        //
        if (e->type == ButtonRelease) {
                return;
        }
        if (e->type == ButtonPress) {
                if (e->xbutton.button==1) {
                        //Left button is down
                }
                if (e->xbutton.button==3) {
                        //Right button is down
                }
        }
        if (savex != e->xbutton.x || savey != e->xbutton.y) {
                //Mouse moved
                savex = e->xbutton.x;
                savey = e->xbutton.y;
        }
}

void clear_screen()
{
	XSetForeground(x11.dpy, x11.gc, 0xffffffff);
	XFillRectangle(x11.dpy, x11.back_buffer, x11.gc, 0, 0, g.xres, g.yres);
}

void destroy()
{
	if(!XdbeDeallocateBackBufferName(x11.dpy, x11.back_buffer)) {
		fprintf(stderr, "ERROR: unable to deallocate back buffer\n");
	}
	XFreeGC(x11.dpy, x11.gc);
	XDestroyWindow(x11.dpy, x11.win);
	XCloseDisplay(x11.dpy);
}
//END X11 Defintions////////////////////////////////////////////////////

//Physics
void physics() {}

//Rendering
void render()
{
	clear_screen();
}

//Main Loop
int main()
{
	setup();
	int done = 0;
	while(!done) {
		while(get_pending()) {
			XEvent e;
			get_next_event(&e);
			check_resize(&e);
			check_mouse(&e);
			done = check_keys(&e);
		}
		physics();
		render();
		swap_buffers();
	}
	destroy();
	return 0;
}
