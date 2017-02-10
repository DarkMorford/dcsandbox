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
		size_t bytesRead = fillSyncBuffer(params->mediaFile, *(params->mediaSync), params->bufferSize);
		if (bytesRead == 0)
			return NULL;
		
		// Pump it into the streams
		flushSyncBuffer(*(params->mediaSync), *(params->oggStreams));
		
		// Signal that we've added data to the streams
		*params->streamNeedsData = false;
		
		// Release the mutex
		mutex_unlock(params->mutex);
	}
	
	return NULL;
}
