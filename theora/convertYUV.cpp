#include "convertYUV.h"

#include <kos/dbglog.h>

inline int calculateArrayIndex(int x, int y, int width)
{
	return (y * width) + x;
}

void convertYUV420to422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer)
{
	const th_img_plane &Y  = inBuffer[0];
	dbglog(DBG_INFO, "Y Plane : %d x %d, %d-byte stride\n", Y.width, Y.height, Y.stride);
	
	const th_img_plane &Cb = inBuffer[1];
	dbglog(DBG_INFO, "Cb Plane: %d x %d, %d-byte stride\n", Cb.width, Cb.height, Cb.stride);
	
	const th_img_plane &Cr = inBuffer[2];
	dbglog(DBG_INFO, "Cr Plane: %d x %d, %d-byte stride\n", Cr.width, Cr.height, Cr.stride);
	
	const int frameWidth  = Y.width;
	const int frameHeight = Y.height;
	
	const unsigned char *YData  = Y.data;
	const unsigned char *CbData = Cb.data;
	const unsigned char *CrData = Cr.data;
	
	for (int y = 0; y < frameHeight; ++y)
	{
		for (int x = 0; x < frameWidth; ++x)
		{
			int Yindex = calculateArrayIndex(x, y, frameWidth) * 2;
			outBuffer[Yindex] = YData[x];
			outBuffer[Yindex + 1] = 0;
		}
		
		YData += Y.stride;
		
		if (y % 2 == 1)
		{
			CbData += Cb.stride;
			CrData += Cr.stride;
		}
	}
}
