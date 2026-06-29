// FlPicsee (AKA FL-PicSee)  -- A FLTK-based picture viewer 
// See About_text[] below for copyright and distribution license

#define APP_VER "1.4.0" // Last update 2025-03-04

#ifndef NO_MENU
#define INCL_MENU
#endif

#ifndef NO_PAGING
#define INCL_PAGING
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b))?(a):(b)
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b))?(a):(b)
#endif

#ifndef abs
#define abs(a) ((a < 0) ? (-1*(a)):(a))
#endif

/* ------------------------------------------------------- */

#ifdef INCL_MENU
const char About_text[] = 
"FlPicsee version %s\n"
"copyright 2010-2025 by Michael A. Losh\n"
"\n"
"FlPicsee is free software: you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation, version 3.\n"
"\n"
"FlPicsee is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"See http://www.gnu.org/licenses/ for more details.";
#endif

const char LeftClickHelp_text[] = 
"Zoom: left-click or mouse scroll wheel;   "
"Panning: left-click + drag, or keyboard arrows";
const char RightClickHelp_text[] = 
"Menu: right-click to access menu with specific zoom settings and\n"
"additional commands.";

#ifdef INCL_MENU
const char Help_text[] = 
"FlPicsee Help\n"
"\n"
"Mouse Control:\n"
"%s\n" // LeftClickHelp_text
"%s\n" // RightClickHelp_text
#ifdef INCL_PAGING
"\n"
"Multi-Image Viewing:\n"
"You may specify multiple files and/or directories on the command line.\n"
"Or start FlPicsee with no arguments and Ctrl-click multiple files in \n"
"the File Chooser dialog box. Page to next or previous image by\n"
"clicking in upper left or right window corners, or use any of these keys:\n"
"    Forward: Space, PgDn, Ctrl+N, Ctrl+DownArrow\n"
"    Backward: Backpace, PgUp, Ctrl+P, Ctrl+UpArrow\n"
"Ctrl+S will start or stop the slideshow mode";
#endif
;   
#endif
/*-----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_BMP_Image.H>
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_XBM_Image.H>
#include <FL/Fl_XPM_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>
//#include <FL/Fl_Widget.H>
#include <FL/names.h> 

float ScreenScaleFactor = 1.000;
int Scn_w;
int Scn_h;
float ScnHWRatio;
int TimeToManageImageCache = 0;

enum {NICE_ZOOM = 0, NATIVE_ZOOM = 1, WHOLE_IMG_ZOOM = 2, FILL_ZOOM = 3, SPEC_ZOOM = 4 };

enum {  MI_025_ZOOM = 0, MI_033_ZOOM, MI_050_ZOOM, MI_067_ZOOM, 
        MI_100_ZOOM, MI_150_ZOOM, MI_200_ZOOM, MI_400_ZOOM, MI_800_ZOOM,
        MI_NICE_ZOOM, MI_WHOLE_ZOOM, MI_FILL_ZOOM, 
        MI_ABOUT, MI_HELP, MI_OPEN, MI_IMAGE_PROPS,
#ifdef INCL_PAGING
        MI_PREV, MI_NEXT, MI_SLIDESHOW,
#endif
        MI_RETURN, MI_QUIT
};
    
enum {JPG_TYPE = 1, PNG_TYPE, GIF_TYPE, BMP_TYPE, XPM_TYPE, XBM_TYPE};

float ZoomFactor[] = {
        0.25, 0.3333, 0.50, 0.6667,
        1.00, 1.50, 2.00, 4.00, 8,00};
enum {PAGING_PREV = 1, PAGING_NEXT = 2, PAGING_NEW = 3};

class Fl_Pic_Window; // forward reference

class image_set_item
{
    public:
        Fl_Pic_Window*  wnd_p;
        //Fl_Box*    pic_box_p;

#ifdef INCL_PAGING
        image_set_item* prev_p;
        image_set_item* next_p;
		int index;
#endif
        char fullfilename[256];
        char imgname[256];
        char imgpath[256];
        char ext[8];
        time_t filedate;
        int image_type; 
        Fl_Image * im_orig_p; 
        Fl_Image * im_spec_p;   // zoomed to a specified level
        int zoommode;
        int old_zoommode;
        int img_h, img_w;   // original size of image
        int spec_h, spec_w; // size of zoomed image at a specified "nice" zoom ratio
        int wnd_h, wnd_w;   // current window size
        int xoffset, yoffset;
        float zoompct;      // scaling "zoom" as ratio, 1.00 = 100%
        float spec_zoompct; // scaling at a "nice" ratio
        float old_zoompct;
        char scalestr[8];
        char buf[256];
        char *p;
        int len;
        float spec_zoom_ratio;
        
        float img_hwratio;

        image_set_item(Fl_Pic_Window*  my_wnd_p, char* filename);
        ~image_set_item() ;
#ifdef INCL_PAGING
        image_set_item* link_after(image_set_item* newitem_p);
#endif
        void prep_image(void); 
        void rescale(void);
        void handle_zoom(int dy);
        int  load_image_file(void);
        int  load_image(void);
        void uncache(void);        
};

#ifdef INCL_PAGING
int Slideshow = 0;
int TimeForNextSlide_t = -1;
int SlideInterval_sec = 6;
class rect
{
    public:
        int x;
        int y;
        int w;
        int h;
        rect(int xx, int yy, int ww, int hh) : x(xx), y(yy), w(ww), h(hh) {};
};

// Constrained string copy func that ensures result is zero-terminated
char* strnzcpy(char* dstr, const char* sstr, size_t buflen) {
    int n = buflen - 1;
    if (n < 0) n = 0;
    char* ret = strncpy(dstr, sstr, n);
    dstr[n] = '\0';
    return ret;
} 

// The class Fl_DND_Box is based on an article "Initiating Drag and Drop by Example" 
// by "alvin" on the FLTK.org website, which carries a general copyright
// "1998-2008 by Bill Spitzak and others"
class Fl_DND_Box : public Fl_Box
{
    public:
        static void callback_deferred(void *v);
        Fl_DND_Box(int X, int Y, int W, int H, const char *L = 0);
        virtual ~Fl_DND_Box();
        int event();
        const char* event_text();
        int event_length();
        int handle(int e);

    protected:
        // The event which caused Fl_DND_Box to execute its callback
        int evt;
        char *evt_txt;
        int evt_len;
};

void dnd_cb(Fl_Widget *o, void *v);

void Fl_DND_Box::callback_deferred(void *v)
{
    Fl_DND_Box *w = (Fl_DND_Box*)v;

    w->do_callback();
}

Fl_DND_Box::Fl_DND_Box(int X, int Y, int W, int H, const char *L)
        : Fl_Box(X,Y,W,H,L), evt(FL_NO_EVENT), evt_txt(0), evt_len(0)
{
    labeltype(FL_NO_LABEL);
    box(FL_NO_BOX);
    clear_visible_focus();
}

Fl_DND_Box::~Fl_DND_Box()
{
    delete [] evt_txt;
}

int Fl_DND_Box::event()
{
    return evt;
}

const char* Fl_DND_Box::event_text()
{
    return evt_txt;
}

int Fl_DND_Box::event_length()
{
    return evt_len;
}


int Fl_DND_Box::handle(int e)
{
    //printf("Fl_DND_Box event %d\n", e); fflush(0);
    switch(e)
    {

        /* Receiving Drag and Drop */
        case FL_DND_ENTER:
        case FL_DND_RELEASE:
        case FL_DND_LEAVE:
            evt = e;
            return 1;
            
        case FL_DND_DRAG:
            evt = e;
            //printf("DND Drag event... mouse y =%d\n", Fl::event_y());
            do_callback();
            return 1;

        case FL_PASTE:
            evt = e;

            // make a copy of the DND payload
            evt_len = Fl::event_length() + 8;

            delete [] evt_txt;

            evt_txt = new char[evt_len];
            strnzcpy(evt_txt, Fl::event_text(), evt_len);

            // If there is a callback registered, call it.
            // The callback must access Fl::event_text() to
            // get the string or file path(s) that was dropped.
            // Note that do_callback() is not called directly.
            // Instead it will be executed by the FLTK main-loop
            // once we have finished handling the DND event.
            // This allows caller to popup a window or change widget focus.
            if(callback() && ((when() & FL_WHEN_RELEASE) || (when() & FL_WHEN_CHANGED)))
                Fl::add_timeout(0.0, Fl_DND_Box::callback_deferred, (void*)this);
            return 1;
    }

    return Fl_Box::handle(e);
}

#endif

#ifdef INCL_MENU
static void MenuCB(Fl_Widget* window_p, void *userdata);
Fl_Menu_Item right_click_menu[22] = {
    {"image prop&erties",   FL_CTRL+'e', MenuCB, (void*)MI_IMAGE_PROPS},
    {"&about FlPicsee",     FL_F + 10, MenuCB, (void*)MI_ABOUT},
    {"&help",               FL_F + 1,  MenuCB, (void*)MI_HELP,FL_MENU_DIVIDER},
#ifdef INCL_PAGING
    {"&prev. Image",        FL_BackSpace,    MenuCB, (void*)MI_PREV},
    {"&next Image",         ' ',    MenuCB, (void*)MI_NEXT},
    {"&slideshow start/stop",FL_CTRL+'s',    MenuCB, (void*)MI_SLIDESHOW,FL_MENU_DIVIDER},
#endif
    
    {"25% zoom",            0,          MenuCB, (void*)MI_025_ZOOM},
    {"&33% zoom",           FL_CTRL+'3', MenuCB, (void*)MI_033_ZOOM},
    {"&50% zoom",           FL_CTRL+'5', MenuCB, (void*)MI_050_ZOOM},
    {"&67% zoom",           FL_CTRL+'6', MenuCB, (void*)MI_067_ZOOM},
    {"&100% zoom",          FL_CTRL+'1', MenuCB, (void*)MI_100_ZOOM},
    {"150% zoom",           0,          MenuCB, (void*)MI_150_ZOOM},
    {"&200% zoom",          FL_CTRL+'2', MenuCB, (void*)MI_200_ZOOM},
    {"&400% zoom",          FL_CTRL+'4', MenuCB, (void*)MI_400_ZOOM,},
    {"&800% zoom",          FL_CTRL+'8', MenuCB, (void*)MI_800_ZOOM,},
    {"\"ni&ce\" zoom",      FL_CTRL+'c', MenuCB, (void*)MI_NICE_ZOOM},
    {"&whole image",        FL_CTRL+'w', MenuCB, (void*)MI_WHOLE_ZOOM},
    {"&fill window",        FL_CTRL+'f', MenuCB, (void*)MI_FILL_ZOOM,FL_MENU_DIVIDER},
    {"&open another...",    FL_CTRL+'o', MenuCB, (void*)MI_OPEN},
    {"&return",             FL_CTRL+'r', MenuCB, (void*)MI_RETURN},
    {"&quit",               FL_CTRL+'q', MenuCB, (void*)MI_QUIT},
    {0}
};
#endif

int Running = 0;
int GetAnotherImage = 0;
short int Images = 0;
char Filename[256] = {0};
image_set_item* HeadImgItem_p = NULL;
Fl_File_Chooser* FCD_p = NULL;

char * select_image_file(void) {
    FCD_p->show();
    while (FCD_p->visible() == 1) {
        Fl::wait();
    }
    if (FCD_p->value()) {
        strncpy(Filename, FCD_p->value(), 255);
        return Filename;
    }
    else {
        *Filename = '\0';
        return NULL;
    }
}

#ifdef INCL_PAGING
static void PagingCB(Fl_Widget* window_p, void *data);

int pt_in_rect(int x, int y, rect* r)
{
    if (x < r->x)           return 0;
    if (x >= r->x + r->w)   return 0;
    if (y < r->y)           return 0;
    if (y >= r->y + r->h)   return 0;

    // otherwise...
    return 1;
}
#endif

class Fl_Pic_Window : public Fl_Double_Window {
protected:  
    char wintitle[256];
    //char tiptitle[256];
    int handle(int e) ;

public:
    Fl_Box*         pic_box_p;
    Fl_Scroll*      scroll_p; //scroll(0, 0, wnd_w, wnd_h);
    image_set_item* img_p;
#ifdef INCL_PAGING
    Fl_Button*      prev_paging_btn_p;
    rect            prev_paging_rect;
    Fl_Button*      next_paging_btn_p;
    rect            next_paging_rect;
    Fl_DND_Box*     dnd_p;
#endif
    
    Fl_Pic_Window();
    
    ~Fl_Pic_Window();

    void resize(int x, int y, int w, int h);
    
    void specify_zoom(int zoom_mode, float zoom_pct);

    void set_titles(void);

    void delete_image(void);
    
    void setup(void);
    
    void do_menu(void);
    
    void show_image(void);
    
#ifdef INCL_PAGING      
    void scale_paging_buttons(int wnd_w, int wnd_h);
    int image_index(void);
#endif
    
};

Fl_Pic_Window* MainWnd_p = NULL;

void Redraw(void) 
{
    MainWnd_p->redraw();
}

image_set_item::image_set_item(Fl_Pic_Window*  my_wnd_p, char* filename) 
{
    wnd_p = my_wnd_p;
    wnd_w = wnd_p->w();
    wnd_h = wnd_p->h();
    bzero(fullfilename, 256);
    strncpy(fullfilename, filename, 255);
    
#ifdef INCL_PAGING
    prev_p      = this; // linked to just itself initially
    next_p      = this;
#endif
    imgname[0]  = '\0';
    imgpath[0]  = '\0';
    ext[0]      = '\0';
    filedate    = (time_t)0;
    im_orig_p   = NULL; 
    //im_full_p   = NULL;
    //im_fit_p    = NULL;
    im_spec_p   = NULL;
    index       = 0;
    old_zoompct = 0.0;
    old_zoommode = -1;
    zoommode = NICE_ZOOM;
    image_type = 0; // undefined type
}

image_set_item::~image_set_item() {
	uncache();
}

#ifdef INCL_PAGING

void print_img_list(void) 
{
    image_set_item * img_p;
    image_set_item * start_p = NULL;
    for (img_p = HeadImgItem_p; img_p != start_p; img_p = img_p->next_p) {
        printf("\nImage %d (%s) \tprev %d\tnext %d\n",
                img_p->index + 1, img_p->fullfilename, 
                img_p->prev_p->index + 1, img_p->next_p->index + 1); fflush(0); 
        if (!start_p) start_p = HeadImgItem_p;
    }
}

image_set_item* image_set_item::link_after(image_set_item* newitem_p) 
{
    image_set_item * nextnext_p = next_p; 
    newitem_p->next_p = nextnext_p;
    nextnext_p->prev_p = newitem_p;
    
    newitem_p->prev_p = this;
    next_p = newitem_p;
    
    newitem_p->index = index + 1;
//print_img_list();
    return newitem_p;
}
#endif
void image_set_item::uncache(void) {	
	if (im_orig_p) {
		//printf("Uncaching image %d\n", index + 1);
		delete im_orig_p;
		im_orig_p   = NULL;
	} /*
	if (im_full_p) {
		delete im_full_p;
		im_full_p   = NULL;
	} 
	if (im_fit_p) {
		delete im_fit_p;
		im_fit_p   = NULL;
	} */
	if (im_spec_p) {
		delete im_spec_p;
		im_spec_p   = NULL;
	} 
}

void image_set_item::prep_image(void) 
{
	/*
	printf("-----------------------\n");
	printf("prep_image: %s\n", fullfilename); 
	printf("prep_image: wnd_w %d, wnd_h %d; w() %d, h() %d\n", wnd_w, wnd_h, wnd_p->w(), wnd_p->h()); 
	printf("prep_image: zoommode %d, old_zoommode %d\n", zoommode, old_zoommode); 
	printf("prep_image: zoompct %1.3f, old_zoompct %1.3f\n", zoompct, old_zoompct); 
	printf("prep_image: spec_w %d, spec_h %d\n", spec_w, spec_h); 
	printf("prep_image: im_spec_p 0x%08X\n", (unsigned int)im_spec_p); fflush(0); */
	if (	(wnd_w != wnd_p->w()) 
		|| 	(wnd_h != wnd_p->h()) 
		|| 	(old_zoommode != zoommode)
		||  (old_zoompct != zoompct)
		||  ((zoompct != 1.000) && (!im_spec_p))) {
		wnd_w = wnd_p->w();
		wnd_h = wnd_p->h();
		rescale();
	}
}

void image_set_item::rescale(void) {
    float wnd_hwratio = (float)wnd_h / (float)wnd_w;
    xoffset = 0;
    yoffset = 0;

	switch (zoommode) {
		case NATIVE_ZOOM:
			zoompct = 1.0;
			break;
			
		case FILL_ZOOM:
			if (img_hwratio > wnd_hwratio) {
				zoompct = (float)(wnd_w - 36) / (float)img_w;
			}
			else {
				zoompct = (float)(wnd_h - 36) / (float)img_h;
			}
			break;
			
		case SPEC_ZOOM:
			zoompct = spec_zoompct;
			break;
			
		case NICE_ZOOM:
		case WHOLE_IMG_ZOOM:
			if (img_hwratio > wnd_hwratio) {
				zoompct = (float)wnd_h / (float)img_h;
			}
			else {
				zoompct = (float)wnd_w / (float)img_w;
			}
			if (zoommode == NICE_ZOOM) {
				if (zoompct > 1.880) {
					spec_zoompct = zoompct = 2.000;
				}
				else if (zoompct > 0.880) {
					spec_zoompct = zoompct = 1.000;
				}
				else if (zoompct > 0.700) {
					spec_zoompct = zoompct = 0.750;
				}
				else if (zoompct > 0.600) {
					spec_zoompct = zoompct = 0.667;
				}
				else if (zoompct > 0.420) {
					spec_zoompct = zoompct = 0.500;
				}
				else if (zoompct > 0.300) {
					spec_zoompct = zoompct = 0.333;
				}
				else if (zoompct > 0.220) {
					spec_zoompct = zoompct = 0.250;
				}
			}
			break;
			
	} 
	//printf("Zoommode %d, Old zoompct %1.2f, zoompct %1.2f\n", zoommode, old_zoompct, zoompct); fflush(0);
	if (zoompct != old_zoompct) {
		if (im_spec_p) {
			delete im_spec_p;
			im_spec_p = NULL;
		}
		spec_h = (int)((float)img_h * zoompct);
		spec_w = (int)((float)img_w * zoompct);
	}
	if (zoompct != 1.000 && !im_spec_p) {
		
		im_spec_p = im_orig_p->copy(spec_w, spec_h);
		//printf("Copied %s (orig %d by %d) to size %d by %d, im_spec_p is now 0x%08X\n", fullfilename, img_w, img_h, spec_w, spec_h, im_spec_p); fflush(0);
	}
	old_zoompct = zoompct;
	old_zoommode = zoommode;
}

void image_set_item::handle_zoom(int dy) {
    if (zoompct > 1.0) {
        dy *= -1;
    }
    if (dy > 0) {
        if (zoommode < 2) zoommode++;
    }
    else if (dy < 0) {
        if (zoommode > 0) zoommode--;
    }
    prep_image();
}

// returns 1 or higher if filename has a known file type, 0 if not
int file_image_type(const char* fullfilename) 
{
    int img_type = 0; 
    int imfd = open(fullfilename, O_RDONLY);
    if (-1 == imfd) {
        perror("open");
        return img_type;
    }
    
    char imbyte[16];
    unsigned char* imubyte = (unsigned char*)imbyte; 
    if (16 != read(imfd, imbyte, 16)) {
        perror("read");
        close(imfd);
        return img_type;
    }

    if      (!strncmp(imbyte+6, "JFIF", 4))  img_type = JPG_TYPE;
    else if (!strncmp(imbyte+6, "Exif", 4))  img_type = JPG_TYPE;
    else if (   (imubyte[0] == 0xFF) 
             && (imubyte[1] == 0xD8) )       img_type = JPG_TYPE;
    else if (!strncmp(imbyte+1, "PNG",  3))  img_type = PNG_TYPE;
    else if (!strncmp(imbyte+0, "GIF",  3))  img_type = GIF_TYPE;
    else if (!strncmp(imbyte+0, "BM",   2))  img_type = BMP_TYPE;
    else if (!strncmp(imbyte+3, "XPM",  3))  img_type = XPM_TYPE;
    else if (!strncmp(imbyte+0, "#def", 4))  img_type = XBM_TYPE;
    close(imfd);
/*
printf("Image type is %d\nHeader bytes: ", img_type); fflush(0);
int n;
for (n = 0; n < 16; n++) printf("0x%02X ", imubyte[n]);
printf("\n"); */
    return img_type;
}

int image_set_item::load_image_file(void) {
	im_orig_p = NULL;
	if (!image_type) {
		image_type = file_image_type(fullfilename);
	}
    switch(image_type) {
        case JPG_TYPE:
            im_orig_p = new Fl_JPEG_Image(fullfilename); 
            break;
        case PNG_TYPE:
            im_orig_p = new Fl_PNG_Image(fullfilename);
            break;
        case BMP_TYPE:
            im_orig_p = new Fl_BMP_Image(fullfilename);
            break;
        case GIF_TYPE:
            im_orig_p = new Fl_GIF_Image(fullfilename); 
            break;
        case XBM_TYPE:
            im_orig_p = new Fl_XBM_Image(fullfilename); 
            break;
        case XPM_TYPE:
            im_orig_p = new Fl_XPM_Image(fullfilename); 
            break;
        default:
            im_orig_p = NULL;
    }
#ifdef DEBUG
    printf("Loaded image %d (file %s), type is %d, img pointer is 0x%16llX\n", 
			index + 1, fullfilename, image_type, (unsigned long long)(void*)im_orig_p);
#endif
	return (im_orig_p != NULL);
}

int image_set_item::load_image(void)
{
    char *fn_p;
    bzero(imgpath, 256);
    bzero(imgname, 256);
    bzero(scalestr, 8);
    
    //static int first_image = 1;

    len = strlen(fullfilename);
    if (len > 4 && strstr(fullfilename, "/")) {
        strncpy(imgpath, fullfilename, 255);
        fn_p = imgpath + len - 1;
        while (*fn_p != '/'&& fn_p > imgpath) {
            fn_p--;
        }
        if (*fn_p == '/') { 
            *fn_p = '\0';
            fn_p++;
        }
        strncpy(imgname, fn_p, 255);
    }
    else {
        getcwd(imgpath, 255);
        strcpy(imgname, fullfilename);
    }

	if (!load_image_file()) {
		return 0;
	}
    img_h = im_orig_p->h(); 
    img_w = im_orig_p->w(); 

    img_hwratio = (float)img_h / (float)img_w;
    xoffset = 0;
    yoffset = 0;

    //~ printf("load_image(): Scn_h %d, img_h %d; Scn_w %d, img_w %d\n", Scn_w, img_w, Scn_w, img_w);
	wnd_h = MAX(MIN(Scn_h, img_h + 32), 90);
	wnd_w = MAX(MIN(Scn_w, img_w + 32), 120);

    return 1;
}

Fl_Pic_Window::Fl_Pic_Window() 
    :   Fl_Double_Window(Fl::w(), Fl::h())
#ifdef INCL_PAGING
    , 
        prev_paging_rect(10, 10, 64, 32),
        next_paging_rect(50, 10, 64, 32)
#endif
{
    scroll_p  = NULL;
    pic_box_p = NULL;
    
    img_p = NULL; 
    
}

Fl_Pic_Window::~Fl_Pic_Window() {
    delete pic_box_p;
    delete scroll_p; 
    delete img_p;
}

void Fl_Pic_Window::resize(int x, int y, int w, int h)  {
	//~ printf("Fl_Pic_Window::resize(): x %d, y %d, w %d, h %d\n", x, y, w, h);
    //pic_box_p->resize(x, y, w, h);  //Fl_Group::resize()
    Fl_Double_Window::resize(x, y, w, h);
//    img_p->rescale(w, h);
#ifdef INCL_PAGING
    dnd_p->resize(x, y, w, h);
#endif      
    show_image();
    
    Redraw();
}
    
void Fl_Pic_Window::set_titles(void) {
	char fname[64] = {0};
	char fpath[64] = {0};
    sprintf(img_p->scalestr, "%4d%%", (int) (100.0 * img_p->zoompct));
    if (strlen(img_p->imgname) < 64) {
		strcpy(fname, img_p->imgname);
	}
	else {
		strncpy(fname, img_p->imgname, 50);
		strcpy(fname + 48,  "...");
		strcat(fname, img_p->imgname + strlen(img_p->imgname) - 12);
	}
    if (strlen(img_p->imgpath) < 64) {
		strcpy(fpath, img_p->imgpath);
	}
	else {
		strncpy(fpath, img_p->imgpath, 48);
		strcpy(fpath + 48,  "...");
		strcat(fpath, img_p->imgpath + strlen(img_p->imgpath) - 12);
	}
    int th, tw;
#ifdef INCL_PAGING
    char dir_index_str[128] = {0};
    if (Images > 1) {
        sprintf(dir_index_str, " (in dir. %s) %d of %d ", fpath, img_p->index + 1, Images);
    }
    sprintf(wintitle, "%s%s  '%s' %s -  FlPicsee %s", 
            Slideshow ? "SLIDESHOW: " : "", (img_p->zoompct != 1.0) ? img_p->scalestr : "", 
            fname, (Images > 1) ? dir_index_str : "", APP_VER);
     fl_measure(wintitle, tw, th, 0);
     if (tw > (w() - 64)) {
		sprintf(wintitle, "%s%s  '%s' %s", 
				Slideshow ? "SLIDESHOW: " : "", (img_p->zoompct != 1.0) ? img_p->scalestr : "", 
				fname, (Images > 1) ? dir_index_str : "");
	 }
     fl_measure(wintitle, tw, th, 0);
     if (tw > (w() - 64)) {
		sprintf(wintitle, "%s%s  '%s'", Slideshow ? "SLIDESHOW: " : "", 
				(img_p->zoompct != 1.0) ? img_p->scalestr : "", fname);
	 }
#else
     sprintf(wintitle, "%s  '%s' - FlPicsee %s", (img_p->zoompct != 1.0) ? img_p->scalestr : "", 
 			fname, APP_VER);
     fl_measure(wintitle, tw, th, 0);
     if (tw > (w() - 64)) {
		 sprintf(wintitle, "%s  '%s'", (img_p->zoompct != 1.0) ? img_p->scalestr : "", 
				fname);
	 }
#endif
    label(wintitle);
}

#ifdef INCL_PAGING
void Fl_Pic_Window::scale_paging_buttons(int wnd_w, int wnd_h) 
{
    prev_paging_rect.y = 8;
    prev_paging_rect.h = MAX(24, wnd_h/20);
    prev_paging_btn_p->resize(prev_paging_rect.x, prev_paging_rect.y,prev_paging_rect.w, prev_paging_rect.h);

    next_paging_rect.x = wnd_w - 64;
    next_paging_rect.y = 8;
    next_paging_rect.h = MAX(24, wnd_h/20);
    next_paging_btn_p->resize(next_paging_rect.x, next_paging_rect.y,next_paging_rect.w, next_paging_rect.h);
}
#endif

#ifdef INCL_MENU
void Fl_Pic_Window::do_menu(void) {
    const Fl_Menu_Item *m = right_click_menu->popup(Fl::event_x(), Fl::event_y(), "FlPicsee", 0, 0);
    if ( m ) m->do_callback(this, m->user_data());
};
#endif

int Fl_Pic_Window::handle(int e) {
    //char s[200];
    int ret = 0;
    int dy = 0;
    //int needredraw = 0;
    int dragmovx, dragmovy;
    static int dragx = 0;
    static int dragy = 0;

	static int sbsx = 0;  // scroll bar start x for drag event
	static int sbsy = 0;  // .. y
	static int saw = 0;   // scrollbar area width
	static int sah = 0;   // .. height
	static int pbw = 0;   // pic box width
	static int pbh = 0;   //.. height
	static int sbw = 0;   // vscrollbar width  or zero if hidden
	static int sbh = 0;   // hscrollbar height or zero if hidden
	static int scrlxend = 0;  // farthest to right in image you can scroll
	static int scrlyend = 0;  // .. to bottom
	
    
    static int ctrl_pressed = 0;
    static char tiptext[300] = {0};
    int key;
	//~ printf("MainWnd Event %d %s\n", e,  fl_eventnames[e]);
    switch ( e ) {
#ifdef INCL_MENU
        case FL_KEYDOWN:
            key = Fl::event_key();
            //~ printf("You pressed key %d (0x%4X)\n", key, key);
            if (key == FL_Control_L || key == FL_Control_R) {
                ctrl_pressed = 1;
            }
            if (key == FL_Menu || key == 0xFFEC || key == 0 ) {                
                do_menu();
                return 1;
            } 
            else if (key == '-') {
                dy = +1;
                img_p->handle_zoom(dy);
				show_image();
                return 1;                
            }
            else if (key == '=') {
                dy = -1;
                img_p->handle_zoom(dy);
                show_image();
                return 1;                
            }
#ifdef INCL_PAGING
            else if (key == FL_Page_Up || key == FL_BackSpace) {
                PagingCB((Fl_Widget*)NULL, (void *)PAGING_PREV);
            }
            else if (key == FL_Page_Down || key == ' ') {
                PagingCB((Fl_Widget*)NULL, (void *)PAGING_NEXT);
            }
#endif
            else if (ctrl_pressed) {
                long int choice = 0;
                switch(key) {
                    case 'e': choice = MI_IMAGE_PROPS; break;
                    case FL_F + 10:
                    case 'a': choice = MI_ABOUT; break;
                    case FL_F + 1:
                    case 'h': choice = MI_HELP; break;
#ifdef INCL_PAGING
                    case 'p': 
                    case FL_BackSpace:
                    case FL_Up: choice = MI_PREV; break;
                    case 'n':
                    case ' ':
                    case FL_Down: choice = MI_NEXT; break;
                    case 's': choice = MI_SLIDESHOW; break;
#endif
                    case '3': choice = MI_033_ZOOM; break;
                    case '5': choice = MI_050_ZOOM; break;
                    case '6': choice = MI_067_ZOOM; break;
                    case '1': choice = MI_100_ZOOM; break;
                    case '2': choice = MI_200_ZOOM; break;
                    case '4': choice = MI_400_ZOOM; break;
                    case '8': choice = MI_800_ZOOM; break;
                    case 'c': choice = MI_NICE_ZOOM; break;
                    case 'w': choice = MI_WHOLE_ZOOM; break;
                    case 'f': choice = MI_FILL_ZOOM; break;
                    case 'o': choice = MI_OPEN; break;
                    case 'r': choice = MI_RETURN; break;
                    case 'q': choice = MI_QUIT; break;
                    default: 
                        //printf("You pressed key %d (0x%4X)\n", key, key);
                        break;
                }
                if (choice) MenuCB(this, (void*)choice);
            }
            break;
        case FL_KEYUP:
            key = Fl::event_key();
            if (key == FL_Control_L || key == FL_Control_R) {
                ctrl_pressed = 0;
            }
            if ((key == FL_Up || key == '=')) {             
                return 1;
            }
			//printf("keyup event: scroll pos x %d, y %d\n", scroll_p->xposition(), scroll_p->yposition());

            break;
#endif
        case FL_PUSH:
            dragx = Fl::event_x();
            dragy = Fl::event_y();
            if ( Fl::event_button() == FL_LEFT_MOUSE ) {
                if (!Fl_Double_Window::handle(e)) {
                    label(LeftClickHelp_text);
                    sbsx = scroll_p->xposition();
                    sbsy = scroll_p->yposition();
                    pbw = pic_box_p->w();
                    pbh = pic_box_p->h();
                    //~ printf("LftClk: scrollX %d, scrollY %d, pic_box X %d, Y %d, W %d, H %d\n", 
							//~ sbsx, sbsy, pic_box_p->x(), pic_box_p->y(), pbw, pbh);
                    saw = scroll_p->w();
                    sah = scroll_p->h();
                    sbw = scroll_p->scrollbar.visible()  ? scroll_p->scrollbar.w()  : 0;  // size v scrollbar takes in width, if any
                    sbh = scroll_p->hscrollbar.visible() ? scroll_p->hscrollbar.h() : 0;  // size h scrollbar takes in height, if any 
					scrlxend = pbw - (saw - sbw);
					scrlyend = pbh - (sah - sbh);

                    //~ printf("   scrollbar sizes: w %d, h %d; scroll end x %d, t %d\n", sbw, sbh, scrlxend, scrlyend);
							
                    fl_cursor(FL_CURSOR_MOVE, FL_BLACK, FL_WHITE);
                    return 1;
                }
            }
#ifdef INCL_MENU
            else if (Fl::event_button() == FL_RIGHT_MOUSE ) {
                tooltip("");
                do_menu();
                return(1);          // (tells caller we handled this event)
            }
#endif
            break;
        case FL_DRAG:
            if ( Fl::event_button() == FL_LEFT_MOUSE ) {
                if (!Fl_Double_Window::handle(e)) {
                    dragmovx = Fl::event_x() - dragx; //scroll_p->xposition();
                    dragmovy = Fl::event_y() - dragy; //scroll_p->yposition();
                    //~ printf("Drag: dragmovx %d, dragmovy %d, mouseX %d, mouseY %d\n", dragmovx, dragmovy, Fl::event_x(), Fl::event_y());
                    
                    int sbx = scroll_p->xposition();
                    int sby = scroll_p->yposition();
                    int nsbx = sbx;
                    int nsby = sby;
                    if (saw < pbw && dragmovx != 0) {
						nsbx = sbsx - dragmovx;
						nsbx = MAX(nsbx, 0);
						nsbx = MIN(nsbx, scrlxend);
                        //~ printf("   scrollbar pos x %d; dragging x to %d\n", sbx, nsbx);
                    }
                    if (sah < pbh && dragmovy != 0) {
						nsby = sbsy - dragmovy;
						nsby = MAX(nsby, 0);
						nsby = MIN(nsby, scrlyend);
                        //~ printf("   scrollbar pos y %d; dragging y to %d\n", sby, nsby);
                    } 
                    if (nsbx != sbx || nsby != sby) {
                        scroll_p->scroll_to(nsbx, nsby);
                        Redraw();
                    }
					//~ printf("   new scroll pos x %d, y %d\n", scroll_p->xposition(), scroll_p->yposition());
                    return 1;
                }
            }
            break;
        case FL_RELEASE:
            if ( Fl::event_button() == FL_LEFT_MOUSE) {
                if (!Fl_Double_Window::handle(e)) {
                    if (abs(dragx - Fl::event_x()) < 3 && abs(dragy - Fl::event_y()) < 3) {
                        img_p->zoommode = (img_p->zoommode + 1) % 3;
                        show_image();
                        Redraw(); 
                    }
                    fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
                    set_titles();
                    return 0;
                }
            }
            break;
            
#ifdef INCL_PAGING              
        case FL_MOVE:
            if (Images > 1 && pt_in_rect( Fl::event_x(), Fl::event_y(), &prev_paging_rect   )) {
                prev_paging_btn_p->show();
                sprintf(tiptext, "Prev: %s", img_p->prev_p->fullfilename);
                prev_paging_btn_p->tooltip(tiptext);
                Fl_Tooltip::enter(prev_paging_btn_p);
                return 0;
            } 
            else if (Images > 1 && pt_in_rect( Fl::event_x(), Fl::event_y(), &next_paging_rect )) {
                next_paging_btn_p->show();
                sprintf(tiptext, "Next: %s", img_p->next_p->fullfilename);
                next_paging_btn_p->tooltip(tiptext);
                Fl_Tooltip::enter(next_paging_btn_p);
                return 0;
            } 
            else {
                prev_paging_btn_p->hide();
                next_paging_btn_p->hide();
                set_titles();
                Fl_Tooltip::enter(pic_box_p);
                return 0;
            }
            break;
#endif
        case FL_MOUSEWHEEL:
            dy = Fl::event_dy();
            img_p->handle_zoom(dy);
            show_image();
            //redraw(); 
            return 1;
            break;
                
        case FL_HIDE:
            return 1;
            break;

        case FL_SHOW:
            return 1;
            break;
            
        default:
            ret = Fl_Double_Window::handle(e);
            break;

    }

    return(ret);
}

void Fl_Pic_Window::specify_zoom(int zoom_mode, float zoom_pct) {
    img_p->zoommode = zoom_mode;
    if (img_p->zoommode == SPEC_ZOOM) {
        img_p->spec_zoompct = zoom_pct;
        img_p->zoompct = zoom_pct;
    }
    show_image();
}

void Fl_Pic_Window::delete_image(void) {
    delete img_p;       img_p = NULL;
    delete pic_box_p;   pic_box_p = NULL;
    delete scroll_p;    scroll_p = NULL;
}

void report_invalid_file(char* filename) {
    printf("Image '%s' not found or viewable.\n", filename);
    fl_alert("Image '%s' not found or viewable.\n", filename);
}

void Fl_Pic_Window::setup(void) 
{
    int wnd_x, wnd_y;

    if (!img_p->load_image()) {
        report_invalid_file(HeadImgItem_p->fullfilename);
        return;
    }
    wnd_x = (Scn_w / 2) - (img_p->wnd_w / 2); 
    wnd_y = (Scn_h  / 2) - (img_p->wnd_h / 2);
    if (wnd_y < 24) wnd_y = 24; // don't position it under task bar
//    w(img_p->wnd_w);
//    h(img_p->wnd_h);
//    x(wnd_x);
//    y(wnd_y);
//    resize(wnd_x, wnd_y, img_p->wnd_w, img_p->wnd_h);
    if (!scroll_p) {
		begin();
#ifdef INCL_PAGING
        dnd_p = new Fl_DND_Box(0, 0, w(), h());
        dnd_p->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
        dnd_p->callback(dnd_cb);
#endif      
        Fl_Group::resize(wnd_x, wnd_y, img_p->wnd_w, img_p->wnd_h);
    //~ printf("Resizing window to %d wide by %d high\n", img_p->wnd_w, img_p->wnd_h); fflush(0);
        scroll_p = new Fl_Scroll(0, 0, img_p->wnd_w, img_p->wnd_h);
        scroll_p->box(FL_NO_BOX);
        scroll_p->begin();
        if (!pic_box_p) pic_box_p = new Fl_Box(0, 0, img_p->wnd_w, img_p->wnd_h);
        scroll_p->end();
        
#ifdef INCL_PAGING
        prev_paging_btn_p = new Fl_Button(10, 10, 64, 32,  "@< Prev");
        prev_paging_btn_p->box(FL_RFLAT_BOX);
        prev_paging_btn_p->color(FL_WHITE);
        prev_paging_btn_p->labelsize(14);
        prev_paging_btn_p->callback(PagingCB, (void*)PAGING_PREV);
        prev_paging_btn_p->hide();
        prev_paging_btn_p->clear_visible_focus();
        
        
        next_paging_btn_p = new Fl_Button(10, 40, 64, 32, "Next @>");
        next_paging_btn_p->box(FL_RFLAT_BOX);
        next_paging_btn_p->color(FL_WHITE);
        next_paging_btn_p->labelsize(14);
        next_paging_btn_p->callback(PagingCB, (void*)PAGING_NEXT);
        next_paging_btn_p->hide();
        next_paging_btn_p->clear_visible_focus();
#endif      
        resizable(pic_box_p); 
        end();
    }
    else {
        Fl_Group::resize(wnd_x,wnd_y, img_p->wnd_w, img_p->wnd_h);
        Fl_Double_Window::resize(wnd_x, wnd_y, img_p->wnd_w, img_p->wnd_h);
        scroll_p->resize(0, 0, img_p->wnd_w, img_p->wnd_h);
    }
    show_image();
}

void Fl_Pic_Window::show_image(void) 
{
	img_p->prep_image();
	if (img_p->zoompct == 1.0) {
		pic_box_p->image(img_p->im_orig_p);
		//printf("Orig Image %s: spec_h %d, spec_w %d, ptr 0x%08X\n", 
		//		img_p->fullfilename, img_p->spec_h, img_p->spec_w, img_p->im_orig_p); fflush(0);
	    //~ printf("Fl_Pic_Window::show_image(): x %d, y %d, w %d, h %d\n", 
				//~ (w() - img_p->img_h) / 2, (h() - img_p->img_h) / 2, 
							//~ img_p->img_w, img_p->img_h);
		pic_box_p->resize((w() - img_p->img_w) / 2, (h() - img_p->img_h) / 2, 
							img_p->img_w, img_p->img_h);
	}
	else {
		pic_box_p->image(img_p->im_spec_p);
		//printf("Scaled Image %s: spec_h %d, spec_w %d, ptr 0x%08X\n", 
		//		img_p->fullfilename, img_p->spec_h, img_p->spec_w, img_p->im_spec_p); fflush(0);
		//~ printf("Fl_Pic_Window::show_image(): x %d, y %d, w %d, h %d\n", 
				//~ (w() - img_p->spec_w) / 2, (h() - img_p->spec_h) / 2, 
							//~ img_p->spec_w, img_p->spec_h);
		pic_box_p->resize((w() - img_p->spec_w) / 2, (h() - img_p->spec_h) / 2, 
							img_p->spec_w, img_p->spec_h);
	}
	
    set_titles();
#ifdef INCL_PAGING
	scale_paging_buttons(w(), h());
#endif
	Redraw();
}

#ifdef INCL_PAGING
/*
int Fl_Pic_Window::image_index(void)
{
    int index = 0;
    image_set_item* set_img_p = NULL;
    if (img_p == HeadImgItem_p) return index + 1;
    set_img_p = HeadImgItem_p;
    while (img_p != set_img_p) {
        index++;
        set_img_p = set_img_p->next_p;
    }
    return index + 1;
    return index;
}
    */

void manage_image_precaching(image_set_item* cur_img_p) {
#define PRECACHE_EACH_DIRECTION_COUNT 2
    //int precache_each_direction_count = PRECACHE_EACH_DIRECTION_COUNT;
    
    int ci = cur_img_p->index;  // current index
    int hi; // possible "higher" index
    int li; // possible "lower" index
    int n;  // distance from specific image
    int cached; 

	image_set_item* img_p = HeadImgItem_p;
	for (int i = 0 ; i < Images ; i++) {
        hi = ci;
        li = ci;
        n = 0;
        cached = 0;
        while (n <= PRECACHE_EACH_DIRECTION_COUNT) {
			//printf("  i %d, li %d, hi %d\n", i, li, hi); fflush(0);
            if (i == hi || i == li) {
                if(img_p->load_image()) {
					img_p->prep_image();
					cached = 1;
					break;
				}
            }
            hi++; if (hi >= Images)  hi -= Images;
            li--; if (li < 0)  li += Images;
            n++;
        }
        if (!cached) img_p->uncache();
        img_p = img_p->next_p;
	}
	//printf("==========\n"); fflush(0);
}

static void PagingCB(Fl_Widget* window_p, void *data)
{
    long direction = (long)data;
    
    if (direction == PAGING_NEXT) {
        MainWnd_p->img_p = MainWnd_p->img_p->next_p;
    }
    else if (direction == PAGING_PREV) {
        MainWnd_p->img_p = MainWnd_p->img_p->prev_p;
    }
    TimeForNextSlide_t = time(NULL) + SlideInterval_sec;
    if (!MainWnd_p->img_p->im_orig_p) {
        if (!MainWnd_p->img_p->load_image()) {
			report_invalid_file(MainWnd_p->img_p->fullfilename);
			MainWnd_p->img_p = MainWnd_p->img_p->next_p;
		}
    }
	//manage_image_precaching(MainWnd_p->img_p);
	TimeToManageImageCache = time(NULL) + 1;
    MainWnd_p->show_image();
}

void dnd_cb(Fl_Widget *o, void *v)
{
    Fl_DND_Box *dnd = (Fl_DND_Box*)o;
    //static int c = 0;

    switch(dnd->event())
    {
        case FL_DND_DRAG:           // User is dragging widget
            //printf("DND drag event %d\n", c++); fflush(0);
            break;

        case FL_PASTE:          // DND Drop
        {
            const char* c = dnd->event_text(); 
            //~ printf("DND paste event:\n======\n%s\n======\n", c); fflush(0);
            char dropfile_sz[1024];
            char *p = dropfile_sz;
            int cnt = 0;
            int t = 0;
            while( *c != '\0')
            {
                if(*c == '\n')
                    cnt++;
                c++;
            }
            image_set_item* lead_img_item_p = NULL;
            //printf("%d file(s) dropped onto FlPicsee\n", cnt); fflush(0);
            c = dnd->event_text(); 
            while(*c && t < cnt) {
                while(*c && strncmp(c, "file://", 7)) c++;
                //printf("REMAINING IN COPY:\n%s\n=============\n", c); fflush(0);
                c += 7;
                while(*c && *c != '\n') {
                    *p++ = *c++;
                }
                *p = '\0';
                c++;
                image_set_item* img_item_p;
                img_item_p = new image_set_item(MainWnd_p, dropfile_sz);
                Images++;
                
                //image_set_item* next_p = MainWnd_p->img_p->next_p;
                HeadImgItem_p->prev_p->link_after(img_item_p);
                img_item_p->load_image();
                if (!lead_img_item_p) lead_img_item_p = img_item_p;
                //printf("Image info:\n  name %s\n  path %s\n  type %d\n  -----\n",
                //        img_item_p->imgname, img_item_p->imgpath, img_item_p->image_type);
                //        fflush(0);                  
                t++;
                p = dropfile_sz;
            }
            MainWnd_p->img_p = lead_img_item_p;
            PagingCB((Fl_Widget*)NULL, (void *)PAGING_NEW);
            break;
        } // end paste case
    } // end switch
}

#endif

#ifdef INCL_MENU
#include <FL/Fl_PNG_Image.H>
static void MenuCB(Fl_Widget* window_p, void *userdata) 
{
    long choice = (long)userdata;
    //int spec_w, spec_h;
    //float speczoomfactor;
    image_set_item* new_img_p;
#ifdef INCL_PAGING
    char fullfilename[256] = {0};
    image_set_item* next_new_img_p = NULL;
#endif 

    Fl_Pic_Window* mainwnd_p = (Fl_Pic_Window *)window_p;
    switch (choice) {
		case MI_IMAGE_PROPS:
			fl_message("File: %s\nDimensions: %d W by %d H", MainWnd_p->img_p->fullfilename, MainWnd_p->img_p->img_w, MainWnd_p->img_p->img_h);
			break;
        case MI_ABOUT:
            fl_message(About_text, APP_VER);
            break;
        case MI_HELP:
            fl_message(Help_text, LeftClickHelp_text, RightClickHelp_text);
            break;
        case MI_025_ZOOM:
        case MI_033_ZOOM:
        case MI_050_ZOOM:
        case MI_067_ZOOM: 
        case MI_150_ZOOM: 
        case MI_200_ZOOM: 
        case MI_400_ZOOM:
        case MI_800_ZOOM:
            mainwnd_p->specify_zoom(SPEC_ZOOM, ZoomFactor[choice]);
            break;
        case MI_NICE_ZOOM:
            mainwnd_p->specify_zoom(NICE_ZOOM, 0);
            break;
        case MI_WHOLE_ZOOM:
            mainwnd_p->specify_zoom(WHOLE_IMG_ZOOM, 0);
            break;
        case MI_FILL_ZOOM:
            mainwnd_p->specify_zoom(FILL_ZOOM, 0);
            break;
        case MI_100_ZOOM:
            mainwnd_p->specify_zoom(NATIVE_ZOOM, 0);
            break;
        case MI_OPEN:
            select_image_file();
        
            if (strlen(Filename) > 0) {
                if (file_image_type(Filename)) {
//                    printf("Loading image %s\n", Filename);
                    new_img_p = new image_set_item(mainwnd_p, Filename);
#ifdef INCL_PAGING
                    HeadImgItem_p->prev_p->link_after(new_img_p);
                    HeadImgItem_p->prev_p = new_img_p;
                    Images++;
                    int c, i;
                    i = 2;
                    c = FCD_p->count();
                    while (i <= c) {
                        if (file_image_type(FCD_p->value(i)) > 0) {
                            strncpy(fullfilename, FCD_p->value(i), 255);
//                            printf("Loading image %s\n", fullfilename);
                            next_new_img_p = new image_set_item(MainWnd_p, fullfilename);
                            if (HeadImgItem_p->prev_p) {
                                HeadImgItem_p->prev_p->link_after(next_new_img_p);
                                HeadImgItem_p->prev_p = next_new_img_p;
                            }
                            Images++;
                        }
                        i++;
                    }
                    mainwnd_p->img_p = new_img_p;
                    PagingCB((Fl_Widget*)NULL, (void *)PAGING_NEW);
#else
                    delete mainwnd_p->img_p;
                    mainwnd_p->img_p = new_img_p;
                    if (!MainWnd_p->img_p->im_orig_p) {
                        MainWnd_p->img_p->load_image();
                    }
                    MainWnd_p->img_p->resize(MainWnd_p->w(), MainWnd_p->h());
#endif
                }
                else {
                    report_invalid_file(Filename);
                }
            }
            break;
#ifdef INCL_PAGING
        case MI_PREV:
            PagingCB((Fl_Widget*)NULL, (void *)PAGING_PREV);
            break;
        case MI_NEXT:
            PagingCB((Fl_Widget*)NULL, (void *)PAGING_NEXT);
            break;
        case MI_SLIDESHOW:
            if (Images > 1) {
                Slideshow = !Slideshow;
                if (Slideshow) TimeForNextSlide_t = time(NULL) + SlideInterval_sec;
                else TimeForNextSlide_t = -1;
            }
            break;
#endif
        case MI_QUIT:
            Running = 0;
            break;
            
        default:
            break;
    }
    mainwnd_p->set_titles();

}
#endif

int main(int argc, char** argv) 
{
    int a = 1;
    char fullfilename[256] = {0};


#ifdef INCL_PAGING
    image_set_item* prev_img_item_p = NULL;
    image_set_item* img_item_p = NULL;
    TimeForNextSlide_t = time(NULL) + SlideInterval_sec;
#endif 

    fl_register_images();

    FCD_p = new Fl_File_Chooser("~", 
                "Image Files (*.{jpg,jpeg,png,gif,bmp,xbm,xpm})"
                "\tJPEG Files (*.jpg)\tPNG Files (*.png)\tGIF Files (*.gif)\tBMP Files (*.bmp)"
                "\tXBM FIles (*.xbm)\tXPM Files (*.xpm)\tAll Files (*)", 
#ifdef INCL_PAGING
                Fl_File_Chooser::MULTI,
#else
                Fl_File_Chooser::SINGLE,
#endif 
                "Select Image File");

    //int args_ret = 0;
    //int i;
    if (argc > 1) {
        MainWnd_p = new Fl_Pic_Window();
		Scn_w = Fl::w() - 8;
		Scn_h = Fl::h() - 24;  // Amount claimed by window frame/title and perhaps system taskbar
		ScnHWRatio = (float)Scn_h / (float)Scn_w;
		//~ printf("screen w, h is %d, %d; scaling factor is %1.3f\n", Scn_w, Scn_h, Fl::screen_scale(0));
		int screen_num = Fl::screen_num(MainWnd_p->x(), MainWnd_p->y());
		ScreenScaleFactor = Fl::screen_scale(screen_num);
		printf("Screen Num is %d, scale factor is %1.3f\n", screen_num, ScreenScaleFactor);
		// Force scale factor to 1.0 to make image scaling and scrolling consistent.
		Fl::screen_scale(0, 1.0);
		//int font_sz = fl_height();
		//int new_font_sz = (int)((float)font_sz * ScreenScaleFactor);
		fl_height(fl_font(), 17); //new_font_sz);
		fl_message_font(fl_font(), 17); // new_font_sz);
		//printf("Font height was %d, now set to %d\n", font_sz, new_font_sz);
		Scn_w = Fl::w() - 8;
		Scn_h = Fl::h() - 24;  // Amount claimed by window frame/title and perhaps system taskbar
		ScnHWRatio = (float)Scn_h / (float)Scn_w;
		//~ printf("screen w, h is %d, %d; scaling factor is now %1.3f\n", Scn_w, Scn_h, Fl::screen_scale(0));
        char* name = argv[a];
        if (!strncmp(name, "file://", 5)) name += 7;
#ifdef INCL_PAGING
        //DIR *dir = NULL;
        struct dirent *dp;          /* returned from readdir() */
        a = 1;
        while (a < argc) {
            DIR *dir = opendir (name);    
            if (dir == NULL) {
                // try to open it as a file
                prev_img_item_p = img_item_p;
                //~ printf("Loading image %s\n", argv[a]); 
                img_item_p = new image_set_item(MainWnd_p, name);
                if (!HeadImgItem_p) 
                {
                    HeadImgItem_p = img_item_p;
                    //print_img_list();
                }
                else if (prev_img_item_p) {
                    prev_img_item_p->link_after(img_item_p);
                }
                Images++;
                a++;
                name = argv[a];
            }
            else {
                    while ((dp = readdir (dir)) != NULL) {
                    strncpy(fullfilename, name, 255);
                    if (fullfilename[strlen(fullfilename) - 1] != '/') {
                        strcat(fullfilename, "/");
                    }
                    strcat(fullfilename, dp->d_name);
                    if (file_image_type(fullfilename) > 0) {
                        prev_img_item_p = img_item_p;
                        //~ printf("Loading image %s\n", fullfilename); fflush(0);
                        img_item_p = new image_set_item(MainWnd_p, fullfilename);
                        if (!HeadImgItem_p) HeadImgItem_p = img_item_p;
                        if (prev_img_item_p) {
                            prev_img_item_p->link_after(img_item_p);
                        }
                        Images++;
                    }
                    else {
                        printf("Image '%s' has no recognized type.\n", fullfilename); fflush(0);
                    }
                }
                a++;
                name = argv[a];
            }
        }
#else
 // NO_PAGING
        HeadImgItem_p = new image_set_item(MainWnd_p, name);
#endif
    }
    else {
        select_image_file();
        MainWnd_p = new Fl_Pic_Window();
		Scn_w = Fl::w() - 8;
		Scn_h = Fl::h() - 24;  // Amount claimed by window frame/title and perhaps system taskbar
        HeadImgItem_p = new image_set_item(MainWnd_p, Filename);
        Images = 1;
#ifdef INCL_PAGING
        a = FCD_p->count();
        img_item_p = HeadImgItem_p;
        while (a > 1) {
            if (file_image_type(FCD_p->value(Images + 1)) > 0) {
                strncpy(fullfilename, FCD_p->value(Images + 1), 255);
                prev_img_item_p = img_item_p;
                //~ printf("Loading image %s\n", fullfilename);
                img_item_p = new image_set_item(MainWnd_p, fullfilename);
                if (prev_img_item_p) {
                    prev_img_item_p->link_after(img_item_p);
                }
                Images++;
            }
            a--;
        }
#endif
    }
    
    MainWnd_p->img_p = HeadImgItem_p;
    if (!strlen(MainWnd_p->img_p->fullfilename)) {
        printf("No image file given.\n");
        return 0;
    }
    
    MainWnd_p->setup();
    if (HeadImgItem_p->im_orig_p) {
        Running = 1;
    }

    MainWnd_p->show();

    const char* iconfilepath1 = {"./flpicsee.png"};
    const char* iconfilepath2 = {"/usr/local/share/pixmaps/flpicsee.png"};
    const char* iconfilepath = NULL;
    Fl_Box* icon_p = NULL;
    Fl_PNG_Image* icon_img_p = NULL;
    
    if (access(iconfilepath1, F_OK) == 0) {
		iconfilepath = iconfilepath1;
	}
	else if (access(iconfilepath2, F_OK) == 0) {
		iconfilepath = iconfilepath2;
	}
	if (iconfilepath) {
		icon_p = (Fl_Box*)fl_message_icon();
		icon_p->label("");
		icon_p->align(FL_ALIGN_IMAGE_BACKDROP);
		icon_img_p = new Fl_PNG_Image(iconfilepath);
		icon_p->image(icon_img_p);
	}

    int t = time(NULL);
    while(Running && (0 <= Fl::wait(1.0))) {
#ifdef INCL_PAGING
		if (TimeToManageImageCache && (TimeToManageImageCache < t)) {
			TimeToManageImageCache = 0;
			manage_image_precaching(MainWnd_p->img_p);
		}
        t = time(NULL);
        if (Slideshow) {
            t = time(NULL);
            if ((unsigned int)TimeForNextSlide_t <= (unsigned int)t) {
                TimeForNextSlide_t = t + SlideInterval_sec;
                PagingCB((Fl_Widget*)MainWnd_p, (void*)PAGING_NEXT);
            }
             
        }
#endif
        if (!MainWnd_p->visible()) {
            if (MainWnd_p->shown()) {
                Running = 1;    // it's only iconified
            }
            else {
                Running = 0;    // it's really going away
            }
        }
    };
    return 0;
} 
