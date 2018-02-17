#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "avisynth_c.h"



// our filter's userdata
typedef struct
{
  int index; // index of the remembered frame
  unsigned size; // byte size of the data
  unsigned char *data; // data itself

} udata_t;

AVS_VideoFrame *AVSC_CC framercache_get_frame (AVS_FilterInfo *fi, int n)
{
  udata_t *ud = (udata_t *) fi->user_data;
  AVS_VideoFrame *ret;

  if (n == ud->index)
  {
    ret = avs_new_video_frame (fi->env, &fi->vi);
    memcpy (avs_get_write_ptr (ret), ud->data, ud->size);
    return ret;
  }
  if (ud->data)
    free (ud->data);
  ret = avs_get_frame (fi->child, n);

  ud->index = n;
  ud->size = avs_get_pitch (ret) * avs_get_height (ret);
  ud->data = malloc (ud->size);
  memcpy (ud->data, avs_get_read_ptr (ret), ud->size);

  return ret;  
}


void AVSC_CC framercache_free (AVS_FilterInfo *fi)
{
  udata_t *ud = (udata_t *) fi->user_data;
  if (ud->data)
    free (ud->data);
  free (ud);
}
  

AVS_Value AVSC_CC framercache_create (AVS_ScriptEnvironment *env, AVS_Value args, void *user_data)
{
  // all of the arguments we were passed, in easier to handle form
  AVS_Clip *clip;
  
  AVS_Value ret;

  
  
  clip = avs_take_clip (avs_array_elt (args, 0), env);
  const AVS_VideoInfo *vi = avs_get_video_info (clip);
 
 
 
  // input clip must have video
  if (vi->width <= 0)
  {
    ret = avs_new_value_error ("Clip must have video!");
	return ret;
  }

	
  udata_t *ud = malloc (sizeof (*ud));
  if (!ud)
  {
    ret = avs_new_value_error ("Out of memory!");
    return ret;
  }
  ud->data = NULL;
  ud->index = -1;

  
  
    
  AVS_FilterInfo *fi;
  AVS_Clip *clipout;

  clipout = avs_new_c_filter (env, &fi, avs_array_elt (args, 0), 1);

  fi->user_data = ud;
  
  fi->get_frame = framercache_get_frame;
  ret = avs_new_value_clip (clipout);
  avs_release_clip (clipout);
  //avs_at_exit (env, framercachesub_destroy, ud);
  fi->free_filter = framercache_free;
  
  return ret;
}


const char *AVSC_CC avisynth_c_plugin_init (AVS_ScriptEnvironment *env)
{
  avs_add_function (env,
    "framercache", "c", 
    framercache_create, 0);
  return "FramerCache: simple hard frame cache";
}



BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}

