#include "ffmpeg_demuxing_decode.h"
#include <QDebug>
#include <logfromqt.h>

ffmpegDemuxingDecode::ffmpegDemuxingDecode()
{
    setLog(true);
    setTimeTest(false);
    setDebugOrInfo(true);

    //strInputPath  = "D:/DataSet/video-h265.mkv";//输入，可以用命令的argv指定
}

ffmpegDemuxingDecode::~ffmpegDemuxingDecode()
{
    release();
}

void ffmpegDemuxingDecode::setLog(bool _isEnable)
{
    bLogEnable = _isEnable;

    if (bLogEnable) {
        qInstallMessageHandler(outputMessage);
        if (bIsDebugOrInfo) {
            qDebug() << "================================" << endl;
            qDebug() << "ffmpegDemuxingDecode.setLog -- Enable log file" << endl;
        }

    }
}

void ffmpegDemuxingDecode::setTimeTest(bool _isTimeTest)
{
    bTimeTestEnable = _isTimeTest;

    if (bLogEnable) {
        if (bIsDebugOrInfo) {
            qDebug() << "ffmpegDemuxingDecode.setTimeTest -- Enable time test" << endl;
        }
    }
}

bool ffmpegDemuxingDecode::isOpened()
{
    return bIsInit;
}

bool ffmpegDemuxingDecode::isFinished()
{
    return bIsFinsh;
}

void ffmpegDemuxingDecode::setDebugOrInfo(bool _isDebugOrInfo)
{
    bIsDebugOrInfo = _isDebugOrInfo;
}

void ffmpegDemuxingDecode::set_srcFilename(const char *_name)
{
    strInputPath = _name;
}

void ffmpegDemuxingDecode::set_srcFilename(std::string _name)
{
    strInputPath = _name;
}

int ffmpegDemuxingDecode::release()
{
    safeDelete();
    bIsInit = false;
    bIsFinsh = false;
    decodeIndx = 0;
    pInputOptions = nullptr;
    pCodecOptions = nullptr;
    m_pSwsCtx = nullptr;
    m_bUseGPU = false;
    m_nDeviceID = -1;

    return 0;
}

void ffmpegDemuxingDecode::safeDelete()
{
    if (m_pFmtCtx != nullptr) {
        avio_close(m_pFmtCtx->pb);
        avformat_free_context(m_pFmtCtx);
        m_pFmtCtx = nullptr;
    }

    if(m_pCodecCtx != nullptr)
    {
        if(m_pCodecCtx->hw_device_ctx != nullptr)
            av_buffer_unref(&m_pCodecCtx->hw_device_ctx);

        if(m_pCodecCtx->hw_frames_ctx)
            av_buffer_unref(&m_pCodecCtx->hw_frames_ctx);

        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }

    if (m_pSwsCtx != nullptr)
    {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = nullptr;
    }

    pCodec = nullptr;
}

/***
 * 解封装 解码 初始化
 * 返回： 0-成功 -1-初始化失败
***/
int ffmpegDemuxingDecode::init()
{
    if (strInputPath.empty()) {
        qCritical() << "ffmpegDemuxingDecode.init -- strInputPath is null" << endl;
        bIsInit = false;
        return -1;
    }

    qDebug() << QString::fromStdString(strInputPath) << endl;

    av_register_all();//注册(初始化)所有组件,然后才能使用复用器和解码器
    //avcodec_register_all();//解码器注册,因为之前已经注册过了所有组件,这里可以不注册
    avformat_network_init();//网络初始化,如果使用旧GnuTLS或OpenSSL库,则需要在库使用前初始化这个函数
    m_pFmtCtx = avformat_alloc_context();//申请一个上下文格式的内存
    if(strInputPath.substr(0,4) == "rtsp") { //如果路径字符串是rtsp开始的rtsp流
        av_dict_set(&pInputOptions,"rtsp_transport", "tcp", 0);//字典中添加一对,rtsp_transport的值为tcp
        av_dict_set(&pInputOptions, "buffer_size", "20480000", 0);
        av_dict_set(&pInputOptions,"stimeout","10000000",0);//字典中添加一对,stimeout值为10000000(超时参数stimeout,单位us)
    }
    else if (strInputPath.substr(0,3) == "udp") {
        av_dict_set(&pInputOptions,"rtsp_transport", "udp", 0);//字典中添加一对,rtsp_transport的值为tcp
        av_dict_set(&pInputOptions, "buffer_size", "20480000", 0);
        av_dict_set(&pInputOptions, "timeout", "5000000", 0);//设置超时3秒
    }

    int nRet = avformat_open_input(&m_pFmtCtx, strInputPath.c_str(), nullptr, &pInputOptions);//打开输入流,第三个nullptr自动检测格式,也可以指定,返回0表示成功,负数表示失败
    if(nRet < 0) {
        qCritical() << "ffmpegDemuxingDecode.init -- avformat_open_input failed!\n" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }

    nRet = avformat_find_stream_info(m_pFmtCtx, nullptr);//读取媒体包的文件获取流信息,因为有些媒体流比如(MPEG)是没有header的,需要另外获取流信息,但无法保证读取出所有解码器格式
    if(nRet < 0) {
        qCritical() << "ffmpegDemuxingDecode.init -- avformat_find_stream_info Failed!\n" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }
    m_nStreamIdx = -1;//流ID初始化为-1
    for (size_t i = 0; i < m_pFmtCtx->nb_streams; i++) { //遍历媒体流的所有元素
        if(m_pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //流媒体结构体->第i个流->编解码器参数信息->编解码器类型. 直到类型等于视频类型,就是跳过header
            m_nStreamIdx = static_cast<int>(i);//记录第一个流ID为i
            break;
        }
    }
    if(m_nStreamIdx == -1) {
        qCritical() << "ffmpegDemuxingDecode.init -- avformat_find_stream_info Failed!\n" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }

    m_pCodecCtx = avcodec_alloc_context3(nullptr);//为编解码器结构体分配内存,并初始化缺省值
    if(m_pCodecCtx == nullptr)
    {
        qCritical() << "ffmpegDemuxingDecode.init -- avcodec_alloc_context3 Failed!\n" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }

    auto* pStream = m_pFmtCtx->streams[m_nStreamIdx];//指向第一个视频数据流
    nRet = avcodec_parameters_to_context(m_pCodecCtx, pStream->codecpar);//用编码器参数中的值填充编码器结构体
    if(nRet < 0) {
        qCritical() << "ffmpegDemuxingDecode.init -- avcodec_parameters_to_context Failed!\n" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }

    m_nInputHeight = m_pCodecCtx->height;//输入的高度
    m_nInputWidth =  m_pCodecCtx->width;//输入的宽度

    if(m_bUseGPU) { //如果使用gpu
        if(m_pCodecCtx->codec_id == AV_CODEC_ID_H264)
            pCodec = avcodec_find_decoder_by_name("h264_cuvid");//找具有指定名称的已注册编解码器
        else if(m_pCodecCtx->codec_id == AV_CODEC_ID_MPEG4)
            pCodec = avcodec_find_decoder_by_name("mpeg4_cuvid");
        else if(m_pCodecCtx->codec_id == AV_CODEC_ID_H265)
            pCodec = avcodec_find_decoder_by_name("hevc_cuvid");
        else { //需要其他的格式可以去ffmpeg源码里找
            qCritical() << "ffmpegDemuxingDecode.init -- this hw decoder is can not support: " << m_pCodecCtx->codec_id << endl;
            safeDelete();
            bIsInit = false;
            return -1;
        }
        m_pSwsCtx = sws_getContext(m_nInputWidth, m_nInputHeight, AV_PIX_FMT_NV12,
                                   m_nInputWidth, m_nInputHeight,
                                   AV_PIX_FMT_BGR24, SWS_BICUBIC, nullptr, nullptr, nullptr);//按输入图像的宽度和高度设置输出的宽度和高度,像素格式由YUV4:2:0两个平面变成BGR8:8:8

        if (bIsDebugOrInfo) {
            qDebug() << "ffmpegDemuxingDecode.init -- hw m_nDeviceID " << m_nDeviceID << endl;
        }

        av_buffer_unref(&m_pCodecCtx->hw_device_ctx);//释放硬件编解码的引用
        int nRet = av_hwdevice_ctx_create(&m_pCodecCtx->hw_device_ctx, AV_HWDEVICE_TYPE_CUDA,
                                          std::to_string(m_nDeviceID).c_str(), nullptr, 0);//打开指定类型的设备并为其创建硬件上下文引用,这里指定硬件类型为cuda。返回0表示成功,返回负数表示失败
        if (nRet < 0) {
            qCritical() << "ffmpegDemuxingDecode.init -- av_hwdevice_ctx_create Failed, Error creating a CUDA device" << endl;
            safeDelete();
            bIsInit = false;
            return -1;
        }

        av_buffer_unref(&m_pCodecCtx->hw_frames_ctx);//释放输入或输出编解码的引用,这里解码是输出帧
        m_pCodecCtx->hw_frames_ctx = av_hwframe_ctx_alloc(m_pCodecCtx->hw_device_ctx);//给硬件解码设备引用分配绑定输出帧引用
        if (!m_pCodecCtx->hw_frames_ctx) {
            qCritical() << "ffmpegDemuxingDecode.init -- av_hwframe_ctx_alloc Failed" << endl;
            safeDelete();
            bIsInit = false;
            return -1;
        }

        AVHWFramesContext *pFrameCtx = (AVHWFramesContext*)m_pCodecCtx->hw_frames_ctx->data;//将编解码器结构体->硬件帧->数据 给 硬件帧上下文结构体
        pFrameCtx->format = AV_PIX_FMT_CUDA; //格式设置为cuda
        pFrameCtx->sw_format = AV_PIX_FMT_NV12; //像素格式为NV12(YUV420)
        pFrameCtx->width = m_pCodecCtx->width;
        pFrameCtx->height = m_pCodecCtx->height;

        if (bIsDebugOrInfo) {
            qDebug() << "ffmpegDemuxingDecode.init -- Initializing CUDA frames context: sw_format = "
                     << av_get_pix_fmt_name(pFrameCtx->sw_format) << ", width = " << pFrameCtx->width
                     << ", height = " << pFrameCtx->height << endl;
        }

        nRet = av_hwframe_ctx_init(m_pCodecCtx->hw_frames_ctx);//硬件帧初始化,使用前必须完成填充帧的所有信息,成功返回0,失败负数
        if (nRet < 0) {
            qCritical() << "ffmpegDemuxingDecode.init -- av_hwframe_ctx_init Failed, Error creating a CUDA frame pool" << endl;
            safeDelete();
            bIsInit = false;
            return -1;
        }
    }
    else { //如果不使用gpu
        pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);//编解码器指针指向已注册编解码器
        m_pSwsCtx = sws_getContext(m_nInputWidth, m_nInputHeight, AV_PIX_FMT_YUV420P,
                                   m_nInputWidth, m_nInputHeight,
                                   AV_PIX_FMT_BGR24, SWS_BICUBIC, nullptr, nullptr, nullptr);//按输入图像的宽度和高度设置输出的宽度和高度,像素格式由YUV4:2:0变成BGR8:8:8
    }

    if(pCodec == nullptr) {
        qCritical() << "ffmpegDemuxingDecode.init -- Cant find Codec" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }
    if(m_pSwsCtx == nullptr) {
        qCritical() << "ffmpegDemuxingDecode.init -- Cant create Sws" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }

    nRet = avcodec_open2(m_pCodecCtx, pCodec, &pCodecOptions);//初始化音视频编解码器的AVCodecContext,根据编解码器类型和参数设置音视频解码器的上下文
    if(nRet < 0)
    {
        qCritical() << "ffmpegDemuxingDecode.init -- avcodec_open2 Failed" << endl;
        safeDelete();
        bIsInit = false;
        return -1;
    }

    bIsInit = true;
    return 0;
}

/***
 * 返回： 0-成功 -1-失败  1-已经解码完毕
***/
cv::Mat ffmpegDemuxingDecode::demux_decode_a_frame(int &decodeStatu)
{
    cv::Mat frame;
    if (bIsFinsh) {
        if(bIsDebugOrInfo) {
            qDebug() << "ffmpegDemuxingDecode.demux_decode_a_frame -- decode is finish" << endl;
        }
        safeDelete();
        decodeStatu = -1;
        return frame;
    }
    if (!bIsInit) {
        qCritical() << "ffmpegDemuxingDecode.demux_decode_a_frame -- bIsInit is false" << endl;
        safeDelete();
        decodeStatu = -1;
        return frame;
    }

    bool isDecodeAFrame = false;
decode:
    AVPacket packet;//定义音视频的包,需要填充包含压缩数据和复用信息
    if(av_read_frame(m_pFmtCtx, &packet) >= 0) { //从复用器封装中读取具体的音视频帧数据给视频包,返回0表示成功,负数失败
        if((packet.buf != nullptr) && (packet.stream_index == m_nStreamIdx)) { //包非空且流ID对应视频流
            int nRet = avcodec_send_packet(m_pCodecCtx, &packet);//将包发送给编解码器结构体,调用前必须用avcodec_open2打开编解码器结构体,返回0表示成功
            if(nRet != 0) {
                qCritical() << "ffmpegDemuxingDecode.demux_decode_a_frame -- avcodec_send_packet failed" << endl;
                safeDelete();
                av_free_packet(&packet);
                decodeStatu = -1;
                return frame;
            }

            while(true) { //可能一个包里有多帧 一般是一帧
                AVFrame* pFrame = av_frame_alloc();//创建解压缩的输出数据帧,并分配空间
                if(avcodec_receive_frame(m_pCodecCtx, pFrame) == 0) { //将解码器解码的数据给输出数据帧
                    avpicture_alloc(&m_avPicture, AV_PIX_FMT_BGR24, m_nInputWidth, m_nInputHeight);//解码后的图片根据高度,宽度,像素格式分配空间
                    sws_scale(m_pSwsCtx, (const uint8_t* const *)pFrame->data, pFrame->linesize, 0 , m_nInputHeight, m_avPicture.data, m_avPicture.linesize);//无缩放将输出数据帧的数据给输出图片
                    cv::Mat img(m_nInputHeight, m_nInputWidth, CV_8UC3, m_avPicture.data[0]);//创建Mat对象
                    frame = img.clone();

                    isDecodeAFrame = true;
                    decodeIndx++; //计数帧增加
                    if (decodeIndx % 10 == 0 && bIsDebugOrInfo) {
                        qDebug() << "decodeIndx:" << decodeIndx << " w x h = " << m_nInputWidth << "x" << m_nInputHeight << endl;
                    }

                    av_free(m_avPicture.data[0]);
                    av_frame_free(&pFrame);
                }
                else {
                    av_frame_free(&pFrame);
                    break;
                }
            }
        }
        av_free_packet(&packet);
    }
    else { //解封装 解码完成  进入结束
        qDebug() << "ffmpegDemuxingDecode.demux_decode_a_frame -- decode finish" << endl;

        safeDelete();

        bIsFinsh = true;
        decodeStatu = -1;
        return frame;
    }

    av_free_packet(&packet);

    if(isDecodeAFrame)
        decodeStatu = 0;
    else {
        decodeStatu = 2;//没解码出图像 也没结束
        goto decode;
    }

    return frame;
}
