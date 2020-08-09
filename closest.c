#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

/* Macros */
#define BETWEEN(x, a, b)    ((a) <= (x) && (x) <= (b))

/* Direction */
enum Direction {Left, Right, Up, Down};

/* Monitor geometry */
struct Monitor {
	int x, y, w, h;
};

/* Client structure */
struct Client {
	Window win;
	int x, y, w, h;
};

/* Function declarations */
static int checksupportewmh(void);
static enum Direction getdirection(const char *s);
static struct Client getfocusclient(void);
static struct Monitor getmonitor(void);
static struct Client *getclients(unsigned long *len);
static int clientcmp(const void *p, const void *q);
static Window getwintofocus(struct Monitor *, struct Client *, unsigned long);
static void focuswin(Window win);
static void usage(void);

/* Variable declarations */
static Display *dpy;
static int screen;
static Window root;
static Atom netclientlist;
static Atom netactivewindow;
static Atom netsupported;
static enum Direction dir;         /* direction to focus */
static struct Client focused;      /* client currently focused */
static int supportewmh = 0;

/* focus the closest window in a given direction */
int
main(int argc, char *argv[])
{
	struct Client *clients;     /* list of clients */
	struct Monitor mon;         /* monitor where the focused client is in */
	Window tofocus;             /* window to be focused */
	unsigned long nclients;     /* number of clients found */

	if (argc != 2)
		usage();

	dir = getdirection(argv[1]);

	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "could not open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	netclientlist = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	netactivewindow = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netsupported = XInternAtom(dpy, "_NET_SUPPORTED", False);

	supportewmh = checksupportewmh();

	focused = getfocusclient();
	mon = getmonitor();
	clients = getclients(&nclients);
	tofocus = getwintofocus(&mon, clients, nclients);
	free(clients);
	if (tofocus != None)
		focuswin(tofocus);
	XCloseDisplay(dpy);

	return 0;
}

/* check whether window manager supports EWMH _NET_ACTIVE_WINDOW hint */
static int
checksupportewmh(void)
{
	int retval = 0;
	unsigned char *p = NULL;
	unsigned long len, i;
	Atom *atoms;
	Atom da;            /* dummy variable */
	int di;             /* dummy variable */
	unsigned long dl;   /* dummy variable */

	if (XGetWindowProperty(dpy, root, netsupported, 0L, sizeof(Atom),
	                       False, XA_ATOM, &da, &di, &len, &dl, &p) ==
	                       Success && p) {
		atoms = (Atom *)p;
		for (i = 0; i < len; i++) {
			if (atoms[i] == netactivewindow) {
				retval = 1;
				break;
			}
		}
		XFree(p);
	}
 
	return retval;
}

/* get direction from s */
static enum Direction
getdirection(const char *s)
{
	if (strcasecmp(s, "left") == 0)
		return Left;
	if (strcasecmp(s, "right") == 0)
		return Right;
	if (strcasecmp(s, "up") == 0)
		return Up;
	if (strcasecmp(s, "down") == 0)
		return Down;

	errx(1, "unknown direction %s", s);
}

/* get the focused client */
static struct Client
getfocusclient(void)
{
	struct Client client;
	XWindowAttributes wa;
	Window win, focuswin = None, parentwin = None;
	Atom da;            /* dummy variable */
	Window dw, *dws;    /* dummy variable */
	int di;             /* dummy variable */
	unsigned du;        /* dummy variable */
	unsigned long dl;   /* dummy variable */
	unsigned char *u;   /* dummy variable */

	XGetInputFocus(dpy, &win, &di);

	if (win == root || win == None)
		goto error;

	if (supportewmh &&
	    XGetWindowProperty(dpy, root, netactivewindow, 0, 1024, False,
	                       XA_WINDOW, &da, &di, &dl, &dl, &u) ==
	                       Success && u) {
		focuswin = *(Window *)u;
		XFree(u);
	}

	if (focuswin == None) {
		while (parentwin != root) {
			if (XQueryTree(dpy, win, &dw, &parentwin, &dws, &du) && dws)
				XFree(dws);
			focuswin = win;
			win = parentwin;
		}
	}

	if (focuswin == None)
		goto error;

	if (!XGetWindowAttributes(dpy, focuswin, &wa))
		goto error;

	client.win = focuswin;
	client.x = wa.x;
	client.y = wa.y;
	client.w = wa.width;
	client.h = wa.height;

	return client;

error:
	errx(1, "could not get focused client");
}

/* get geometry of monitor where focused is in */
static struct Monitor
getmonitor(void)
{
	struct Monitor mon;
	XineramaScreenInfo *info = NULL;
	int nmons;
	int x, y, i;

	if ((info = XineramaQueryScreens(dpy, &nmons)) != NULL) {
		x = focused.x + focused.w / 2;
		y = focused.y + focused.h / 2;
		for (i = 0; i < nmons; i++) {
			if (BETWEEN(x, info[i].x_org, info[i].x_org + info[i].width) &&
			    BETWEEN(y, info[i].y_org, info[i].y_org + info[i].height)) {
				mon.x = info[i].x_org;
				mon.y = info[i].y_org;
				mon.w = info[i].width;
				mon.h = info[i].height;

				XFree(info);

				return mon;
			}
		}
	}

	mon.x = mon.y = 0;
	mon.w = DisplayWidth(dpy, screen);
	mon.h = DisplayHeight(dpy, screen);

	XFree(info);

	return mon;
}

/* get clients */
static struct Client *
getclients(unsigned long *len)
{
	struct Client *array = NULL;
	XWindowAttributes wa;
	unsigned char *list;
	Window *winlist;
	unsigned long i;
	Window dw;          /* dummy variable */
	unsigned long dl;   /* dummy variable */
	unsigned int du;    /* dummy variable */
	int di;             /* dummy variable */
	Atom da;            /* dummy variable */

	if (XGetWindowProperty(dpy, root, netclientlist, 0, 1024, False,
	                       XA_WINDOW, &da, &di, len, &dl, &list) ==
	                       Success) {
		winlist = (Window *)list;
	} else if (XQueryTree(dpy, root, &dw, &dw, &winlist, &du)) {
		*len = du;
	} else {
		goto error;
	}

	array = calloc(*len, sizeof *array);

	for (i = 0; i < *len; i++) {
		if (!XGetWindowAttributes(dpy, winlist[i], &wa))
			goto error;

		array[i].win = winlist[i];
		array[i].x = wa.x;
		array[i].y = wa.y;
		array[i].w = wa.width;
		array[i].h = wa.height;
	}

	XFree(winlist);
	
	return array;

error:
	errx(1, "could not get list of clients");
}

/* compare two clients */
static int
clientcmp(const void *p, const void *q)
{
	struct Client *a, *b;
	a = (struct Client *)p;
	b = (struct Client *)q;

	switch (dir) {
	case Left: case Right:
		if (a->y == focused.y && b->y != focused.y)
			return -1;
		if (a->y != focused.y && b->y == focused.y)
			return +1;
		if (a->x > b->x)
			return (dir == Left) ? -1 : +1;
		if (a->x < b->x)
			return (dir == Left) ? +1 : -1;
		if (abs(a->y - focused.y) < abs(b->y - focused.y))
			return -1;
		if (abs(a->y - focused.y) > abs(b->y - focused.y))
			return +1;
		break;
	case Up: case Down:
		if (a->x == focused.x && b->x != focused.x)
			return -1;
		if (a->x != focused.x && b->x == focused.x)
			return +1;
		if (a->y > b->y)
			return (dir == Up) ? -1 : +1;
		if (a->y < b->y)
			return (dir == Up) ? +1 : -1;
		if (abs(a->x - focused.x) < abs(b->x - focused.x))
			return -1;
		if (abs(a->x - focused.x) > abs(b->x - focused.x))
			return +1;
		break;
	}

	return 0;
}

/* get window to be focused */
static Window
getwintofocus(struct Monitor *mon, struct Client *clients, unsigned long nclients)
{
	Window retval = None;
	struct Client *array;
	size_t arraylen = 0;
	size_t i;

	/* get array of clients in the direction to focus */
	array = calloc(nclients, sizeof *array);
	for (i = 0; i < nclients; i++) {
		if (mon->x <= clients[i].x && clients[i].x <= mon->x + mon->w &&
			mon->y <= clients[i].y && clients[i].y <= mon->y + mon->h) {
			switch (dir) {
			case Left:
				if (clients[i].x < focused.x) {
					memcpy(array+arraylen, clients+i, sizeof *array);
					arraylen++;
				}
				break;
			case Right:
				if (clients[i].x > focused.x) {
					memcpy(array+arraylen, clients+i, sizeof *array);
					arraylen++;
				}
				break;
			case Up:
				if (clients[i].y < focused.y) {
					memcpy(array+arraylen, clients+i, sizeof *array);
					arraylen++;
				}
				break;
			case Down:
				if (clients[i].y > focused.y) {
					memcpy(array+arraylen, clients+i, sizeof *array);
					arraylen++;
				}
				break;
			}
		}
	}

	/* sort array of clients from the closest to the currently focused client
	 * to the farthest */
	if (arraylen) {
		qsort(array, arraylen, sizeof *array, clientcmp);
		retval = array[0].win;
	}

	free(array);

	return retval;
}

/* focus window */
static void
focuswin(Window win)
{
	XEvent ev;
	long mask = SubstructureRedirectMask | SubstructureNotifyMask;

	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = netactivewindow;
	ev.xclient.window = win;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 0;
	ev.xclient.data.l[1] = 0;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	if (!supportewmh || !XSendEvent(dpy, root, False, mask, &ev))
		XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
}

/* show usage */
static void
usage(void)
{
	(void)fprintf(stderr, "usage: focus direction\n");
	exit(1);
}
