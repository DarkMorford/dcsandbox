#ifndef CONVERT_YUV_H
#define CONVERT_YUV_H

#include <theora/codec.h>

void convertYUV420to422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer);

#endif
