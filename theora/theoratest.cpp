#include <kos.h>

#include <plx/matrix.h>
#include <plx/prim.h>

#include "convertYUV.h"
#include "oggHelpers.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

int theoraSerial = 0;

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

	// Set up PVR for video display
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

	plx_mat_identity();
	plx_mat3d_apply_all();

	vector_t transformedVerts[4];
	plx_mat_transform(verts, transformedVerts, 4, 16);

	// Open the video file
	FILE *videoFile = fopen("/rd/320x240.ogg", "rb");

	// Initialize Ogg sync layer
	ogg_sync_state syncState;
	ogg_sync_init(&syncState);

	fillSyncBuffer(videoFile, syncState);

	OggStreamCollection streams = getOggStreams(syncState);
	dbglog(DBG_INFO, "Collection contains %d stream(s)\n", streams.size());

	flushSyncBuffer(syncState, streams);

	for (OggStreamCollection::iterator it = streams.begin(); it != streams.end(); ++it)
	{
		if (isTheoraStream(it->second))
			theoraSerial = it->first;
	}

	if (theoraSerial != 0)
	{
		dbglog(DBG_INFO, "Found Theora stream with serial number %d\n", theoraSerial);
	}
	else
	{
		dbglog(DBG_WARNING, "No Theora stream found\n");
		return -1;
	}

	// Initialize Theora data structures
	th_info theoraInfo;
	th_info_init(&theoraInfo);

	th_comment theoraComments;
	th_comment_init(&theoraComments);

	ogg_stream_state &theoraStream = streams[theoraSerial];
	th_dec_ctx *theoraDecoder = initializeTheoraDecoder(theoraInfo, theoraComments, theoraStream);

	if (theoraDecoder == NULL)
		dbglog(DBG_ERROR, "Error initializing Theora decoder for stream %d\n", theoraSerial);

	// Get frame metadata
	int textureWidth  = nextPowerOfTwo(theoraInfo.frame_width);
	int textureHeight = nextPowerOfTwo(theoraInfo.frame_height);
	int texBufferSize = textureWidth * textureHeight * 2;

	// Allocate a memory buffer for the YUV422 data
	pvr_ptr_t textureData = pvr_mem_malloc(texBufferSize);
	unsigned char *yuvData = reinterpret_cast<unsigned char*>(memalign(32, texBufferSize));
	sq_set32(yuvData, 0x00800080, texBufferSize);

	pvr_poly_cxt_t stripContext;
	pvr_poly_hdr_t stripHeader;
	pvr_poly_cxt_txr(&stripContext, PVR_LIST_OP_POLY, PVR_TXRFMT_YUV422 | PVR_TXRFMT_NONTWIDDLED,
		textureWidth, textureHeight, textureData, PVR_FILTER_NONE);
	stripContext.gen.culling = PVR_CULLING_CW;
	pvr_poly_compile(&stripHeader, &stripContext);

	// For testing, just grab the first frame
	ogg_packet dataPacket;
	th_ycbcr_buffer videoFrame;

	float rightLimit  = theoraInfo.frame_width  / (float)textureWidth;
	float bottomLimit = theoraInfo.frame_height / (float)textureHeight;

	bool exitProgram = false;
	while(!exitProgram && !feof(videoFile))
	{
		while (ogg_stream_packetout(&theoraStream, &dataPacket) != 1)
		{
			fillSyncBuffer(videoFile, syncState);
			flushSyncBuffer(syncState, streams);
		}

		th_decode_packetin(theoraDecoder, &dataPacket, NULL);
		th_decode_ycbcr_out(theoraDecoder, videoFrame);
		
		// Clear out any other streams
		for (OggStreamCollection::iterator it = streams.begin(); it != streams.end(); ++it)
		{
			if (it->first != theoraSerial)
				flushOggStream(it->second);
		}

		// Copy the YUV422 data to the PVR's memory
		convertYUV420pTo422(yuvData, videoFrame, textureWidth);
//		sq_cpy(textureData, yuvData, texBufferSize);
		pvr_txr_load_dma(yuvData, textureData, texBufferSize, 1, NULL, NULL);

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

	th_decode_free(theoraDecoder);
	th_comment_clear(&theoraComments);
	th_info_clear(&theoraInfo);
	ogg_stream_clear(&theoraStream);

	ogg_sync_clear(&syncState);
	fclose(videoFile);
	return 0;
}
