extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#include <cstdio>
#include <cstring>
#include <cinttypes>

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
        return 1;
    }
    if(avformat_find_stream_info(fmt,NULL)<0){
        std::printf("cannot find stream info %s\n",filename);
        avformat_close_input(&fmt);
        return 1;
    }
    std::printf("Format %s, duration %.6fs\n",
                fmt->iformat->long_name, fmt->duration/1000000.0);

    avformat_close_input(&fmt);
    return 0;
}