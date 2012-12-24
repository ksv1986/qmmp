/***************************************************************************
 *   Copyright (C) 2009-2012 by Ilya Kotov                                 *
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

#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QMutexLocker>
#include "decoder.h"
#include "decoderfactory.h"
#include "abstractengine.h"
#include "inputsource.h"
#include "qmmpsettings.h"
#include "metadatamanager.h"

MetaDataManager* MetaDataManager::m_instance = 0;

MetaDataManager::MetaDataManager() : m_mutex(QMutex::Recursive)
{
    if(m_instance)
        qFatal("MetaDataManager is already created");
    m_instance = this;
    m_settings = QmmpSettings::instance();
}

MetaDataManager::~MetaDataManager()
{
    m_instance = 0;
}

QList <FileInfo *> MetaDataManager::createPlayList(const QString &fileName, bool useMetaData) const
{
    QMutexLocker locker(&m_mutex);
    QList <FileInfo *> list;
    DecoderFactory *fact = 0;
    EngineFactory *efact = 0;

    if (!fileName.contains("://")) //local file
    {
        if(!QFile::exists(fileName))
            return list;
        else if((fact = Decoder::findByPath(fileName, m_settings->determineFileTypeByContent())))
            return fact->createPlayList(fileName, useMetaData);
        else if((efact = AbstractEngine::findByPath(fileName)))
            return efact->createPlayList(fileName, useMetaData);
        return list;
    }
    else
    {
        QString scheme = fileName.section("://",0,0);
        if(InputSource::protocols().contains(scheme))
        {
            list << new FileInfo(fileName);
            return list;
        }
        foreach(fact, *Decoder::factories())
        {
            if(fact->properties().protocols.contains(scheme) && Decoder::isEnabled(fact))
                return fact->createPlayList(fileName, useMetaData);
        }
    }
    return list;
}

MetaDataModel* MetaDataManager::createMetaDataModel(const QString &path, QObject *parent) const
{
    DecoderFactory *fact = 0;
    EngineFactory *efact = 0;

    if (!path.contains("://")) //local file
    {
        if(!QFile::exists(path))
            return 0;
        else if((fact = Decoder::findByPath(path, m_settings->determineFileTypeByContent())))
            return fact->createMetaDataModel(path, parent);
        else if((efact = AbstractEngine::findByPath(path)))
            return efact->createMetaDataModel(path, parent);
        return 0;
    }
    else
    {
        QString scheme = path.section("://",0,0);
        MetaDataModel *model = 0;
        if((fact = Decoder::findByProtocol(scheme)))
        {
            return fact->createMetaDataModel(path, parent);
        }
        foreach(efact, *AbstractEngine::factories())
        {
            if(efact->properties().protocols.contains(scheme) && AbstractEngine::isEnabled(efact))
                model = efact->createMetaDataModel(path, parent);
            if(model)
                return model;
        }
    }
    return 0;
}

QStringList MetaDataManager::filters() const
{
    QStringList filters;
    foreach(DecoderFactory *fact, *Decoder::factories())
    {
        if (Decoder::isEnabled(fact) && !fact->properties().filters.isEmpty())
            filters << fact->properties().description + " (" + fact->properties().filters.join(" ") + ")";
    }
    foreach(EngineFactory *fact, *AbstractEngine::factories())
    {
        if (AbstractEngine::isEnabled(fact) && !fact->properties().filters.isEmpty())
            filters << fact->properties().description + " (" + fact->properties().filters.join(" ") + ")";
    }
    return filters;
}

QStringList MetaDataManager::nameFilters() const
{
    QStringList filters;
    foreach(DecoderFactory *fact, *Decoder::factories())
    {
        if (Decoder::isEnabled(fact))
            filters << fact->properties().filters;
    }
    foreach(EngineFactory *fact, *AbstractEngine::factories())
    {
        if (AbstractEngine::isEnabled(fact))
            filters << fact->properties().filters;
    }
    if(m_settings->determineFileTypeByContent())
        filters << "*";
    return filters;
}

QStringList MetaDataManager::protocols() const
{
    QStringList p;
    p << InputSource::protocols();
    p << Decoder::protocols();
    p << AbstractEngine::protocols();
    p.removeDuplicates();
    return p;
}

bool MetaDataManager::supports(const QString &fileName) const
{
    DecoderFactory *fact = 0;
    EngineFactory *efact = 0;
    if (!fileName.contains("://")) //local file
    {
        if (!QFile::exists(fileName))
            return false;
        if((fact = Decoder::findByPath(fileName)))
            return true;
        else if((efact = AbstractEngine::findByPath(fileName)))
            return true;
        return false;
    }
    return false;
}

QPixmap MetaDataManager::getCover(const QString &url)
{
    if(!url.contains("://") && m_settings->useCoverFiles())
    {
        QString p = getCoverPath(url);
        if(!p.isEmpty())
            return QPixmap(p);
    }
    MetaDataModel *model = createMetaDataModel(url);
    if(model)
    {
        QPixmap pix = model->cover();
        model->deleteLater();
        return pix;
    }
    return QPixmap();
}

QString MetaDataManager::getCoverPath(const QString &url)
{
    if(!m_settings->useCoverFiles())
        return QString();
    if(url.contains("://")) //url
    {
        MetaDataModel *model = createMetaDataModel(url);
        if(model)
        {
            QString coverPath = model->coverPath();
            model->deleteLater();
            return coverPath;
        }
    }
    else //local file
    {
        QString p = QFileInfo(url).absolutePath();
        if(m_cover_cache.keys().contains(p))
            return m_cover_cache.value(p);
        QFileInfoList l = findCoverFiles(p, m_settings->coverSearchDepth());
        QString cover_path = l.isEmpty() ? QString() : l.at(0).filePath();
        m_cover_cache.insert (p, cover_path);
        return cover_path;
    }
    return QString();
}

QFileInfoList MetaDataManager::findCoverFiles(QDir dir, int depth) const
{
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList file_list = dir.entryInfoList(m_settings->coverNameFilters());
    foreach(QFileInfo i, file_list)
    {
        foreach(QString pattern, m_settings->coverNameFilters(false))
        {
            if(QRegExp (pattern, Qt::CaseInsensitive, QRegExp::Wildcard).exactMatch(i.fileName()))
            {
                file_list.removeAll(i);
                break;
            }
        }
    }
    if(!depth || !file_list.isEmpty())
        return file_list;
    depth--;
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name);
    QFileInfoList dir_info_list = dir.entryInfoList();
    foreach(QFileInfo i, dir_info_list)
    {
        file_list << findCoverFiles(QDir(i.absoluteFilePath()), depth);
    }
    return file_list;
}

void MetaDataManager::clearCoverChache()
{
    m_cover_cache.clear();
}

void MetaDataManager::prepareForAnotherThread()
{
    protocols(); //this hack should load all requied plugins
}

MetaDataManager *MetaDataManager::instance()
{
    if(!m_instance)
        new MetaDataManager();
    return m_instance;
}

void MetaDataManager::destroy()
{
    if(m_instance)
        delete m_instance;
}
