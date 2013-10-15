/***************************************************************************
 *   Copyright (C) 2011-2013 by Ilya Kotov                                 *
 *   forkotov02@hotmail.ru                                                 *
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

#include <stdio.h>
#include <QStringList>
#include <QDesktopServices>
#include <qmmp/inputsourcefactory.h>
#include <qmmp/decoderfactory.h>
#include <qmmp/metadatamanager.h>
#include <qmmpui/metadataformatter.h>
#include <QtEndian>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include "converter.h"

#define QStringToTString_qt4(s) TagLib::String(s.toUtf8().constData(), TagLib::String::UTF8)

//static functions
static inline void s8_to_s16_2(qint8 *in, qint16 *out, qint64 samples)
{
    for(qint64 i = 0; i < samples; ++i)
        out[i] = in[i] << 8;
    return;
}

static inline void s24_to_s16_2(qint32 *in, qint16 *out, qint64 samples)
{
    for(qint64 i = 0; i < samples; ++i)
        out[i] = in[i] >> 8;
    return;
}

static inline void s32_to_s16_2(qint32 *in, qint16 *out, qint64 samples)
{
    for(qint64 i = 0; i < samples; ++i)
        out[i] = in[i] >> 16;
    return;
}

Converter::Converter(QObject *parent) : QThread(parent)
{}

Converter::~Converter()
{
    stop();
}

void Converter::add(const QStringList &urls, const QVariantMap &preset)
{
    foreach(QString url, urls)
        add(url, preset);
}

void Converter::add(const QString &url, const QVariantMap &preset)
{
    InputSource *source = InputSource::create(url, this);
    if(!source->initialize())
    {
        delete source;
        qWarning("Converter: Invalid url");
        return;
    }

    if(source->ioDevice())
    {
        if(!source->ioDevice()->open(QIODevice::ReadOnly))
        {
            source->deleteLater();
            qWarning("Converter: cannot open input stream, error: %s",
                     qPrintable(source->ioDevice()->errorString()));
            return;
        }
    }

    DecoderFactory *factory = 0;

    if(!source->url().contains("://"))
        factory = Decoder::findByPath(source->url());
    if(!factory)
        factory = Decoder::findByMime(source->contentType());
    if(!factory && source->ioDevice() && source->url().contains("://")) //ignore content of local files
        factory = Decoder::findByContent(source->ioDevice());
    if(!factory && source->url().contains("://"))
        factory = Decoder::findByProtocol(source->url().section("://",0,0));
    if(!factory)
    {
        qWarning("Converter: unsupported file format");
        source->deleteLater();
        return;
    }
    qDebug("Converter: selected decoder: %s",qPrintable(factory->properties().shortName));
    if(factory->properties().noInput && source->ioDevice())
        source->ioDevice()->close();
    Decoder *decoder = factory->create(source->url(), source->ioDevice());
    if(!decoder->initialize())
    {
        qWarning("Converter: invalid file format");
        source->deleteLater();
        delete decoder;
        return;
    }
    m_decoders.enqueue(decoder);
    m_inputs.insert(decoder, source);
    m_presets.insert(decoder, preset);
    if(!decoder->totalTime())
        source->setOffset(-1);
    source->setParent(this);
}

void Converter::stop()
{
    m_mutex.lock();
    m_user_stop = true;
    m_mutex.unlock();
    wait();
    m_presets.clear();
    qDeleteAll(m_inputs.values());
    m_inputs.clear();
    qDeleteAll(m_decoders);
    m_decoders.clear();
}

void Converter::run()
{
    qDebug("Converter: staring thread");
    m_user_stop = false;
    MetaDataFormatter desc_formatter("%p%if(%p&%t, - ,)%t [%l]");

    while(!m_decoders.isEmpty())
    {
        Decoder *decoder = m_decoders.dequeue();
        QVariantMap preset = m_presets.take(decoder);
        AudioParameters ap = decoder->audioParameters();
        QString url = m_inputs[decoder]->url();
        QString out_path = preset["out_dir"].toString();
        QString pattern = preset["file_name"].toString();

        QList <FileInfo *> list = MetaDataManager::instance()->createPlayList(url);

        if(list.isEmpty() || out_path.isEmpty() || pattern.isEmpty())
        {
            qWarning("Converter: invalid parameters");
            m_inputs.take(decoder)->deleteLater();
            delete decoder;
            continue;
        }

        QMap<Qmmp::MetaData, QString> metadata = list[0]->metaData();
        MetaDataFormatter formatter(pattern);

        QString desc = tr("Track: %1").arg(desc_formatter.parse(list[0]->metaData(), list[0]->length()));
        desc += "\n";
        desc += tr("Preset: %1").arg(preset["name"].toString());
        emit desriptionChanged(desc);

        QString name = formatter.parse(list[0]->metaData(), list[0]->length());
        QString full_path = out_path + "/" + name + "." + preset["ext"].toString();

        if(QFile::exists(full_path))
        {
            if(preset["overwrite"].toBool()) //remove previous file
                QFile::remove(full_path);
            else
            {
                int i = 0;
                while(QFile::exists(full_path)) //create file with another name
                {
                    ++i;
                    full_path = out_path + "/" + name + QString("_%1.").arg(i) + preset["ext"].toString();
                }
            }
        }

        QString command = preset["command"].toString();
        command.replace("%o", "\"" + full_path + "\"");
        QString tmp_path = QDesktopServices::storageLocation(QDesktopServices::TempLocation) + "/tmp.wav";
        bool use_file = command.contains("%i");
        command.replace("%i", "\"" + tmp_path + "\"");

        qDebug("Converter: starting task '%s'", qPrintable(preset["name"].toString()));

        qDeleteAll(list);
        list.clear();

        char wave_header[] = { 0x52, 0x49, 0x46, 0x46, //"RIFF"
                               0x00, 0x00, 0x00, 0x00, //(file size) - 8
                               0x57, 0x41, 0x56, 0x45, //WAVE
                               0x66, 0x6d, 0x74, 0x20, //"fmt "
                               0x10, 0x00, 0x00, 0x00, //16 + extra format bytes
                               0x01, 0x00, 0x02, 0x00, //PCM/uncompressed, channels
                               0x00, 0x00, 0x00, 0x00, //sample rate
                               0x00, 0x00, 0x00, 0x00, //average bytes per second
                               0x04, 0x00, 0x10, 0x00, //block align, significant bits per sample
                               0x64, 0x61, 0x74, 0x61, //"data"
                               0x00, 0x00, 0x00, 0x00 }; //chunk size*/

        quint16 sample_size = preset["use_16bit"].toBool() ? 2 : ap.sampleSize();
        quint32 sample_rate = qToLittleEndian(ap.sampleRate());
        quint16 channels = qToLittleEndian((quint16)ap.channels());
        quint16 block_align = qToLittleEndian((quint16)sample_size * ap.channels());
        quint16 bps = qToLittleEndian((quint16)sample_size * 8);
        quint32 size = decoder->totalTime() * ap.sampleRate() * ap.channels() * sample_size / 1000;
        size = qToLittleEndian(size);

        memcpy(&wave_header[22], &channels, 2);
        memcpy(&wave_header[24], &sample_rate, 4);
        memcpy(&wave_header[32], &block_align, 2);
        memcpy(&wave_header[34], &bps, 2);
        memcpy(&wave_header[40], &size, 4);

        FILE *enc_pipe = use_file ? fopen(qPrintable(tmp_path), "w+b") : popen(qPrintable(command), "w");
        if(!enc_pipe)
        {
            qWarning("Converter: unable to open pipe");
            m_inputs.take(decoder)->deleteLater();
            delete decoder;
            continue;

        }
        size_t to_write = sizeof(wave_header);
        if(to_write != fwrite(&wave_header, 1, to_write, enc_pipe))
        {
            qWarning("Converter: output file write error");
            m_inputs.take(decoder)->deleteLater();
            delete decoder;
            use_file ? fclose(enc_pipe) : pclose(enc_pipe);
            continue;
        }

        convert(decoder, enc_pipe, preset["use_16bit"].toBool());
        use_file ? fclose(enc_pipe) : pclose(enc_pipe);
        m_inputs.take(decoder)->deleteLater();
        delete decoder;
        m_mutex.lock();
        if(m_user_stop)
        {
            qDebug("Converter: task '%s' aborted", qPrintable(preset["name"].toString()));
            m_mutex.unlock();
            return;
        }
        else
            qDebug("Converter: task '%s' finished with success", qPrintable(preset["name"].toString()));
        m_mutex.unlock();

        if(use_file)
        {
            qDebug("Converter: starting file encoding...");
            emit desriptionChanged(tr("Encoding..."));
            enc_pipe = popen(qPrintable(command), "w");
            if(!enc_pipe)
            {
                qWarning("Converter: unable to start encoder");
                QFile::remove(tmp_path);
                continue;
            }
            pclose(enc_pipe);
            qDebug("Converter: encoding finished");
            QFile::remove(tmp_path);
        }

        if(preset["tags"].toBool())
        {
            qDebug("Converter: writing tags");
            TagLib::FileRef file(qPrintable(full_path));
            if(file.tag())
            {
                file.tag()->setTitle(QStringToTString_qt4(metadata[Qmmp::TITLE]));
                file.tag()->setArtist(QStringToTString_qt4(metadata[Qmmp::ARTIST]));
                file.tag()->setAlbum(QStringToTString_qt4(metadata[Qmmp::ALBUM]));
                file.tag()->setGenre(QStringToTString_qt4(metadata[Qmmp::GENRE]));
                file.tag()->setComment(QStringToTString_qt4(metadata[Qmmp::COMMENT]));
                file.tag()->setYear(metadata[Qmmp::YEAR].toUInt());
                file.tag()->setTrack(metadata[Qmmp::TRACK].toUInt());

                if(full_path.endsWith(".mp3"), Qt::CaseInsensitive)
                {
                    TagLib::MPEG::File *mpeg_file = dynamic_cast <TagLib::MPEG::File *> (file.file());
                    mpeg_file->save(TagLib::MPEG::File::ID3v2, true);
                }
                else
                    file.save();
            }
        }
    }
    qDebug("Converter: thread finished");
}

bool Converter::convert(Decoder *decoder, FILE *file, bool use16bit)
{
    const int buf_size = 8192;
    AudioParameters ap = decoder->audioParameters();
    Qmmp::AudioFormat format = ap.format();
    unsigned char output_buf[(use16bit && format == Qmmp::PCM_S8) ? buf_size : buf_size * 2];
    qint64 output_at = 0;
    qint64 total = 0;
    quint64 len = 0;
    qint64 totalSize = decoder->totalTime() * ap.sampleRate() * ap.channels() * ap.sampleSize() / 1000;

    int percent = 0;
    int prev_percent = 0;
    forever
    {
        // decode
        len = decoder->read((char *)(output_buf + output_at), buf_size - output_at);

        if (len > 0)
        {
            output_at += len;
            total += len;

            if(use16bit) //TODO dithering support
            {
                if(format == Qmmp::PCM_S8)
                {
                    s8_to_s16_2((qint8 *)output_buf, (qint16 *)output_buf, output_at);
                    output_at <<= 1;
                }
                else if(format == Qmmp::PCM_S24LE)
                {
                    s24_to_s16_2((qint32 *)output_buf, (qint16 *)output_buf, output_at >> 2);
                    output_at >>= 1;
                }
                else if(format == Qmmp::PCM_S32LE)
                {
                    s32_to_s16_2((qint32 *)output_buf, (qint16 *)output_buf, output_at >> 2);
                    output_at >>= 1;
                }
            }
            while(output_at > 0)
            {
                len = fwrite(output_buf, 1, output_at, file);
                if(len == 0)
                {
                    qWarning("Converter: error");
                    return false;
                }
                output_at -= len;
                memmove(output_buf, output_buf + len, output_at);
            }
            percent = 100 * total / totalSize;
            if(percent != prev_percent)
            {
                prev_percent = percent;
                emit progress(percent);
            }
        }
        else if (len <= 0)
        {
            qDebug("Converter: total written: %lld bytes", total);
            qDebug("finished!");
            return true;
        }

        m_mutex.lock();
        if(m_user_stop)
        {
            m_mutex.unlock();
            return false;
        }
        m_mutex.unlock();
    }
    return false;
}
