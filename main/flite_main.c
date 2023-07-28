/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  Simple top level program                                             */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "flite.h"
#include "flite_version.h"

cst_val *flite_set_voice_list(const char *voxdir);

#ifdef WASM32_WASI
void flite_set_lang_list(void);
#else
void *flite_set_lang_list(void);
#endif

void cst_alloc_debug_summary();

/* Its not very appropriate that these are declared here */
void usenglish_init(cst_voice *v);
cst_lexicon *cmu_lex_init(void);

static void flite_version()
{
    printf("  Carnegie Mellon University, Copyright (c) 1999-2016, all rights reserved\n");
    printf("  version: %s-%s-%s %s (http://cmuflite.org)\n",
	   FLITE_PROJECT_PREFIX,
	   FLITE_PROJECT_VERSION,
	   FLITE_PROJECT_STATE,
	   FLITE_PROJECT_DATE);
}

static void flite_usage()
{
    printf("flite: a small simple speech synthesizer\n");
    flite_version();
    printf("usage: flite TEXT/FILE [WAVEFILE]\n"
           "  Converts text in TEXTFILE to a waveform in WAVEFILE\n"
           "  If text contains a space the it is treated as a literal\n"
           "  textstring and spoken, and not as a file name\n"
           "  if WAVEFILE is unspecified or \"play\" the result is\n"
           "  played on the current systems audio device.  If WAVEFILE\n"
           "  is \"none\" the waveform is discarded (good for benchmarking)\n"
           "  Other options must appear before these options\n"
           "  --version   Output flite version number\n"
           "  --help      Output usage string\n"
           "  -o WAVEFILE Explicitly set output filename\n"
           "  -f TEXTFILE Explicitly set input filename\n"
           "  -t TEXT     Explicitly set input textstring\n"
           "  -p PHONES   Explicitly set input textstring and synthesize as phones\n"
           "  --set F=V   Set feature (guesses type)\n"
           "  -s F=V      Set feature (guesses type)\n"
           "  --seti F=V  Set int feature\n"
           "  --setf F=V  Set float feature\n"
           "  --sets F=V  Set string feature\n"
	   "  -ssml       Read input text/file in ssml mode\n"
	   "  -b          Benchmark mode\n"
	   "  -l          Loop endlessly\n"
	   "  -voice NAME Use voice NAME (NAME can be pathname/url to flitevox file)\n"
	   "  -voicedir NAME Directory containing (clunit) voice data\n"
	   "  -lv         List voices available\n"
	   "  -add_lex FILENAME add lex addenda from FILENAME\n"
	   "  -pw         Print words\n"
	   "  -ps         Print segments\n"
           "  -psdur      Print segments and their durations (end-time)\n"
	   "  -pr RelName Print relation RelName\n"
           "  -voicedump FILENAME Dump selected (cg) voice to FILENAME\n"
           "  -v          Verbose mode\n");
    exit(0);
}

static void flite_voice_list_print(void)
{
    cst_voice *voice;
    const cst_val *v;

    printf("Voices available: ");
    for (v=flite_voice_list; v; v=val_cdr(v))
    {
        voice = val_voice(val_car(v));
        printf("%s ",voice->name);
    }
    printf("\n");

    return;
}

static cst_utterance *print_info(cst_utterance *u)
{
    cst_item *item;
    const char *relname;
    int printEndTime = 0;
    int printStress = 0;
    
    relname = utt_feat_string(u,"print_info_relation");
    if (cst_streq(relname, "SegmentEndTime"))
    {
        relname = "Segment";
        printEndTime = 1;
    }
    if (cst_streq(relname, "SegmentStress"))
    {
        relname = "Segment";
        printStress = 1;
    }
      
    for (item=relation_head(utt_relation(u,relname)); 
	 item; 
	 item=item_next(item))
    {
      if (!printEndTime)
        printf("%s",item_feat_string(item,"name"));
      else
        printf("%s:%1.3f",item_feat_string(item,"name"), item_feat_float(item,"end"));
      if (printStress == 1)
      {
        if (cst_streq("+",ffeature_string(item,"ph_vc")))
            printf("%s",ffeature_string(item,"R:SylStructure.parent.stress"));
      }
      printf(" ");
    }
    printf("\n");

    return u;
}

static void ef_set(cst_features *f,const char *fv,const char *type)
{
    /* set feature from fv (F=V), guesses type if not explicit type given */
    const char *val;
    char *feat;
    const char *fname;

    if ((val = strchr(fv,'=')) == 0)
    {
	fprintf(stderr,
		"flite: can't find '=' in featval \"%s\", ignoring it\n",
		fv);
    }
    else
    {
	feat = cst_strdup(fv);
	feat[cst_strlen(fv)-cst_strlen(val)] = '\0';
        fname=feat_own_string(f,feat);
	val = val+1;
	if ((type && cst_streq("int",type)) ||
	    ((type == 0) && (cst_regex_match(cst_rx_int,val))))
	    feat_set_int(f,fname,atoi(val));
	else if ((type && cst_streq("float",type)) ||
		 ((type == 0) && (cst_regex_match(cst_rx_double,val))))
	    feat_set_float(f,fname,atof(val));
	else
	    feat_set_string(f,fname,val);
        cst_free(feat);
    }
}

void init() {
	flite_init();
	flite_set_lang_list();

	if(flite_voice_list == NULL)
		flite_set_voice_list(NULL);
}

void speak(char *voice, char *text, char *filename) {
	cst_voice *desired_voice = flite_voice_select(voice);
	flite_text_to_speech(text, desired_voice, filename);
}

void cleanup() {
	delete_val(flite_voice_list); flite_voice_list=0;
}
