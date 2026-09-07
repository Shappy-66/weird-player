extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include <cstdio>
#include <cstring>
#include <cinttypes>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::printf("usage: %s <file>\n", argv[0]);
        return 1;
    }
    char filename[256];
    std::strncpy(filename, argv[1], sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    AVFormatContext *fmt = avformat_alloc_context();
    if (avformat_open_input(&fmt, filename, nullptr, nullptr) < 0) {
        std::printf("cannot open %s\n", filename);
        return 1;
    }
    avformat_find_stream_info(fmt,NULL);
    std::printf("Format %s, duration %.6fs\n",
                fmt->iformat->long_name, fmt->duration/1000000.0);

    avformat_close_input(&fmt);
    return 0;
}