#include "oggHelpers.h"

OggStreams getOggStreams(ogg_sync_state &oggSync)
{
	OggStreams streams;
	
	// Pull the first available page of data
	ogg_page dataPage;
	ogg_sync_pageout(&oggSync, &dataPage);
	
	while (ogg_page_bos(&dataPage))
	{
		// Get the serial number of this page's stream
		int streamSerial = ogg_page_serialno(&dataPage);
		
		// Create a stream with this serial number
		ogg_stream_state newStream;
		ogg_stream_init(&newStream, streamSerial);
		
		// Insert the page of data into the stream
		ogg_stream_pagein(&newStream, &dataPage);
		
		// Insert the stream into the mapping
		streams[streamSerial] = newStream;
		
		// Get the next page
		ogg_sync_pageout(&oggSync, &dataPage);
	}
	
	// Don't leak the last page, put it where it belongs
	queueDataPage(&dataPage, streams);
	
	return streams;
}

void queueDataPage(ogg_page *dataPage, OggStreams &streams)
{
	int serial = ogg_page_serialno(dataPage);
	
	if (streams.count(serial) == 1)
	{
		ogg_stream_pagein(&(streams[serial]), dataPage);
	}
}

void flushSyncBuffer(ogg_sync_state &oggSync, OggStreams &streams)
{
	ogg_page dataPage;
	
	while (ogg_sync_pageout(&oggSync, &dataPage) == 1)
	{
		queueDataPage(&dataPage, streams);
	}
}
