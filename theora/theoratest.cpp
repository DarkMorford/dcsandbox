#include <kos.h>

#include <ogg/ogg.h>
#include <theora/codec.h>
#include <theora/theoradec.h>

KOS_INIT_FLAGS(INIT_DEFAULT);

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

const size_t BUFFER_SIZE = 4096;

int main(int argc, char *argv[])
{
	dbglog_set_level(DBG_NOTICE);
	dbglog(DBG_NOTICE, "%s\n", th_version_string());

	// Initialize Ogg sync layer
	ogg_sync_state oggSync;
	ogg_sync_init(&oggSync);

	// Set up Theora data structures
	th_comment theoraComment;
	th_comment_init(&theoraComment);

	th_info theoraInfo;
	th_info_init(&theoraInfo);

	// Open the video file
	FILE *videoFile = fopen("/rd/320x240.ogg", "rb");

	// Load up the buffer with data
	char *dataBuffer = ogg_sync_buffer(&oggSync, BUFFER_SIZE);
	size_t bytesRead = fread(dataBuffer, 1, BUFFER_SIZE, videoFile);
	ogg_sync_wrote(&oggSync, bytesRead);
	
	// Get the first data page
	ogg_page oggSyncPage;
	ogg_sync_pageout(&oggSync, &oggSyncPage);

	// Shut things down
	ogg_sync_clear(&oggSync);
	fclose(videoFile);
	return 0;
}
