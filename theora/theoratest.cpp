#include <kos.h>

#include <ogg/ogg.h>
#include <plx/matrix.h>
#include <plx/prim.h>
#include <theora/codec.h>
#include <theora/theoradec.h>

#include "convertYUV.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

ogg_stream_state theoraStream;
ogg_stream_state vorbisStream;

void queueDataPage(ogg_page *dataPage)
{
	ogg_stream_pagein(&theoraStream, dataPage);
}

size_t fillSyncBuffer(FILE *inFile, ogg_sync_state *sync, size_t bufferSize = 4096)
{
	char *buffer = ogg_sync_buffer(sync, bufferSize);
	size_t bytesRead = fread(buffer, 1, bufferSize, inFile);
	ogg_sync_wrote(sync, bytesRead);

	return bytesRead;
}

inline int nextPowerOfTwo(int n)
{
	// Special-case negative values
	if (n < 0)
		return 0;
	
	--n;
	
	n |= (n >> 1);
	n |= (n >> 2);
	n |= (n >> 4);
	n |= (n >> 8);
	n |= (n >> 16);
	
	return n + 1;
}

vector_t verts[4] = {
	{ -2.0f,  1.5f, 0.0f, 1.0f },
	{ -2.0f, -1.5f, 0.0f, 1.0f },
	{  2.0f,  1.5f, 0.0f, 1.0f },
	{  2.0f, -1.5f, 0.0f, 1.0f }
};

int main(int argc, char *argv[])
{
	dbglog_set_level(DBG_INFO);
	dbglog(DBG_NOTICE, "%s\n", th_version_string());
	
	pvr_init_defaults();
	plx_mat3d_init();
	
	pvr_set_bg_color(0.2f, 0.2f, 0.2f);
	
	plx_mat3d_mode(PLX_MAT_PROJECTION);
	plx_mat3d_identity();
	plx_mat3d_perspective(60.0f, 640.0f / 480.0f, 0.1f, 100.0f);

	plx_mat3d_mode(PLX_MAT_MODELVIEW);
	plx_mat3d_identity();

	point_t cameraPosition = { 0.0f, 0.0f,  5.0f, 1.0f };
	point_t cameraTarget   = { 0.0f, 0.0f,  0.0f, 1.0f };
	vector_t cameraUp      = { 0.0f, 1.0f,  0.0f, 0.0f };
	plx_mat3d_lookat(&cameraPosition, &cameraTarget, &cameraUp);

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
	th_dec_ctx *theoraDecoder = NULL;
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

	// Send any more pages we have to the appropriate streams
	ogg_page dataPage;
	while (ogg_sync_pageout(&syncState, &dataPage) > 0)
		queueDataPage(&dataPage);

	// For testing, just grab the first frame
	ogg_packet dataPacket;
	ogg_stream_packetout(&theoraStream, &dataPacket);
	th_decode_packetin(theoraDecoder, &dataPacket, NULL);

	th_ycbcr_buffer videoFrame;
	th_decode_ycbcr_out(theoraDecoder, videoFrame);

	// Get frame metadata
	int textureWidth  = nextPowerOfTwo(videoFrame[0].width);
	int textureHeight = nextPowerOfTwo(videoFrame[0].height);
	int texBufferSize = textureWidth * textureHeight * 2;

	// Allocate a memory buffer for the YUV422 data
	pvr_ptr_t textureData = pvr_mem_malloc(texBufferSize);
	unsigned char *yuvData = reinterpret_cast<unsigned char*>(memalign(32, texBufferSize));
	sq_set32(yuvData, 0x00800080, texBufferSize);

	// TODO: Fix this function to consider the texture width
	convertYUV420pTo422(yuvData, videoFrame, textureWidth);
	
	// Copy the YUV422 data to the PVR's memory
	sq_cpy(textureData, yuvData, texBufferSize);
	
	pvr_poly_cxt_t stripContext;
	pvr_poly_cxt_txr(&stripContext, PVR_LIST_OP_POLY, PVR_TXRFMT_YUV422 | PVR_TXRFMT_NONTWIDDLED,
		textureWidth, textureHeight, textureData, PVR_FILTER_NONE);
	stripContext.gen.culling = PVR_CULLING_CW;
	pvr_poly_hdr_t stripHeader;
	pvr_poly_compile(&stripHeader, &stripContext);
	
	plx_mat_identity();
	plx_mat3d_apply_all();
		
	vector_t transformedVerts[4];
	plx_mat_transform(verts, transformedVerts, 4, 16);
	
	float rightLimit = 320.0f / (float)textureWidth;
	float bottomLimit = 240.0f / (float)textureHeight;
	
	bool exitProgram = false;
	while(!exitProgram)
	{
		plx_mat_identity();
		plx_mat3d_apply_all();
		
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_OP_POLY);
		
		pvr_prim(&stripHeader, sizeof(stripHeader));
		plx_vert_ffp(PVR_CMD_VERTEX,     transformedVerts[0].x, transformedVerts[0].y, transformedVerts[0].z, 1, 1, 1, 1, 0, 0);
		plx_vert_ffp(PVR_CMD_VERTEX,     transformedVerts[1].x, transformedVerts[1].y, transformedVerts[1].z, 1, 1, 1, 1, 0, bottomLimit);
		plx_vert_ffp(PVR_CMD_VERTEX,     transformedVerts[2].x, transformedVerts[2].y, transformedVerts[2].z, 1, 1, 1, 1, rightLimit, 0);
		plx_vert_ffp(PVR_CMD_VERTEX_EOL, transformedVerts[3].x, transformedVerts[3].y, transformedVerts[3].z, 1, 1, 1, 1, rightLimit, bottomLimit);
		
		pvr_list_finish();
		pvr_scene_finish();
		
		maple_device_t *controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
		cont_state_t *controllerState = reinterpret_cast<cont_state_t*>(maple_dev_status(controller));
		if (controllerState->buttons & CONT_START)
			exitProgram = true;
	}
	
	// Shut things down
	pvr_mem_free(textureData);
	pvr_shutdown();
	free(yuvData);
	if (foundTheora)
	{
		ogg_stream_clear(&theoraStream);
		th_decode_free(theoraDecoder);
		th_comment_clear(&theoraComments);
		th_info_clear(&theoraInfo);
	}
	ogg_sync_clear(&syncState);
	fclose(videoFile);
	return 0;
}
