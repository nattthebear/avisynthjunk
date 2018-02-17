// dolphin magic

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "avisynth_c.h"

AVS_VideoFrame *AVSC_CC dm_get_frame (AVS_FilterInfo *fi, int n)
{
  int *framemappings = (int *) fi->user_data;
  
  return avs_get_frame (fi->child, framemappings[n]);
}

void AVSC_CC dm_free (AVS_FilterInfo *fi)
{
  free (fi->user_data);
}

  
  
AVS_Value AVSC_CC dm_create (AVS_ScriptEnvironment *env, AVS_Value args, void *unused)
{
  AVS_Clip *clip;

  clip = avs_take_clip (avs_array_elt (args, 0), env);
  const AVS_VideoInfo *vi = avs_get_video_info (clip);
  
  const char *fname = avs_as_string (avs_array_elt (args, 1));
  
  FILE *fil = fopen (fname, "r");
  if (!fil)
    return avs_new_value_error ("couldn't open file for reading!");

  // read in file
  int ndup = 0;
  int ndupspace = 0;
  int *dups = NULL;
  char buff[1024];
  while (fgets (buff, 1024, fil))
  {
    if (ndup == ndupspace)
	{
	  ndupspace = ndupspace ? ndupspace * 2 : 1024;
	  dups = realloc (dups, ndupspace * sizeof (int));
	  if (!dups)
	  {
	    fclose (fil);
		return avs_new_value_error ("out of memory!");
	  }
	}
    dups[ndup++] = atoi (buff);
  }
  fclose (fil);

  int *framemappings = malloc ((ndup + vi->num_frames) * sizeof (int));
  
  if (!framemappings)
  {
    free (dups);
	return avs_new_value_error ("out of memory!");
  }
  
  int fpos = 0;
  int dpos = 0;
  int i;
  for (i = 0; i < vi->num_frames; i++)
  {
    framemappings[fpos++] = i;
	// while instead of if means a frame can be multiply dupped by specifying it
	// multiple times in a row in the dups list
	while /*if*/ (dpos < ndup && i == dups[dpos])
	{
	  dpos++;
	  framemappings[fpos++] = i;
	}
  }
  
  free (dups);
  

  AVS_FilterInfo *fi;
  AVS_Clip *clipout;

  clipout = avs_new_c_filter (env, &fi, avs_array_elt (args, 0), 1);

  fi->get_frame = dm_get_frame;
  fi->vi.num_frames = ndup + vi->num_frames;
  
  fi->user_data = framemappings;

  fi->free_filter = dm_free;

  
  AVS_Value ret = avs_new_value_clip (clipout);
  avs_release_clip (clipout);

  return ret;
}  

const char *AVSC_CC avisynth_c_plugin_init (AVS_ScriptEnvironment *env)
{
  avs_add_function (env,
    "dolphinmagic", "cs", 
    dm_create, 0);
  return "dolphinfixup";
}




BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}



