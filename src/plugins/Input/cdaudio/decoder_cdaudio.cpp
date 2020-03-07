/***************************************************************************
 *   Copyright (C) 2009-2020 by Ilya Kotov                                 *
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
#include <QRegExp>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <cdio/cdio.h>
#if LIBCDIO_VERSION_NUM <= 83
#include <cdio/cdda.h>
#else
#include <cdio/paranoia/cdda.h>
#endif
#include <cdio/audio.h>
#include <cdio/cd_types.h>
#include <cdio/logging.h>
#include <qmmp/buffer.h>
#include <qmmp/output.h>
#include <qmmp/qmmpsettings.h>

#define CDDA_SECTORS 4
#define CDDA_BUFFER_SIZE (CDDA_SECTORS*CDIO_CD_FRAMESIZE_RAW)

#include "decoder_cdaudio.h"

QList <CDATrack> DecoderCDAudio::m_track_cache;

static void log_handler (cdio_log_level_t level, const char *message)
{
    QString str = QString::fromLocal8Bit(message).trimmed();
    switch (level)
    {
    case CDIO_LOG_DEBUG:
        qDebug("DecoderCDAudio: cdio message: %s (level=debug)", qPrintable(str));
        return;
    case CDIO_LOG_INFO:
        qDebug("DecoderCDAudio: cdio message: %s (level=info)", qPrintable(str));
        return;
    default:
        qWarning("DecoderCDAudio: cdio message: %s (level=error)", qPrintable(str));
    }
}

// Decoder class

DecoderCDAudio::DecoderCDAudio(const QString &url) : Decoder()
{
    m_bitrate = 0;
    m_totalTime = 0;
    m_first_sector = -1;
    m_last_sector  = -1;
    m_current_sector  = -1;
    m_url = url;
    m_cdio = nullptr;
    m_buffer_at = 0;
    m_buffer = new char[CDDA_BUFFER_SIZE];
}

DecoderCDAudio::~DecoderCDAudio()
{
    m_bitrate = 0;
    if (m_cdio)
    {
        cdio_destroy(m_cdio);
        m_cdio = nullptr;
    }
    delete [] m_buffer;
}

QList<CDATrack> DecoderCDAudio::generateTrackList(const QString &device, TrackInfo::Parts parts)
{
    //read settings
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    int cd_speed = settings.value("cdaudio/speed", 0).toInt();
    bool use_cd_text = settings.value("cdaudio/cdtext", true).toBool();
    QList <CDATrack> tracks;
    cdio_log_set_handler(log_handler); //setup cdio log handler
    CdIo_t *cdio = nullptr;
    QString device_path = device;
    if (device_path.isEmpty() || device_path == "/")
        device_path = settings.value("cdaudio/device").toString();
    if (device_path.isEmpty() || device_path == "/")
    {
        char **cd_drives = cdio_get_devices_with_cap(nullptr, CDIO_FS_AUDIO, true); //get drive list with CDA disks
        // open first audio capable cd drive
        if (cd_drives && *cd_drives)
        {
            cdio = cdio_open_cd(*cd_drives);
            if (!cdio)
            {
                qWarning("DecoderCDAudio: failed to open CD.");
                cdio_free_device_list(cd_drives);
                return tracks;
            }
            qDebug("DecoderCDAudio: found cd audio capable drive \"%s\"", *cd_drives);
            device_path = QString(*cd_drives);
            cdio_free_device_list(cd_drives); //free device list
        }
        else
        {
            qWarning("DecoderCDAudio: unable to find cd audio drive.");
            cdio_free_device_list(cd_drives);
            return tracks;
        }
    }
    else
    {
        cdio = cdio_open_cd(device_path.toLatin1().constData());
        if (!cdio)
        {
            qWarning("DecoderCDAudio: failed to open CD.");
            return tracks;
        }
        qDebug("DecoderCDAudio: using cd audio capable drive \"%s\"", qPrintable(device_path));
    }

    if(!m_track_cache.isEmpty() && !cdio_get_media_changed(cdio))
    {
        qDebug("DecoderCDAudio: using track cache...");
        cdio_destroy(cdio);
        return m_track_cache;
    }

    if (cd_speed)
    {
        qDebug("DecoderCDAudio: setting drive speed to %dX.", cd_speed);
        if (cdio_set_speed(cdio, 1) != DRIVER_OP_SUCCESS)
            qWarning("DecoderCDAudio: unable to set drive speed to %dX.", cd_speed);
    }

    cdrom_drive_t *pcdrom_drive = cdio_cddap_identify_cdio(cdio, 1, nullptr); //create paranoya CD-ROM object
    //get first and last track numbers
    int first_track_number = cdio_get_first_track_num(pcdrom_drive->p_cdio);
    int last_track_number = cdio_get_last_track_num(pcdrom_drive->p_cdio);

    if ((first_track_number == CDIO_INVALID_TRACK) || (last_track_number == CDIO_INVALID_TRACK))
    {
        qWarning("DecoderCDAudio: invalid first (last) track number.");
        cdio_destroy(cdio);
        cdio = nullptr;
        return tracks;
    }
    //fill track list
    for (int i = first_track_number; i <= last_track_number; ++i)
    {
        CDATrack t;
        t.first_sector = cdio_get_track_lsn(pcdrom_drive->p_cdio, i);
        t.last_sector = cdio_get_track_last_lsn(pcdrom_drive->p_cdio, i);
        t.info.setDuration((t.last_sector - t.first_sector +1) * 1000 / 75);
        t.info.setValue(Qmmp::TRACK, i);
        t.info.setPath(QString("cdda://%1#%2").arg(device_path).arg(i));

        if(parts & TrackInfo::Properties)
        {
            t.info.setValue(Qmmp::BITRATE, 1411);
            t.info.setValue(Qmmp::SAMPLERATE, 44100);
            t.info.setValue(Qmmp::CHANNELS, 2);
            t.info.setValue(Qmmp::BITS_PER_SAMPLE, 16);
            t.info.setValue(Qmmp::FORMAT_NAME, "CDDA");
        }

        if ((t.first_sector == CDIO_INVALID_LSN) || (t.last_sector== CDIO_INVALID_LSN))
        {
            qWarning("DecoderCDAudio: invalid stard(end) lsn for the track %d.", i);
            tracks.clear();
            cdio_destroy(cdio);
            cdio = nullptr;
            return tracks;
        }
        //cd text
#if LIBCDIO_VERSION_NUM <= 83
        cdtext_t *cdtext = use_cd_text ? cdio_get_cdtext(pcdrom_drive->p_cdio, i) : nullptr;
        if (cdtext && cdtext->field[CDTEXT_TITLE])
        {
            t.info.setValue(Qmmp::TITLE, QString::fromLocal8Bit(cdtext->field[CDTEXT_TITLE]));
            t.info.setValue(Qmmp::ARTIST, QString::fromLocal8Bit(cdtext->field[CDTEXT_PERFORMER]));
            t.info.setValue(Qmmp::GENRE, QString::fromLocal8Bit(cdtext->field[CDTEXT_GENRE]));
            use_cddb = false;
        }
#else
        cdtext_t *cdtext = use_cd_text ? cdio_get_cdtext(pcdrom_drive->p_cdio) : nullptr;
        if (cdtext)
        {
            t.info.setValue(Qmmp::TITLE, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_TITLE,i)));
            t.info.setValue(Qmmp::ARTIST, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_PERFORMER,i)));
            t.info.setValue(Qmmp::GENRE, QString::fromUtf8(cdtext_get_const(cdtext,CDTEXT_FIELD_GENRE,i)));
        }
#endif
        else
            t.info.setValue(Qmmp::TITLE, QString("CDA Track %1").arg(i, 2, 10, QChar('0')));
        tracks  << t;
    }
    qDebug("DecoderCDAudio: found %d audio tracks", tracks.size());

    cdio_destroy(cdio);
    cdio = nullptr;
    m_track_cache = tracks;
    return tracks;
}

void DecoderCDAudio::saveToCache(QList <CDATrack> tracks,  uint disc_id)
{
    QDir dir(Qmmp::configDir());
    if(!dir.exists("cddbcache"))
        dir.mkdir("cddbcache");
    dir.cd("cddbcache");
    QString path = dir.absolutePath() + QString("/%1").arg(disc_id, 0, 16);
    QSettings settings(path, QSettings::IniFormat);
    settings.clear();
    settings.setValue("count", tracks.size());
    for(int i = 0; i < tracks.size(); ++i)
    {
        CDATrack track = tracks[i];
        QMap<Qmmp::MetaData, QString> meta = track.info.metaData();
        settings.setValue(QString("artist%1").arg(i), meta[Qmmp::ARTIST]);
        settings.setValue(QString("title%1").arg(i), meta[Qmmp::TITLE]);
        settings.setValue(QString("genre%1").arg(i), meta[Qmmp::GENRE]);
        settings.setValue(QString("album%1").arg(i), meta[Qmmp::ALBUM]);
    }
}

bool DecoderCDAudio::readFromCache(QList <CDATrack> *tracks, uint disc_id)
{
    QString path = Qmmp::configDir();
    path += QString("/cddbcache/%1").arg(disc_id, 0, 16);
    if(!QFile::exists(path))
        return false;
    QSettings settings(path, QSettings::IniFormat);
    int count = settings.value("count").toInt();
    if(count != tracks->count())
        return false;
    for(int i = 0; i < count; ++i)
    {
        (*tracks)[i].info.setValue(Qmmp::ARTIST, settings.value(QString("artist%1").arg(i)).toString());
        (*tracks)[i].info.setValue(Qmmp::TITLE, settings.value(QString("title%1").arg(i)).toString());
        (*tracks)[i].info.setValue(Qmmp::GENRE, settings.value(QString("genre%1").arg(i)).toString());
        (*tracks)[i].info.setValue(Qmmp::ALBUM, settings.value(QString("album%1").arg(i)).toString());
    }
    return true;
}

qint64 DecoderCDAudio::calculateTrackLength(lsn_t startlsn, lsn_t endlsn)
{
    return ((endlsn - startlsn + 1) * 1000) / 75;
}

void DecoderCDAudio::clearTrackCache()
{
    m_track_cache.clear();
}

bool DecoderCDAudio::initialize()
{
    m_bitrate = 0;
    m_totalTime = 0;
    //extract track from url
    int track_number = m_url.section("#", -1).toInt();
    QString device_path = m_url;
    device_path.remove("cdda://");
    device_path.remove(QRegExp("#\\d+$"));

    track_number = qMax(track_number, 1);
    QList <CDATrack> tracks = DecoderCDAudio::generateTrackList(device_path); //generate track list
    if (tracks.isEmpty())
    {
        qWarning("DecoderCDAudio: initialize failed");
        return false;
    }
    //find track by number
    int track_at = -1;
    for (int i = 0; i < tracks.size(); ++i)
        if (tracks[i].info.value(Qmmp::TRACK).toInt() == track_number)
        {
            track_at = i;
            break;
        }
    if (track_at < 0)
    {
        qWarning("DecoderCDAudio: invalid track number");
        return false;
    }

    if (device_path.isEmpty() || device_path == "/") //try default path from config
    {
        QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
        device_path = settings.value("cdaudio/device").toString();
        m_url = QString("cdda://%1#%2").arg(device_path).arg(track_number);
    }

    if (device_path.isEmpty() || device_path == "/")
    {
        char **cd_drives = cdio_get_devices_with_cap(nullptr, CDIO_FS_AUDIO, true); //get drive list with CDA disks
        // open first audio capable cd drive
        if (cd_drives && *cd_drives)
        {
            m_cdio = cdio_open_cd(*cd_drives);
            if (!m_cdio)
            {
                qWarning("DecoderCDAudio: failed to open CD.");
                cdio_free_device_list(cd_drives);
                return false;
            }
            qDebug("DecoderCDAudio: found cd audio capable drive \"%s\"", *cd_drives);
            cdio_free_device_list(cd_drives);  //free device list
        }
        else
        {
            qWarning("DecoderCDAudio: unable to find cd audio drive.");
            return false;
        }
    }
    else
    {
        m_cdio = cdio_open_cd(device_path.toLatin1().constData());
        if (!m_cdio)
        {
            qWarning("DecoderCDAudio: failed to open CD.");
            return false;
        }
        qDebug("DecoderCDAudio: using cd audio capable drive \"%s\"", qPrintable(device_path));
    }
    configure(44100, 2, Qmmp::PCM_S16LE);
    m_bitrate = 1411;
    m_totalTime = tracks[track_at].info.duration();
    m_first_sector = tracks[track_at].first_sector;
    m_current_sector = tracks[track_at].first_sector;
    m_last_sector = tracks[track_at].last_sector;
    addMetaData(tracks[track_at].info.metaData()); //send metadata
    setProperty(Qmmp::FORMAT_NAME, "CDDA");
    setProperty(Qmmp::BITRATE, m_bitrate);
    qDebug("DecoderCDAudio: initialize succes");
    return true;
}


qint64 DecoderCDAudio::totalTime() const
{
    return m_totalTime;
}

int DecoderCDAudio::bitrate() const
{
    return m_bitrate;
}

qint64 DecoderCDAudio::read(unsigned char *audio, qint64 maxSize)
{
    if(!m_buffer_at)
    {

        lsn_t secorts_to_read = qMin(CDDA_SECTORS, (m_last_sector - m_current_sector + 1));

        if (secorts_to_read <= 0)
            return 0;

        if (cdio_read_audio_sectors(m_cdio, m_buffer,
                                    m_current_sector, secorts_to_read) != DRIVER_OP_SUCCESS)
        {
            m_buffer_at = 0;
            return -1;
        }
        else
        {
            m_buffer_at = secorts_to_read * CDIO_CD_FRAMESIZE_RAW;
            m_current_sector += secorts_to_read;
        }
    }

    if(m_buffer_at > 0)
    {
        long len = qMin(maxSize, m_buffer_at);
        memcpy(audio, m_buffer, len);
        m_buffer_at -= len;
        memmove(m_buffer, m_buffer + len, m_buffer_at);
        return len;
    }
    return 0;
}

void DecoderCDAudio::seek(qint64 pos)
{
    m_current_sector = m_first_sector + pos * 75 / 1000;
    m_buffer_at = 0;
}
