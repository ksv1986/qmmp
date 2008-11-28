/***************************************************************************
 *   Copyright (C) 2008 by Ilya Kotov                                      *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QFileInfo>
#include <QtPlugin>

#include "plsplaylistformat.h"

bool PLSPlaylistFormat::hasFormat(const QString & f)
{
    foreach(QString s,m_supported_formats)
    if (f == s)
        return true;

    return false;
}

QStringList PLSPlaylistFormat::getExtensions() const
{
    return m_supported_formats;
}

PLSPlaylistFormat::PLSPlaylistFormat()
{
    m_supported_formats << "pls";
}

QString PLSPlaylistFormat::name() const
{
    return "PLSPlaylistFormat";
}


QStringList PLSPlaylistFormat::decode(const QString & contents)
{
    QStringList out;
    QStringList splitted = contents.split("\n");
    if (!splitted.isEmpty())
    {
        if (splitted.takeAt(0).toLower().contains("[playlist]"))
        {
            foreach(QString str, splitted)
            {
                if (str.startsWith("File"))
                {
                    QString unverified = str.remove(0,str.indexOf(QChar('=')) + 1);
                    unverified = unverified.trimmed();
                    if (unverified.startsWith("http://"))
                    {
                        out << unverified;
                    }
                    else /*if (QFileInfo(unverified).exists())*/
                        out << QFileInfo(unverified).absoluteFilePath();
                    /*else
                        qWarning("File %s does not exist", qPrintable(unverified));*/
                }
            }
            return out;
        }
    }
    else
        qWarning("Error parsing PLS format");

    return QStringList();
}

QString PLSPlaylistFormat::encode(const QList<AbstractPlaylistItem *> & contents)
{
    QStringList out;
    out << QString("[playlist]");
    int counter = 1;
    foreach(AbstractPlaylistItem* f,contents)
    {
        QString begin = "File" + QString::number(counter) + "=";
        out.append(begin + f->url());
        begin = "Title" + QString::number(counter) + "=";
        out.append(begin + f->title());
        begin = "Length" + QString::number(counter) + "=";
        out.append(begin + QString::number(f->length()));
        counter ++;
    }
    out << "NumberOfEntries=" + QString::number(contents.count());
    return out.join("\n");
}

Q_EXPORT_PLUGIN(PLSPlaylistFormat)
