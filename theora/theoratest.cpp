#include <kos.h>

#include <ogg/ogg.h>
#include <theora/codec.h>
#include <theora/theoradec.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);


size_t fillSyncBuffer(FILE *inFile, ogg_sync_state *sync)
{
	const size_t BUFFER_SIZE = 4096;

	char *buffer = ogg_sync_buffer(sync, BUFFER_SIZE);
	size_t bytesRead = fread(buffer, 1, BUFFER_SIZE, inFile);
	ogg_sync_wrote(sync, bytesRead);

	return bytesRead;
}

int main(int argc, char *argv[])
{
	dbglog_set_level(DBG_NOTICE);
	dbglog(DBG_NOTICE, "%s\n", th_version_string());

	// Open the video file
	FILE *videoFile = fopen("/rd/320x240.ogg", "rb");

	// Initialize Ogg sync layer
	ogg_sync_state syncState;
	ogg_sync_init(&syncState);

	// Initialize Theora data structures
	th_info theoraInfo;
	th_info_init(&theoraInfo);

	th_comment theoraComments;
	th_comment_init(&theoraComments);

	th_setup_info *theoraSetup = NULL;
	ogg_stream_state theoraStream;

	// Parse the Ogg headers and find a Theora stream
	bool parsingOggHeaders = true;
	bool parsingTheoraHeaders = false;;
	bool foundTheora = false;
	while (parsingOggHeaders)
	{
		// Load up the buffer with data
		size_t bytesRead = fillSyncBuffer(videoFile, &syncState);

		// Get the next data page
		ogg_page dataPage;
		while (ogg_sync_pageout(&syncState, &dataPage) > 0)
		{
			// Check to see if this is a header page
			if (!ogg_page_bos(&dataPage))
			{
				// Headers are done
				parsingOggHeaders = false;
				break;
			}

			// Set up a test stream to identify the codec
			ogg_stream_state codecTest;
			ogg_stream_init(&codecTest, ogg_page_serialno(&dataPage));

			// Send the header data into the stream
			ogg_stream_pagein(&codecTest, &dataPage);

			// Look at the first packet in the stream
			ogg_packet headerPacket;
			int gotPacket = ogg_stream_packetpeek(&codecTest, &headerPacket);

			// Identify the codec
			if ((gotPacket == 1) && !foundTheora && 
				(parsingTheoraHeaders = th_decode_headerin(&theoraInfo, &theoraComments, &theoraSetup, &headerPacket)) >= 0)
			{
				dbglog(DBG_NOTICE, "Found Theora stream\n");
				memcpy(&theoraStream, &codecTest, sizeof(codecTest));
				foundTheora = true;
				
				if (parsingTheoraHeaders)
					ogg_stream_packetout(&theoraStream, NULL);
			}
			else
			{
				// Not a Theora stream, move on
				ogg_stream_clear(&codecTest);
			}
		}
	}

	// Shut things down
	ogg_sync_clear(&syncState);
	fclose(videoFile);
	return 0;
}
