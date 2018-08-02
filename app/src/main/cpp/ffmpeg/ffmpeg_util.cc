#include "ffmpeg_util.h"
#include "android/log.h"
#include "android_log.h"

//ffmpeg header file
extern "C" {
#include  "libavcodec/avcodec.h"
#include  "libavformat/avformat.h"
#include  "libavfilter/avfiltergraph.h"
#include  "libavfilter/buffersink.h"
#include  "libavfilter/buffersrc.h"
#include  "libswresample/swresample.h"
#include  "libavutil/opt.h"
#include  "libavutil/time.h"
#include  "libavutil/mathematics.h"
#include  "jpeglib.h"
}

int MT_VIDEO_ROTATE_90 = 90;
int MT_VIDEO_ROTATE_180 = 180;
int MT_VIDEO_ROTATE_270 = 270;
int MT_VIDEO_ROTATE_0 = 0;

static int combine_video(const std::vector<std::string> &inputFileList, const char *outputFile) {
    if (inputFileList.size() <= 0)
        return 0;
    std::string inputFile = inputFileList[0];
    //注册组件
    av_register_all();
    avcodec_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVFormatContext *pFormatCtx2 = avformat_alloc_context();
    //video parm
    int64_t dts, pts;
    int64_t dts2, pts2;
    //audio parm
    int64_t adts, apts;
    int64_t adts2, apts2;

    //打开视频文件
    if (avformat_open_input(&pFormatCtx, inputFile.c_str(), NULL, NULL) != 0) {
        printf("%s, path:%s\n", "无法打开视频文件", inputFile.c_str());
        return -1;
    }

    //获取输入文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("%s", "无法获取输入文件信息\n");
        return -2;
    }

    //获取流索引位置
    int i = 0, video_stream_idx = -1, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
        } else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
        }
    }

    AVFormatContext *pOutFormatContext = avformat_alloc_context();
    avformat_alloc_output_context2(&pOutFormatContext,
                                   NULL, NULL, outputFile);
    if (pOutFormatContext == NULL) {
        printf("Could not allocate output context");
        return -3;
    }

    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        AVStream *inAVStream = pFormatCtx->streams[i];
        AVStream *outAVStream = avformat_new_stream(pOutFormatContext, inAVStream->codec->codec);
        if (avcodec_copy_context(outAVStream->codec, inAVStream->codec) < 0) {
            printf("Failed to copy codec context");
            return -4;
        }

        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ||
            pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            outAVStream->time_base = inAVStream->time_base;
            outAVStream->r_frame_rate = inAVStream->r_frame_rate;
        }

        outAVStream->codec->codec_tag = 0;
        if (pOutFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
            outAVStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    avio_open(&pOutFormatContext->pb, outputFile, AVIO_FLAG_WRITE);
    if (pOutFormatContext->pb == NULL) {
        printf("Could not open for writing");
        return -6;
    }

    if (avformat_write_header(pOutFormatContext, NULL) < 0) {
        printf("Could not write header");
        return -7;
    }

    //压缩数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            dts = packet->dts + packet->duration;
            pts = packet->pts + packet->duration;
            av_interleaved_write_frame(pOutFormatContext, packet);
        } else if (packet->stream_index == audio_stream_idx) {
            //计算延时后，重新指定时间戳
            packet->pts = av_rescale_q_rnd(packet->pts,
                                           pFormatCtx->streams[audio_stream_idx]->time_base,
                                           pOutFormatContext->streams[audio_stream_idx]->time_base,
                                           AV_ROUND_INF);
            packet->dts = packet->pts;
            packet->duration = av_rescale_q_rnd(packet->duration,
                                                pFormatCtx->streams[audio_stream_idx]->time_base,
                                                pOutFormatContext->streams[audio_stream_idx]->time_base,
                                                AV_ROUND_INF);
            //字节流的位置，-1 表示不知道字节流位置
            packet->pos = -1;

            adts = packet->dts + packet->duration;
            apts = packet->pts + packet->duration;
            av_interleaved_write_frame(pOutFormatContext, packet);
        }
    }

    //取出input列表所有视频流
    for (int j = 1; j < inputFileList.size(); ++j) {
        avformat_close_input(&pFormatCtx2);
        avformat_free_context(pFormatCtx2);
        pFormatCtx2 = avformat_alloc_context();
        if (avformat_open_input(&pFormatCtx2, inputFileList[j].c_str(), NULL, NULL) != 0) {
            printf("%s, path:%s\n", "无法打开视频文件", inputFileList[j].c_str());
            return -8;
        }

        //获取输入文件信息
        if (avformat_find_stream_info(pFormatCtx2, NULL) < 0) {
            printf("%s", "无法获取输入文件信息\n");
            return -9;
        }

        i = 0;
        int video_stream_idx2 = -1, audio_stream_idx2 = -1;
        for (; i < pFormatCtx2->nb_streams; i++) {
            if (pFormatCtx2->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_idx2 = i;
            } else if (pFormatCtx2->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_idx2 = i;
            }
        }

        bool bAudioFirstPeriod = true;
        int audioTsOffset = 0;
        while (av_read_frame(pFormatCtx2, packet) >= 0) {
            if (packet->stream_index == video_stream_idx2) {
                packet->dts += dts;
                packet->pts += pts;
                packet->pos = -1;
                dts2 = packet->dts + packet->duration;
                pts2 = packet->pts + packet->duration;
                packet->stream_index = video_stream_idx;
                av_interleaved_write_frame(pOutFormatContext, packet);
            } else if (packet->stream_index == audio_stream_idx2) {
                //计算延时后，重新指定时间戳
                packet->pts = av_rescale_q_rnd(packet->pts,
                                               pFormatCtx->streams[audio_stream_idx]->time_base,
                                               pOutFormatContext->streams[audio_stream_idx]->time_base,
                                               AV_ROUND_INF);
                packet->dts = packet->pts;
                packet->duration = av_rescale_q_rnd(packet->duration,
                                                    pFormatCtx->streams[audio_stream_idx]->time_base,
                                                    pOutFormatContext->streams[audio_stream_idx]->time_base,
                                                    AV_ROUND_INF);
                //字节流的位置，-1 表示不知道字节流位置
                packet->pos = -1;

                if (bAudioFirstPeriod) {
                    bAudioFirstPeriod = false;
                    if (packet->pts < 0)
                        audioTsOffset = -packet->pts;
                    adts += audioTsOffset;
                    apts = adts;
                }
                packet->dts += adts;
                packet->pts += apts;

                adts2 = packet->dts + packet->duration;
                apts2 = packet->pts + packet->duration;
                packet->stream_index = audio_stream_idx;

                av_interleaved_write_frame(pOutFormatContext, packet);
            }
        }
        dts = dts2;
        pts = pts2;
        adts = adts2;
        apts = apts2;
    }

    //写入文件
    av_write_trailer(pOutFormatContext);

    //关闭相关context，回收资源
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
    avformat_close_input(&pFormatCtx2);
    avformat_free_context(pFormatCtx2);
    avformat_close_input(&pOutFormatContext);
    avformat_free_context(pOutFormatContext);

    return 0;
}

static int
WriteJpegFile(const char *jpegFileName, unsigned char *inputData, int nWidth, int nHeight,
              int nChannel, int nQuality, J_COLOR_SPACE colorSpace = JCS_RGB) {
    int fileSize = 0;
    /* This struct contains the JPEG compression parameters and pointers to
    * working space (which is allocated as needed by the JPEG library).
    * It is possible to have several such structures, representing multiple
    * compression/decompression processes, in existence at once.  We refer
    * to any one struct (and its associated working data) as a "JPEG object".
    */
    struct jpeg_compress_struct cinfo;

    /* This struct represents a JPEG error handler.  It is declared separately
    * because applications often want to supply a specialized error handler
    * (see the second half of this file for an example).  But here we just
    * take the easy way out and use the standard error handler, which will
    * print a message on stderr and call exit() if compression fails.
    * Note that this struct must live as long as the main JPEG parameter
    * struct, to avoid dangling-pointer problems.
    */
    struct jpeg_error_mgr jerr;

    /* More stuff */
    FILE *outfile;                  /* target file */
    JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
    int row_stride;             /* physical row width in image buffer */

    /* Step 1: allocate and initialize JPEG compression object */

    /* We have to set up the error handler first, in case the initialization
    * step fails.  (Unlikely, but it could happen if you are out of memory.)
    * This routine fills in the contents of struct jerr, and returns jerr's
    * address which we place into the link field in cinfo.
    */
    cinfo.err = jpeg_std_error(&jerr);

    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);  /* Now we can initialize the JPEG compression object. */

    /* Step 2: specify data destination (eg, a file) */
    /* Note: steps 2 and 3 can be done in either order. */

    /* Here we use the library-supplied code to send compressed data to a
    * stdio stream.  You can also write your own code to do something else.
    * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
    * requires it in order to write binary files.
    */
    if ((outfile = fopen(jpegFileName, "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", jpegFileName);
        return -1;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = nWidth;                /* image width and height, in pixels */
    cinfo.image_height = nHeight;
    cinfo.input_components = nChannel;         /* # of color components per pixel */

    if (nChannel == 1) {
        cinfo.in_color_space = JCS_GRAYSCALE;  /* colorspace of input image */
    } else if (nChannel == 3) {
        cinfo.in_color_space = colorSpace;        /* colorspace of input image */
    }

    /* Now use the library's routine to set default compression parameters.
    * (You must set at least cinfo.in_color_space before calling this,
    * since the defaults depend on the source color space.)
    */
    jpeg_set_defaults(&cinfo);

    //jpeg_set_colorspace(&cinfo, cinfo.in_color_space);

    // Now you can set any non-default parameters you wish to.
    // Here we just illustrate the use of quality (quantization table) scaling:
    jpeg_set_quality(&cinfo, nQuality, TRUE); /* limit to baseline-JPEG values */

    /* Step 4: Start compressor */

    /* TRUE ensures that we will write a complete interchange-JPEG file.
    * Pass TRUE unless you are very sure of what you're doing.
    */
    jpeg_start_compress(&cinfo, TRUE);

    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */

    /* Here we use the library's state variable cinfo.next_scanline as the
    * loop counter, so that we don't have to keep track ourselves.
    * To keep things simple, we pass one scanline per call; you can pass
    * more if you wish, though.
    */
    row_stride = nWidth * nChannel; /* JSAMPLEs per row in image_buffer */

    while (cinfo.next_scanline < cinfo.image_height) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
        * Here the array is only one element long, but you could pass
        * more than one scanline at a time if that's more convenient.
        */
        row_pointer[0] = &inputData[cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Step 6: Finish compression */
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    /* After finish_compress, we can close the output file. */
    fileSize = ftell(outfile);
    fclose(outfile);
    return fileSize;
}

/*参数为：
 *返回图片的宽度(w_Dest),
 *返回图片的高度(h_Dest),
 *返回图片的位深(bit_depth),
 *源图片的RGB数据(src),
 *源图片的宽度(w_Src),
 *源图片的高度(h_Src)
 */
unsigned char *
do_Stretch_Linear(int w_Dest, int h_Dest, int bit_depth, unsigned char *src, int w_Src, int h_Src) {
    int sw = w_Src - 1, sh = h_Src - 1, dw = w_Dest - 1, dh = h_Dest - 1;
    int B, N, x, y;
    int nPixelSize = bit_depth / 8;
    unsigned char *pLinePrev, *pLineNext;
    unsigned char *pDest = new unsigned char[w_Dest * h_Dest * bit_depth / 8];
    unsigned char *tmp;
    unsigned char *pA, *pB, *pC, *pD;

    for (int i = 0; i <= dh; ++i) {
        tmp = pDest + i * w_Dest * nPixelSize;
        y = i * sh / dh;
        N = dh - i * sh % dh;
        pLinePrev = src + (y++) * w_Src * nPixelSize;
        //pLinePrev =(unsigned char *)aSrc->m_bitBuf+((y++)*aSrc->m_width*nPixelSize);
        pLineNext = (N == dh) ? pLinePrev : src + y * w_Src * nPixelSize;
        //pLineNext = ( N == dh ) ? pLinePrev : (unsigned char *)aSrc->m_bitBuf+(y*aSrc->m_width*nPixelSize);
        for (int j = 0; j <= dw; ++j) {
            x = j * sw / dw * nPixelSize;
            B = dw - j * sw % dw;
            pA = pLinePrev + x;
            pB = pA + nPixelSize;
            pC = pLineNext + x;
            pD = pC + nPixelSize;
            if (B == dw) {
                pB = pA;
                pD = pC;
            }

            for (int k = 0; k < nPixelSize; ++k) {
                *tmp++ = (unsigned char) (int) (
                        (B * N * (*pA++ - *pB - *pC + *pD) + dw * N * *pB++
                         + dh * B * *pC++ + (dw * dh - dh * B - dw * N) * *pD++
                         + dw * dh / 2) / (dw * dh));
            }
        }
    }
    return pDest;
}

/**
 * 将AVFrame(YUV420格式)保存为JPEG格式的图片
 *
 * @param width YUV420的宽
 * @param height YUV420的高
 *
 */
static unsigned char *
FrameWriteInToJPEG(AVFrame *pFrame, int width, int height, int pad_size, jint *inputInt32,
                   bool debug
) {

    uint8_t *rgb = new uint8_t[width * height * 3];
    uint8_t *data_Y = pFrame->data[0]; //width * height
    uint8_t *data_U = pFrame->data[1]; //width * height / 4
    uint8_t *data_V = pFrame->data[2]; //width * height / 4

    const int B = 0;
    const int G = 1;
    const int R = 2;
    int tmp_b, tmp_g, tmp_r;

    int new_width = width - pad_size;
    int new_height = height;

    for (int i = 0; i < new_height; ++i) {
        for (int j = 0; j < new_width; ++j) {
            int Y = i * width + j;
            int V = i / 2 * width / 2 + j / 2;
            int U = i / 2 * width / 2 + j / 2;
            int index = 3 * (i * new_width + j);
            tmp_b = data_Y[Y] + 1.403 * (data_V[V] - 128);
            tmp_g = data_Y[Y] - 0.343 * (data_U[U] - 128) - 0.714 * (data_V[V] - 128);
            tmp_r = data_Y[Y] + 1.770 * (data_U[U] - 128);

            if (tmp_b > 255)
                tmp_b = 255;
            else if (tmp_b < 0)
                tmp_b = 0;
            if (tmp_g > 255)
                tmp_g = 255;
            else if (tmp_g < 0)
                tmp_g = 0;
            if (tmp_r > 255)
                tmp_r = 255;
            else if (tmp_r < 0)
                tmp_r = 0;

            rgb[index + B] = tmp_b;
            rgb[index + G] = tmp_g;
            rgb[index + R] = tmp_r;
        }
    }


    if (debug) {
        char *outFile = "/sdcard/xianyu/jni_out.jpeg";
        WriteJpegFile(outFile, rgb, new_width, new_height, 3, 100);
    }
    return rgb;
}

int getRotateAngle(AVStream *avStream) {
    AVDictionaryEntry *tag = NULL;
    int m_Rotate = -1;
    tag = av_dict_get(avStream->metadata, "rotate", tag, 0);
    if (tag == NULL) {
        m_Rotate = 0;
    } else {
        int angle = atoi(tag->value);
        angle %= 360;
        if (angle == 90) {
            m_Rotate = MT_VIDEO_ROTATE_90;
        } else if (angle == 180) {
            m_Rotate = MT_VIDEO_ROTATE_180;
        } else if (angle == 270) {
            m_Rotate = MT_VIDEO_ROTATE_270;
        } else {
            m_Rotate = MT_VIDEO_ROTATE_0;
        }
    }

//    AVDictionary *d = avStream->metadata;
//    AVDictionaryEntry *t = NULL;
//    while (t = av_dict_get(d, "", t, AV_DICT_IGNORE_SUFFIX)) {
//        LOGD("key=%s,value=%s", t->key, t->value);
//    }
////    av_dict_free(&d);

    return m_Rotate;
}

int getVideoRotate(const char *inputFilePath) {
    //注册组件
    av_register_all();
    avcodec_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //打开视频文件
    if (avformat_open_input(&pFormatCtx, inputFilePath, NULL, NULL) != 0) {
        printf("%s, path:%s\n", "无法打开视频文件", inputFilePath);
        return -1;
    }

    //获取输入文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("%s", "无法获取输入文件信息\n");
        return -2;
    }

    //获取视频索引位置
    int i = 0, video_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    int result = getRotateAngle(pFormatCtx->streams[video_stream_idx]);

    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    return result;
}


int extractFrame2(const char *inputFilePath, const int startTimeMs, jint *outputJints, jint dstW,
                  jint dstH, bool debug) {

    //注册组件
    av_register_all();
    avcodec_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //打开视频文件
    if (avformat_open_input(&pFormatCtx, inputFilePath, NULL, NULL) != 0) {
        printf("%s, path:%s\n", "无法打开视频文件", inputFilePath);
        return -1;
    }

    //获取输入文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("%s", "无法获取输入文件信息\n");
        return -2;
    }

    //获取视频索引位置
    int i = 0, video_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    //获取解码器
    AVCodecContext *codecCtx = pFormatCtx->streams[video_stream_idx]->codec;
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    if (codec == NULL) {
        printf("%s", "无法获取解码器\n");
        return -3;
    }

    //打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        printf("%s", "无法打开解码器\n");
        return -4;
    }

    //压缩数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    //解压缩数据
    AVFrame *frame = av_frame_alloc();

    int got_frame = 0, index = 0, ret = 0, keyFrameNum = 0;
    int timeBase = 1.0 * pFormatCtx->streams[video_stream_idx]->time_base.den /
                   pFormatCtx->streams[video_stream_idx]->time_base.num;
    int timeStart =
            1.0 * pFormatCtx->start_time * pFormatCtx->streams[video_stream_idx]->time_base.den /
            pFormatCtx->streams[video_stream_idx]->time_base.num;

    if (timeStart < 0)
        timeStart = 0;

    bool bFoundKeyFrame = false;
    bool bFoundFrame = false;
    int bestDelta = 1000000000;

    time_t t_start_found_frame, t_end_found_frame;
    t_start_found_frame = clock();
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index != video_stream_idx)
            continue;
        //解码
        time_t time_decode_start, time_decode_end;
        time_decode_start = clock();
        ret = avcodec_decode_video2(codecCtx, frame, &got_frame, packet);
        time_decode_end = clock();
        LOGE("time:解码一帧所用时间 %.0f ms\n",
             (float) (time_decode_end - time_decode_start) * 1000 / CLOCKS_PER_SEC);
        if (ret < 0) {
            //printf("%s\n", "解码完成");
            break;
        }

        //解码一帧成功
        if (got_frame > 0) {
            if (bFoundFrame) {
                t_end_found_frame = clock();
                LOGE("time:找到匹配帧的时间 %.0f ms\n",
                     (float) (t_end_found_frame - t_start_found_frame) * 1000 / CLOCKS_PER_SEC);
                time_t t_start, t_end;
                t_start = clock();
                unsigned char *outRgb = FrameWriteInToJPEG(frame, frame->linesize[0], frame->height,
                                                           frame->linesize[0] - frame->width,
                                                           outputJints, debug);
                t_end = clock();
                LOGE("time:完成完整帧解析的时间 %.0f ms\n",
                     (float) (t_end - t_start) * 1000 / CLOCKS_PER_SEC);

                t_start = clock();
//                LOGE("width=%d,height=%d,pad_size=%d",frame->linesize[0],frame->height,frame->linesize[0] - frame->width);

                unsigned char *scaleRgb = do_Stretch_Linear(dstW, dstH, 24, outRgb,
                                                            frame->width, frame->height);
                t_end = clock();
                LOGE("time:缩放帧使用的时间 %.0f ms\n", (float) (t_end - t_start) * 1000 / CLOCKS_PER_SEC);

                t_start = clock();
                jint len = dstW * dstH;
                int p = 0;
                int r, g, b;
                for (int i = 0; i < len; i++) {
                    p = 0;

                    r = scaleRgb[i * 3];
                    g = scaleRgb[i * 3 + 1];
                    b = scaleRgb[i * 3 + 2];
//
                    p = p | (255 << 24);
                    p = p | (r << 16);
                    p = p | (g << 8);
                    p = p | b;

                    outputJints[i] = p;
                }

                if (debug) {
                    char *outFile = "/sdcard/xianyu/jni_out_scale.jpeg";
                    WriteJpegFile(outFile, scaleRgb, dstW, dstH, 3, 100);
                }
                t_end = clock();
                LOGE("time:位移合并成为像素的时间 %.0f ms\n",
                     (float) (t_end - t_start) * 1000 / CLOCKS_PER_SEC);

                break;
            }

            //找非关键帧，如果找到则继续找非关键帧
            if (bFoundKeyFrame) {
                //TODO
                int tmpDelta = abs(frame->pkt_pts - startTimeMs * timeBase / 1000l + timeStart);
                if (bestDelta > tmpDelta)
                    bestDelta = tmpDelta;
                else if (bestDelta < tmpDelta) {
                    bFoundFrame = true;
                }
            } else {
                ret = av_seek_frame(pFormatCtx, video_stream_idx,
                                    startTimeMs * timeBase / 1000l + timeStart,
                                    AVSEEK_FLAG_BACKWARD);
                if (ret >= 0) {
                    bFoundKeyFrame = true;
                }
            }
        }

        av_free_packet(packet);
    }

    av_frame_free(&frame);
    avcodec_close(codecCtx);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    return 0;
}