/***************************************************************************
 *   Copyright (C) 2006-2020 by Ilya Kotov                                 *
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

#include <qmmp/metadatamanager.h>
#include <QRegExp>
#include <QMetaType>
#include <QDir>
#include <QApplication>
#include <QMutexLocker>
#include "fileloader_p.h"
#include "qmmpuisettings.h"
#include "playlistitem.h"
#include "playlistparser.h"
#include "playlisttrack.h"

FileLoader::FileLoader(QObject *parent) : QThread(parent)
{
    qRegisterMetaType<QList<PlayListTrack*> >("QList<PlayListTrack*>");
    m_settings = QmmpUiSettings::instance();
    m_finished = false;
    m_readMetaDataForPlayLists = true;
    m_parts = TrackInfo::Parts();
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(finish()));
}

QList<PlayListTrack *> FileLoader::processFile(const QString &path, QStringList *ignoredPaths)
{
    QList<PlayListTrack *> tracks;
    const QList<TrackInfo *> infoList = MetaDataManager::instance()->createPlayList(path, m_parts, ignoredPaths);

    for(TrackInfo *info : qAsConst(infoList))
    {
        tracks.append(new PlayListTrack(info));
    }
    qDeleteAll(infoList);
    return tracks;
}

void FileLoader::insertPlayList(const QString &fmt, const QByteArray &contents, PlayListItem *before)
{
    QList<PlayListTrack *> tracks = PlayListParser::loadPlaylist(fmt, contents);

    while (!tracks.isEmpty() && !m_finished)
    {
        PlayListTrack *t = tracks.takeFirst();
        QList<TrackInfo *> infoList = MetaDataManager::instance()->createPlayList(t->path(), m_parts);
        if(infoList.count() != 1) //invalid or unsupported track
        {
            qDeleteAll(infoList);
            infoList.clear();
            delete t;
            continue;
        }

        TrackInfo *info = infoList.first();
        if(!info->value(Qmmp::ALBUM).isEmpty() && !info->value(Qmmp::ARTIST).isEmpty())
            t->updateMetaData(infoList.first());

        emit newTracksToInsert(before, QList<PlayListTrack *>() << t);
        delete info;
    }
    //clear remaining tracks
    qDeleteAll(tracks);
    tracks.clear();
}

void FileLoader::insertPlayList(const QString &path, PlayListItem *before)
{
    QList<PlayListTrack *> tracks = PlayListParser::loadPlaylist(path);

    if(m_readMetaDataForPlayLists)
    {
        QList<PlayListTrack *> tmp;
        while(!tracks.isEmpty() && !m_finished)
        {
            PlayListTrack *t = tracks.takeFirst();
            QList <TrackInfo *> infoList = MetaDataManager::instance()->createPlayList(t->path(), m_parts);
            if(infoList.count() != 1) //invalid or unsupported track
            {
                qDeleteAll(infoList);
                infoList.clear();
                delete t;
                continue;
            }

            if(!infoList.first()->value(Qmmp::ALBUM).isEmpty() && !infoList.first()->value(Qmmp::ARTIST).isEmpty())
                t->updateMetaData(infoList.first());
            delete infoList.takeFirst();
            tmp << t;
            if(tmp.count() > 30)
            {
                emit newTracksToInsert(before, tmp);
                tmp.clear();
            }
        }
        if(!tmp.isEmpty() && !m_finished)
        {
            emit newTracksToInsert(before, tmp);
            tmp.clear();
        }

        if(!tmp.isEmpty())
        {
            qDeleteAll(tmp);
            tmp.clear();
        }
    }
    else
    {
        QStringList protocols = MetaDataManager::instance()->protocols();
        QList<QRegularExpression> regExps =  MetaDataManager::instance()->regExps();
        QList<QRegExp> filters;
        for(const QString &pattern : MetaDataManager::instance()->nameFilters())
            filters << QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);


        QList<PlayListTrack *>::iterator it = tracks.begin();
        while(it != tracks.end())
        {
            if((*it)->path().contains("://"))
            {
                if(!protocols.contains((*it)->path().section("://", 0, 0)) &&
                        !MetaDataManager::hasMatch(regExps, (*it)->path()))
                {
                    delete (*it);
                    it = tracks.erase(it);
                    continue;
                }
            }
            else if(!QFile::exists((*it)->path()) || !MetaDataManager::hasMatch(filters, (*it)->path()))
            {
                delete (*it);
                it = tracks.erase(it);
                continue;
            }

            ++it;
        }
        if(!m_finished && !tracks.isEmpty())
        {
            emit newTracksToInsert(before, tracks);
            tracks.clear();
        }
    }
    //clear remaining tracks
    qDeleteAll(tracks);
    tracks.clear();
}

void FileLoader::addDirectory(const QString& s, PlayListItem *before)
{
    QList<PlayListTrack *> tracks;
    QStringList ignoredPaths;
    QDir dir(s);
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList l = dir.entryInfoList(m_filters);

    for(const QFileInfo &info : qAsConst(l))
    {
        if(checkRestrictFilters(info) && checkExcludeFilters(info))
        {
            QStringList paths;
            tracks.append(processFile(info.absoluteFilePath (), &paths));
            ignoredPaths.append(paths);
        }

        if (m_finished)
        {
            qDeleteAll(tracks);
            tracks.clear();
            return;
        }

        if(tracks.count() > 30) //do not send more than 30 tracks at once
        {
            removeIgnoredTracks(&tracks, ignoredPaths);
            emit newTracksToInsert(before, tracks);
            tracks.clear();
            ignoredPaths.clear();
        }
    }

    if(!tracks.isEmpty())
    {
        removeIgnoredTracks(&tracks, ignoredPaths);
        emit newTracksToInsert(before, tracks);
        ignoredPaths.clear();
    }

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name);
    l.clear();
    l = dir.entryInfoList();

    for (int i = 0; i < l.size(); ++i)
    {
        QFileInfo fileInfo = l.at(i);
        addDirectory(fileInfo.absoluteFilePath (), before);
        if (m_finished)
            return;
    }
}

void FileLoader::run()
{
    m_finished = false;

    while(!m_finished)
    {
        QMutexLocker locker(&m_mutex);
        if(m_tasks.isEmpty())
            return;

        LoaderTask i = m_tasks.dequeue();
        locker.unlock();
        PlayListItem *before = i.before;
        QString path = i.path;

        if(!path.isEmpty())
        {
            QFileInfo info(path);

            if(info.isDir())
            {
                addDirectory(path, before);
            }
            else if(info.isFile() && PlayListParser::isPlayList(path))
            {
                insertPlayList(path, before);
            }
            else if(info.isFile() || path.contains("://"))
            {
                QList<PlayListTrack *> tracks = processFile(path);
                if(!tracks.isEmpty())
                {
                    emit newTracksToInsert(before, tracks);
                }
            }
        }
        else if(!i.playListContent.isEmpty() && !i.playListFormat.isEmpty())
        {
            insertPlayList(i.playListFormat, i.playListContent, before);
        }
    }
}

void FileLoader::add(const QString &path)
{
    insert(nullptr, QStringList() << path);
}

void FileLoader::add(const QStringList &paths)
{
    insert(nullptr, paths);
}

void FileLoader::processQueue()
{
    if(!isRunning())
    {
        MetaDataManager::instance()->prepareForAnotherThread();
        PlayListParser::loadFormats();
        m_filters = MetaDataManager::instance()->nameFilters();
        m_parts = m_settings->useMetaData() ? TrackInfo::AllParts : TrackInfo::Parts();
        m_readMetaDataForPlayLists = m_settings->readMetaDataForPlayLists();
    }
    start(QThread::IdlePriority);
}

void FileLoader::addPlayList(const QString &fmt, const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);
    LoaderTask task;
    task.before = nullptr;
    task.playListFormat = fmt;
    task.playListContent = data;
    m_tasks.append(task);
    locker.unlock();
    processQueue();
}

void FileLoader::insert(PlayListItem *before, const QString &path)
{
    insert(before, QStringList() << path);
}

void FileLoader::insert(PlayListItem *before, const QStringList &paths)
{
    QMutexLocker locker(&m_mutex);
    for(const QString &path : qAsConst(paths))
    {
        LoaderTask task;
        task.before = before;
        task.path = QDir::fromNativeSeparators(path);
        m_tasks.append(task);
    }
    locker.unlock();
    processQueue();
}

void FileLoader::finish()
{
    m_finished = true;
    wait();
    m_tasks.clear();
}

bool FileLoader::checkRestrictFilters(const QFileInfo &info)
{
    if(m_settings->restrictFilters().isEmpty())
        return true;

    for(const QString &filter : m_settings->restrictFilters())
    {
        QRegExp regexp (filter, Qt::CaseInsensitive, QRegExp::Wildcard);
        if(regexp.exactMatch(info.absoluteFilePath()))
            return true;
    }
    return false;
}

bool FileLoader::checkExcludeFilters(const QFileInfo &info)
{
    if(m_settings->excludeFilters().isEmpty())
        return true;

    for(const QString &filter : m_settings->excludeFilters())
    {
        QRegExp regexp (filter, Qt::CaseInsensitive, QRegExp::Wildcard);
        if(regexp.exactMatch(info.absoluteFilePath()))
            return false;
    }
    return true;
}

void FileLoader::removeIgnoredTracks(QList<PlayListTrack *> *tracks, const QStringList &ignoredPaths)
{
    if(ignoredPaths.isEmpty())
        return;

    QList<PlayListTrack *>::iterator it = tracks->begin();
    while(it != tracks->end())
    {
        if(ignoredPaths.contains((*it)->path()))
        {
            delete (*it);
            it = tracks->erase(it);
        }
        else
        {
            ++it;
        }
    }
}
