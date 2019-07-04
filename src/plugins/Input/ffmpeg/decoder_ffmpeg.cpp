/***************************************************************************
 *   Copyright (C) 2006-2019 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QObject>
#include <QFile>
#include <QtDebug>
#include "replaygainreader.h"
#include "decoder_ffmpeg.h"
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,34,0)) //libav-10: 55.34.1; ffmpeg-2.1: 55.39.100
extern "C"{
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
}
#endif

// callbacks

static int ffmpeg_read(void *data, uint8_t *buf, int size)
{
    DecoderFFmpeg *d = (DecoderFFmpeg*)data;
#if (LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57,84,101))
    if(d->input()->atEnd())
        return AVERROR_EOF;
#endif
    return (int)d->input()->read((char*)buf, size);
}

static int64_t ffmpeg_seek(void *data, int64_t offset, int whence)
{
    DecoderFFmpeg *d = (DecoderFFmpeg*)data;
    int64_t absolute_pos = 0;
    /*if(d->input()->isSequential())
        return -1;*/
    switch( whence )
    {
    case AVSEEK_SIZE:
        return d->input()->size();
    case SEEK_SET:
        absolute_pos = offset;
        break;
    case SEEK_CUR:
        absolute_pos = d->input()->pos() + offset;
        break;
    case SEEK_END:
        absolute_pos = d->input()->size() - offset;
        break;
    default:
        return -1;
    }
    if(absolute_pos < 0 || absolute_pos > d->input()->size())
        return -1;
    return d->input()->seek(absolute_pos);
}

// Decoder class
DecoderFFmpeg::DecoderFFmpeg(const QString &path, QIODevice *i)
        : Decoder(i)
{
    m_bitrate = 0;
    m_totalTime = 0;
    ic = nullptr;
    m_path = path;
    m_temp_pkt.size = 0;
    m_pkt.size = 0;
    m_pkt.data = nullptr;
    m_output_at = 0;
    m_skipBytes = 0;
    m_stream = nullptr;
    m_decoded_frame = nullptr;
    m_channels = 0;
    c = nullptr;
    audioIndex = 0;
    m_seekTime = -1;
    av_init_packet(&m_pkt);
    av_init_packet(&m_temp_pkt);
    m_input_buf = nullptr;
}

DecoderFFmpeg::~DecoderFFmpeg()
{
    m_bitrate = 0;
    m_temp_pkt.size = 0;
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,48,0)) //ffmpeg-3.1:  57.48.101
    if(c)
        avcodec_free_context(&c);
#endif
    if (ic)
        avformat_free_context(ic);
    if(m_pkt.data)
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,102)) //ffmpeg-3.0
        av_packet_unref(&m_pkt);
#else
        av_free_packet(&m_pkt);
#endif
    if(m_stream)
#if (LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 80, 100)) //ffmpeg-3.4
        avio_context_free(&m_stream);
#else
        av_free(m_stream);
#endif

    if(m_decoded_frame)
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,34,0)) //libav-10: 55.34.1; ffmpeg-2.1:  55.39.100
        av_frame_free(&m_decoded_frame);
#else
        av_free(m_decoded_frame);
#endif
}

bool DecoderFFmpeg::initialize()
{
    m_bitrate = 0;
    m_totalTime = 0;
    m_seekTime = -1;

    ic = avformat_alloc_context();

    AVProbeData pd;
    memset(&pd, 0, sizeof(pd));
    uint8_t buf[PROBE_BUFFER_SIZE + AVPROBE_PADDING_SIZE];
    pd.filename = m_path.toLocal8Bit().constData();
    pd.buf_size = input()->peek((char*)buf, sizeof(buf) - AVPROBE_PADDING_SIZE);
    pd.buf = buf;
    if(pd.buf_size < PROBE_BUFFER_SIZE)
    {
        qWarning("DecoderFFmpeg: too small buffer size: %d bytes", pd.buf_size);
        return false;
    }
    AVInputFormat *fmt = av_probe_input_format(&pd, 1);
    if(!fmt)
    {
        qWarning("DecoderFFmpeg: usupported format");
        return false;
    }
    qDebug("DecoderFFmpeg: detected format: %s", fmt->long_name);
    qDebug("=%s=", fmt->name);

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58,10,100)) //ffmpeg-3.5
    m_input_buf = (uchar*)av_malloc(INPUT_BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
#else
    m_input_buf = (uchar*)av_malloc(INPUT_BUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
#endif

    m_stream = avio_alloc_context(m_input_buf, INPUT_BUFFER_SIZE, 0, this, ffmpeg_read, nullptr, ffmpeg_seek);
    if(!m_stream)
    {
        av_free(m_input_buf);
        m_input_buf = nullptr;
        qWarning("DecoderFFmpeg: unable to initialize I/O callbacks");
        return false;
    }
    m_stream->seekable = !input()->isSequential();
    m_stream->max_packet_size = INPUT_BUFFER_SIZE;
    ic->pb = m_stream;

    if(avformat_open_input(&ic, nullptr, fmt, nullptr) != 0)
    {
        qDebug("DecoderFFmpeg: avformat_open_input() failed");
        return false;
    }
    avformat_find_stream_info(ic, nullptr);
    if(ic->pb)
        ic->pb->eof_reached = 0;

    if (input()->isSequential())
    {
        QMap<Qmmp::MetaData, QString> metaData;
        AVDictionaryEntry *album = av_dict_get(ic->metadata,"album",nullptr,0);
        if(!album)
            album = av_dict_get(ic->metadata,"WM/AlbumTitle",nullptr,0);
        AVDictionaryEntry *artist = av_dict_get(ic->metadata,"artist",nullptr,0);
        if(!artist)
            artist = av_dict_get(ic->metadata,"author",nullptr,0);
        AVDictionaryEntry *comment = av_dict_get(ic->metadata,"comment",nullptr,0);
        AVDictionaryEntry *genre = av_dict_get(ic->metadata,"genre",nullptr,0);
        AVDictionaryEntry *title = av_dict_get(ic->metadata,"title",nullptr,0);
        AVDictionaryEntry *year = av_dict_get(ic->metadata,"WM/Year",nullptr,0);
        if(!year)
            year = av_dict_get(ic->metadata,"year",nullptr,0);
        if(!year)
            year = av_dict_get(ic->metadata,"date",nullptr,0);
        AVDictionaryEntry *track = av_dict_get(ic->metadata,"track",nullptr,0);
        if(!track)
            track = av_dict_get(ic->metadata,"WM/Track",nullptr,0);
        if(!track)
            track = av_dict_get(ic->metadata,"WM/TrackNumber",nullptr,0);

        if(album)
            metaData.insert(Qmmp::ALBUM, QString::fromUtf8(album->value).trimmed());
        if(artist)
            metaData.insert(Qmmp::ARTIST, QString::fromUtf8(artist->value).trimmed());
        if(comment)
            metaData.insert(Qmmp::COMMENT, QString::fromUtf8(comment->value).trimmed());
        if(genre)
            metaData.insert(Qmmp::GENRE, QString::fromUtf8(genre->value).trimmed());
        if(title)
            metaData.insert(Qmmp::TITLE, QString::fromUtf8(title->value).trimmed());
        if(year)
            metaData.insert(Qmmp::YEAR, year->value);
        if(track)
            metaData.insert(Qmmp::TRACK, track->value);
        addMetaData(metaData);
    }

    //replay gain
    ReplayGainReader rg(ic);
    setReplayGainInfo(rg.replayGainInfo());

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,4,0)) //ffmpeg-3.1:  57.48.101
    c = avcodec_alloc_context3(nullptr);
#endif

    ic->flags |= AVFMT_FLAG_GENPTS;
    av_read_play(ic);
    audioIndex = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(audioIndex < 0)
    {
        qWarning("DecoderFFmpeg: unable to find audio stream");
        return false;
    }

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,48,0)) //ffmpeg-3.1:  57.48.101
    avcodec_parameters_to_context(c, ic->streams[audioIndex]->codecpar);
#else
    c = ic->streams[audioIndex]->codec;
#endif

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,34,0)) //libav-10: 55.34.1; ffmpeg-2.1:  55.39.100
    if (c->channels == 1)
    {
        c->request_channel_layout = AV_CH_LAYOUT_MONO;
        m_channels = c->channels;
    }
    else
    {
        c->request_channel_layout = AV_CH_LAYOUT_STEREO;
        m_channels = 2;
    }
#else
    if (c->channels > 0)
         c->request_channels = qMin(2, c->channels);
    else
         c->request_channels = 2;
    m_channels = c->request_channels;
#endif

    av_dump_format(ic,0,nullptr,0);

    AVCodec *codec = avcodec_find_decoder(c->codec_id);

    if (!codec)
    {
        qWarning("DecoderFFmpeg: unsupported codec for output stream");
        return false;
    }

    if (avcodec_open2(c, codec, nullptr) < 0)
    {
        qWarning("DecoderFFmpeg: error while opening codec for output stream");
        return false;
    }

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,34,0)) //libav-10: 55.34.1; ffmpeg-2.1:  55.39.100
    m_decoded_frame = av_frame_alloc();
#else
    m_decoded_frame = avcodec_alloc_frame();
#endif

    m_totalTime = input()->isSequential() ? 0 : ic->duration * 1000 / AV_TIME_BASE;

    if(c->codec_id == AV_CODEC_ID_SHORTEN) //ffmpeg bug workaround
        m_totalTime = 0;

    Qmmp::AudioFormat format = Qmmp::PCM_UNKNOWN;

    switch(c->sample_fmt)
    {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        format = Qmmp::PCM_U8;
        break;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        format = Qmmp::PCM_S16LE;
        break;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        format = Qmmp::PCM_S32LE;
        break;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        format = Qmmp::PCM_FLOAT;
        break;
    default:
        qWarning("DecoderFFmpeg: unsupported audio format");
        return false;
    }

    setProperty(Qmmp::FORMAT_NAME, QString::fromLatin1(avcodec_get_name(c->codec_id)));
    configure(c->sample_rate, m_channels, format);

    if(ic->bit_rate)
        m_bitrate = ic->bit_rate/1000;
    if(c->bit_rate)
        m_bitrate = c->bit_rate/1000;
    qDebug("DecoderFFmpeg: initialize succes");
    qDebug() << "DecoderFFmpeg: total time =" << m_totalTime;

    return true;
}

qint64 DecoderFFmpeg::totalTime() const
{
    return m_totalTime;
}

int DecoderFFmpeg::bitrate() const
{
    return m_bitrate;
}

qint64 DecoderFFmpeg::read(unsigned char *audio, qint64 maxSize)
{
    m_skipBytes = 0;

    if(!m_output_at)
        fillBuffer();

    if(!m_output_at)
        return 0;

    qint64 len = qMin(m_output_at, maxSize);

    if(av_sample_fmt_is_planar(c->sample_fmt) && m_channels > 1)
    {
        int bps = av_get_bytes_per_sample(c->sample_fmt);

        for(int i = 0; i < len / bps; i++)
        {
            memcpy(audio + i * bps, m_decoded_frame->extended_data[i % m_channels] + i / m_channels * bps, bps);
        }

        m_output_at -= len;
        for(int i = 0; i < m_channels; i++)
        {
            memmove(m_decoded_frame->extended_data[i],
                    m_decoded_frame->extended_data[i] + len/m_channels, m_output_at/m_channels);
        }
    }
    else
    {
        memcpy(audio, m_decoded_frame->extended_data[0], len);
        m_output_at -= len;
        memmove(m_decoded_frame->extended_data[0], m_decoded_frame->extended_data[0] + len, m_output_at);
    }

    return len;
}

qint64 DecoderFFmpeg::ffmpeg_decode()
{
    int out_size = 0;
    int got_frame = 0;
    if(m_pkt.stream_index == audioIndex)
    {

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,34,0)) //libav-10: 55.34.1; ffmpeg-2.1:  55.39.100

#else
        avcodec_get_frame_defaults(m_decoded_frame);
#endif

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,48,0)) //ffmpeg-3.1:  57.48.101
        int err = 0;
        if(m_temp_pkt.data)
            err = avcodec_send_packet(c, &m_temp_pkt);
        if(err != 0 && err != AVERROR(EAGAIN) && err != AVERROR(EINVAL))
        {
            qWarning("DecoderFFmpeg: avcodec_send_packet error: %d", err);
            return -1;
        }

        int l = (err == AVERROR(EAGAIN)) ? 0 : m_temp_pkt.size;

        if((err = avcodec_receive_frame(c, m_decoded_frame)) < 0)
        {
            if(err == AVERROR(EAGAIN)) //try again
                return 0;
            qWarning("DecoderFFmpeg: avcodec_receive_frame error: %d", err);
            return -1;
        }
        got_frame = m_decoded_frame->pkt_size;
#else
        int l = avcodec_decode_audio4(c, m_decoded_frame, &got_frame, &m_temp_pkt);
#endif

        if(got_frame)
            out_size = av_samples_get_buffer_size(nullptr, c->channels, m_decoded_frame->nb_samples,
                                                  c->sample_fmt, 1);
        else
            out_size = 0;

        if(c->bit_rate)
            m_bitrate = c->bit_rate/1000;
        if(l < 0)
        {
            return l;
        }
        m_temp_pkt.data += l;
        m_temp_pkt.size -= l;
    }
    if (!m_temp_pkt.size && m_pkt.data)
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,102)) //ffmpeg-3.0
        av_packet_unref(&m_pkt);
#else
        av_free_packet(&m_pkt);
#endif

    return out_size;
}

void DecoderFFmpeg::seek(qint64 pos)
{
    int64_t timestamp = int64_t(pos) * AV_TIME_BASE / 1000;
    if (ic->start_time != (qint64)AV_NOPTS_VALUE)
        timestamp += ic->start_time;
    m_seekTime = timestamp;
    av_seek_frame(ic, -1, timestamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(c);
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,102)) //ffmpeg-3.0
    av_packet_unref(&m_pkt);
#else
    av_free_packet(&m_pkt);
#endif
    m_temp_pkt.size = 0;
}

void DecoderFFmpeg::fillBuffer()
{
    while(!m_output_at || m_skipBytes > 0)
    {
        if(!m_temp_pkt.size)
        {
            if (av_read_frame(ic, &m_pkt) < 0)
            {
                m_temp_pkt.size = 0;
                m_temp_pkt.data = nullptr;
            }
            m_temp_pkt.size = m_pkt.size;
            m_temp_pkt.data = m_pkt.data;

            if(m_pkt.stream_index != audioIndex)
            {
                m_temp_pkt.size = 0;
                if(m_pkt.data)
                {
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,102)) //ffmpeg-3.0
                    av_packet_unref(&m_pkt);
#else
                    av_free_packet(&m_pkt);
#endif
                    continue;
                }
                return;
            }

            if(m_seekTime && c->codec_id == AV_CODEC_ID_APE)
            {
                int64_t rescaledPts = av_rescale(m_pkt.pts,
                                                 AV_TIME_BASE * (int64_t)
                                                 ic->streams[m_pkt.stream_index]->time_base.num,
                                                 ic->streams[m_pkt.stream_index]->time_base.den);
                m_skipBytes =  (m_seekTime - rescaledPts) * c->sample_rate * 4 / AV_TIME_BASE;
            }
            m_seekTime = 0;
        }

        if(m_skipBytes > 0 && c->codec_id == AV_CODEC_ID_APE)
        {
            while (m_skipBytes > 0)
            {
                m_output_at = ffmpeg_decode();
                if(m_output_at < 0)
                    break;
                m_skipBytes -= m_output_at;
            }

            if(m_skipBytes < 0)
            {
                qint64 size = m_output_at;
                m_output_at = - m_skipBytes;
                m_output_at = m_output_at - (m_output_at % 4);

                if(av_sample_fmt_is_planar(c->sample_fmt) && m_channels > 1)
                {
                    memmove(m_decoded_frame->extended_data[0],
                            m_decoded_frame->extended_data[0] + (size - m_output_at)/2, m_output_at/2);
                    memmove(m_decoded_frame->extended_data[1],
                            m_decoded_frame->extended_data[1] + (size - m_output_at)/2, m_output_at/2);
                }
                else
                {
                    memmove(m_decoded_frame->extended_data[0],
                            m_decoded_frame->extended_data[0] + size - m_output_at, m_output_at);
                }
                m_skipBytes = 0;
            }
        }
        else
            m_output_at = ffmpeg_decode();

        if(m_output_at < 0)
        {
            m_output_at = 0;
            m_temp_pkt.size = 0;

            if(c->codec_id == AV_CODEC_ID_SHORTEN || c->codec_id == AV_CODEC_ID_TWINVQ)
            {
                if(m_pkt.data)
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,102)) //ffmpeg-3.0
                    av_packet_unref(&m_pkt);
#else
                    av_free_packet(&m_pkt);
#endif
                m_pkt.data = nullptr;
                m_temp_pkt.size = 0;
                break;
            }
            continue;
        }
        else if(m_output_at == 0 && !m_pkt.data)
        {
            return;
        }
        else if(m_output_at == 0)
        {
            if(m_pkt.data)
#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,102)) //ffmpeg-3.0
                av_packet_unref(&m_pkt);
#else
                av_free_packet(&m_pkt);
#endif
            m_pkt.data = nullptr;
            m_temp_pkt.size = 0;
            continue;
        }
    }
}
