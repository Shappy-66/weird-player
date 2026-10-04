extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cinttypes>

static AVCodecContext *video_dec_ctx;
static AVStream       *video_stream=NULL;
static int             video_stream_idx = -1;
static FILE           *video_file;

static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int      video_dst_bufsize;
static int      width,height;
static enum AVPixelFormat pix_fmt;

static AVFrame  *frame;
static AVPacket *pkt;
static int       video_frame_count = 0;

static int output_video_frame(AVFrame *frame){
    if (frame->width != width || frame->height != height ||
        frame->format != pix_fmt){
        printf("Error: Width, height and pixel format have to be "
               "constant in a rawvideo file, but the width, height or "
               "pixel format of the input video changed:\n"
               "old: width = %d, height = %d, format = %s\n"
               "new: width = %d, height = %d, format = %s\n",
               width, height, av_get_pix_fmt_name(pix_fmt),
               frame->width, frame->height,
               av_get_pix_fmt_name((AVPixelFormat)frame->format));
               return -1;
        }
    printf("video_frame n%d \n",video_frame_count++);
    av_image_copy2(video_dst_data,video_dst_linesize,frame->data,frame->linesize,pix_fmt,width,height);
    fwrite(video_dst_data[0],1,video_dst_bufsize,video_file);
    return 0;
}
static int decode_packet(AVCodecContext *dec, const AVPacket *pkt){
    int ret;
    ret = avcodec_send_packet(dec,pkt);
    if (ret < 0){
        printf("Error sending a packet for decoding\n");
        return ret;
    }
    while(ret >= 0){
        ret = avcodec_receive_frame(dec,frame);
            if (ret < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    return 0;
            char buf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, buf, sizeof(buf));
            printf("Error during decoding (%s)\n", av_err2str(ret));//这里也不是很懂这个错误处理,ai告诉的
            return ret;
            }
          if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
            ret = output_video_frame(frame);//写个判断,可以在这里写音频
         av_frame_unref(frame);
    }
    return ret;
}
static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret,stream_index;
    AVStream *st;
    const AVCodec *dec=nullptr;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        std::printf("find video stream failed ");
        return ret;
    }else{
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec){
        printf("find decoder failed");
        return AVERROR(EINVAL);
    }
    *dec_ctx = avcodec_alloc_context3(dec);
    if (!*dec_ctx){
        printf("alloc decoder context failed");
        return AVERROR(ENOMEM);
    }
    ret =avcodec_parameters_to_context(*dec_ctx, st->codecpar);
    if (ret < 0){
        printf("copy decoder parameters to context failed");
        return ret;
    }
    if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
        printf("Failed to open  codec\n");
        return ret;
        }
        *stream_idx = stream_index;
    }
 //错误处理这块先按照官方写的,还不会看
    return 0;
}
int main(int argc, char *argv[]) {
    int ret = 0;
    std::cout << "shappy"; 
    const char *filename, *outfilename;
     if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        exit(1);
    }
    filename = argv[1];
    outfilename = argv[2];
    AVFormatContext *fmt =avformat_alloc_context();
    if (avformat_open_input(&fmt, filename, nullptr, nullptr) < 0) {
        std::printf("cannot open %s\n", filename);
        ret = 1;
        goto end;
    }
    if (avformat_find_stream_info(fmt,NULL)<0){
        std::printf("cannot find stream info %s\n",filename);
        ret = 1;
        goto end;
    }
    std::printf("Format %s, duration %.6fs\n",
                fmt->iformat->long_name, fmt->duration/1000000.0);

    ret = open_codec_context(&video_stream_idx,&video_dec_ctx,fmt,AVMEDIA_TYPE_VIDEO);
    if (ret < 0)
        goto end;  
    video_stream = fmt->streams[video_stream_idx];

    video_file = fopen(outfilename,"wb");
    if (!video_file) {
             printf("Could not open destination file %s\n", outfilename);
             ret = 1;
             goto end;
        }
    width = video_dec_ctx->width;
    height = video_dec_ctx->height;
    pix_fmt = video_dec_ctx->pix_fmt;
    ret = av_image_alloc(video_dst_data, video_dst_linesize, width, height, pix_fmt, 1);
    if (ret < 0){
        printf("alloc image failed");
        goto end;
    }
    video_dst_bufsize = ret;
    av_dump_format(fmt, 0,filename, 0);
    if (!video_stream){
        printf("Could not find video stream in the input\n");
    }
    frame = av_frame_alloc();
    if (!frame) {
        printf("Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }
    pkt = av_packet_alloc();
    if (!pkt) { 
        printf("Could not allocate packet\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }
    if (video_stream)
        printf("Demuxing video from file '%s' into '%s'\n", filename, outfilename);
    while ((ret = av_read_frame(fmt, pkt)) >= 0) {
        if (pkt->stream_index == video_stream_idx)
            ret = decode_packet(video_dec_ctx, pkt);
        av_packet_unref(pkt);
        if (ret < 0)
            break;
    }
    if (ret == AVERROR_EOF)         
        ret = decode_packet(video_dec_ctx, nullptr);

    if (ret >= 0)
        printf("Demuxing succeeded. %d frames written.\n", video_frame_count);
end:
    if (video_file) {
        fclose(video_file);
        video_file = nullptr;
    }
    avcodec_free_context(&video_dec_ctx);
    avformat_close_input(&fmt);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_freep(&video_dst_data[0]);

    if (ret < 0) {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, buf, sizeof(buf)); 
        fprintf(stderr, "Error: %s\n", av_err2str(ret));
        return 1;
    }
    return 0;
}
