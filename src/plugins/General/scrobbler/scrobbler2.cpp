/***************************************************************************
 *   Copyright (C) 2010 by Ilya Kotov                                      *
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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QByteArray>
#include <QCryptographicHash>
#include <QUrl>
#include <QTime>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QDomDocument>
#include <QDesktopServices>
#include <QSettings>
#include <qmmp/soundcore.h>
#include <qmmp/qmmpsettings.h>
#include <qmmp/qmmp.h>
#include "scrobbler2.h"

#define API_KEY "d71c6f01b2ea562d7042bd5f5970041f"
#define SECRET "32d47bc0010473d40e1d38bdcff20968"

Scrobbler2::Scrobbler2(const QString &url, const QString &name, QObject *parent) : QObject(parent)
{
    m_getTokenReply = 0;
    m_getSessionReply = 0;
    m_http = new QNetworkAccessManager(this);
    m_state = Qmmp::Stopped;
    m_server = url;
    m_name = name;
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    m_session = settings.value("Scrobbler/lastfm_session").toString();
    connect(QmmpSettings::instance(), SIGNAL(networkSettingsChanged()), SLOT(setupProxy()));
    setupProxy();
    connect(m_http, SIGNAL(finished (QNetworkReply *)), SLOT(processResponse(QNetworkReply *)));
    m_core = SoundCore::instance();
    connect (m_core, SIGNAL(metaDataChanged()), SLOT(updateMetaData()));
    connect (m_core, SIGNAL(stateChanged (Qmmp::State)), SLOT(setState(Qmmp::State)));
    m_time = new QTime();
    m_submitedSongs = 0;
    m_submitReply = 0;
    m_notificationReply = 0;
    m_ua = QString("qmmp-plugins/%1").arg(Qmmp::strVersion().toLower()).toAscii();

    QFile file(QDir::homePath() +"/.qmmp/scrobbler_" + m_name + ".cache");

    if (file.open(QIODevice::ReadOnly))
    {
        int s;
        QString line, param, value;
        while (!file.atEnd())
        {
            line = QString::fromUtf8(file.readLine()).trimmed();
            if ((s = line.indexOf("=")) < 0)
                continue;

            param = line.left(s);
            value = line.right(line.size() - s - 1);

            if (param == "title")
            {
                m_songCache << SongInfo();
                m_songCache.last().setMetaData(Qmmp::TITLE, value);
            }
            else if (m_songCache.isEmpty())
                continue;
            else if (param == "artist")
                m_songCache.last().setMetaData(Qmmp::ARTIST, value);
            else if (param == "album")
                m_songCache.last().setMetaData(Qmmp::ALBUM, value);
            else if (param == "comment")
                m_songCache.last().setMetaData(Qmmp::COMMENT, value);
            else if (param == "genre")
                m_songCache.last().setMetaData(Qmmp::GENRE, value);
            else if (param == "year")
                m_songCache.last().setMetaData(Qmmp::YEAR, value);
            else if (param == "track")
                m_songCache.last().setMetaData(Qmmp::TRACK, value);
            else if (param == "length")
                m_songCache.last().setLength(value.toInt());
            else if (param == "time")
                m_songCache.last().setTimeStamp(value.toUInt());
        }
        file.close();
    }
    if(m_session.isEmpty())
        getToken();
}


Scrobbler2::~Scrobbler2()
{
    delete m_time;
    syncCache();
}

void Scrobbler2::setState(Qmmp::State state)
{
    m_state = state;
    switch ((uint) state)
    {
    case Qmmp::Playing:
        m_start_ts = QDateTime::currentDateTime().toTime_t();
        m_time->restart();
        break;
    case Qmmp::Stopped:
        if (!m_song.metaData().isEmpty()
            && ((m_time->elapsed ()/1000 > 240) || (m_time->elapsed ()/1000 > int(m_song.length()/2)))
            && (m_song.length() > MIN_SONG_LENGTH))
        {
            m_song.setTimeStamp(m_start_ts);
            m_songCache << m_song;
            syncCache();
        }

        m_song.clear();
        if (m_songCache.isEmpty())
            break;

        if (!m_session.isEmpty() && !m_submitReply)
            submit();
        break;
    default:
        ;
    }
}

void Scrobbler2::updateMetaData()
{
    QMap <Qmmp::MetaData, QString> metadata = m_core->metaData();
    if (m_state == Qmmp::Playing
            && !metadata.value(Qmmp::TITLE).isEmpty()      //skip empty tags
            && !metadata.value(Qmmp::ARTIST).isEmpty()
            && m_core->totalTime())                        //skip stream
    {
        m_song = SongInfo(metadata, m_core->totalTime()/1000);
        if (!m_session.isEmpty() && !m_notificationReply && !m_submitReply)
            sendNotification(m_song);
    }
}

void Scrobbler2::processResponse(QNetworkReply *reply)
{
    QString data = QString::fromUtf8(reply->readAll());
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning("Scrobbler2[%s]: http error: %s", qPrintable(m_name), qPrintable(reply->errorString()));
    }
    QDomDocument document;
    document.setContent(data);
    QDomElement root = document.documentElement();
    QString error_code;
    QString status = root.attribute("status");
    if(status != "ok" && !status.isEmpty())
    {
        QDomElement error = root.firstChildElement("error");
        if(!error.isNull())
        {
            qWarning("Scrobbler2[%s]: status=%s, %s-%s", qPrintable(m_name), qPrintable(status),
                     qPrintable(error.attribute("code")), qPrintable(error.text()));
            error_code = error.attribute("code");
        }
        else
            qWarning("Scrobbler2[%s]: invalid content", qPrintable(m_name));
    }

    data = data.trimmed();

    if (reply == m_getTokenReply)
    {
        m_getTokenReply = 0;
        if(status == "ok")
        {
            m_token = root.firstChildElement("token").text();
            qDebug("Scrobbler2[%s]: token: %s", qPrintable(m_name), qPrintable(m_token));
            QDesktopServices::openUrl("http://www.last.fm/api/auth/?api_key="API_KEY"&token="+m_token);
            QTimer::singleShot(120000, this, SLOT(getSession())); //2 minutes
        }
        else if(error_code == "8" || error_code == "7" ||  error_code == "11" || error_code.isEmpty())
        {
            m_token.clear();
            QTimer::singleShot(120000, this, SLOT(getToken())); // wait 2 minutes and try again
        }
        else
        {
            m_token.clear();
            qWarning("Scrobbler2[%s]: service returned unrecoverable error, scrobbling disabled",
                     qPrintable(m_name));
        }
    }
    else if(reply == m_getSessionReply)
    {
        m_getSessionReply = 0;
        m_session.clear();
        if(status == "ok")
        {
            QDomElement session = root.firstChildElement("session");
            m_session = session.firstChildElement("key").text();
            QString name = session.firstChildElement("name").text();
            QString subscriber = session.firstChildElement("subscriber").text();
            qDebug("Scrobbler2[%s]: name: %s", qPrintable(m_name),qPrintable(name));
            qDebug("Scrobbler2[%s]: key: %s", qPrintable(m_name), qPrintable(m_session));
            qDebug("Scrobbler2[%s]: subscriber: %s",qPrintable(m_name), qPrintable(subscriber));
            QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
            settings.setValue("Scrobbler/lastfm_session", m_session);
        }
        else if(error_code == "4" || error_code == "15") //invalid token
        {
            m_token.clear();
            getToken();
        }
        else if(error_code == "11") //service offline
        {
            QTimer::singleShot(120000, this, SLOT(getSession()));
        }
        else if(error_code == "14") // unauthorized token
        {
            QDesktopServices::openUrl("http://www.last.fm/api/auth/?api_key="API_KEY"&token="+m_token);
            QTimer::singleShot(120000, this, SLOT(getSession())); //2 minutes
        }
        else if (error_code.isEmpty()) //network error
        {
            QTimer::singleShot(120000, this, SLOT(getSession()));
        }
        else
        {
            m_token.clear();
            qWarning("Scrobbler2[%s]: service returned unrecoverable error, scrobbling disabled",
                     qPrintable(m_name));
        }
    }
    else if (reply == m_submitReply)
    {
        m_submitReply = 0;
        if (status == "ok")
        {
            qDebug("Scrobbler2[%s]: submited %d song(s)",qPrintable(m_name), m_submitedSongs);
            while (m_submitedSongs)
            {
                m_submitedSongs--;
                m_songCache.removeFirst ();
            }
            if (!m_songCache.isEmpty()) //submit remaining songs
            {
                submit();
            }
            else 
            {
                syncCache(); // update the cache file to reflect the empty cache
                updateMetaData();
            }
        }
        else if(error_code == "9") //invalid session key
        {
            m_session.clear();
            getToken();
        }
        else if(error_code == "11" || error_code == "16" || error_code.isEmpty()) //unavailable
        {
            QTimer::singleShot(120000, this, SLOT(submit()));
        }
        else
        {
            m_session.clear();
            qWarning("Scrobbler2[%s]: service returned unrecoverable error, scrobbling disabled",
                     qPrintable(m_name));
        }
    }
    else if (reply == m_notificationReply)
    {
        m_notificationReply = 0;
        if(status == "ok")
        {
            qDebug("Scrobbler2[%s]: Now-Playing notification done", qPrintable(m_name));
        }
        else if(error_code == "9") //invalid session key
        {
            m_session.clear();
            getToken();
        }
    }
    reply->deleteLater();
}

void Scrobbler2::setupProxy()
{
    QmmpSettings *gs = QmmpSettings::instance();
    if (gs->isProxyEnabled())
    {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, gs->proxy().host(),  gs->proxy().port());
        if(gs->useProxyAuth())
        {
            proxy.setUser(gs->proxy().userName());
            proxy.setPassword(gs->proxy().password());
        }
        m_http->setProxy(proxy);
    }
    else
        m_http->setProxy(QNetworkProxy::NoProxy);
}

void Scrobbler2::getToken()
{
    qDebug("%s", Q_FUNC_INFO);
    QUrl url(QString("http://") + m_server + "/?");
    url.setPort(80);
    url.addQueryItem("method", "auth.getToken");
    url.addQueryItem("api_key", API_KEY);

    QByteArray data;
    data.append("api_key"API_KEY);
    data.append("methodauth.getToken");
    data.append(SECRET);
    url.addQueryItem("api_sig", QCryptographicHash::hash(data,QCryptographicHash::Md5).toHex());

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent",  m_ua);
    request.setRawHeader("Host",url.host().toAscii());
    request.setRawHeader("Accept", "*/*");
    m_getTokenReply = m_http->get(request);
}

void Scrobbler2::getSession()
{
    qDebug("%s", Q_FUNC_INFO);
    QUrl url(QString("http://") + m_server + "/?");
    url.setPort(80);
    url.addQueryItem("api_key", API_KEY);
    url.addQueryItem("method", "auth.getSession");
    url.addQueryItem("token", m_token);

    QByteArray data;
    data.append("api_key"API_KEY);
    data.append("methodauth.getSession");
    data.append("token" + m_token.toUtf8());
    data.append(SECRET);
    url.addQueryItem("api_sig", QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent",  m_ua);
    request.setRawHeader("Host",url.host().toAscii());
    request.setRawHeader("Accept", "*/*");
    m_getSessionReply = m_http->get(request);
}

void Scrobbler2::submit()
{
    qDebug("Scrobbler2[%s]: submit request", qPrintable(m_name));
    if (m_songCache.isEmpty() || m_session.isEmpty())
        return;
    m_submitedSongs = qMin(m_songCache.size(),25);

    QMap <QString, QString> params;
    for (int i = 0; i < m_submitedSongs; ++i)
    {
        SongInfo info = m_songCache[i];
        params.insert(QString("track[%1]").arg(i),info.metaData(Qmmp::TITLE));
        params.insert(QString("timestamp[%1]").arg(i),QString("%1").arg(info.timeStamp()));
        params.insert(QString("artist[%1]").arg(i),info.metaData(Qmmp::ARTIST));
        if(!info.metaData(Qmmp::ALBUM).isEmpty())
            params.insert(QString("album[%1]").arg(i),info.metaData(Qmmp::ALBUM));
        if(!info.metaData(Qmmp::TRACK).isEmpty())
            params.insert(QString("trackNumber[%1]").arg(i),info.metaData(Qmmp::TRACK));
        params.insert(QString("duration[%1]").arg(i),QString("%1").arg(info.length()));
    }
    params.insert("api_key", API_KEY);
    params.insert("method", "track.scrobble");
    params.insert("sk", m_session);

    foreach (QString key, params) //removes empty keys
    {
        if(params.value(key).isEmpty())
            params.remove(key);
    }

    QUrl url(QString("http://") + m_server + "/");
    url.setPort(80);

    QUrl body("");
    QByteArray data;
    foreach (QString key, params.keys())
    {
        body.addQueryItem(key, params.value(key));
        data.append(key.toUtf8() + params.value(key).toUtf8());
    }
    data.append(SECRET);
    body.addQueryItem("api_sig", QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
    QByteArray bodyData =  body.toEncoded().remove(0,1);
    bodyData.replace("+", QUrl::toPercentEncoding("+"));

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_ua);
    request.setRawHeader("Host", url.host().toAscii());
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader, bodyData.size());
    m_submitReply = m_http->post(request, bodyData);
}

void Scrobbler2::sendNotification(const SongInfo &info)
{
    if(m_session.isEmpty())
        return;
    qDebug("Scrobbler2[%s] sending notification", qPrintable(m_name));

    QMap <QString, QString> params;
    params.insert("track", info.metaData(Qmmp::TITLE));
    params.insert("artist", info.metaData(Qmmp::ARTIST));
    if(!info.metaData(Qmmp::ALBUM).isEmpty())
        params.insert("album", info.metaData(Qmmp::ALBUM));
    if(!info.metaData(Qmmp::TRACK).isEmpty())
        params.insert("trackNumber", info.metaData(Qmmp::TRACK));
    params.insert("duration", QString("%1").arg(info.length()));
    params.insert("api_key", API_KEY);
    params.insert("method", "track.updateNowPlaying");
    params.insert("sk", m_session);

    foreach (QString key, params) //removes empty keys
    {
        if(params.value(key).isEmpty())
            params.remove(key);
    }

    QUrl url(QString("http://") + m_server + "/");
    url.setPort(80);

    QUrl body("");
    QByteArray data;
    foreach (QString key, params.keys())
    {
        body.addQueryItem(key, params.value(key));
        data.append(key.toUtf8() + params.value(key).toUtf8());
    }
    data.append(SECRET);
    body.addQueryItem("api_sig", QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
    QByteArray bodyData =  body.toEncoded().remove(0,1);
    bodyData.replace("+", QUrl::toPercentEncoding("+"));

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_ua);
    request.setRawHeader("Host", url.host().toAscii());
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader,  bodyData.size());
    m_notificationReply = m_http->post(request, bodyData);
}

void Scrobbler2::syncCache()
{
    QFile file(QDir::homePath() +"/.qmmp/scrobbler_" + m_name + ".cache");
    if (m_songCache.isEmpty())
    {
        file.remove();
        return;
    }
    file.open(QIODevice::WriteOnly);
    foreach(SongInfo m, m_songCache)
    {
        file.write(QString("title=%1").arg(m.metaData(Qmmp::TITLE)).toUtf8() +"\n");
        file.write(QString("artist=%1").arg(m.metaData(Qmmp::ARTIST)).toUtf8() +"\n");
        file.write(QString("album=%1").arg(m.metaData(Qmmp::ALBUM)).toUtf8() +"\n");
        file.write(QString("comment=%1").arg(m.metaData(Qmmp::COMMENT)).toUtf8() +"\n");
        file.write(QString("genre=%1").arg(m.metaData(Qmmp::GENRE)).toUtf8() +"\n");
        file.write(QString("year=%1").arg(m.metaData(Qmmp::YEAR)).toUtf8() +"\n");
        file.write(QString("track=%1").arg(m.metaData(Qmmp::TRACK)).toUtf8() +"\n");
        file.write(QString("length=%1").arg(m.length()).toUtf8() +"\n");
        file.write(QString("time=%1").arg(m.timeStamp()).toUtf8() +"\n");
    }
    file.close();
}
