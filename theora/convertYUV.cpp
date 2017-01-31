#include "convertYUV.h"

#include <kos/dbglog.h>

inline int calculateArrayIndex(int x, int y, int width)
{
	return (y * width) + x;
}

void convertYUV420pTo422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer)
{
	const th_img_plane &Y  = inBuffer[0];
	dbglog(DBG_INFO, "Y Plane : %d x %d, %d-byte stride\n", Y.width, Y.height, Y.stride);

	const th_img_plane &Cb = inBuffer[1];
	dbglog(DBG_INFO, "Cb Plane: %d x %d, %d-byte stride\n", Cb.width, Cb.height, Cb.stride);

	const th_img_plane &Cr = inBuffer[2];
	dbglog(DBG_INFO, "Cr Plane: %d x %d, %d-byte stride\n", Cr.width, Cr.height, Cr.stride);

	const int frameWidth  = Y.width;
	const int frameHeight = Y.height;

	const unsigned char *YData  = Y.data;   // Y'
	const unsigned char *CbData = Cb.data;  // U
	const unsigned char *CrData = Cr.data;  // V

	int Yindex = 0;
	for (int y = 0; y < frameHeight; ++y)
	{
		for (int x = 0, uv = 0; x < frameWidth; x += 2, ++uv)
		{
			outBuffer[Yindex + 0] = YData[x];
			outBuffer[Yindex + 1] = CbData[uv];
			outBuffer[Yindex + 2] = YData[x + 1];
			outBuffer[Yindex + 3] = CrData[uv];

			Yindex += 4;
		}

		YData += Y.stride;

		if (y % 2 == 1)
		{
			CbData += Cb.stride;
			CrData += Cr.stride;
		}
	}
}

void convertYUV422pTo422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer)
{
	const th_img_plane &Y  = inBuffer[0];
	dbglog(DBG_INFO, "Y Plane : %d x %d, %d-byte stride\n", Y.width, Y.height, Y.stride);

	const th_img_plane &Cb = inBuffer[1];
	dbglog(DBG_INFO, "Cb Plane: %d x %d, %d-byte stride\n", Cb.width, Cb.height, Cb.stride);

	const th_img_plane &Cr = inBuffer[2];
	dbglog(DBG_INFO, "Cr Plane: %d x %d, %d-byte stride\n", Cr.width, Cr.height, Cr.stride);

	const int frameWidth  = Y.width;
	const int frameHeight = Y.height;

	const unsigned char *YData  = Y.data;   // Y'
	const unsigned char *CbData = Cb.data;  // U
	const unsigned char *CrData = Cr.data;  // V

	int Yindex = 0;
	for (int y = 0; y < frameHeight; ++y)
	{
		for (int x = 0, uv = 0; x < frameWidth; x += 2, ++uv)
		{
			outBuffer[Yindex + 0] = YData[x];
			outBuffer[Yindex + 1] = CbData[uv];
			outBuffer[Yindex + 2] = YData[x + 1];
			outBuffer[Yindex + 3] = CrData[uv];

			Yindex += 4;
		}

		YData += Y.stride;
		CbData += Cb.stride;
		CrData += Cr.stride;
	}
}

void convertYUV444pTo422(unsigned char *outBuffer, const th_ycbcr_buffer &inBuffer)
{
	const th_img_plane &Y  = inBuffer[0];
	dbglog(DBG_INFO, "Y Plane : %d x %d, %d-byte stride\n", Y.width, Y.height, Y.stride);

	const th_img_plane &Cb = inBuffer[1];
	dbglog(DBG_INFO, "Cb Plane: %d x %d, %d-byte stride\n", Cb.width, Cb.height, Cb.stride);

	const th_img_plane &Cr = inBuffer[2];
	dbglog(DBG_INFO, "Cr Plane: %d x %d, %d-byte stride\n", Cr.width, Cr.height, Cr.stride);

	const int frameWidth  = Y.width;
	const int frameHeight = Y.height;

	const unsigned char *YData  = Y.data;   // Y'
	const unsigned char *CbData = Cb.data;  // U
	const unsigned char *CrData = Cr.data;  // V

	int Yindex = 0;
	for (int y = 0; y < frameHeight; ++y)
	{
		for (int x = 0, uv = 0; x < frameWidth; x += 2, uv += 2)
		{
			outBuffer[Yindex + 0] = YData[x];
			outBuffer[Yindex + 1] = (CbData[uv] + CbData[uv + 1]) / 2;
			outBuffer[Yindex + 2] = YData[x + 1];
			outBuffer[Yindex + 3] = (CrData[uv] + CrData[uv + 1]) / 2;

			Yindex += 4;
		}

		YData += Y.stride;
		CbData += Cb.stride;
		CrData += Cr.stride;
	}
}
