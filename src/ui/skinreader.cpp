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

#include <QDir>
#include <QList>
#include <QFileInfo>
#include <QProcess>
#include <QByteArray>
#include <QApplication>
#include <QFile>

#include "skinreader.h"

SkinReader::SkinReader(QObject *parent)
        : QObject(parent)
{
    m_process = new QProcess(this);
    //create cache dir
    QDir dir(QDir::homePath() +"/.qmmp/");
    dir.mkdir("cache");
    dir.cd("cache");
    dir.mkdir("thumbs");
    dir.mkdir("skin");
}


SkinReader::~SkinReader()
{}

void SkinReader::generateThumbs()
{
    m_previewMap.clear();
    QDir dir(QDir::homePath() +"/.qmmp/skins");
    dir.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList f = dir.entryInfoList();
    dir.setPath(qApp->applicationDirPath()+"/../share/qmmp/skins");
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    f << dir.entryInfoList();
    QDir cache_dir(QDir::homePath() +"/.qmmp/cache/thumbs");
    cache_dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList d = cache_dir.entryInfoList();
    //clear removed skins from cache
    foreach(QFileInfo thumbFile, d)
    {
        bool del = true;
        foreach(QFileInfo fileInfo, f)
        {
            if (fileInfo.baseName () == thumbFile.baseName ())
            {
                del = FALSE;
                break;
            }
        }
        if (del)
        {
            qDebug("SkinReader: deleting %s from cache",
                   qPrintable(thumbFile.fileName ()));

            cache_dir.remove(thumbFile.fileName ());
        }
    }
    //add new skins to cache
    foreach(QFileInfo fileInfo, f)
    {
        bool create = true;
        foreach(QFileInfo thumbInfo, d)
        {
            if (fileInfo.baseName () == thumbInfo.baseName ())
            {
                create = FALSE;
                break;
            }
        }
        if (create)
        {
            qDebug("SkinReader: adding %s to cache",
                   qPrintable(fileInfo.fileName ()));
            QString name = fileInfo.fileName ().toLower();

            if (name.endsWith(".tgz") || name.endsWith(".tar.gz") || name.endsWith(".tar.bz2"))
                untar(fileInfo.filePath (), cache_dir.absolutePath (), true);
            if (name.endsWith(".zip") || name.endsWith(".wsz"))
                unzip(fileInfo.filePath (), cache_dir.absolutePath (), true);
        }
    }
    //add thumbs to map
    d = cache_dir.entryInfoList();
    foreach(QFileInfo fileInfo, f)
    {
         foreach(QFileInfo thumbInfo, d)
         {
            if (fileInfo.baseName () == thumbInfo.baseName ())
            {
                m_previewMap.insert(fileInfo.absoluteFilePath (),
                                    thumbInfo.absoluteFilePath ());
                break;
            }
         }
    }
}

void SkinReader::unpackSkin(const QString &path)
{
    //remove old skin
    QDir dir(QDir::homePath() +"/.qmmp/cache/skin");
    dir.setFilter( QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QFileInfoList f = dir.entryInfoList();
    foreach(QFileInfo file, f)
        dir.remove(file.fileName());
    //unpack
    if (path.endsWith(".tgz") || path.endsWith(".tar.gz") || path.endsWith(".tar.bz2"))
        untar(path, QDir::homePath() +"/.qmmp/cache/skin", FALSE);
    if (path.endsWith(".zip") || path.endsWith(".wsz"))
        unzip(path, QDir::homePath() +"/.qmmp/cache/skin", FALSE);
}

const QStringList SkinReader::skins()
{
    return m_previewMap.keys();
}

const QPixmap SkinReader::getPreview(const QString &skinPath)
{
    return QPixmap(m_previewMap.value(skinPath));
}

void SkinReader::untar(const QString &from, const QString &to, bool preview)
{
    QByteArray array;
    QStringList args;
    //list archive
    args << "tf" <<from;
    m_process->start("tar", args);
    m_process->waitForFinished();
    array = m_process->readAllStandardOutput ();
    QString str = QString(array);
    QStringList outputList = str.split("\n", QString::SkipEmptyParts);
    foreach(QString str, outputList)
    {
        str = str.trimmed();
        args.clear();
        if (!preview || (str.contains("/main.", Qt::CaseInsensitive)
                         || str.startsWith("main.", Qt::CaseInsensitive)))
        {
            args << "xvfk" << from << "-O" << str;
            m_process->start("tar", args);
            m_process->waitForStarted();
            m_process->waitForFinished();
            array = m_process->readAllStandardOutput ();

            QString name;
            if (preview)
                name = from.section('/',-1) + (".") + str.section('.', -1);
            else
                name = str.contains('/') ? str.section('/',-1).toLower() : str.toLower();

            QFile file(to+"/"+name);
            file.open(QIODevice::WriteOnly);
            file.write(array);
            file.close();
        }
    }
}

void SkinReader::unzip(const QString &from, const QString &to, bool preview)
{
    QStringList args;
    if (preview)
    {
        args << "-C" << "-j" << "-o" << "-qq" << "-d" << to << from << "main.*" << "*/main.*";
        QProcess::execute("unzip", args);
        QDir dir(to);
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        QFileInfoList fileList = dir.entryInfoList();
        foreach(QFileInfo thumbInfo, fileList)
        {
            if (thumbInfo.fileName().startsWith("main.", Qt::CaseInsensitive))
            {
                dir.rename(thumbInfo.fileName(), from.section('/', -1) +
                           "." + thumbInfo.suffix ());
            }
        }
    }
    else
    {
        args << "-j" << "-o" << "-qq" << "-d" << to << from;
        QProcess::execute("unzip", args);
    }
}
