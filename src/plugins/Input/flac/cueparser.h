/***************************************************************************
 *   Copyright (C) 2008-2016 by Ilya Kotov                                 *
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
#ifndef CUEPARSER_H
#define CUEPARSER_H

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <qmmp/qmmp.h>
#include <qmmp/fileinfo.h>


/**
    @author Ilya Kotov <forkotov02@hotmail.ru>
*/
class CUEParser
{
public:
    CUEParser(const QByteArray &array, const QString &fileName);

    ~CUEParser();

    QList<FileInfo*> createPlayList();
    const QString filePath() const;
    qint64 offset(int track) const;
    qint64 length(int track) const;
    int count() const;
    FileInfo *info(int track);
    const QString trackURL(int track) const;
    const QMap<Qmmp::ReplayGainKey, double> replayGain(int track) const;

private:
    struct CUETrack
    {
        FileInfo info;
        qint64 offset;
        QMap<Qmmp::ReplayGainKey, double> replayGain;
    };
    QList <CUETrack * > m_tracks;
    QStringList splitLine(const QString &line);
    qint64 getLength(const QString &str);
    QString m_filePath;
};

#endif
