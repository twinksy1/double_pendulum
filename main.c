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
	float x;
	float y;
	float mass;
	float angle;
	float length;
	float vel;
	float acc;
};

//GLOBAL OBJECTS///////
struct X11_wrapper x11;
struct global g;
struct pendulum p1, p2;
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

	p1.length = 350.0;
	p2.length = 220.0;
	p1.angle = -90.0;
	p2.angle = 90.0;
	p1.mass = 40.0;
	p2.mass = 20.0;
	p1.vel = 0.005;
	p2.vel = 0.005;

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

void set_color(int r, int g, int b)
{
	unsigned long cref = 0L;
	cref += r;
	cref <<= 8;
	cref += g;
	cref <<= 8;
	cref += b;
	XSetForeground(x11.dpy, x11.gc, cref);
}

void draw_point(int x, int y)
{
	XDrawPoint(x11.dpy, x11.back_buffer, x11.gc, x, y);
}

void draw_line(int x0, int y0, int x1, int y1)
{
	XDrawLine(x11.dpy, x11.back_buffer, x11.gc, x0, y0, x1, y1);
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
void physics()
{
	const float grav = 9.8;
	const float OFFSETX = (float)g.xres / 2.0;
	const float OFFSETY = 100.0;
	
	p1.x = sin(p1.angle) * p1.length + OFFSETX;
	p1.y = cos(p1.angle) * p1.length + OFFSETY;
	
	p2.x = sin(p2.angle) * p2.length + p1.x;
	p2.y = cos(p2.angle) * p2.length + p1.y;

	float numerator = -grav*(2*p1.mass+p2.mass)*sin(p1.angle)-p2.mass*grav*sin(p1.angle-2*p2.angle)-
		2*sin(p1.angle-p2.angle)*p2.mass*(p2.vel*p2.vel*p2.length+p1.vel*p1.vel*p1.length*
				cos(p1.angle-p2.angle));
	float denominator = p1.length*(2*p1.mass+p2.mass-p2.mass*cos(2*p1.angle-2*p2.angle));
	p1.acc = numerator / denominator;
	p1.vel += p1.acc;
	if(p1.vel > 0.15) p1.vel = 0.15;
	if(p1.vel < -0.15) p1.vel = -0.15;
	p1.angle += p1.vel;

	numerator = 2*sin(p1.angle-p2.angle)*(p1.vel*p1.vel*p1.length*(p1.mass+p2.mass)+grav*(p1.mass+p2.mass)*
			cos(p1.angle)+p2.vel*p2.vel*p2.length*p2.mass*cos(p1.angle-p2.angle));
	denominator = p2.length*(2*p1.mass+p2.mass-p2.mass*cos(2*p1.angle-2*p2.angle));
	p2.acc = numerator / denominator;
	p2.vel += p2.acc;
	if(p2.vel > 0.2) p2.vel = 0.2;
	if(p2.vel < -0.2) p2.vel = -0.2;
	p2.angle += p2.vel;

	usleep(2000);
}

//Rendering
void render()
{
	clear_screen();
	const float OFFSETX = (float)g.xres / 2.0;
	const float OFFSETY = 100.0;
	set_color(255, 0, 0);
	
	draw_line(OFFSETX, OFFSETY, p1.x, p1.y);
	draw_line(p1.x, p1.y, p2.x, p2.y);

	for(int i=0; i<g.yres; i++) {
		for(int j=0; j<g.xres; j++) {
			float xdiff = j - p1.x;
			float ydiff = i - p1.y;
			float dist = sqrt(xdiff*xdiff + ydiff*ydiff);
			if(dist <= p1.mass) {
				(dist == p1.mass) ? set_color(0, 0, 0) : set_color(255, 0, 0);
				draw_point(j, i);
			}
			xdiff = j - p2.x;
			ydiff = i - p2.y;
			dist = sqrt(xdiff*xdiff + ydiff*ydiff);
			if(dist <= p2.mass) {
				(dist == p2.mass) ? set_color(0, 0, 0) : set_color(0, 0, 255);
				draw_point(j, i);
			}
		}
	}
}

//Main Loop
int main()
{
	srand(time(NULL));
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
