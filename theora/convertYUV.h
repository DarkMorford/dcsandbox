#ifndef CONVERT_YUV_H
#define CONVERT_YUV_H

#include <theora/codec.h>

void convertYUV420pTo422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer, int textureWidth);
void convertYUV422pTo422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer);
void convertYUV444pTo422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer);

void convertYUV420pTo422(unsigned char *outBuffer, const unsigned char *inBuffer, int width, int height);

#endif
