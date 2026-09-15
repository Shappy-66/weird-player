extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}
#include <cstdio>
#include <cstring>
#include <cinttypes>

static AVCodecContext *video_dec_ctx;
static AVStream *video_stream=NULL;
static int video_stream_idx = -1;
static FILE *video_file;
static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int width,height;
static enum AVPixelFormat pix_fmt;
static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret,stream_index;
    AVStream *st;
    const AVCodec *dec=nullptr;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    if(ret < 0)
    {
        std::printf("find video stream failed ");
        avformat_close_input(&fmt_ctx);
    }else{
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if(!dec){
        printf("find decoder failed");
    }
    *dec_ctx = avcodec_alloc_context3(dec);
    if(!*dec_ctx){
        printf("alloc decoder context failed");
    }
    ret =avcodec_parameters_to_context(*dec_ctx, st->codecpar);
    if(ret < 0){
        printf("copy decoder parameters to context failed");
    }
    if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
        printf("Failed to open  codec\n");
        }
        *stream_idx = stream_index;
    }
 
    return 0;
}
int main(int argc, char *argv[]) {
    int ret = 0;
    const char *filename, *outfilename;
     if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n"
                "And check your input file is encoded by mpeg1video please.\n", argv[0]);
        exit(0);
    }
    filename = argv[1];
    outfilename = argv[2];
    AVFormatContext *fmt = avformat_alloc_context();
    if (avformat_open_input(&fmt, filename, nullptr, nullptr) < 0) {
        std::printf("cannot open %s\n", filename);
        ret = 1;
        goto end;
    }
    if(avformat_find_stream_info(fmt,NULL)<0){
        std::printf("cannot find stream info %s\n",filename);
        ret = 1;
        goto end;
    }
    std::printf("Format %s, duration %.6fs\n",
                fmt->iformat->long_name, fmt->duration/1000000.0);

    if (open_codec_context(&video_stream_idx,&video_dec_ctx,fmt,AVMEDIA_TYPE_VIDEO)>=0)
        video_stream = fmt->streams[video_stream_idx];
    video_file = fopen(filename,"wb");
    if (!video_file) {
             printf("Could not open destination file %s\n", filename);
             ret = 1;
             goto end;
        }
    width = video_dec_ctx->width;
    height = video_dec_ctx->height;
    pix_fmt = video_dec_ctx->pix_fmt;
    if(av_image_alloc(video_dst_data, video_dst_linesize, width, height, pix_fmt, 1)<0){
        printf("alloc image failed");
        ret = 1;
        goto end;
    }

    end:
    avcodec_free_context(&video_dec_ctx);
    avformat_close_input(&fmt);
    return 0;
}