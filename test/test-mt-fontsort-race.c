/* Copyright (C) 2026 fontconfig Authors */
/* SPDX-License-Identifier: HPND */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <fontconfig/fontconfig.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef FONTFILE
#  define FONTFILE "no_family_name.ttf"
#endif

#define NUM_READERS    2
#define NUM_ITERATIONS 1000

static atomic_bool go;
static atomic_bool stop;

static void *
reader (void *arg)
{
    (void)arg;
    while (!atomic_load_explicit (&go, memory_order_acquire)) {
    }
    while (!atomic_load_explicit (&stop, memory_order_relaxed)) {
	FcPattern *pattern = FcPatternCreate();
	if (!pattern)
	    continue;
	FcPatternAddString (pattern, FC_LANG, (const FcChar8 *)"xx");
	FcPatternAddBool (pattern, FC_SCALABLE, FcTrue);
	FcConfigSubstitute (NULL, pattern, FcMatchPattern);
	FcDefaultSubstitute (pattern);
	FcResult   result;
	FcFontSet *fs = FcFontSort (NULL, pattern, FcFalse, NULL, &result);
	if (fs)
	    FcFontSetDestroy (fs);
	FcPatternDestroy (pattern);
    }
    return NULL;
}

static void *
writer (void *arg)
{
    FcConfig *config = (FcConfig *)arg;

    while (!atomic_load_explicit (&go, memory_order_acquire)) {
    }
    for (int i = 0; i < NUM_ITERATIONS && !atomic_load_explicit (&stop, memory_order_relaxed); i++) {
	FcConfigAppFontAddFile (config, (const FcChar8 *)FONTFILE);
    }
    atomic_store_explicit (&stop, 1, memory_order_relaxed);
    return NULL;
}

int
main (int argc, char **argv)
{
    pthread_t  readers[NUM_READERS];
    pthread_t  w;
    FcConfig  *config;
    FcFontSet *app_set;
    int        i;

    config = FcConfigCreate();
    if (!config) {
	fprintf (stderr, "FcConfigCreate failed\n");
	return 1;
    }
    if (!FcConfigParseAndLoadFromMemory (config, (const FcChar8 *)"<fontconfig></fontconfig>", FcTrue)) {
	fprintf (stderr, "FcConfigParseAndLoadFromMemory failed\n");
	return 1;
    }
    if (!FcConfigSetCurrent (config)) {
	fprintf (stderr, "FcConfigSetCurrent failed\n");
	return 1;
    }
    FcConfigDestroy (config);
    config = FcConfigGetCurrent();

    if (!FcConfigAppFontAddFile (config, (const FcChar8 *)FONTFILE)) {
	fprintf (stderr, "FcConfigAppFontAddFile failed for %s\n", FONTFILE);
	return 1;
    }

    app_set = FcConfigGetFonts (config, FcSetApplication);
    if (!app_set || app_set->nfont < 1) {
	fprintf (stderr, "Application font set is empty\n");
	return 1;
    }

    atomic_init (&go, 0);
    atomic_init (&stop, 0);

    for (i = 0; i < NUM_READERS; i++) {
	if (pthread_create (&readers[i], NULL, reader, NULL) != 0) {
	    fprintf (stderr, "Failed to create reader thread %d\n", i);
	    return 1;
	}
    }
    if (pthread_create (&w, NULL, writer, config) != 0) {
	fprintf (stderr, "Failed to create writer thread\n");
	return 1;
    }

    atomic_store_explicit (&go, 1, memory_order_release);

    pthread_join (w, NULL);
    atomic_store_explicit (&stop, 1, memory_order_relaxed);
    for (i = 0; i < NUM_READERS; i++) {
	pthread_join (readers[i], NULL);
    }

    FcFini();
    return 0;
}
