#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>

#include "avisynth_c.h"

#include <windows.h>

// filler functions
// the script can function if these entries are set to NULL instead,
// but some things may crash (in particular, Info() crashes)
int AVSC_CC filler_get_parity (AVS_FilterInfo *fi, int n)
{
  return 0;
}
int AVSC_CC filler_get_audio (AVS_FilterInfo *fi, void * buf, INT64 start, INT64 count)
{
  return 0;
}
int AVSC_CC filler_set_cache_hints (AVS_FilterInfo *fi, int cachehints, int frame_range)
{
  return 0;
}



// our filter's userdata
typedef struct
{
  FILE *f; // input file
  int nframein; // number of frames from input
  unsigned long long *seekspots; // array of fseek() locations for each input frame with nframein elts
  int nframeout; // number of frames for output
  int *remaps; // array of input frame numbers that each output frame has with nframeout elts
  
  int w; // output dimensions
  int h;
  
  int cachenum; // currently cached frame, -1 for none
  unsigned long *cachedata; // cached frame (RGB0, output dimensions)

} udata_t;

/*AVSC_API(AVS_Value, avs_invoke)(AVS_ScriptEnvironment *, const char * name, 
                               AVS_Value args, const char** arg_names);
*/


AVS_VideoFrame *AVSC_CC jmdsource_get_frame2 (AVS_FilterInfo *fi, int n)
{
  AVS_Value *resizers = (AVS_Value *) fi->user_data;
  /*
  if (resizers[n])
  {
	char buffra[256];
	sprintf (buffra, "#%08x", (int) resizers[n]);
	MessageBox (0, buffra, "GENESIS", 0);
  }*/


  
  if (/*resizers[n] &&*/ avs_is_clip (resizers[n]))
    return avs_get_frame (avs_take_clip (resizers[n], fi->env), n);  

  // oops?  or possibly null frame
  AVS_VideoFrame *ret = avs_new_video_frame (fi->env, &fi->vi);
  
  memset (avs_get_write_ptr (ret), 0x00, avs_get_pitch (ret) * avs_get_height (ret));
  
  
  return ret;

}

void AVSC_CC jmdsource_free_filter2 (AVS_FilterInfo *fi)
{
  //AVS_Value *resizers = (AVS_Value *) fi->user_data;
  
  free (fi->user_data);

}






// vflip, swap R and B.  otherwise, just 1:1 scale
static void resizeblit (void *vdst, int dstw, int dsth, int dstp, const void *vsrc, int srcw, int srch, int srcp, udata_t *u)
{
  unsigned char *dst = vdst;
  const unsigned char *src = vsrc;
  
  memset (dst, 0, dstp * dsth);
  
  dst += (dsth - 1) * dstp; // vflip
  dstp = -dstp;
  
  int j, i;
  for (j = 0; j < srch; j++)
  {
    for (i = 0; i < srcw ; i++)
	{
	  dst[4 * i + 0] = src[4 * i + 2];
	  dst[4 * i + 1] = src[4 * i + 1];
	  dst[4 * i + 2] = src[4 * i + 0];
	  dst[4 * i + 3] = 0;
	}
    //memcpy (dst, src + srcp * j, srcw * 4);
	src += srcp;
	dst += dstp;
  }
  
  /*
  int i, j;
  for (j = 0; j < dsth; j++)
  {
    for (i = 0; i < dstw; i++)
	{
	  int xsrc = i * srcw / dstw;
	  int ysrc = j * srch / dsth;
	  const unsigned char *srcpx = src + ysrc * srcp + xsrc * 4;
	  unsigned char *dstpx = dst + j * dstp + i * 4;
	  dstpx[0] = srcpx[2];
	  dstpx[1] = srcpx[1];
	  dstpx[2] = srcpx[0];
	  dstpx[3] = 0;
	}
  }
  */

  
}



AVS_VideoFrame *AVSC_CC jmdsource_get_frame (AVS_FilterInfo *fi, int n)
{
  udata_t *u = (udata_t *) fi->user_data;
  
  int infr = u->remaps[n];
  
  if (ferror (u->f))
  {
    fi->error = "io error!";
	return NULL;
  }
  
  
  if (u->cachenum != infr)
  {
    u->cachenum = infr;
    
	if (fseeko64 (u->f, u->seekspots[infr], SEEK_SET) != 0)
    {
	  fi->error = "bad fseek?";
	  return NULL;
	}
	// 1 char: minor type
	unsigned char rp_minor;
	fread (&rp_minor, 1, 1, u->f);
	if (rp_minor != 0 && rp_minor != 1)
	{
	  fi->error = "unknown minor in frame";
	  return NULL;
	}
	// read in variable length
	unsigned long datalen = 0;
    unsigned char c;
	while (fread (&c, 1, 1, u->f) == 1)
    {
	  datalen = (datalen << 7) + (c & 0x7f);
	  if (!(c & 0x80))
		break;
    }
	if (feof (u->f) || ferror (u->f))
	{
	  fi->error = "huh?";
	  return NULL;
	}
	if (datalen > 512 * 1024 * 1024)
	{
	  fi->error = "giant frame";
	  return NULL;
	}
	unsigned char *indata = malloc (datalen);
	if (!indata)
	{
	  fi->error = "ENOMEM";
	  return NULL;
	}
	if (fread (indata, 1, datalen, u->f) != datalen)
	{
	  free (indata);
	  fi->error = "io error?";
	  return NULL;
	}
	int in_w = indata[0] << 8 | indata[1];
	int in_h = indata[2] << 8 | indata[3];
	
	if (rp_minor == 1) // compessed
	{
	  unsigned char *indata2 = malloc (in_w * in_h * 4 + 4);
	  if (!indata2)
	  {
	    free (indata);
		fi->error = "ENOMEM";
		return NULL;
	  }
	  z_stream zs = {indata + 4, datalen - 4, 0, indata2 + 4, in_w * in_h * 4, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, 0};
	  if (inflateInit (&zs) != Z_OK)
	  {
	    free (indata2);
		free (indata);
		fi->error = "zlib error";
		return NULL;
	  }
	  inflate (&zs, Z_FINISH);
	  inflateEnd (&zs);
	  free (indata);
	  indata = indata2;
	}

	// add pitch adjust, resize
	// from: indata + 4: RGB32,  in_w * in_h, no special pitch
	// to: u->cachedata: BGR32 vflip, u->w * u->h, pitch is ??'
	
	resizeblit (u->cachedata, u->w, u->h, u->w * 4, indata + 4, in_w, in_h, in_w * 4, u);
	//memcpy (u->cachedata, indata + 4, in_w * in_h * 4);
    
	free (indata);
  }
  
  AVS_VideoFrame *ret = avs_new_video_frame (fi->env, &fi->vi);
  // since we've adjusted pitch already, this is just a straight copy
  memcpy (avs_get_write_ptr (ret), u->cachedata, avs_get_row_size (ret) * u->h);
  return ret;
}

void AVSC_CC jmdsource_free_filter (AVS_FilterInfo *fi)
{
  udata_t *u = (udata_t *) fi->user_data;
  if (!u)
    return;
  if (u->seekspots)
    free (u->seekspots);
  if (u->remaps);
    free (u->remaps);
  if (u->cachedata);
    free (u->cachedata);  
  if (u->f)
    fclose (u->f);
  free (u);
}


static int skip_channel_table (FILE *f)
{
  unsigned char scratch[64];
  
  // nchannels: 2 bytes BE
  if (fread (scratch, 1, 2, f) != 2)
    return -1;
  int nch = scratch[0] << 8 | scratch[1];
  
  int i;
  int vidchannel = -1;
  
  for (i = 0; i < nch; i++)
  {
    // two bytes each: channel number, channel type, channel name
    if (fread (scratch, 1, 6, f) != 6)
	  return -1;
	if (scratch[2] == 0 && scratch[3] == 0) // video
	  vidchannel = i;
	
	int namelen = scratch[4] << 8 | scratch[5];
	while (namelen > 64)
	{
	  if (fread (scratch, 1, 64, f) != 64)
	    return -1;
	  namelen -= 64;
	}
	if (fread (scratch, 1, namelen, f) != namelen)
	  return -1;
  }
	
  
  return vidchannel;
}


// NULL on success
const char *index_jmd (FILE *f, udata_t *u, int fpsnum, int fpsden, int **inwidth, int **inheight)
{
  unsigned char scratch[64];
  
  int indimspc = 0;
  int indims = 0;
  int *in_widths = NULL;
  int *in_heights = NULL;
  
  // must start with newsegment
  fread (scratch, 1, 16, f);
  if (strncmp ((const char *) scratch, "\xff\xffJPCRRMULTIDUMP", 16) != 0)
    return "bad file signature";
  
  int vidchannel = -1;
  u->nframein = 0;
  u->nframeout = 0;
  u->seekspots = NULL;
  int seekspotspace = 0;
  u->remaps = NULL;
  int remapspace = 0;
  u->cachenum = -1;
  u->cachedata = NULL;
  u->f = f;
  
  u->w = 0;
  u->h = 0;
  
  unsigned long long timestampoff = 0;
  
  unsigned long long *vidframestarts = NULL;
  
  unsigned long long lasttimestamp = 0;
  
  if (fseeko64 (f, 0LL, SEEK_SET) != 0)
    return "couldn't seek input";
  
  #define FAIL(s) {\
    if (vidframestarts) free (vidframestarts);\
	if (in_widths) free (in_widths);\
	if (in_heights) free (in_heights);\
	return s;\
  }\
  
  while (!feof (f) && !ferror (f))
  {
    int pchan;
	if (fread (scratch, 1, 2, f) != 2)
	  break;
	pchan = scratch[0] << 8 | scratch[1];
	if (pchan == 65535) // special
	{
	  fread (scratch, 1, 4, f);
	  if (strncmp ((const char *) scratch, "\xff\xff\xff\xff", 4) == 0)
	  {
	    // update offset
		timestampoff += 0xffffffffull;
      }
	  else if (strncmp ((const char *) scratch, "JPCR", 4) == 0)
	  {
	    // segment header
		fread (scratch, 1, 10, f);
		if (strncmp ((const char *) scratch, "RMULTIDUMP", 10) != 0)
		  FAIL ("bad segment table");
		vidchannel = skip_channel_table (f);
		if (vidchannel < 0)
		  FAIL ("bad segment table");
      }
	  else 
	  {
	    FAIL ("unknown special packet");
      }
	  continue;
	}
	// parse time delta
	fread (scratch, 1, 4, f);
	lasttimestamp = (scratch[0] << 24 | scratch[1] << 16 | scratch[2] << 8 | scratch[3]) + timestampoff;
		
	if (pchan == vidchannel)
	{ // found a video packet, store location
	  if (seekspotspace == u->nframein)
	  {
	    seekspotspace = seekspotspace ? seekspotspace * 2 : 128;
		u->seekspots = realloc (u->seekspots, sizeof (u->seekspots[0]) * seekspotspace);
		vidframestarts = realloc (vidframestarts, sizeof (vidframestarts[0]) * seekspotspace);
		if (!u->seekspots || !vidframestarts)
		  FAIL ("out of memory");
      }
	  // will seek to right after the timestamp
	  u->seekspots[u->nframein] = ftello64 (f);
      vidframestarts[u->nframein] = lasttimestamp;
	  u->nframein++;
	}
    // skip "minor" type for now
	fread (scratch, 1, 1, f);
	// read length
	unsigned long long plen = 0;
    unsigned char c;
	while (fread (&c, 1, 1, f) == 1)
    {
	  plen = (plen << 7) + (c & 0x7f);
	  if (!(c & 0x80))
		break;
    }
	if (feof (f) || ferror (f))
	  FAIL ("malformed varlen");
	
	// for video channels, peek the resolution now
	if (pchan == vidchannel)
	{
	  fread (scratch, 1, 4, f);
	  int temp_w = scratch[0] << 8 | scratch[1];
	  int temp_h = scratch[2] << 8 | scratch[3];
	  if (temp_w > u->w)
	    u->w = temp_w;
	  if (temp_h > u->h)
	    u->h = temp_h;
	  if (fseek ( f, -4, SEEK_CUR != 0))
	    FAIL ("unexpected seek error");
    
	  // store input dimensions for further processing
      if (indims == indimspc)
	  {
	    indimspc = indimspc ? indimspc * 2 : 128;
	    in_widths = realloc (in_widths, sizeof (int) * indimspc);
	    in_heights = realloc (in_heights, sizeof (int) * indimspc);
	  }
	  in_widths[indims] = temp_w;
	  in_heights[indims] = temp_h;
	  indims++;
	}	
	// skip data
	if (fseeko64 (f, plen, SEEK_CUR) != 0)
	  FAIL ("unexpected error/eof");
  }
  
  if (ferror (f))
    FAIL ("i/o error!");

  if (u->nframein == 0)
    FAIL ("no video data in jmd?");
	
  // dedup
  if (fpsden == 0) // VFR
  { // todo: output timecodes to a file?
    u->nframeout = u->nframein;
	u->remaps = malloc (sizeof (u->remaps[0]) * u->nframeout);
	if (!u->remaps)
	  FAIL ("out of memory");
	int i;
	for (i = 0 ; i < u->nframeout; i++)
	{
	  u->remaps[i] = i;
	}
  }
  else // CFR
  {
    // frame timestamps are in nanoseconds.
	// each frame ends when the next one starts.  the last frame ends on the last packet timestamp
	// (for any type of packet), which we've saved
    // todo
	FAIL ("cfr not yet implemented");
  }

  free (vidframestarts);
  *inwidth = in_widths;
  *inheight = in_heights;
  
  
  return NULL; // no error
  
}


char frrret[256];
const char *find_resizer (int inw, int inh, const char *resizers)
{
  // string is something like
  // "256 224 Point!512 448 Bilinear!0 0 Lanczos"
  // with whatever comes last being default

  char scratch[256];
  
  strcpy (frrret, "PointResize");
  
  
  int foundw;
  int foundh;
  
  int strpos = 0;
  
  while (1)
  {
    if (sscanf (resizers + strpos, "%u %u %255s", &foundw, &foundh, scratch) != 3)
	  break;
	if (foundw == inw && foundh == inh)
	  strcpy (frrret, scratch);
	  
    if (!strchr (resizers + strpos, '!'))
	  break;
	strpos = strchr (resizers + strpos, '!') - resizers + 1;
  
  }
  
  return frrret;

}

typedef struct
{
  int inw;
  int inh;
  char nom[256];
  AVS_Value result;
} tempresizer_t;


AVS_Value *setup_resizer (int *inwidth, int *inheight, int nframein, int *remaps, int nframeout, int width, int height, const char *resizers, AVS_Clip *clipout, AVS_ScriptEnvironment *env)
{
  AVS_Value *ret = malloc (sizeof (*ret) * nframeout);
  if (!ret)
    return ret;

	
  tempresizer_t *trs = NULL;
  int ntrs = 0;
  int ntrs_sz = 0;
  
	
  int i, j;
  
  
  
  for (i = 0; i < nframeout; i++)
  {
    // 0 width or 0 height frames can happen; we'll just output black there
	if (!inwidth[remaps[i]] || !inheight[remaps[i]])
	{
	  ret[i] = avs_void;
	  continue;
	}
  
    const char *desired = find_resizer (inwidth[remaps[i]], inheight[remaps[i]], resizers);

	// see if we've stocked this resizer already
	for (j = 0; j < ntrs; j++)
	{
	  if (trs[j].inw == inwidth[remaps[i]] && trs[j].inh == inheight[remaps[i]] && strcmp (trs[j].nom, desired) == 0)
	  {
	    avs_copy_value (ret + i, trs[j].result);
		break;
	  }
	}
	if (j < ntrs)
	  continue;
	// create resizer
	// hard_crop
	AVS_Value dims[5];
	dims[0] = avs_new_value_clip (clipout);
	dims[1] = avs_new_value_int (0);
	dims[2] = avs_new_value_int (0);
	dims[3] = avs_new_value_int (inwidth[remaps[i]]);
	dims[4] = avs_new_value_int (inheight[remaps[i]]);
	
	AVS_Value clipped = avs_invoke (env, "Crop", avs_new_value_array (dims, 5), NULL);
	
	avs_release_value (dims[0]);
	avs_release_value (dims[1]);
	avs_release_value (dims[2]);
	avs_release_value (dims[3]);
	avs_release_value (dims[4]);
	
	if (!avs_is_clip (clipped))
	{
	  char buffra[256];
	  sprintf (buffra, "%s (%i, %i, %i, %i)", "Crop", 0, 0, inwidth[remaps[i]], inheight[remaps[i]]);
	  MessageBox (0, buffra, "Failed", 0);
	}
	
	
	dims[0] = clipped;
	dims[1] = avs_new_value_int (width);
	dims[2] = avs_new_value_int (height);
	
	
	AVS_Value resized = avs_invoke (env, desired, avs_new_value_array (dims, 3), NULL);
	avs_release_value (dims[0]);
	avs_release_value (dims[1]);
	avs_release_value (dims[2]);

	if (!avs_is_clip (resized))
	{
	  char buffra[256];
	  sprintf (buffra, "%s (%i, %i)", desired, width, height);
	  MessageBox (0, buffra, "Failed", 0);
	}

	
	if (ntrs == ntrs_sz)
	{
	  ntrs_sz = ntrs_sz ? ntrs_sz * 2 : 128;
	  trs = realloc (trs, sizeof (*trs) * ntrs_sz);
	  if (!trs)
	  {
	    free (ret);
		return NULL;
	  }	
	}
    if (!avs_is_clip (resized))
	{
	  free (ret);
	  free (trs);
	  return NULL;
	}
	
	
	{
	  char buffra[256];
	  sprintf (buffra, "genesis: %s (%i, %i)->(%i,%i)", desired, inwidth[remaps[i]], inheight[remaps[i]], width, height);
	  MessageBox (0, buffra, "GENESIS", 0);
	}
	
	
	trs[ntrs].inw = inwidth[remaps[i]];
	trs[ntrs].inh = inheight[remaps[i]];
	strcpy (trs[ntrs].nom, desired);
	avs_copy_value (&trs[ntrs].result, resized);
	
	avs_copy_value (ret + i, trs[ntrs].result);
	
	ntrs++;
  }



  free (trs);
  return ret;


}




AVS_Value AVSC_CC jmdsource_create (AVS_ScriptEnvironment *env, AVS_Value args, void *unused)
{
  udata_t *ud;
  FILE *fin;

  // parse arguments
  const char *filename;
  const char *resizers;
  int fpsnum, fpsden, width, height;
  //const char *timecodes;
  
  if (!avs_is_string (avs_array_elt (args, 0)))
    return avs_new_value_error ("Need to give filename");
  
  filename = avs_as_string (avs_array_elt (args, 0));

  /*
  // default to VFR
  fpsnum = avs_is_int (avs_array_elt (args, 1)) ?
           avs_as_int (avs_array_elt (args, 1)) : 1;
  fpsden = avs_is_int (avs_array_elt (args, 2)) ?
           avs_as_int (avs_array_elt (args, 2)) : 0;
  */
  fpsnum = 1;
  fpsden = 0;
		   
		   
  // default width/height set on file parse
  width = avs_is_int (avs_array_elt (args, 1)) ?
          avs_as_int (avs_array_elt (args, 1)) : -1;
  height = avs_is_int (avs_array_elt (args, 2)) ?
           avs_as_int (avs_array_elt (args, 2)) : -1;

  resizers = avs_is_string (avs_array_elt (args, 3)) ?
             avs_as_string (avs_array_elt (args, 3)) : "";
  //timecodes = avs_is_string (avs_array_elt (args, 4)) ?
  //            avs_as_string (avs_array_elt (args, 4)) : "";

			 
  fin = fopen (filename, "rb");
  if (!fin)
    return avs_new_value_error ("Couldn't open infile for reading");
  /*
  if (strcmp (timecodes, filename) == 0)
  {
    fclose (fin);
    return avs_new_value_error ("timecode and infile are same?")
  }*/
  
  ud = malloc (sizeof (*ud));
  if (!ud)
  {
    fclose (fin);
    return avs_new_value_error ("out of memory");
  }
  
  int *inwidth;
  int *inheight;
  
  
  
  const char *err = index_jmd (fin, ud, fpsnum, fpsden, &inwidth, &inheight);
  if (err)
  {
    fclose (fin);
	if (ud->seekspots)
	  free (ud->seekspots);
	if (ud->remaps)
	  free (ud->remaps);
	free (ud);
    return avs_new_value_error (err);
  }

  
  
  /* don't do this, as it's handled by the resizers creator
  // index_jmd () set us a width and height (equal to the largest ones seen in the file)
  // overwrite only if user gave us overwrites
  if (width > 0)
    ud->w = width;
  if (height > 0)
    ud->h = height;
  */
  
  
  
  // for great pitchstice!
  ud->cachedata = malloc (sizeof (ud->cachedata[0]) * (ud->w + 4) * ud->h);
  if (!ud->cachedata)
  {
    fclose (fin);
	if (ud->seekspots)
	  free (ud->seekspots);
	if (ud->remaps)
	  free (ud->remaps);
	free (ud);
    return avs_new_value_error ("out of memory");
  }
	
  
  
  AVS_FilterInfo *fi;
  AVS_Clip *clipout;

  clipout = avs_new_c_filter (env, &fi, avs_void, 0);

  fi->user_data = ud;
  fi->get_frame = jmdsource_get_frame;
  fi->free_filter = jmdsource_free_filter;
  fi->get_parity = filler_get_parity; // ok for frame based?
  fi->set_cache_hints = filler_set_cache_hints; // ??
  fi->get_audio = filler_get_audio; // ok for no audio?

  fi->vi.width = ud->w;
  fi->vi.height = ud->h;
  fi->vi.fps_numerator = fpsden ? fpsnum : 25;
  fi->vi.fps_denominator = fpsden ? fpsden : 1;
  fi->vi.num_frames = ud->nframeout;
  fi->vi.pixel_type = AVS_CS_BGR32;
  fi->vi.audio_samples_per_second = 0;
  fi->vi.sample_type = 0;
  fi->vi.num_audio_samples = 0;
  fi->vi.nchannels = 0;
  fi->vi.image_type = 0; // framebased?


  
  // create second filter (resizer) as well

  AVS_Value *resizevals = setup_resizer (inwidth, inheight, ud->nframein, ud->remaps, ud->nframeout, width, height, resizers, clipout, env);
  
  if (!resizevals)
  {

    fclose (fin);
	if (ud->seekspots)
	  free (ud->seekspots);
	if (ud->remaps)
	  free (ud->remaps);
	free (ud);
    return avs_new_value_error ("setup on resizers failed");
  }  
  
  
  AVS_FilterInfo *fi2;
  AVS_Clip *clipout2;
  
  clipout2 = avs_new_c_filter (env, &fi2, avs_void, 0);
  
  fi2->user_data = resizevals;
  fi2->get_frame = jmdsource_get_frame2;
  fi2->free_filter = jmdsource_free_filter2;
  fi2->get_parity = filler_get_parity; // ok for frame based?
  fi2->set_cache_hints = filler_set_cache_hints; // ??
  fi2->get_audio = filler_get_audio; // ok for no audio?
  
  fi2->vi.width = width;
  fi2->vi.height = height;
  fi2->vi.fps_numerator = fpsden ? fpsnum : 25;
  fi2->vi.fps_denominator = fpsden ? fpsden : 1;
  fi2->vi.num_frames = ud->nframeout;
  fi2->vi.pixel_type = AVS_CS_BGR32;
  fi2->vi.audio_samples_per_second = 0;
  fi2->vi.sample_type = 0;
  fi2->vi.num_audio_samples = 0;
  fi2->vi.nchannels = 0;
  fi2->vi.image_type = 0; // framebased?
 
  return avs_new_value_clip (clipout2);
  
  
  //return ret;  
}


const char *AVSC_CC avisynth_c_plugin_init (AVS_ScriptEnvironment *env)
{
  avs_add_function (env,
    "jmdsource", "[filename]s[width]i[height]i[resizers]s", 
    jmdsource_create, 0);
  return "JMDSource: Load JPC-RR dump files (video only)";
}

/* optional
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}
*/
