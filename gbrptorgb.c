// gbrp (as yv24) to rgb

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "avisynth_c.h"

AVS_VideoFrame *AVSC_CC torgb_get_frame (AVS_FilterInfo *fi, int n)
{
  AVS_VideoFrame *ret;

  AVS_VideoFrame *framein = avs_get_frame (fi->child, n);

  ret = avs_new_video_frame (fi->env, &fi->vi);

  const unsigned char *src_y = avs_get_read_ptr_p (framein, AVS_PLANAR_Y);
  const unsigned char *src_u = avs_get_read_ptr_p (framein, AVS_PLANAR_U);
  const unsigned char *src_v = avs_get_read_ptr_p (framein, AVS_PLANAR_V);
  
  const int src_yp = avs_get_pitch_p (framein, AVS_PLANAR_Y);
  const int src_up = avs_get_pitch_p (framein, AVS_PLANAR_U);
  const int src_vp = avs_get_pitch_p (framein, AVS_PLANAR_V);
  
  
  unsigned char *dst = avs_get_write_ptr (ret);


  const int dst_p = avs_get_pitch (ret);
  
  const int w = fi->vi.width;
  const int h = fi->vi.height;
  
  int i, j;
  // vflip
  dst += dst_p * (h - 1);

  for (j = 0; j < h; j++)
  {
    for (i = 0; i < w; i++)
	{
      dst[i * 4 + 0] = src_y[i];
      dst[i * 4 + 1] = src_v[i];
      dst[i * 4 + 2] = src_u[i];
      dst[i * 4 + 3] = 0;
	}
	src_y += src_yp;
	src_u += src_up;
	src_v += src_vp;
	dst -= dst_p;
  }
  
  // since we don't return the old frame, we have to release it
  avs_release_video_frame (framein);
  
  return ret;
}
  
  
AVS_Value AVSC_CC torgb_create (AVS_ScriptEnvironment *env, AVS_Value args, void *unused)
{
  AVS_Clip *clip;

  clip = avs_take_clip (avs_array_elt (args, 0), env);
  const AVS_VideoInfo *vi = avs_get_video_info (clip);
  
  if (vi->width <= 0)
    return avs_new_value_error ("video clips only!");

  if (vi->pixel_type != AVS_CS_YV24)
    return avs_new_value_error ("yv24 input only!");
	

  AVS_FilterInfo *fi;
  AVS_Clip *clipout;

  clipout = avs_new_c_filter (env, &fi, avs_array_elt (args, 0), 1);

  fi->get_frame = torgb_get_frame;
  fi->vi.pixel_type = AVS_CS_BGR32;
  AVS_Value ret = avs_new_value_clip (clipout);
  avs_release_clip (clipout);

  return ret;
}  

const char *AVSC_CC avisynth_c_plugin_init (AVS_ScriptEnvironment *env)
{
  avs_add_function (env,
    "gbrptorgb", "c", 
    torgb_create, 0);
  return "gbrp (as \"yv24\") to rgb";
}




BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}



