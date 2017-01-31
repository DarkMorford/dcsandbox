#include <kos.h>

#include <ogg/ogg.h>
#include <theora/codec.h>
#include <theora/theoradec.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

ogg_stream_state theoraStream;
ogg_stream_state vorbisStream;

void queueDataPage(ogg_page *dataPage)
{
	ogg_stream_pagein(&theoraStream, dataPage);
}

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

	// Parse the Ogg headers and find a Theora stream
	bool parsingOggHeaders = true;
	int parsingTheoraHeaders = 0;
	bool foundTheora = false;
	while (parsingOggHeaders)
	{
		// Load up the buffer with data
		fillSyncBuffer(videoFile, &syncState);

		// Get the next data page
		ogg_page dataPage;
		while (ogg_sync_pageout(&syncState, &dataPage) > 0)
		{
			// Check to see if this is a header page
			if (!ogg_page_bos(&dataPage))
			{
				// Put the page into its stream
				queueDataPage(&dataPage);

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

				// Already processed this, so just dump it
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

	// Read the Theora headers
	while (foundTheora && parsingTheoraHeaders)
	{
		int gotPacket;
		ogg_packet dataPacket;
		while (parsingTheoraHeaders && (gotPacket = ogg_stream_packetpeek(&theoraStream, &dataPacket)))
		{
			if (gotPacket < 0) continue;
			parsingTheoraHeaders = th_decode_headerin(&theoraInfo, &theoraComments, &theoraSetup, &dataPacket);
			if (parsingTheoraHeaders > 0)
				// Dump the header packet
				ogg_stream_packetout(&theoraStream, NULL);
		}

		ogg_page dataPage;
		if (ogg_sync_pageout(&syncState, &dataPage) > 0)
		{
			queueDataPage(&dataPage);
		}
		else
		{
			fillSyncBuffer(videoFile, &syncState);
		}
	}

	// Initialize the Theora decoder
	th_dec_ctx *theoraDecoder;
	if (foundTheora)
	{
		theoraDecoder = th_decode_alloc(&theoraInfo, theoraSetup);
		dbglog(DBG_NOTICE, "Video is %lu x %lu, %lu / %lu fps\n", theoraInfo.frame_width, theoraInfo.frame_height,
			theoraInfo.fps_numerator, theoraInfo.fps_denominator);
	}
	else
	{
		th_info_clear(&theoraInfo);
		th_comment_clear(&theoraComments);
	}
	th_setup_free(theoraSetup);

	// Shut things down
	ogg_sync_clear(&syncState);
	fclose(videoFile);
	return 0;
}
