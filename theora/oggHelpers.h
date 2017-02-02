#ifndef OGGHELPERS_H
#define OGGHELPERS_H

#include <map>
#include <ogg/ogg.h>

typedef std::map<int, ogg_stream_state> OggStreams;

// This function assumes that the sync state's buffer
// already contains enough data to locate all the streams
OggStreams getOggStreams(ogg_sync_state &oggSync);

void queueDataPage(ogg_page *dataPage, OggStreams &streams);

void flushSyncBuffer(ogg_sync_state &oggSync, OggStreams &streams);

#endif
