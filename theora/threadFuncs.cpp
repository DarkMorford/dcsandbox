#include "threadFuncs.h"

void* fileReaderThread(void *threadParams)
{
	fileReaderParams *params = reinterpret_cast<fileReaderParams*>(threadParams);
	
	while (!feof(params->mediaFile))
	{
		// Acquire mutex for stream collection
		mutex_lock(params->mutex);
		
		// Check to see if any streams need data
		while (*params->streamNeedsData == false)
		{
			// If not, wait until they do
			cond_wait(params->condition, params->mutex);
		}
		
		// Read data from file
		char *dataBuffer = ogg_sync_buffer(params->mediaSync, params->bufferSize);
		size_t bytesRead = fread(dataBuffer, 1, params->bufferSize, params->mediaFile);
		ogg_sync_wrote(params->mediaSync, bytesRead);
		if (bytesRead == 0)
			return NULL;
		
		// Pump it into the streams
		ogg_page dataPage;
		while(ogg_sync_pageout(params->mediaSync, &dataPage) == 1)
		{
			int pageSerial = ogg_page_serialno(&dataPage);
			ogg_stream_pagein(&(*params->oggStreams)[pageSerial], &dataPage);
		}
		
		// Signal that we've added data to the streams
		*params->streamNeedsData = false;
		
		// Release the mutex
		mutex_unlock(params->mutex);
	}
	
	return NULL;
}
