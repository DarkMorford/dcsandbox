#include "oggHelpers.h"

#include <kos/dbglog.h>

OggStreamCollection getOggStreams(ogg_sync_state &oggSync)
{
	OggStreamCollection streams;

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
		dbglog(DBG_INFO, "Created stream with serial number %d\n", streamSerial);

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

void queueDataPage(ogg_page *dataPage, OggStreamCollection &streams)
{
	int serial = ogg_page_serialno(dataPage);

	if (streams.count(serial) == 1)
	{
		ogg_stream_pagein(&(streams[serial]), dataPage);
	}
}

void flushSyncBuffer(ogg_sync_state &oggSync, OggStreamCollection &streams)
{
	ogg_page dataPage;

	while (ogg_sync_pageout(&oggSync, &dataPage) == 1)
	{
		queueDataPage(&dataPage, streams);
	}
}

bool isTheoraStream(ogg_stream_state &stream)
{
	bool isTheora = false;

	// Initialize temporary data structures
	th_info theoraInfo;
	th_info_init(&theoraInfo);

	th_comment theoraComments;
	th_comment_init(&theoraComments);

	th_setup_info *theoraSetup = NULL;

	// Grab a copy of the first packet in the stream
	ogg_packet dataPacket;
	ogg_stream_packetpeek(&stream, &dataPacket);

	// Attempt to parse it as a Theora header
	int status = th_decode_headerin(&theoraInfo, &theoraComments, &theoraSetup, &dataPacket);
	isTheora = (status > 0);

	// Clean up everything
	th_setup_free(theoraSetup);
	th_comment_clear(&theoraComments);
	th_info_clear(&theoraInfo);

	return isTheora;
}

th_dec_ctx* initializeTheoraDecoder(th_info &info, th_comment &comments, ogg_stream_state &stream)
{
	th_setup_info *setup = NULL;

	ogg_packet dataPacket;
	ogg_stream_packetpeek(&stream, &dataPacket);
	int status = th_decode_headerin(&info, &comments, &setup, &dataPacket);
	while (status > 0)
	{
		ogg_stream_packetout(&stream, NULL);

		ogg_stream_packetpeek(&stream, &dataPacket);
		status = th_decode_headerin(&info, &comments, &setup, &dataPacket);
	}

	th_dec_ctx *decoder = th_decode_alloc(&info, setup);
	th_setup_free(setup);

	return decoder;
}
