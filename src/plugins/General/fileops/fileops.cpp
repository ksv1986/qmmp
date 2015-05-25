/***************************************************************************
 *   Copyright (C) 2009-2015 by Ilya Kotov                                 *
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

#include <QAction>
#include <QSettings>
#include <QApplication>
#include <QSignalMapper>
#include <QProgressDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <qmmp/soundcore.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/playlistitem.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/metadataformatter.h>
#include "fileops.h"

#define COPY_BLOCK_SIZE 102400

FileOps::FileOps(QObject *parent) : QObject(parent)
{
    //separators
    QAction *separator1 = new QAction(this);
    separator1->setSeparator (true);
    QAction *separator2 = new QAction(this);
    separator2->setSeparator (true);
    //load settings
    QSignalMapper *mapper = new QSignalMapper(this);
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("FileOps");
    int count = settings.value("count", 0).toInt();
    if (count > 0)
        UiHelper::instance()->addAction(separator1, UiHelper::PLAYLIST_MENU);
    else
        return;

    for (int i = 0; i < count; ++i)
    {

        if (settings.value(QString("enabled_%1").arg(i), true).toBool())
        {
            m_types << settings.value(QString("action_%1").arg(i), FileOps::COPY).toInt();
            QString name = settings.value(QString("name_%1").arg(i), "Action").toString();
            m_patterns << settings.value(QString("pattern_%1").arg(i)).toString();
            m_destinations << settings.value(QString("destination_%1").arg(i)).toString();
            QAction *action = new QAction(name, this);
            action->setShortcut(settings.value(QString("hotkey_%1").arg(i)).toString());
            connect (action, SIGNAL (triggered (bool)), mapper, SLOT (map()));
            mapper->setMapping(action, i);
            UiHelper::instance()->addAction(action, UiHelper::PLAYLIST_MENU);
        }
    }
    settings.endGroup();
    connect(mapper, SIGNAL(mapped(int)), SLOT(execAction(int)));
    UiHelper::instance()->addAction(separator2, UiHelper::PLAYLIST_MENU);
}

FileOps::~FileOps()
{}

void FileOps::execAction(int n)
{
    int type = m_types.at(n);
    QString pattern = m_patterns.at(n);
    MetaDataFormatter formatter(pattern);
    QString destination = m_destinations.at(n);

    PlayListModel *model = MediaPlayer::instance()->playListManager()->selectedPlayList();
    QList<PlayListTrack*> tracks = model->selectedTracks();

    switch (type)
    {
    case COPY:
    {
        qDebug("FileOps: copy");
        if (!QDir(destination).exists ())
        {
            QMessageBox::critical (qApp->activeWindow (), tr("Error"),
                                   tr("Destination directory doesn't exist"));
            break;
        }
        QProgressDialog progress(qApp->activeWindow ());
        progress.setWindowModality(Qt::WindowModal);
        progress.setWindowTitle(tr("Copying"));
        progress.setCancelButtonText(tr("Stop"));
        progress.show();
        progress.setAutoClose (false);
        int i  = 0;
        foreach(PlayListTrack *item, tracks)
        {
            if (!QFile::exists(item->url()))
                continue;
            //generate file name
            QString fname = formatter.format(item);
            //append extension
            QString ext = QString(".") + item->url().split('.',QString::SkipEmptyParts).takeLast ();
            if (!fname.endsWith(ext))
                fname += ext;
            QString path = destination + "/" + fname;
            QDir dir = QFileInfo(path).dir();
            if(!dir.exists())
            {
                if(!dir.mkpath(dir.absolutePath()))
                {
                    qWarning("FileOps: unable to create directory");
                    continue;
                }
            }
            //copy file
            QFile in(item->url());
            QFile out(path);
            if (!in.open(QIODevice::ReadOnly))
            {
                qWarning("FileOps: %s", qPrintable(in.errorString ()));
                continue;
            }
            if (!out.open(QIODevice::WriteOnly))
            {
                qWarning("FileOps: %s", qPrintable(out.errorString ()));
                continue;
            }

            progress.setMaximum(int(in.size()/COPY_BLOCK_SIZE));
            progress.setValue(0);
            progress.setLabelText (QString(tr("Copying file %1/%2")).arg(++i).arg(tracks.size()));
            progress.update();

            while (!in.atEnd ())
            {
                progress.wasCanceled ();
                out.write(in.read(COPY_BLOCK_SIZE));
                progress.setValue(int(out.size()/COPY_BLOCK_SIZE));
                qApp->processEvents();
            }
        }
        progress.close();
        break;
    }
    case RENAME:
        qDebug("FileOps: rename");
        foreach(PlayListTrack *item, tracks)
        {
            if (!QFile::exists(item->url()))
                continue;
            //generate file name
            QString fname = formatter.format(item);
            //append extension
            QString ext = QString(".") + item->url().split('.',QString::SkipEmptyParts).takeLast ();
            if (!fname.endsWith(ext))
                fname += ext;
            //rename file
            QFile file(item->url());
            destination = QFileInfo(item->url()).absolutePath ();
            if (file.rename(destination + "/" + fname))
            {
                item->insert(Qmmp::URL, destination + "/" + fname);
                model->doCurrentVisibleRequest();
            }
            else
                continue;
        }
        break;
        /*case MOVE:
            break;*/
    case REMOVE:
        qDebug("FileOps: remove");
        if (QMessageBox::question (qApp->activeWindow (), tr("Remove Files"),
                                   tr("Are you sure you want to remove %n file(s) from disk?",
                                      "",tracks.size()),
                                   QMessageBox::Yes | QMessageBox::No) !=  QMessageBox::Yes)
            break;

        foreach(PlayListTrack *track, tracks)
        {
            if (QFile::exists(track->url()) && QFile::remove(track->url()))
                model->removeTrack(track);
        }
    }
}
