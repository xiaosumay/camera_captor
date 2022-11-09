#include "mp4maker.h"

#include <QImage>
#include <QDebug>
#include <QThread>
#include <QDateTime>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>
#include "libavformat/avio.h"

#include <libswresample/swresample.h>

#include <libswscale/swscale.h>

#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#ifdef __cplusplus
}
#endif

QStringList Mp4Maker::get_vdec_support_hwdevices()
{
    QStringList hwdevs;
    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
        hwdevs.push_back(av_hwdevice_get_type_name(type));
    }
    return hwdevs;
}

Mp4Maker::Mp4Maker(QString hwdevice, QObject *parent) : QObject(parent), m_hwdevice(hwdevice)
{
    av_register_all();
    avcodec_register_all();
}

Mp4Maker::~Mp4Maker()
{
    if (pFormatCtx) {
        if (m_unit_started) {
            // 写入文件尾
            auto errnum = av_write_trailer(pFormatCtx);
            if (errnum != 0) {
                qDebug() << "av_write_trailer failed";
            }
            errnum = avio_closep(&pFormatCtx->pb); // 关闭AVIO流
            if (errnum != 0) {
                qDebug() << "avio_close failed";
            }

            avformat_close_input(&pFormatCtx); // 关闭封装上下文
        }

        avformat_free_context(pFormatCtx);
    }

    // 关闭编码器和清理上下文的所有空间
    if (pVideoCodecCtx) {
        avcodec_close(pVideoCodecCtx);
        avcodec_free_context(&pVideoCodecCtx);
    }

    if (pAudioCodecCtx) {
        avcodec_close(pAudioCodecCtx);
        avcodec_free_context(&pAudioCodecCtx);
    }

    // 音视频转换上下文
    if (pSwsCtx) {
        sws_freeContext(pSwsCtx);
        pSwsCtx = NULL;
    }

    if (swrCtx) {
        swr_free(&swrCtx);
        swrCtx = NULL;
    }

    // 清理音视频帧
    if (vframe) { av_frame_free(&vframe); }
    if (aframe) { av_frame_free(&aframe); }
}

bool Mp4Maker::init(const QSize &size, const QString &save_path)
{
    if (m_unit_started) { return true; }

    char errbuf[1024] = {0};

    auto errnum = avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, save_path.toStdString().c_str());
    if (errnum < 0) {
        qDebug() << "avcodec_find_encoder failed!";
        return false;
    }

    // h264视频编码器
    const AVCodec *vcodec = avcodec_find_encoder_by_name("h264_qsv");
    //const AVCodec *vcodec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_H264);

    if (!vcodec) {
        qDebug() << "avcodec_find_encoder_by_name failed!";
        return false;
    }

    // 创建编码器上下文
    pVideoCodecCtx = avcodec_alloc_context3(vcodec);
    if (!pVideoCodecCtx) {
        qDebug() << "avcodec_alloc_context3 failed!";
        return false;
    }

    pVideoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

    // 比特率、宽度、高度
    pVideoCodecCtx->bit_rate = size.width() * size.height() * 6;
    pVideoCodecCtx->width = size.width();   // 视频宽度
    pVideoCodecCtx->height = size.height(); // 视频高度
    // 时间基数、帧率
    pVideoCodecCtx->time_base = {1, 25};
    pVideoCodecCtx->framerate = {25, 1};
    // 关键帧间隔
    pVideoCodecCtx->gop_size = 6;
    // 不使用b帧
    pVideoCodecCtx->max_b_frames = 0;
    // 帧、编码格式
    pVideoCodecCtx->pix_fmt = *vcodec->pix_fmts;
    pVideoCodecCtx->codec_id = vcodec->id;
    // 预设：快速

    auto *p = vcodec->pix_fmts;
    while (*p != AV_PIX_FMT_NONE) {
        qDebug() << "pVideoCodecCtx->pix_fmt: " << *p;
        p++;
    }

    av_opt_set(pVideoCodecCtx->priv_data, "tune", "zerolatency", NULL);
    av_opt_set(pVideoCodecCtx->priv_data, "profile", "high", NULL);
    av_opt_set(pVideoCodecCtx->priv_data, "preset", "slow", NULL);
    av_opt_set(pVideoCodecCtx->priv_data, "qp", "23", AV_OPT_SEARCH_CHILDREN);
    av_opt_set(pVideoCodecCtx->priv_data, "crf", "18", AV_OPT_SEARCH_CHILDREN);

    // 全局头
    pVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    pVideoCodecCtx->thread_count = QThread::idealThreadCount() / 2;

#if 0
    QByteArray hardwareData = m_hwdevice.toUtf8();
    enum AVHWDeviceType type = av_hwdevice_find_type_by_name(hardwareData.data());
    qDebug() << "AVHWDeviceType" << type;

    AVBufferRef *pAVBufferRef = av_hwdevice_ctx_alloc(type);
#endif

    errnum = avcodec_open2(pVideoCodecCtx, vcodec, NULL);
    if (errnum < 0) {
        char errbuf[1024] = {0};
        av_strerror(errnum, errbuf, sizeof(errbuf));
        qDebug() << "avcodec_open2 failed " << errbuf;
        return false;
    }

    // 为封装器创建视频流
    pVideoStream = avformat_new_stream(pFormatCtx, NULL);
    if (!pVideoStream) {
        qDebug() << "avformat_new_stream video stream failed!";
        return false;
    }

    pVideoStream->codecpar->codec_tag = 0;

    // 配置视频流的编码参数
    avcodec_parameters_from_context(pVideoStream->codecpar, pVideoCodecCtx);

    pSwsCtx = sws_getContext(size.width(), size.height(),
                             AVPixelFormat::AV_PIX_FMT_BGRA, // 输入
                             size.width(), size.height(),
                             pVideoCodecCtx->pix_fmt, // 输出
                             SWS_BICUBIC,             // 算法
                             0, 0, 0);

    if (!pSwsCtx) {
        qDebug() << "sws_getContext failed";
        return false;
    }

    // 编码阶段的视频帧结构
    vframe = av_frame_alloc();
    vframe->format = pVideoCodecCtx->pix_fmt;
    vframe->width = size.width();
    vframe->height = size.height();
    vframe->pts = 0;

    // 为视频帧分配空间
    errnum = av_frame_get_buffer(vframe, 32);
    if (errnum < 0) {
        qDebug() << "av_frame_get_buffer failed";
        return false;
    }

#if defined(MAKE_AUDIO)
    // 创建音频编码器，指定类型为AAC
    const AVCodec *acodec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_AAC);
    if (!acodec) {
        qDebug() << "avcodec_find_encoder failed!";
        return false;
    }

    // 根据编码器创建编码器上下文
    pAudioCodecCtx = avcodec_alloc_context3(acodec);
    if (!pAudioCodecCtx) {
        qDebug() << "avcodec_alloc_context3 failed!";
        return false;
    }

    pAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;

    pAudioCodecCtx->sample_rate = 16000;
    pAudioCodecCtx->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_FLTP;
    pAudioCodecCtx->channels = 1;
    // 根据音频通道数自动选择输出类型
    pAudioCodecCtx->channel_layout = av_get_default_channel_layout(pAudioCodecCtx->channels);
    pAudioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    pAudioCodecCtx->time_base = {1, 64};
    pAudioCodecCtx->framerate = {64, 1};

    // 打开编码器
    errnum = avcodec_open2(pAudioCodecCtx, acodec, NULL);
    if (errnum < 0) {
        av_strerror(errnum, errbuf, sizeof(errbuf));

        avcodec_free_context(&pAudioCodecCtx);
        qDebug() << "avcodec_open2 failed: " << errbuf;
        return false;
    }

    // 添加音频流
    pAudioStream = avformat_new_stream(pFormatCtx, NULL);
    if (!pAudioStream) {
        qDebug() << "avformat_new_stream failed";
        return false;
    }

    pAudioStream->codec->codec_tag = 0;
    pAudioStream->codecpar->codec_tag = 0;
    // 配置音频流的编码器参数
    avcodec_parameters_from_context(pAudioStream->codecpar, pAudioCodecCtx);

    swrCtx = swr_alloc_set_opts(swrCtx,
                                av_get_default_channel_layout(1),
                                AVSampleFormat::AV_SAMPLE_FMT_FLTP,
                                16000, // 输出
                                av_get_default_channel_layout(1),
                                AVSampleFormat::AV_SAMPLE_FMT_S16,
                                16000, // 输入
                                0,
                                0);
    errnum = swr_init(swrCtx);
    if (errnum < 0) {
        qDebug() << "swr_init failed";
        return false;
    }

    // 创建音频帧
    aframe = av_frame_alloc();
    aframe->format = AVSampleFormat::AV_SAMPLE_FMT_FLTP;
    aframe->channels = 1;
    aframe->channel_layout = av_get_default_channel_layout(1);
    aframe->nb_samples = pAudioCodecCtx->frame_size;

    // 为音频帧分配空间
    errnum = av_frame_get_buffer(aframe, 0);
    if (errnum < 0) { qDebug() << "av_frame_get_buffer failed"; }
#endif

    // 打开输出流IO
    errnum = avio_open(&pFormatCtx->pb, save_path.toStdString().c_str(), AVIO_FLAG_WRITE); // 打开AVIO流
    if (errnum < 0) {
        avio_close(pFormatCtx->pb);
        qDebug() << "avio_open failed";
    }

    // 写文件头
    AVDictionary *opt = NULL;
    av_dict_set_int(&opt, "video_track_timescale", 25, 0);

    errnum = avformat_write_header(pFormatCtx, &opt);
    if (errnum < 0) {
        av_strerror(errnum, errbuf, sizeof(errbuf));
        qDebug() << "avformat_write_header failed " << errbuf;
    }
    av_dict_free(&opt);

    m_unit_started = true;
    return true;
}

void Mp4Maker::addAudio(QByteArray audio)
{
#if defined(MAKE_AUDIO)
    if (!m_unit_started) return;

    int errnum = 0;
    char errbuf[1024] = {0};

    const uint8_t *in[AV_NUM_DATA_POINTERS] = {0};
    in[0] = (uint8_t *) audio.constData();

    int sizea = av_samples_get_buffer_size(NULL, pAudioCodecCtx->channels,
                                           pAudioCodecCtx->frame_size,
                                           AVSampleFormat::AV_SAMPLE_FMT_S16, 1);

    //qDebug() << "av_samples_get_buffer_size: " << sizea;

    // 音频重采样
    int len = swr_convert(swrCtx,
                          aframe->data,
                          aframe->nb_samples, // 输出
                          in,
                          aframe->nb_samples); // 输入
    if (len < 0) {
        qDebug() << "audio: swr_convert failed";
        return;
    }

    aframe->pts = apts++;

    // 音频编码
    errnum = avcodec_send_frame(pAudioCodecCtx, aframe);
    if (errnum < 0) {
        av_strerror(errnum, errbuf, sizeof(errbuf));
        qDebug() << "avcodec_send_frame failed " << errbuf;
        return;
    }

    while (true) {
        // 音频编码报文
        AVPacket *apkt = av_packet_alloc();

        errnum = avcodec_receive_packet(pAudioCodecCtx, apkt);
        if (errnum < 0) {
            av_packet_unref(apkt);
            av_strerror(errnum, errbuf, sizeof(errbuf));
            return;
        }

        // 转换pts
        av_packet_rescale_ts(apkt, pAudioCodecCtx->time_base, pAudioStream->time_base);
        apkt->stream_index = pAudioStream->index;

        qDebug() << "apkt->pts: " << apkt->dts;

        // 写音频帧
        errnum = av_interleaved_write_frame(pFormatCtx, apkt);
        if (errnum < 0) {
            av_strerror(errnum, errbuf, sizeof(errbuf));
            qDebug() << "av_interleaved_write_frame failed " << errbuf;
            return;
        }
    }
#endif
}

extern QImage QVideoFrameToQImage(const QVideoFrame &videoFrame);

void Mp4Maker::addImage(const QVideoFrame &frame)
{
    if (!m_unit_started)
        return;

#if 0
    qint64 start = QDateTime::currentMSecsSinceEpoch();
#endif

    int errnum = 0;

    QImage img = QVideoFrameToQImage(frame);

    // 视频编码
    const uchar *rgb = img.bits();

    // 固定写法：配置1帧原始视频画面的数据结构通常为RGBA的形式
    uint8_t *srcSlice[AV_NUM_DATA_POINTERS] = {0};
    srcSlice[0] = (uint8_t *)rgb;
    int srcStride[AV_NUM_DATA_POINTERS] = {0};
    srcStride[0] = img.width() * 4;

    // 转换
    int h = sws_scale(pSwsCtx, srcSlice, srcStride, 0, img.height(), vframe->data, vframe->linesize);
    if (h < 0) {
        qDebug() << "image: sws_scale failed";
        return;
    }

    // pts递增
    vframe->pts = vpts++;

    errnum = avcodec_send_frame(pVideoCodecCtx, vframe);
    if (errnum < 0) {
        return;
    }

    while (true) {
        // 视频编码报文
        AVPacket *vpkt = av_packet_alloc();

        errnum = avcodec_receive_packet(pVideoCodecCtx, vpkt);
        if (errnum == AVERROR(EAGAIN) || errnum == AVERROR_EOF) {
            av_packet_unref(vpkt);
            break;
        }

        // 转换pts
        av_packet_rescale_ts(vpkt, pVideoCodecCtx->time_base, pVideoStream->time_base);
        vpkt->stream_index = pVideoStream->index;

        // 向封装器中写入压缩报文，该函数会自动释放pkt空间，不需要调用者手动释放
        errnum = av_interleaved_write_frame(pFormatCtx, vpkt);

        if (errnum < 0) {
            break;
        }
    }

#if 0
    qint64 stop = QDateTime::currentMSecsSinceEpoch();
    qDebug("sppend: %lld ms", stop - start);
#endif
}
