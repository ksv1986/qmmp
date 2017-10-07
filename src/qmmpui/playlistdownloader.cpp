/***************************************************************************
 *   Copyright (C) 2012-2017 by Ilya Kotov                                 *
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

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <qmmp/qmmp.h>
#include <qmmp/qmmpsettings.h>
#include "playlistparser.h"
#include "playlistdownloader.h"

PlayListDownloader::PlayListDownloader(QObject *parent) : QObject(parent)
{
    m_getReply = 0;
    m_ua = QString("qmmp/%1").arg(Qmmp::strVersion()).toLatin1();
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, SIGNAL(finished (QNetworkReply *)), SLOT(readResponse(QNetworkReply *)));
    //load global proxy settings
    QmmpSettings *gs = QmmpSettings::instance();
    if (gs->isProxyEnabled())
    {
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, gs->proxy().host(),  gs->proxy().port());
        if(gs->useProxyAuth())
        {
            proxy.setUser(gs->proxy().userName());
            proxy.setPassword(gs->proxy().password());
        }
        m_manager->setProxy(proxy);
    }
}

void PlayListDownloader::start(const QUrl &url, PlayListModel *model)
{
    m_model = model;
    if(!PlayListParser::findByUrl(url)) //is it playlist?
    {
        m_model->add(url.toString());
        emit finished(true);
        return;
    }
    m_url = url;
    QNetworkRequest r;
    r.setUrl(url);
    r.setRawHeader("User-Agent", m_ua);
    m_getReply = m_manager->get(r);
    m_redirect_url.clear();
}

void PlayListDownloader::readResponse(QNetworkReply *reply)
{
    if(reply != m_getReply)
    {
        reply->deleteLater();
        return;
    }

    if(reply->error() != QNetworkReply::NoError)
    {
        emit finished(false, reply->errorString() + " (" + reply->error() + ")");
        reply->deleteLater();
        return;
    }

    QUrl url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if(!url.isEmpty() && url != m_redirect_url)
    {
        m_redirect_url = url;
        qDebug("PlayListDownloader: redirect to %s", qPrintable(url.toString()));
        QNetworkRequest request(url);
        request.setRawHeader("User-Agent", m_ua);
        if(reply == m_getReply)
            m_getReply = m_manager->get(request);
        reply->deleteLater();
        return;
    }
    m_redirect_url.clear();

    if(reply == m_getReply)
    {
        m_getReply = 0;

        if(m_model.isNull())
        {
            emit finished(true);
            return;
        }

        QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        qDebug("PlayListDownloader: content type: %s", qPrintable(contentType));
        PlayListFormat *fmt = PlayListParser::findByMime(contentType);
        if(!fmt)
            fmt = PlayListParser::findByUrl(m_url);
        if(fmt)
        {
            m_model->loadPlaylist(fmt->properties().shortName, reply->readAll());
            emit finished(true);
        }
        else
        {
            emit finished(false, tr("Unsupported playlist format"));
        }
    }
    reply->deleteLater();
}
