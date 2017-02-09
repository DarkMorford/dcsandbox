#ifndef THREADFUNCS_H
#define THREADFUNCS_H

#include <cstdio>
#include <kos/cond.h>

#include "oggHelpers.h"

struct fileReaderParams
{
	FILE *mediaFile;
	ogg_sync_state *mediaSync;
	OggStreamCollection *oggStreams;
	
	unsigned short bufferSize;
	
	bool *streamNeedsData;
	condvar_t *condition;
	mutex_t *mutex;
};

void* fileReaderThread(void *threadParams);

#endif
