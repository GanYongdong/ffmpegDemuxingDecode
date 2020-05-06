#ifndef FFMPEG_DEMUXING_DECODE_H
#define FFMPEG_DEMUXING_DECODE_H

#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <QDebug>
extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext.h>
}

class ffmpegDemuxingDecode
{
public:
    ffmpegDemuxingDecode();
    ~ffmpegDemuxingDecode();

    int init();
    cv::Mat demux_decode_a_frame(int &decodeStatu);
    void safeDelete();

private:
    bool m_bUseGPU = false;//是否使用gpu
    int m_nDeviceID = -1;//设备号,初始化为-1
    AVFormatContext *m_pFmtCtx = nullptr;//创建音视频格式上下文类型的指针,AVFormatContext描述了一个媒体文件或媒体流的构成和基本信息
    AVCodecContext *m_pCodecCtx = nullptr;//创建音视频编解码上下文类型的指针,AVCodecContext描述编解码器上下文的数据结构，包含了众多编解码器需要的参数信息
    //AVFrame *m_pFrame;//AVFrame结构体一般用于存储原始数据（即非压缩数据，例如对视频来说是YUV，RGB，对音频来说是PCM），此外还包含了一些相关的信息。
    //AVFrame表示解码过后的一个数据帧 AVFrame 通过使用 av_frame_alloc()来创建. 这个函数只是创建了AVFrame结构本身，在结构 中定义的指向其他内存块的缓冲区指针要用其他方法来分配 使用 av_frame_free()来释放AVFrame.
    //AVPacket m_packet;//AVPacket保存了解复用（demuxer)之后，解码（decode）之前的数据（仍然是压缩后的数据）和关于这些数据的一些附加的信息。
    //如显示时间戳（pts），解码时间戳（dts）,数据时长（duration），所在流媒体的索引（stream_index）等等
    //AVPacket 作为解码器的输入 或 编码器的输出。 当作为解码器的输入时，它由demuxer生成，然后传递给解码器 当作为编码器的输出时，由编码器生成，然后传递给muxer 在视频中，AVPacket 只能包含不大于1帧的内容，而视频的1帧可能要包含在多个AVPacket中，AVPacket < AVFrame
    AVPicture m_avPicture;//包含指向解码后的图片数据的指针,对于视频来说是YUV/RGB数据，对于音频来说是PCM数据。 每行数据的大小。对于视频数据，行宽必须是16/32字节对齐（取决于cpu）, 因此未必等于图片的宽，一般大于图像的宽。
    SwsContext* m_pSwsCtx = nullptr;//进行图像数据格式的转换以及图片的缩放应用
    int m_nStreamIdx;//流ID
    int m_nInputHeight;//输入图像高度
    int m_nInputWidth;//输入图像宽度
    AVDictionary* pInputOptions = nullptr;//创建一个字典,存储参数,方便后面打开输入媒体流
    std::string strInputPath = ""; //输入文件路径或url
    const AVCodec* pCodec = nullptr;//编解码器指针
    AVDictionary* pCodecOptions = nullptr;//创建一个字典,存储编解码器参数

public:
    void setLog(bool _isEnable);
    void setTimeTest(bool _isTimeTest);
    void setDebugOrInfo(bool _isDebugOrInfo);
    bool isOpened();
    bool isFinished();
    int release();
    void set_srcFilename(const char *_name);
    void set_srcFilename(std::string _name);

private:



private:
    bool bLogEnable = false;
    bool bTimeTestEnable = false;
    bool bIsInit = false;
    bool bIsDebugOrInfo = true; //是否debug和info消息显示
    int decodeIndx = 0; //解码到第几帧
    bool bIsFinsh = false;




};

#endif // FFMPEG_DEMUXING_DECODE_H
