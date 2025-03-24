/*
 * image-and-videos-on-ASCII: Convert images and videos to ASCII with colors
 * Copyright (C) 2025 ChosenSoul
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

const char *ascii_chars = " .:-=+*#%@";

char get_ascii_char(int brightness) {
    if (brightness < 30)
        return ascii_chars[0];
    else if (brightness < 60)
        return ascii_chars[1];
    else if (brightness < 90)
        return ascii_chars[2];
    else if (brightness < 120)
        return ascii_chars[3];
    else if (brightness < 150)
        return ascii_chars[4];
    else if (brightness < 180)
        return ascii_chars[5];
    else if (brightness < 210)
        return ascii_chars[6];
    else if (brightness < 240)
        return ascii_chars[7];
    else
        return ascii_chars[8];
}

void print_colored_char(int r, int g, int b, char c) {
    printf("\033[38;2;%d;%d;%dm%c\033[0m", r, g, b, c);
}

void process_image(const char *filename, int write_to_file, const char *output_file, int max_width, int max_height) {
    int width, height, channels;
    unsigned char *img = stbi_load(filename, &width, &height, &channels, 0);
    if (!img) {
        printf("Error loading images\n");
        return;
    }

    int new_width = width;
    int new_height = height;
    unsigned char *scaled_img = img;
    if (width > max_width || height > max_height) {
        float scale = fminf((float)max_width / width, (float)max_height / height);
        new_width = (int)(width * scale);
        new_height = (int)(height * scale);

        scaled_img = (unsigned char *)malloc(new_width * new_height * channels);
        if (!scaled_img) {
            printf("Error allocating memory for scaled image\n");
            stbi_image_free(img);
            return;
        }

        for (int y = 0; y < new_height; ++y) {
            for (int x = 0; x < new_width; ++x) {
                int src_x = (int)(x / scale);
                int src_y = (int)(y / scale);
                for (int c = 0; c < channels; ++c) {
                    scaled_img[(y * new_width + x) * channels + c] = img[(src_y * width + src_x) * channels + c];
                }
            }
        }
        stbi_image_free(img);
        img = scaled_img;
    }

    FILE *out_file = NULL;
      if (write_to_file) {
        out_file = fopen(output_file, "w");
        if (!out_file) {
            printf("Error opening file %s\n", output_file);
            stbi_image_free(img);
            return;
        }
    }

    for (int y = 0; y < new_height; y += 2) {
        for (int x = 0; x < new_width; x++) {
            int pixel_idx = (y * new_width + x) * channels;
            if (channels == 4 && img[pixel_idx + 3] == 0) {
                // Прозрачный пиксель
                if (write_to_file) fputc(' ', out_file);
                else printf(" ");
            } else {
                int r = img[pixel_idx];
                int g = channels > 1 ? img[pixel_idx + 1] : r;
                int b = channels > 2 ? img[pixel_idx + 2] : r;
                int brightness = (r + g + b) / 3;
                char c = get_ascii_char(brightness);
                if (write_to_file) fputc(c, out_file);
                else print_colored_char(r, g, b, c);
            }
        }
        if (write_to_file) fputc('\n', out_file);
        else printf("\n");
    }
    if (out_file) fclose(out_file);
    stbi_image_free(img);
}

void process_video(const char *filename, int write_to_file, const char *output_file, int max_width, int max_height) {
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        printf("Memory allocation error for frame\n");
        return;
    }
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        printf("Error allocating memory for package\n");
        av_frame_free(&frame);
        return;
    }

    FILE *out_file = NULL;
    if (write_to_file) {
        out_file = fopen(output_file, "w");
        if (!out_file) {
            printf("Error opening file %s\n", output_file);
             av_packet_free(&packet);
            av_frame_free(&frame);
            return;
        }
    }

    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        printf("Error opening video\n");
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("Error getting flow information\n");
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }

    int video_stream = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }
    if (video_stream == -1) {
        printf("Video stream not found\n");
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }

    const AVCodec *codec = avcodec_find_decoder(fmt_ctx->streams[video_stream]->codecpar->codec_id);
    if (!codec) {
        printf("Codec not found\n");
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        printf("Error allocating memory for codec context\n");
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    if (avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream]->codecpar) < 0) {
        printf("Error copying codec parameters to context\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        printf("Error opening codec\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }

    int new_width = codec_ctx->width > max_width ? max_width : codec_ctx->width;
    int new_height = codec_ctx->height > max_height ? max_height : codec_ctx->height;
    float aspect = (float)codec_ctx->width / codec_ctx->height;
    if (new_width / aspect > new_height)
        new_width = (int)(new_height * aspect);
    else
        new_height = (int)(new_width / aspect);

    struct SwsContext *sws_ctx = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
        new_width, new_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        printf("Error initializing SwsContext\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }

    AVFrame *rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        printf("Error allocating memory for rgb_frame\n");
        sws_freeContext(sws_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width = new_width;
    rgb_frame->height = new_height;
    if (av_frame_get_buffer(rgb_frame, 0) < 0) {
        printf("Error allocating buffer for rgb_frame\n");
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
         av_packet_free(&packet);
        av_frame_free(&frame);
        return;
    }
    av_packet_unref(packet);
    int frame_number = 0;

    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream) {
            if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                    sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
                              frame->height, rgb_frame->data, rgb_frame->linesize);
                    if (!write_to_file) system("clear");
                    if (write_to_file) fprintf(out_file, "Frame %d:\n", ++frame_number);
                    else  printf("Frame %d:\n", ++frame_number);
                    for (int y = 0; y < new_height; y += 2) {
                        for (int x = 0; x < new_width; x++) {
                            int pixel_idx = y * rgb_frame->linesize[0] + x * 3;
                            int r = rgb_frame->data[0][pixel_idx];
                            int g = rgb_frame->data[0][pixel_idx + 1];
                            int b = rgb_frame->data[0][pixel_idx + 2];
                            int brightness = (r + g + b) / 3;
                            char c = get_ascii_char(brightness);
                            if (write_to_file) fputc(c, out_file);
                            else print_colored_char(r, g, b, c);
                        }
                        if (write_to_file) fputc('\n', out_file);
                        else printf("\n");
                    }
                   if (write_to_file) fprintf(out_file, "\n");
                }
            }
        }
        av_packet_unref(packet);
    }
    avcodec_send_packet(codec_ctx, NULL);
     while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
                         frame->height, rgb_frame->data, rgb_frame->linesize);
        if (!write_to_file) system("clear");
        if (write_to_file) fprintf(out_file, "Frame %d:\n", ++frame_number);
        else  printf("Frame %d:\n", ++frame_number);
        for (int y = 0; y < new_height; y += 2) {
            for (int x = 0; x < new_width; x++) {
                int pixel_idx = y * rgb_frame->linesize[0] + x * 3;
                int r = rgb_frame->data[0][pixel_idx];
                int g = rgb_frame->data[0][pixel_idx + 1];
                int b = rgb_frame->data[0][pixel_idx + 2];
                int brightness = (r + g + b) / 3;
                char c = get_ascii_char(brightness);
                if (write_to_file) fputc(c, out_file);
                else print_colored_char(r, g, b, c);
            }
           if (write_to_file) fputc('\n', out_file);
           else printf("\n");
        }
        if (write_to_file) fprintf(out_file, "\n");
    }

    if (out_file) fclose(out_file);
    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
}

int is_video(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    return (strcmp(ext, ".mp4") == 0 || strcmp(ext, ".avi") == 0 || strcmp(ext, ".mkv") == 0);
}

int main(int argc, char *argv[]) {
    int max_width = 600; 
    int max_height = 140;
    
    if (argc < 3 || strcmp(argv[1], "-i") != 0) {
        printf("Usage: %s -i <file> [-w <width>] [-h <height>] [--output]\n", argv[0]);
        printf("  -i <file>: Specifies the input file (image or video).\n");
        printf("  -w <width>: Maximum width (default 600).\n");
        printf("  -h <height>: Maximum height (default 140).\n");
        printf("  --output: (optional) Outputs ASCII-art to a text file (output.txt).\n");
        return 1;
    }

    const char *filename = NULL;
    int write_to_file = 0;
    const char *output_file = "output.txt";

    // Парсим аргументы
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            filename = argv[++i];
        }
        else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            max_width = atoi(argv[++i]);
            if (max_width <= 0) {
                printf("Error: width must be a + not - number\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            max_height = atoi(argv[++i]);
            if (max_height <= 0) {
                printf("Error: height must be a + not - number\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--output") == 0) {
            write_to_file = 1;
        }
    }

    if (!filename) {
        printf("Error: Input file not specified\n");
        return 1;
    }

    if (is_video(filename)) {
        process_video(filename, write_to_file, output_file, max_width, max_height);
    } else {
        process_image(filename, write_to_file, output_file, max_width, max_height);
    }

    return 0;
}