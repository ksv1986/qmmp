/***************************************************************************
 *   Copyright (C) 2009-2013 by Ilya Kotov                                 *
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

#include <QByteArray>
#include <QFile>
#include <QBuffer>
#include <QDir>
#include <QTimer>
#include <QSettings>
#include <qmmp/fileinfo.h>
#include "qmmpuisettings.h"
#include "playlistmanager.h"

PlayListManager *PlayListManager::m_instance = 0;

PlayListManager::PlayListManager(QObject *parent) : QObject(parent)
{
    if(m_instance)
        qFatal("PlayListManager: only one instance is allowed");
    m_instance = this;
    m_current = 0;
    m_selected = 0;
    m_repeatable = false;
    m_shuffle = false;
    m_autosave_playlist = false;
    m_groups_enabled = false;
    m_update = false;
    m_timer = new QTimer(this);
    m_timer->setInterval(5000);
    m_timer->setSingleShot(true);

    connect(m_timer, SIGNAL(timeout()), SLOT(writePlayLists()));
    readSettings();  //read settings
    readPlayLists(); //read playlists
}

PlayListManager::~PlayListManager()
{
    writePlayLists();
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("Playlist/repeatable", m_repeatable);
    settings.setValue("Playlist/shuffle", m_shuffle);
    settings.setValue("Playlist/groups", m_groups_enabled);
    m_instance = 0;
}

PlayListManager* PlayListManager::instance()
{
    return m_instance;
}

PlayListModel *PlayListManager::selectedPlayList() const
{
    return m_selected;
}

PlayListModel *PlayListManager::currentPlayList() const
{
    return m_current;
}

int PlayListManager::selectedPlayListIndex() const
{
    return indexOf(m_selected);
}

int PlayListManager::currentPlayListIndex() const
{
    return indexOf(m_current);
}

QList <PlayListModel *> PlayListManager::playLists() const
{
    return m_models;
}

QStringList PlayListManager::playListNames() const
{
    QStringList names;
    foreach(PlayListModel *model, m_models)
        names << model->name();
    return names;
}

void PlayListManager::selectPlayList(PlayListModel *model)
{
    if(model != m_selected && m_models.contains(model))
    {
        PlayListModel *prev = m_selected;
        m_selected = model;
        emit selectedPlayListChanged(model, prev);
        emit playListsChanged();
    }
}

void PlayListManager::selectPlayList(int i)
{
    if(i > m_models.count() - 1)
        return;
    selectPlayList(playListAt(i));
}

void PlayListManager::selectPlayList(const QString &name)
{
    int index = playListNames().indexOf(name);
    if(index >= 0)
        selectPlayList(playListAt(index));
}

void PlayListManager::selectNextPlayList()
{
    int i = m_models.indexOf(m_selected);
    i++;
    if( i >= 0 && i < m_models.size())
        selectPlayList(i);
}

void PlayListManager::selectPreviousPlayList()
{
    int i = m_models.indexOf(m_selected);
    i--;
    if( i >= 0 && i < m_models.size())
        selectPlayList(i);
}

void PlayListManager::activatePlayList(PlayListModel *model)
{
    if(model != m_current && m_models.contains(model))
    {
        PlayListModel *prev = m_current;
        m_current = model;
        emit currentPlayListChanged(model, prev);
        emit playListsChanged();
    }
}

void PlayListManager::activatePlayList(int index)
{
    activatePlayList(playListAt(index));
}

PlayListModel *PlayListManager::createPlayList(const QString &name)
{
    PlayListModel *model = new PlayListModel (name.isEmpty() ? tr("Playlist") : name, this);
    QString pl_name = model->name();
    if(playListNames().contains(pl_name))
    {
        int i = 0;
        forever
        {
            i++;
            if(!playListNames().contains(pl_name + QString(" (%1)").arg(i)))
            {
                pl_name = pl_name + QString(" (%1)").arg(i);
                break;
            }
        }
        model->setName(pl_name);
    }
    m_models.append(model);
    model->prepareForRepeatablePlaying(m_repeatable);
    model->prepareForShufflePlaying(m_shuffle);
    model->prepareGroups(m_groups_enabled);
    connect(model, SIGNAL(nameChanged(QString)), SIGNAL(playListsChanged()));
    if (m_autosave_playlist)
        connect(model, SIGNAL(countChanged()), m_timer, SLOT(start()));
    emit playListAdded(m_models.indexOf(model));
    emit playListsChanged();
    return model;
}

void PlayListManager::removePlayList(PlayListModel *model)
{
     if(m_models.count() < 2 || !m_models.contains(model))
        return;

     int i = m_models.indexOf(model);

     if(m_current == model)
     {
         m_current = m_models.at((i > 0) ? (i - 1) : (i + 1));
         emit currentPlayListChanged(m_current, model);
     }
     if(m_selected == model)
     {
         m_selected = m_models.at((i > 0) ? (i - 1) : (i + 1));
         emit selectedPlayListChanged(m_selected, model);
     }
     m_models.removeAt(i);
     model->deleteLater();
     emit playListRemoved(i);
     emit playListsChanged();
}

void PlayListManager::removePlayList(int index)
{
    removePlayList(playListAt(index));
}

void PlayListManager::move(int i, int j)
{
    if(i < 0 || j < 0 || i == j)
        return;
    if(i < m_models.count() && j < m_models.count())
    {
        m_models.move(i,j);
        emit playListMoved(i,j);
        emit playListsChanged();
    }
}

void PlayListManager::setRepeatableList(bool r)
{
    if(m_repeatable == r)
        return;
    m_repeatable = r;
    foreach(PlayListModel *model, m_models)
        model->prepareForRepeatablePlaying(r);
    emit repeatableListChanged(r);
}

void PlayListManager::setShuffle(bool s)
{
    if(m_shuffle == s)
        return;
    m_shuffle = s;
    foreach(PlayListModel *model, m_models)
        model->prepareForShufflePlaying(s);
    emit shuffleChanged(s);
}

int PlayListManager::count() const
{
    return m_models.count();
}

int PlayListManager::indexOf(PlayListModel *model) const
{
    return m_models.indexOf(model);
}

PlayListModel *PlayListManager::playListAt(int i) const
{
    if(i >= 0 && i < m_models.count())
        return m_models.at(i);
    return 0;
}

bool PlayListManager::isRepeatableList() const
{
    return m_repeatable;
}

bool PlayListManager::isShuffle() const
{
    return m_shuffle;
}

bool PlayListManager::isGroupsEnabled() const
{
    return m_groups_enabled;
}

void PlayListManager::readPlayLists()
{
    QString line, param, value;
    int s = 0, current = 0, pl = 0;
    QList <PlayListTrack *> tracks;
    QFile file(QDir::homePath() +"/.qmmp/playlist.txt");
    file.open(QIODevice::ReadOnly);
    QByteArray array = file.readAll();
    file.close();
    QBuffer buffer(&array);
    buffer.open(QIODevice::ReadOnly);

    while (!buffer.atEnd())
    {
        line = QString::fromUtf8(buffer.readLine()).trimmed();
        if ((s = line.indexOf("=")) < 0)
            continue;

        param = line.left(s);
        value = line.right(line.size() - s - 1);

        if(param == "current_playlist")
            pl = value.toInt();
        else if(param == "playlist")
        {
            if(!m_models.isEmpty() && !tracks.isEmpty())
            {
                m_models.last()->add(tracks);
                m_models.last()->setCurrent(tracks.at(qBound(0, current, tracks.count()-1)));
            }
            tracks.clear();
            current = 0;
            m_models << new PlayListModel(value, this);
            if(m_groups_enabled)
                m_models.last()->prepareGroups(true);
        }
        else if (param == "current")
        {
            current = value.toInt();
        }
        else if (param == "file")
        {
            tracks << new PlayListTrack();
            tracks.last()->insert(Qmmp::URL, value);
        }
        else if (tracks.isEmpty())
            continue;
        else if (param == "title")
            tracks.last()->insert(Qmmp::TITLE, value);
        else if (param == "artist")
            tracks.last()->insert(Qmmp::ARTIST, value);
        else if (param == "album")
            tracks.last()->insert(Qmmp::ALBUM, value);
        else if (param == "comment")
            tracks.last()->insert(Qmmp::COMMENT, value);
        else if (param == "genre")
            tracks.last()->insert(Qmmp::GENRE, value);
        else if (param == "composer")
            tracks.last()->insert(Qmmp::COMPOSER, value);
        else if (param == "year")
            tracks.last()->insert(Qmmp::YEAR, value);
        else if (param == "track")
            tracks.last()->insert(Qmmp::TRACK, value);
        else if (param == "disc")
            tracks.last()->insert(Qmmp::DISCNUMBER, value);
        else if (param == "length")
            tracks.last()->setLength(value.toInt());
    }
    buffer.close();
    if(m_models.isEmpty())
    {
        m_models << new PlayListModel(tr("Playlist"),this);
        if(m_groups_enabled)
            m_models.last()->prepareGroups(true);
    }
    else if(!tracks.isEmpty())
    {
        m_models.last()->add(tracks);
        m_models.last()->setCurrent(tracks.at(qBound(0, current, tracks.count()-1)));
    }
    if(pl < 0 || pl >= m_models.count())
        pl = 0;
    m_selected = m_models.at(pl);
    m_current = m_models.at(pl);
    foreach(PlayListModel *model, m_models)
    {
        connect(model, SIGNAL(nameChanged(QString)), SIGNAL(playListsChanged()));
        if (m_autosave_playlist)
            connect(model, SIGNAL(countChanged()), m_timer, SLOT(start()));
        if(m_repeatable)
            model->prepareForRepeatablePlaying(true);
        if(m_shuffle)
            model->prepareForShufflePlaying(true);
    }
}

void PlayListManager::writePlayLists()
{
    qDebug("PlayListManager: saving playlists...");
    QFile file(QDir::homePath() +"/.qmmp/playlist.txt");
    if(!file.open(QIODevice::WriteOnly))
    {
        qDebug("PlayListManager: error: %s", qPrintable(file.errorString()));
        return;
    }
    file.write(QString("current_playlist=%1\n").arg(m_models.indexOf(m_current)).toUtf8());
    foreach(PlayListModel *model, m_models)
    {
        QList<PlayListItem *> items = model->items();
        file.write(QString("playlist=%1\n").arg(model->name()).toUtf8());
        file.write(QString("current=%1\n").arg(model->numberOfTrack(model->currentIndex())).toUtf8());
        foreach(PlayListItem* m, items)
        {
            if(m->isGroup())
                continue;
            PlayListTrack *t = dynamic_cast<PlayListTrack *>(m);
            file.write(QString("file=%1\n").arg(t->url()).toUtf8());
            file.write(QString("title=%1\n").arg(t->value(Qmmp::TITLE)).toUtf8());
            file.write(QString("artist=%1\n").arg(t->value(Qmmp::ARTIST)).toUtf8());
            file.write(QString("album=%1\n").arg(t->value(Qmmp::ALBUM)).toUtf8());
            file.write(QString("comment=%1\n").arg(t->value(Qmmp::COMMENT)).toUtf8());
            file.write(QString("genre=%1\n").arg(t->value(Qmmp::GENRE)).toUtf8());
            file.write(QString("composer=%1\n").arg(t->value(Qmmp::COMPOSER)).toUtf8());
            file.write(QString("year=%1\n").arg(t->value(Qmmp::YEAR)).toUtf8());
            file.write(QString("track=%1\n").arg(t->value(Qmmp::TRACK)).toUtf8());
            file.write(QString("disc=%1\n").arg(t->value(Qmmp::DISCNUMBER)).toUtf8());
            file.write(QString("length=%1\n").arg(t->length()).toUtf8());
        }
    }
    file.close();
}

void PlayListManager::clear()
{
    m_selected->clear();
}

void PlayListManager::clearSelection()
{
    m_selected->clearSelection();
}

void PlayListManager::removeSelected()
{
    m_selected->removeSelected();
}

void PlayListManager::removeUnselected()
{
    m_selected->removeUnselected();
}

void PlayListManager::removeTrack (int i)
{
    m_selected->removeTrack(i);
}

void PlayListManager::removeTrack (PlayListTrack *track)
{
    m_selected->removeTrack(track);
}

void PlayListManager::invertSelection()
{
    m_selected->invertSelection();
}

void PlayListManager::selectAll()
{
    m_selected->selectAll();
}

void PlayListManager::showDetails()
{
    m_selected->showDetails();
}

void PlayListManager::add(const QStringList &paths)
{
    m_selected->add(paths);
}

void PlayListManager::randomizeList()
{
    m_selected->randomizeList();
}

void PlayListManager::reverseList()
{
    m_selected->reverseList();
}

void PlayListManager::sortSelection(int mode)
{
    m_selected->sortSelection(mode);
}

void PlayListManager::sort(int mode)
{
    m_selected->sort(mode);
}

void PlayListManager::addToQueue()
{
    m_selected->addToQueue();
}

void PlayListManager::removeInvalidTracks()
{
    m_selected->removeInvalidTracks();
}

void PlayListManager::removeDuplicates()
{
    m_selected->removeDuplicates();
}

void PlayListManager::clearQueue()
{
    m_selected->clearQueue();
}

void PlayListManager::stopAfterSelected()
{
    m_selected->stopAfterSelected();
}

void PlayListManager::setGroupsEnabled(bool enabled)
{
    if(m_groups_enabled == enabled)
        return;

    m_groups_enabled = enabled;
    foreach(PlayListModel *model, m_models)
        model->prepareGroups(enabled);
}

void PlayListManager::readSettings()
{
    if(!m_update)
    {
        m_update = true;
        QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
        setRepeatableList(settings.value("Playlist/repeatable",false).toBool());
        setShuffle(settings.value("Playlist/shuffle",false).toBool());
        setGroupsEnabled(settings.value("Playlist/groups",false).toBool());
    }

    QmmpUiSettings *ui_settings = QmmpUiSettings::instance();
    if (m_autosave_playlist != ui_settings->autoSavePlayList())
    {
        m_autosave_playlist = ui_settings->autoSavePlayList();
        foreach(PlayListModel *model, m_models)
        {
            if (m_autosave_playlist)
                connect(model, SIGNAL(countChanged()), m_timer, SLOT(start()), Qt::UniqueConnection);
            else
                disconnect(model, SIGNAL(countChanged()), m_timer, SLOT(start()));
        }
    }
}
