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
#ifndef FILELOADER_H
#define FILELOADER_H

#include <QObject>
#include <QQueue>
#include <QStringList>
#include <QFileInfo>
#include <QMutex>
#include <QThread>
#include <qmmp/trackinfo.h>

class PlayListTrack;
class PlayListItem;
class QmmpUiSettings;

/*! @internal
 * @brief File loader class.
 *
 * This class represents fileloader object that
 * processes file list in separate thread and emits
 * \b newPlayListItem(PlayListItem*) signal for every newly
 * created media file.
 * @author Ilya Kotov <forkotov02@ya.ru>
 */
class FileLoader : public QThread
{
    Q_OBJECT
public:
    /*!
     * Constructs FileLoader object.
     * @param parent QObject parent
     */
    FileLoader(QObject *parent = nullptr);
    /*!
     * Sets file/directory to load
     */
    void add(const QString &path);
    /*!
     * Sets files/directories to load
     */
    void add(const QStringList &paths);
    void addPlayList(const QString &fmt, const QByteArray &data);

    void insert(PlayListItem *before, const QString &path);
    void insert(PlayListItem *before, const QStringList &paths);


public slots:
    /*!
     * Removes files and directories from queue and waits until thread is finished
     */
    void finish();

signals:
    /*!
     * Emitted when new playlist tracks are available.
     * @param tracks List of the pointers to the \b PlayListTrack objects.
     */
    void newTracksToInsert(PlayListItem *before, QList<PlayListTrack *> tracks);

private:
    void run() override;
    QList<PlayListTrack*> processFile(const QString &path, QStringList *ignoredPaths = nullptr);
    void insertPlayList(const QString &fmt, const QByteArray &contents, PlayListItem *before);
    void insertPlayList(const QString &path, PlayListItem *before);
    void addDirectory(const QString &s, PlayListItem *before = nullptr);
    bool checkRestrictFilters(const QFileInfo &info);
    bool checkExcludeFilters(const QFileInfo &info);
    void removeIgnoredTracks(QList<PlayListTrack *> *tracks, const QStringList &ignoredPaths);
    void processQueue();
    struct LoaderTask
    {
        QString path;
        PlayListItem *before;
        QString playListFormat;
        QByteArray playListContent;
    };
    QQueue <LoaderTask> m_tasks;
    QStringList m_filters;
    QmmpUiSettings *m_settings;
    bool m_finished;
    bool m_readMetaDataForPlayLists;
    TrackInfo::Parts m_parts;
    QMutex m_mutex;

};

#endif
