#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "avisynth_c.h"

#include "SDL_bdf.h"


// our filter's userdata
typedef struct
{
  unsigned char *bitmap; // rendered and aligned text
  int first; // first frame
  int last; // last frame
  unsigned char color[4]; // draw color, ARGB
  unsigned char halocolor[4]; // halo color, ARGB
} udata_t;

AVS_VideoFrame *AVSC_CC freesub_get_frame (AVS_FilterInfo *fi, int n)
{
  udata_t *ud = (udata_t *) fi->user_data;
  AVS_VideoFrame *ret;
  
  // passthrough outside appropriate frame range
  if (n < ud->first || n > ud->last)
  {
    ret = avs_get_frame (fi->child, n);
    return ret;
  }
  // process
  ret = avs_get_frame (fi->child, n);
  avs_make_writable (fi->env, &ret);
  
  unsigned char *data = avs_get_write_ptr (ret);
  int pitch = avs_get_pitch (ret);
  //int rowlen = avs_get_row_size (ret); // in bytes
  int height = avs_get_height (ret);
  int width = fi->vi.width;
  
  int j, i;
  int bloc = 0;
  
  
  if (fi->vi.pixel_type == AVS_CS_BGR24)
  {
    unsigned char r = ud->color[1];
    unsigned char g = ud->color[2];
    unsigned char b = ud->color[3];
    int a = ud->color[0];
    unsigned char hr = ud->halocolor[1];
    unsigned char hg = ud->halocolor[2];
    unsigned char hb = ud->halocolor[3];
    int ha = ud->halocolor[0];
    //char buf[512];
    //sprintf (buf, "A%u R%u G%u B%u", a, r, g, b);
    //MessageBox (0, buf, "fo", 0);
    
    
    for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
      {
        if (ud->bitmap[bloc] == 1)
        {
          data[i * 3 + 0] = (data[i * 3 + 0] * a + 127) / 255 + b;
          data[i * 3 + 1] = (data[i * 3 + 1] * a + 127) / 255 + g;
          data[i * 3 + 2] = (data[i * 3 + 2] * a + 127) / 255 + r;
        }
        else if (ud->bitmap[bloc] == 2)
        {
          data[i * 3 + 0] = (data[i * 3 + 0] * ha + 127) / 255 + hb;
          data[i * 3 + 1] = (data[i * 3 + 1] * ha + 127) / 255 + hg;
          data[i * 3 + 2] = (data[i * 3 + 2] * ha + 127) / 255 + hr;
        }
        bloc++;
      }
      data += pitch;
    }
  }  
  else if (fi->vi.pixel_type == AVS_CS_BGR32)
  {
    unsigned char r = ud->color[1];
    unsigned char g = ud->color[2];
    unsigned char b = ud->color[3];
    int a = ud->color[0];
    unsigned char hr = ud->halocolor[1];
    unsigned char hg = ud->halocolor[2];
    unsigned char hb = ud->halocolor[3];
    int ha = ud->halocolor[0];

    
    for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
      {
        if (ud->bitmap[bloc] == 1)
        {
          data[i * 4 + 0] = (data[i * 4 + 0] * a + 127) / 255 + b;
          data[i * 4 + 1] = (data[i * 4 + 1] * a + 127) / 255 + g;
          data[i * 4 + 2] = (data[i * 4 + 2] * a + 127) / 255 + r;
        }
        else if (ud->bitmap[bloc] == 2)
        {
          data[i * 4 + 0] = (data[i * 4 + 0] * ha + 127) / 255 + hb;
          data[i * 4 + 1] = (data[i * 4 + 1] * ha + 127) / 255 + hg;
          data[i * 4 + 2] = (data[i * 4 + 2] * ha + 127) / 255 + hr;
        }
        bloc++;
      }
      data += pitch;
    }
  }  
    
  else if (fi->vi.pixel_type == AVS_CS_YV24 || fi->vi.pixel_type == AVS_CS_I420)
  {
    unsigned char y = ud->color[1];
    unsigned char u = ud->color[2];
    unsigned char v = ud->color[3];
    int a = ud->color[0];
    unsigned char hy = ud->halocolor[1];
    unsigned char hu = ud->halocolor[2];
    unsigned char hv = ud->halocolor[3];
    int ha = ud->halocolor[0];

    unsigned char *yp = data + height * pitch * 0;
    unsigned char *up = data + height * pitch * 1;
    unsigned char *vp = data + height * pitch * 2;
        
    for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
      {
        if (ud->bitmap[bloc] == 1)
        {
          yp[i] = (yp[i] * a + 127) / 255 + y;
          up[i] = (up[i] * a + 127) / 255 + u;
          vp[i] = (vp[i] * a + 127) / 255 + v;
        }
        else if (ud->bitmap[bloc] == 2)
        {
          yp[i] = (yp[i] * ha + 127) / 255 + hy;
          up[i] = (up[i] * ha + 127) / 255 + hu;
          vp[i] = (vp[i] * ha + 127) / 255 + hv;
        }
        
        bloc++;
      }
      yp += pitch;
      up += pitch;
      vp += pitch;
    }
  }  

 
  return ret;
}

/*
void AVSC_CC freesub_destroy (void *udv, AVS_ScriptEnvironment *env)
{
  udata_t *ud = (udata_t *) udv;
  free (ud->bitmap);
  free (ud);
}
*/

void AVSC_CC freesub_free (AVS_FilterInfo *fi)
{
  udata_t *ud = (udata_t *) fi->user_data;
  free (ud->bitmap);
  free (ud);
}
  

AVS_Value AVSC_CC freesub_create (AVS_ScriptEnvironment *env, AVS_Value args, void *user_data)
{
  // all of the arguments we were passed, in easier to handle form
  AVS_Clip *clip;
  const char *text;
  int xanchor, yanchor;
  int first_frame, last_frame;
  const char *fontname;
  int ymag;
  unsigned int text_color, halo_color;
  int align;
  int lsp;
  int multiline;
  int xmag;
  int halo_width, halo_height;
  
  AVS_Value ret;


  
  udata_t *ud = malloc (sizeof (*ud));
  if (!ud)
  {
    ret = avs_new_value_error ("Out of memory!");
    return ret;
  }
  
  #define LIMIT(a,m,M) if (a < m) a = m; if (a > M) a = M
  
  clip = avs_take_clip (avs_array_elt (args, 0), env);
  const AVS_VideoInfo *vi = avs_get_video_info (clip);
  
  text = avs_is_string (avs_array_elt (args, 1)) ?
         avs_as_string (avs_array_elt (args, 1)) : "The quick brown fox jumps over the lazy dog.";
  xanchor = avs_is_int (avs_array_elt (args, 2)) ?
            avs_as_int (avs_array_elt (args, 2)) : vi->width / 2;
  yanchor = avs_is_int (avs_array_elt (args, 3)) ?
            avs_as_int (avs_array_elt (args, 3)) : vi->height / 2;
  first_frame = avs_is_int (avs_array_elt (args, 4)) ?
                avs_as_int (avs_array_elt (args, 4)) : 0;
  LIMIT (first_frame, 0, INT_MAX);
  last_frame = avs_is_int (avs_array_elt (args, 5)) ?
               avs_as_int (avs_array_elt (args, 5)) : INT_MAX;
  LIMIT (last_frame, 0, INT_MAX);
  fontname = avs_is_string (avs_array_elt (args, 6)) ?
             avs_as_string (avs_array_elt (args, 6)) : "default.bdf";
  ymag = avs_is_int (avs_array_elt (args, 7)) ?
         avs_as_int (avs_array_elt (args, 7)) : 1;
  LIMIT (ymag, 1, 64);
  text_color = avs_is_int (avs_array_elt (args, 8)) ?
               avs_as_int (avs_array_elt (args, 8)) : 0x20ffffff;
  halo_color = avs_is_int (avs_array_elt (args, 9)) ?
               avs_as_int (avs_array_elt (args, 9)) : 0x20000000;
  align = avs_is_int (avs_array_elt (args, 10)) ?
          avs_as_int (avs_array_elt (args, 10)) : 5;
  LIMIT (align, 1, 9);
  if (avs_is_int (avs_array_elt (args, 11)))
  {
    lsp = avs_as_int (avs_array_elt (args, 11));
    LIMIT (lsp, -10, 400);
    multiline = 1;
  }
  else
  {
    lsp = 0;
    multiline = 0;
  }
  xmag = avs_is_int (avs_array_elt (args, 12)) ?
         avs_as_int (avs_array_elt (args, 12)) : ymag;
  LIMIT (xmag, 1, 64);
  halo_width = avs_is_int (avs_array_elt (args, 13)) ?
               avs_as_int (avs_array_elt (args, 13)) : xmag;
  LIMIT (halo_width, 0, 64);
  halo_height = avs_is_int (avs_array_elt (args, 14)) ?
               avs_as_int (avs_array_elt (args, 14)) : ymag;
  LIMIT (halo_height, 0, 64);

  
  
  #undef LIMIT
  #define FAIL(a) \
  {\
    ret = avs_new_value_error (a);\
    free (ud);\
    goto fail;\
  }
  
  // input clip must have video
  if (vi->width <= 0)
    FAIL ("Only works on video clips!");
  if
  (
    vi->pixel_type != AVS_CS_BGR24 &&
    vi->pixel_type != AVS_CS_BGR32 &&
    vi->pixel_type != AVS_CS_YV24 &&
    vi->pixel_type != AVS_CS_I420
  )
    FAIL ("Invalid Colorspace!");
  
  
  // load and render
  FILE *f = fopen (fontname, "rb"); // binary mode is intentional, SDL_bdf handles it internally
  if (!f)
    FAIL ("Couldn't open fontfile");

  int err;
  
  BDF_Font *dafont = BDF_OpenFont ((BDF_ReadByte) fgetc, f, &err);
  fclose (f);
  if (!dafont)
  {
    FAIL ("Font load error!");
  }

  int x0, y0, w, h;
  
  
  if (multiline)
  {
    BDF_SizeML (dafont, text, &w, &h, lsp);
    x0 = y0 = 0;
  }
  else
  {
    BDF_SizeH (dafont, text, &x0, &y0, &w, &h);
    x0 *= xmag;
    y0 *= ymag;
  }  
  
  w *= xmag;
  h *= ymag;
  
  int xstart, ystart;
  // now determine actual location.
  switch (align)
  {
    case 1:
    case 4:
    case 7:
      xstart = xanchor - 0;
      break;
    case 2:
    case 5:
    case 8:
      xstart = xanchor - w / 2;
      break;
    case 3:
    case 6:
    case 9:
      xstart = xanchor - w;
      break;
  }
  switch (align)
  {
    case 1:
    case 2:
    case 3:
      ystart = yanchor - h;
      break;
    case 4:
    case 5:
    case 6:
      ystart = yanchor - h / 2;
      break;
    case 7:
    case 8:
    case 9:
      ystart = yanchor - 0;
      break;
  }
  // prepare output bitmap
  unsigned char *bitmap = malloc (vi->height * vi->width);
  if (!bitmap)
    FAIL ("ENOMEM");
  memset (bitmap, 0, vi->height * vi->width);
  

  // putpixel helper
  void putpixel_ex (int xd, int yd)
  {
    if (xd < 0 || yd < 0 || xd >= vi->width || yd >= vi->height)
      return;
    if (vi->pixel_type == AVS_CS_BGR24 || vi->pixel_type == AVS_CS_BGR32)
      bitmap[(vi->height - yd - 1) * vi->width + xd] = 1;
    else if (vi->pixel_type == AVS_CS_YV24 || vi->pixel_type == AVS_CS_I420)
      bitmap[yd * vi->width + xd] = 1;
  }
  
  // putpixel function
  void putpixel (void *unused, int xd, int yd, unsigned int unused2)
  {
    xd = xd * xmag + xstart;
    yd = yd * ymag + ystart;
    int i, j;
    for (j = 0; j < ymag; j++)
      for (i = 0; i < xmag; i++)
        putpixel_ex (xd + i, yd + j);
  }
  
  if (multiline)
    BDF_DrawML (NULL, putpixel, dafont, text, 0, align, lsp);
  else
    BDF_DrawH (NULL, putpixel, dafont, text, x0, y0, 0);
 
  BDF_CloseFont (dafont);

  // process halo
  {
    int i, j;
    for (j = 0; j < vi->height; j++)
    {
      for (i = 0; i < vi->width; i++)
      {
        if (bitmap[vi->width * j + i] == 1)
          continue;
        int zx, zy;
        for (zx = -halo_width; zx <= halo_width; zx++)
        {
          for (zy = -halo_height; zy <= halo_height; zy++)
          {
            int xl = i + zx;
            int yl = j + zy;
            if (xl < 0 || yl < 0 || xl >= vi->width || yl >= vi->height)
              continue;
            if (bitmap[vi->width * yl + xl] == 1)
            {
              bitmap[vi->width * j + i] = 2;
              goto nextp;
            }
          }
        }
        nextp:
        ;
      }
    }
  }
        

  
  // set up userdata
  ud->bitmap = bitmap;
  ud->first = first_frame;
  ud->last = last_frame;
  
  // decompose colors
  {
    //char buf[512];

    unsigned char s[4];
    s[3] = (text_color >> 0) & 0xff; //B/V   (U for YV24)
    s[2] = (text_color >> 8) & 0xff; //G/U   (V for YV24)
    s[1] = (text_color >> 16) & 0xff; //R/Y
    s[0] = (text_color >> 24) & 0xff;  //A
    //sprintf (buf, "A%u R%u G%u B%u", s[0], s[1], s[2], s[3]);
    //MessageBox (0, buf, "fo", 0);
    // invert alpha, premultiply
    s[0] = 255 - s[0];
    s[1] = (s[1] * (int) s[0] + 127) / 255;
    s[2] = (s[2] * (int) s[0] + 127) / 255;
    s[3] = (s[3] * (int) s[0] + 127) / 255;
    s[0] = 255 - s[0];
    
    memcpy (ud->color, s, 4);

    //sprintf (buf, "A%u R%u G%u B%u", s[0], s[1], s[2], s[3]);
    //MessageBox (0, buf, "fo", 0);

    s[3] = (halo_color >> 0) & 0xff; //B/V   (U for YV24)
    s[2] = (halo_color >> 8) & 0xff; //G/U   (V for YV24)
    s[1] = (halo_color >> 16) & 0xff; //R/Y
    s[0] = (halo_color >> 24) & 0xff;  //A
    //sprintf (buf, "A%u R%u G%u B%u", s[0], s[1], s[2], s[3]);
    //MessageBox (0, buf, "fo", 0);
    // invert alpha, premultiply
    s[0] = 255 - s[0];
    s[1] = (s[1] * (int) s[0] + 127) / 255;
    s[2] = (s[2] * (int) s[0] + 127) / 255;
    s[3] = (s[3] * (int) s[0] + 127) / 255;
    s[0] = 255 - s[0];
    
    memcpy (ud->halocolor, s, 4);
    
    //sprintf (buf, "A%u R%u G%u B%u", s[0], s[1], s[2], s[3]);
    //MessageBox (0, buf, "fo", 0);
    
    
  }
    
  
  
    
  AVS_FilterInfo *fi;
  AVS_Clip *clipout;

  clipout = avs_new_c_filter (env, &fi, avs_array_elt (args, 0), 1);

  fi->user_data = ud;
  
  fi->get_frame = freesub_get_frame;
  ret = avs_new_value_clip (clipout);
  avs_release_clip (clipout);
  //avs_at_exit (env, freesub_destroy, ud);
  fi->free_filter = freesub_free;
  
  fail: // from fail
  
  return ret;
}


const char *AVSC_CC avisynth_c_plugin_init (AVS_ScriptEnvironment *env)
{
  avs_add_function (env,
    "freesub", "c[text]s[x]i[y]i[first_frame]i[last_frame]i[font]s[size]i[text_color]i[halo_color]i[align]i[lsp]i[font_width]i[halo_width]i[halo_height]i", 
    freesub_create, 0);
  return "Freesub: simple subtitling with BDF";
}



BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}

