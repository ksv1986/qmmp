/***************************************************************************
 *   Copyright (C) 2014 by Ilya Kotov                                      *
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

#include <QFileInfo>
#include <QDateTime>
#include "playlisttrack.h"
#include "playlisttask_p.h"

struct TrackField
{
    PlayListTrack *track;
    QString value;
};

////===============THE BEGINNING OF SORT IMPLEMENTATION =======================////

// First we'll implement bundle of static compare procedures
// to sort items in different ways

//by string
static bool _stringLessComparator(TrackField* s1, TrackField* s2)
{
    return QString::localeAwareCompare (s1->value, s2->value) < 0;
}

static bool _stringGreaterComparator(TrackField* s1, TrackField* s2)
{
    return QString::localeAwareCompare (s1->value, s2->value) > 0;
}
//by number
static bool _numberLessComparator(TrackField* s1, TrackField* s2)
{
    return s1->value.toInt() < s2->value.toInt();
}

static bool _numberGreaterComparator(TrackField* s1, TrackField* s2)
{
    return s1->value.toInt() > s2->value.toInt();
}
//by file creation date
static bool _fileCreationDateLessComparator(TrackField* s1, TrackField* s2)
{
    return QFileInfo(s1->value).created() < QFileInfo(s2->value).created();
}

static bool _fileCreationDateGreaterComparator(TrackField* s1, TrackField* s2)
{
    return QFileInfo(s1->value).created() > QFileInfo(s2->value).created();
}
//by file modification date
static bool _fileModificationDateLessComparator(TrackField* s1, TrackField* s2)
{
    return QFileInfo(s1->value).lastModified() < QFileInfo(s2->value).lastModified();
}

static bool _fileModificationDateGreaterComparator(TrackField* s1, TrackField* s2)
{
    return QFileInfo(s1->value).lastModified() > QFileInfo(s2->value).lastModified();
}
//by file name
static bool _filenameLessComparator(TrackField* s1, TrackField* s2)
{
    QFileInfo i_s1(s1->value);
    QFileInfo i_s2(s2->value);
    return QString::localeAwareCompare (i_s1.baseName(), i_s2.baseName()) < 0;
}

static bool _filenameGreaterComparator(TrackField* s1, TrackField* s2)
{
    QFileInfo i_s1(s1->value);
    QFileInfo i_s2(s2->value);
    return QString::localeAwareCompare (i_s1.baseName(), i_s2.baseName()) > 0;
}
////=============== THE END OF SORT IMPLEMENTATION =======================////

PlayListTask::PlayListTask(QObject *parent) : QThread(parent)
{
    m_reverted = true;
    m_task = EMPTY;
}

PlayListTask::~PlayListTask()
{
    qDebug("%s", Q_FUNC_INFO);
}

void PlayListTask::sort(QList<PlayListTrack *> tracks, int mode)
{
    if(isRunning())
        return;
    m_fields.clear();
    m_tracks.clear();
    m_reverted = !m_reverted;
    m_sort_mode = mode;
    m_task = SORT;
    Qmmp::MetaData key = findSortKey(mode);

    foreach (PlayListTrack *t, tracks)
    {
        t->beginUsage();
        TrackField *f = new TrackField;
        f->track = t;
        f->value = (mode == PlayListModel::GROUP) ? t->groupName() : t->value(key);
        m_fields.append(f);
    }

    start();
}

void PlayListTask::sortSelection(QList<PlayListTrack *> tracks, int mode)
{
    if(isRunning())
        return;
    m_fields.clear();
    m_tracks.clear();
    m_indexes.clear();
    m_reverted = !m_reverted;
    m_sort_mode = mode;
    m_task = SORT_SELECTION;
    m_tracks = tracks;
    Qmmp::MetaData key = findSortKey(mode);

    for(int i = 0; i < tracks.count(); ++i)
    {
        tracks[i]->beginUsage();
        if(!tracks[i]->isSelected())
            continue;

        TrackField *f = new TrackField;
        f->track = tracks[i];
        f->value = (mode == PlayListModel::GROUP) ? f->track->groupName() : f->track->value(key);
        m_fields.append(f);
        m_indexes.append(i);
    }

    start();
}

void PlayListTask::run()
{
    qDebug("started");
    bool(*compareLessFunc)(TrackField*, TrackField*) = 0;
    bool(*compareGreaterFunc)(TrackField*, TrackField*) = 0;

    QList<TrackField*>::iterator begin = m_fields.begin();
    QList<TrackField*>::iterator end = m_fields.end();

    if(m_sort_mode == PlayListModel::FILE_CREATION_DATE)
    {
        compareLessFunc = _fileCreationDateLessComparator;
        compareGreaterFunc = _fileCreationDateGreaterComparator;
    }
    else if(m_sort_mode == PlayListModel::FILE_MODIFICATION_DATE)
    {
        compareLessFunc = _fileModificationDateLessComparator;
        compareGreaterFunc = _fileModificationDateGreaterComparator;
    }
    else if(m_sort_mode == PlayListModel::TRACK || m_sort_mode == PlayListModel::DATE)
    {
        compareLessFunc = _numberLessComparator;
        compareGreaterFunc = _numberGreaterComparator;
    }
    else if(m_sort_mode == PlayListModel::FILENAME)
    {
        compareLessFunc = _filenameLessComparator;
        compareGreaterFunc = _filenameGreaterComparator;
    }
    else
    {
        compareLessFunc = _stringLessComparator;
        compareGreaterFunc = _stringGreaterComparator;
    }

    if(m_reverted)
        qStableSort(begin,end,compareGreaterFunc);
    else
        qStableSort(begin,end,compareLessFunc);
    qDebug("finished");
}

QList<PlayListTrack *> PlayListTask::takeResults()
{
    if(m_task == SORT)
    {
        foreach (TrackField *f, m_fields)
            m_tracks.append(f->track);
    }
    else if(m_task == SORT_SELECTION)
    {
        for (int i = 0; i < m_indexes.count(); i++)
            m_tracks.replace(m_indexes[i], m_fields[i]->track);
    }

    qDeleteAll(m_fields);
    m_fields.clear();
    foreach (PlayListTrack *t, m_tracks)
    {
        t->endUsage();
        if(!t->isUsed() && t->isSheduledForDeletion())
        {
            m_tracks.removeAll(t);
            delete t;
        }
    }
    return m_tracks;
}

Qmmp::MetaData PlayListTask::findSortKey(int mode)
{
    switch (mode)
    {
    case PlayListModel::TITLE:
        return Qmmp::TITLE;
    case PlayListModel::DISCNUMBER:
        return Qmmp::DISCNUMBER;
    case PlayListModel::ALBUM:
        return Qmmp::ALBUM;
    case PlayListModel::ARTIST:
        return Qmmp::ARTIST;
    case PlayListModel::ALBUMARTIST:
        return Qmmp::ALBUMARTIST;
    case PlayListModel::FILENAME:
    case PlayListModel::PATH_AND_FILENAME:
        return Qmmp::URL;
    case PlayListModel::DATE:
        return Qmmp::YEAR;
    case PlayListModel::TRACK:
        return Qmmp::TRACK;
    case PlayListModel::FILE_CREATION_DATE:
    case PlayListModel::FILE_MODIFICATION_DATE:
        return Qmmp::URL;
    default:
        return Qmmp::TITLE;
    }
}
