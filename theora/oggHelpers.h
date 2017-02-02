#ifndef OGGHELPERS_H
#define OGGHELPERS_H

#include <map>
#include <ogg/ogg.h>
#include <theora/theoradec.h>

typedef std::map<int, ogg_stream_state> OggStreamCollection;

// This function assumes that the sync state's buffer
// already contains enough data to locate all the streams
OggStreamCollection getOggStreams(ogg_sync_state &oggSync);

void queueDataPage(ogg_page *dataPage, OggStreamCollection &streams);

void flushSyncBuffer(ogg_sync_state &oggSync, OggStreamCollection &streams);

bool isTheoraStream(ogg_stream_state &stream);

// This function assumes that the info and comment
// structures have already been initialized.
th_dec_ctx* initializeTheoraDecoder(th_info &info, th_comment &comments, ogg_stream_state &stream);

#endif
