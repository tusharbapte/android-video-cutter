#include <jni.h>
#include "include/net_avc_video_cutter_natives_Natives.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"


void log_message(char* message) {
	FILE *pFile;
	char szFilename[32];
	int y;

	// Open file
	sprintf(szFilename, "/sdcard/log.txt");
	pFile = fopen(szFilename, "a");
	if (pFile == NULL)
		return;

	// Write header
	fprintf(pFile, message);

	// Close file
	fclose(pFile);
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int y;

	log_message("Saving Frame\n");

	// Open file
	sprintf(szFilename, "/sdcard/frame%d.bmp", iFrame);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y < height; y++)
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}

JNIEXPORT jint JNICALL Java_net_avc_video_cutter_natives_Natives_takePics(
		JNIEnv *env, jclass someclass) {
	av_register_all();
	AVFormatContext *pFormatCtx;

	log_message("I am in Main...Just started!!\n");

	char* filename = "/sdcard/video-2010-12-09-19-17-44.3gp";
	log_message("After File name");
	// Open video file
	if (av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL) != 0)
		return -1; // Couldn't open file
//	 Retrieve stream information
	if (av_find_stream_info(pFormatCtx) < 0)
		return -1; // Couldn't find stream information
	int i;
	AVCodecContext *pCodecCtx;

	// Find the first video stream
	int videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	// Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	AVCodec *pCodec;

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		log_message("Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Open codec
	if (avcodec_open(pCodecCtx, pCodec) < 0)
		return -1; // Could not open codec

	AVFrame *pFrame;

	// Allocate video frame
	pFrame = avcodec_alloc_frame();

	int frameFinished;
	AVPacket packet;

	i = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,
					packet.data, packet.size);

			// Did we get a video frame?
			if (frameFinished) {

				// Save the frame to disk
				if (++i <= 5)
					SaveFrame(pFrame, pCodecCtx->width, pCodecCtx->height, i);
			}
		}
	}
	// Free the packet that was allocated by av_read_frame
	av_free_packet(&packet);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	av_close_input_file(pFormatCtx);
	return 0;

}
