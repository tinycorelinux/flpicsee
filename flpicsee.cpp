// FL-PicSee  -- A FLTK-based picture viewer 
// See About_text[] below for copyright and distribution license

#define APP_VER "1.3.1" // Last update 2010-10-01

#ifndef NO_MENU
#define INCL_MENU
#endif

#ifndef NO_PAGING
#define INCL_PAGING
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b))?(a):(b)
#endif

/* ------------------------------------------------------- */

#ifdef INCL_MENU
const char About_text[] = 
"FL-PicSee version %s\n"
"copyright 2010 by Michael A. Losh\n"
"\n"
"FL-PicSee is free software: you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation, version 3.\n"
"\n"
"FL-PicSee is distributed in the hope that it will be useful,\n"
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
"FL-PicSee Help\n"
"\n"
"Mouse Control:\n"
"%s\n" // LeftClickHelp_text
"%s\n" // RightClickHelp_text
#ifdef INCL_PAGING
"\n"
"Multi-Image Viewing:\n"
"You may specify multiple files and/or directories on the command line.\n"
"Or start FL-PicSee with no arguments and Ctrl-click multiple files in \n"
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
#include <FL/Fl_Window.H>
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

enum {NATIVE_ZOOM = 0, WHOLE_IMG_ZOOM = 1, FIT_ZOOM = 2, SPEC_ZOOM};

enum {  MI_025_ZOOM = 0, MI_033_ZOOM, MI_050_ZOOM, MI_067_ZOOM, 
        MI_100_ZOOM, MI_150_ZOOM, MI_200_ZOOM, MI_400_ZOOM,
        MI_WHOLE_ZOOM, MI_FIT_ZOOM, 
        MI_ABOUT, MI_HELP, MI_OPEN,
#ifdef INCL_PAGING
        MI_PREV, MI_NEXT, MI_SLIDESHOW,
#endif
        MI_RETURN, MI_QUIT
};
    
enum {JPG_TYPE = 1, PNG_TYPE, GIF_TYPE, BMP_TYPE, XPM_TYPE, XBM_TYPE};

float ZoomFactor[] = {
        0.25, 0.3333, 0.50, 0.6667,
        1.00, 1.50, 2.00, 4.00};
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
#endif
        char fullfilename[256];
        char imgname[256];
        char imgpath[256];
        char ext[8];
        time_t filedate;
        Fl_Image * im_orig_p; 
        Fl_Image * im_full_p;
        Fl_Image * im_fit_p;
        Fl_Image * im_spec_p;   // zoomed to a specified level
        int zoommode;
        int img_h, img_w;   // original size of image
        int ful_h, ful_w;   // size of image so that the full image is visible without scrolling in current window
        int fit_h, fit_w;   // size of image so it fills current window with scrolling in only one dimension
        int spec_h, spec_w; // size of zoomed image at a specified "nice" zoom ratio
        int scn_h, scn_w;   // screen size (X-Windows screen dimension)
        int wnd_h, wnd_w;   // current window size
        int xoffset, yoffset;
        float zoompct;      // scaling "zoom" as ratio, 1.00 = 100%
        char scalestr[8];
        char buf[256];
        char *p;
        int len;
        float spec_zoom_ratio;
        
        float img_hwratio, wnd_hwratio, scn_hwratio;

        image_set_item(Fl_Pic_Window*  my_wnd_p, char* filename);
        ~image_set_item() ;
#ifdef INCL_PAGING
        image_set_item* link_after(image_set_item* newitem_p);
#endif
        void scale_image(void); 
        void resize(int x, int y, int w, int h);
        void handle_zoom(int dy);
        int  load_image(void);

        
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
#endif

#ifdef INCL_MENU
static void MenuCB(Fl_Widget* window_p, void *userdata);
Fl_Menu_Item right_click_menu[20] = {
    {"&about",              FL_CTRL+'a', MenuCB, (void*)MI_ABOUT},
    {"&help",               FL_CTRL+'h', MenuCB, (void*)MI_HELP,FL_MENU_DIVIDER},
#ifdef INCL_PAGING
    {"&prev. Image",        FL_CTRL+'p',    MenuCB, (void*)MI_PREV},
    {"&next Image",         FL_CTRL+'n',    MenuCB, (void*)MI_NEXT},
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
    {"&whole image",        FL_CTRL+'w', MenuCB, (void*)MI_WHOLE_ZOOM},
    {"&fit window",         FL_CTRL+'f', MenuCB, (void*)MI_FIT_ZOOM,FL_MENU_DIVIDER},
    {"&open another...",    FL_CTRL+'o', MenuCB, (void*)MI_OPEN},
    {"&return",             FL_CTRL+'r', MenuCB, (void*)MI_RETURN},
    {"&quit",               FL_CTRL+'q', MenuCB, (void*)MI_QUIT},
    {0}
};
#endif

int Running = 0;
int GetAnotherImage = 0;
int Images = 0;
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

class Fl_Pic_Window : public Fl_Window {
protected:  
    char wintitle[256];
    //char tiptitle[256];
    int handle(int e) ;

public:
    Fl_Box*    pic_box_p;
    Fl_Scroll* scroll_p; //scroll(0, 0, wnd_w, wnd_h);
    image_set_item* img_p;
#ifdef INCL_PAGING
    Fl_Button* prev_paging_btn_p;
    rect       prev_paging_rect;
    Fl_Button* next_paging_btn_p;
    rect       next_paging_rect;
#endif
    
    Fl_Pic_Window();
    
    ~Fl_Pic_Window();

    void resize(int x, int y, int w, int h);
    
    void specify_zoom(int zoom_mode, float zoom_pct);

    void set_titles(void);

    void delete_image(void);
    
    void setup(void);
    
    void do_menu(void);
    
#ifdef INCL_PAGING      
    void scale_paging_buttons(int wnd_w, int wnd_h);
    int image_index(void);
#endif
    
};

Fl_Pic_Window* MainWnd_p = NULL;


image_set_item::image_set_item(Fl_Pic_Window*  my_wnd_p, char* filename) 
{
    wnd_p = my_wnd_p;
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
    im_full_p   = NULL;
    im_fit_p    = NULL;
    im_spec_p   = NULL;
}

image_set_item::~image_set_item() {
    if (im_orig_p)  delete im_orig_p; 
    if (im_full_p)  delete im_full_p;
    if (im_fit_p)   delete im_fit_p;
    if (im_spec_p)  delete im_spec_p;
}

#ifdef INCL_PAGING
image_set_item* image_set_item::link_after(image_set_item* newitem_p) {
    image_set_item * nextnext_p = next_p; 
    
    if (next_p == this) {
        // special case: there is no other node yet, link them to each other
        newitem_p->next_p = this;
        newitem_p->prev_p = this;
        prev_p = newitem_p;
        next_p = newitem_p;
    }
    newitem_p->next_p = nextnext_p;
    nextnext_p->prev_p = newitem_p;
    
    newitem_p->prev_p = this;
    next_p = newitem_p;
    
    return newitem_p;
}
#endif

void image_set_item::scale_image(void) 
{
    static float oldzoompct = 1.00;
    wnd_w = wnd_p->w();
    wnd_h = wnd_p->h();
    switch(zoommode) {
    case NATIVE_ZOOM:
        wnd_p->pic_box_p->resize((wnd_w - img_w) / 2, (wnd_h - img_h) / 2, img_w, img_h);
        wnd_p->pic_box_p->image(im_orig_p);  
        zoompct = 1.00;
        break;
    case WHOLE_IMG_ZOOM:
        wnd_p->pic_box_p->resize((wnd_w - ful_w) / 2, (wnd_h - ful_h) / 2, ful_w, ful_h);
        if (!im_full_p) {
            im_full_p = im_orig_p->copy(ful_w, ful_h);
        } 
        wnd_p->pic_box_p->image(im_full_p);
        zoompct = ((float)ful_w) / (float)img_w;
        break;
    case FIT_ZOOM:
        wnd_p->pic_box_p->resize((wnd_w - img_w) / 2, (wnd_h - img_h) / 2, img_w, img_h);
        if (!im_fit_p) {
            im_fit_p = im_orig_p->copy(fit_w, fit_h);
        } 
        wnd_p->pic_box_p->image(im_fit_p);
        zoompct = ((float)fit_w)/ (float)img_w;
        break;
    case SPEC_ZOOM:
        if (oldzoompct != zoompct) {
            oldzoompct = zoompct;
            spec_w = (int)(zoompct * (float)img_w);
            spec_h = (int)(zoompct * (float)img_h);
            wnd_p->pic_box_p->resize((wnd_w - spec_w) / 2, (wnd_h - spec_h) / 2, spec_w, spec_h);
            if (im_spec_p) {
                delete im_spec_p;
            }
            im_spec_p = im_orig_p->copy(spec_w, spec_h);
            wnd_p->pic_box_p->image(im_spec_p);
        }
        break;
    }

    #ifdef INCL_PAGING
    wnd_p->scale_paging_buttons(wnd_w, wnd_h);
    #endif

    wnd_p->set_titles();
}

void image_set_item::resize(int x, int y, int w, int h) {
    wnd_w = w;
    wnd_h = h;

    wnd_hwratio = (float)wnd_h / (float)wnd_w;
    xoffset = 0;
    yoffset = 0;

    if (img_hwratio > wnd_hwratio) {
        ful_h = h;
        ful_w = (int)((float)ful_h / img_hwratio);
        fit_w = w - 32;  // allow room for scrollbar
        fit_h = (int)((float)fit_w * img_hwratio);
    }
    else {
        ful_w = w;
        ful_h = (int)((float)ful_w * img_hwratio);
        fit_h = h - 32; // allow room for scrollbar
        fit_w = (int)((float)fit_h / img_hwratio);
    } 
    if (im_full_p) {
        delete im_full_p;
    } 
    if (im_fit_p) {
        delete im_fit_p;
    } 
    im_full_p = NULL;
    im_fit_p  = NULL;
    scale_image();
}

void image_set_item::handle_zoom(int dy) {
    if (fit_w > img_w) {
        dy *= -1;
    }
    if (dy > 0) {
        if (zoommode < 2) zoommode++;
    }
    else if (dy < 0) {
        if (zoommode > 0) zoommode--;
    }
    scale_image();                  
}

// returns 1 or higher if filename has a known file type, 0 if not
int file_image_type(const char* fullfilename) 
{
    int img_type = 0; 
    int imfd = open(fullfilename, O_RDONLY);
    if (-1 == imfd) {
        return img_type;
    }
    
    char imbyte[16];
    unsigned char* imubyte = (unsigned char*)imbyte; 
    if (16 != read(imfd, imbyte, 16)) {
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
//printf("Image type is %d\nHeader bytes: ", img_type); fflush(0);
//int n;
//for (n = 0; n < 16; n++) printf("0x%02X ", imubyte[n]);
//printf("\n");
    return img_type;
}

int image_set_item::load_image(void)
{
    char *fn_p;
    bzero(imgpath, 256);
    bzero(imgname, 256);
    bzero(scalestr, 8);
    
    zoommode = 0;
    zoompct  = 1.00;
    spec_zoom_ratio = 1.00;
    scn_w = Fl::w();
    scn_h = Fl::h() - 48;  // Amount claimed by window frame/title and perhaps system taskbar

    strcpy(scalestr, " 100%");

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

    switch(file_image_type(fullfilename)) {
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
            return 0;
    }
    
    img_h = im_orig_p->h(); 
    img_w = im_orig_p->w(); 
    spec_h = img_h;
    spec_w = img_w;

    img_hwratio = (float)img_h / (float)img_w;
    wnd_hwratio = (float)wnd_h / (float)wnd_w;
    scn_hwratio = (float)scn_h / (float)scn_w;
    xoffset = 0;
    yoffset = 0;

    if ((img_h < scn_h) && (img_w < scn_w)) {
        zoommode = 0; // native 1:1 scaling
        wnd_h = img_h;
        wnd_w = img_w;
        ful_h = img_h;
        ful_w = img_w;
        fit_h = img_h;
        fit_w = img_w;
        if (wnd_h < 90) {
            wnd_h = 90;
        }
        if (wnd_w < 120) {
            wnd_w = 120;
        }
    }
    else {
        zoommode = 1; // view whole image scaling
        if (img_hwratio > scn_hwratio) {
            ful_h = scn_h;
            ful_w = (int)((float)ful_h / img_hwratio);
            fit_w = scn_w - 20;  // allow room for scrollbar
            fit_h = (int)((float)fit_w * img_hwratio);
        }
        else {
            ful_w = scn_h;
            ful_h = (int)((float)ful_w * img_hwratio);
            fit_h = scn_h - 20; // allow room for scrollbar
            fit_w = (int)((float)ful_h / img_hwratio);
        }
        wnd_h = ful_h;
        wnd_w = ful_w;
    }
    im_full_p = im_orig_p->copy(ful_w, ful_h);
    im_fit_p = im_orig_p->copy(fit_w, fit_h);
    return 1;
}

Fl_Pic_Window::Fl_Pic_Window() 
    :   Fl_Window(Fl::w(), Fl::h())
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
    Fl_Group::resize(x, y, w, h);
    img_p->resize(x, y, w, h);
    redraw();
}
    
void Fl_Pic_Window::set_titles(void) {
    sprintf(img_p->scalestr, "%4d%%", (int) (100.0 * img_p->zoompct));
#ifdef INCL_PAGING
    char index_str[16] = {0};
    if (Images > 1) {
        sprintf(index_str, " %d of %d", image_index(), Images);
    }
    sprintf(wintitle, "%s%s %s - FL-PicSee %s (%s/%s)%s", 
            Slideshow ? "SLIDESHOW: " : "",
            img_p->scalestr, img_p->imgname, APP_VER, 
            img_p->imgpath, img_p->imgname, 
            (Images > 1) ? index_str : "");
#else
    sprintf(wintitle, "%s %s - FL-PicSee %s (%s/%s)", img_p->scalestr, img_p->imgname, APP_VER, img_p->imgpath, img_p->imgname);
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
    const Fl_Menu_Item *m = right_click_menu->popup(Fl::event_x(), Fl::event_y(), "FL-PicSee", 0, 0);
    if ( m ) m->do_callback(this, m->user_data());
};
#endif

int Fl_Pic_Window::handle(int e) {
    char s[200];
    int ret = 0;
    int dy = 0;
    int needredraw = 0;
    int newxpos, newypos;
    int scrlxend = 0;
    int scrlyend = 0;
    static int dragx = 0;
    static int dragy = 0;
    static int scrlx = 0;
    static int scrly = 0;
    static int prevxpos = 0;
    static int prevypos = 0;
    static int ctrl_pressed = 0;
    static char tiptext[128] = {0};
    int key;
    switch ( e ) {
#ifdef INCL_MENU
        case FL_KEYDOWN:
            key = Fl::event_key();
//            printf("You pressed key %d (0x%4X)\n", key, key);
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
                redraw(); 
                return 1;                
            }
            else if (key == '=') {
                dy = -1;
                img_p->handle_zoom(dy);
                redraw(); 
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
                int choice = 0;
                switch(key) {
                    case 'a': choice = MI_ABOUT; break;
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
                    case 'w': choice = MI_WHOLE_ZOOM; break;
                    case 'f': choice = MI_FIT_ZOOM; break;
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
            break;
#endif
        case FL_PUSH:
            dragx = Fl::event_x();
            dragy = Fl::event_y();
            if ( Fl::event_button() == FL_LEFT_MOUSE ) {
                if (!Fl_Window::handle(e)) {
                    label(LeftClickHelp_text);
                    scrlx = scroll_p->xposition();
                    prevxpos = scrlx;
                    scrly = scroll_p->yposition();
                    prevypos = scrly;
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
                if (!Fl_Window::handle(e)) {
                    newxpos = scroll_p->xposition();
                    newypos = scroll_p->yposition();
                    if (scroll_p->w() < pic_box_p->w()) {
                        newxpos = scrlx - (Fl::event_x() - dragx);
                        if (newxpos < 0) newxpos = 0;
                        scrlxend = pic_box_p->w() - scroll_p->w() + 16;
                        if (newxpos > scrlxend) newxpos = scrlxend;
                        if (    (newxpos - prevxpos) < -16 || (newxpos - prevxpos) > 16
                            ||  newxpos == 0 || newxpos == scrlxend) {
                            needredraw = 1;
                        }
                    }
                    if (scroll_p->h() < pic_box_p->h()) {
                        newypos = scrly - (Fl::event_y() - dragy);
                        if (newypos < 0) newypos = 0;
                        scrlyend = pic_box_p->h() - scroll_p->h() + 16;
                        if (newypos > scrlyend) newypos = scrlyend;
                        if (    (newypos - prevypos) < -16 || (newypos - prevypos) > 16 
                            ||  newypos == 0 || newypos == scrlyend) {
                            needredraw = 1;
                        }
                    }
                    if (needredraw) {
                        prevxpos = newxpos;
                        prevypos = newypos;
                        scroll_p->position(newxpos, newypos);
                        redraw();
                    }
                    return 1;
                }
            }
            break;
        case FL_RELEASE:
            if ( Fl::event_button() == FL_LEFT_MOUSE) {
                if (!Fl_Window::handle(e)) {
                    if (abs(dragx - Fl::event_x()) < 3 && abs(dragy - Fl::event_y()) < 3) {
                        img_p->zoommode = (img_p->zoommode + 1) % 3;
                        img_p->scale_image();                   
                        redraw(); 
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
#endif
        case FL_MOUSEWHEEL:
            dy = Fl::event_dy();
            img_p->handle_zoom(dy);
            redraw(); 
            return 1;
                
        case FL_SHOW:
            return 1;

        default:
            ret = Fl_Window::handle(e);
            break;

    }

    return(ret);
}

void Fl_Pic_Window::specify_zoom(int zoom_mode, float zoom_pct) {
    img_p->zoommode = zoom_mode;
    if (img_p->zoommode == SPEC_ZOOM) {
        img_p->zoompct = zoom_pct;
    }
    img_p->scale_image();
    redraw();
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
    wnd_x = (img_p->scn_w / 2) - (img_p->wnd_w / 2); 
    wnd_y = (img_p->scn_h  / 2) - (img_p->wnd_h / 2);
    if (wnd_y < 24) wnd_y = 24; // don't position it under task bar
    w(img_p->wnd_w);
    h(img_p->wnd_h);
    x(wnd_x);
    y(wnd_y);
    if (!scroll_p) {
        Fl_Window::resize(wnd_x, wnd_y, img_p->wnd_w, img_p->wnd_h);
        scroll_p = new Fl_Scroll(0, 0, img_p->wnd_w, img_p->wnd_h);
        scroll_p->box(FL_NO_BOX);
        scroll_p->begin();
        if (!pic_box_p) pic_box_p = new Fl_Box(0, 0, img_p->ful_w, img_p->ful_h);
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
        end();
        resizable(pic_box_p); 
    }
    else {
        Fl_Group::resize(wnd_x,wnd_y, img_p->wnd_w, img_p->wnd_h);
        Fl_Window::resize(wnd_x, wnd_y, img_p->wnd_w, img_p->wnd_h);
        scroll_p->resize(0, 0, img_p->wnd_w, img_p->wnd_h);
    }
    img_p->scale_image();
    redraw();

}

#ifdef INCL_PAGING

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
        MainWnd_p->img_p->load_image();
    }
    MainWnd_p->img_p->resize(MainWnd_p->x(), MainWnd_p->y(), MainWnd_p->w(), MainWnd_p->h());
    MainWnd_p->redraw();
}
#endif

#ifdef INCL_MENU
static void MenuCB(Fl_Widget* window_p, void *userdata) 
{
    long choice = (long)userdata;
    int spec_w, spec_h;
    float speczoomfactor;
    image_set_item* new_img_p;
#ifdef INCL_PAGING
    char fullfilename[256] = {0};
    image_set_item* next_new_img_p = NULL;
#endif 

    Fl_Pic_Window* mainwnd_p = (Fl_Pic_Window *)window_p;
    
    switch (choice) {
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
            mainwnd_p->specify_zoom(SPEC_ZOOM, ZoomFactor[choice]);
            break;
        case MI_WHOLE_ZOOM:
            mainwnd_p->specify_zoom(WHOLE_IMG_ZOOM, 0);
            break;
        case MI_FIT_ZOOM:
            mainwnd_p->specify_zoom(FIT_ZOOM, 0);
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
                    MainWnd_p->img_p->resize(MainWnd_p->x(), MainWnd_p->y(), MainWnd_p->w(), MainWnd_p->h());
                    MainWnd_p->redraw();
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
    fl_message_font(fl_font(), 12);

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

    int args_ret = 0;
    int i;

    if (argc > 1) {
        MainWnd_p = new Fl_Pic_Window();
        char* name = argv[a];
        if (!strncmp(name, "file://", 5)) name += 7;
#ifdef INCL_PAGING
        DIR *dir = NULL;
        struct dirent *dp;          /* returned from readdir() */
        a = 1;
        while (a < argc) {
            DIR *dir = opendir (name);    
            if (dir == NULL) {
                // try to open it as a file
                prev_img_item_p = img_item_p;
//                printf("Loading image %s\n", argv[a]); 
                img_item_p = new image_set_item(MainWnd_p, name);
                if (!HeadImgItem_p) HeadImgItem_p = img_item_p;
                if (prev_img_item_p) {
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
                        printf("Loading image %s\n", fullfilename); fflush(0);
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
        HeadImgItem_p = new image_set_item(MainWnd_p, Filename);
        Images = 1;
#ifdef INCL_PAGING
        a = FCD_p->count();
        img_item_p = HeadImgItem_p;
        while (a > 1) {
            if (file_image_type(FCD_p->value(Images + 1)) > 0) {
                strncpy(fullfilename, FCD_p->value(Images + 1), 255);
                prev_img_item_p = img_item_p;
                printf("Loading image %s\n", fullfilename);
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
    int t = time(NULL);
    while(Running && (0 <= Fl::wait(1.0))) {
#ifdef INCL_PAGING
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
            Running = 0;
        }
    };
    return 0;
} 
