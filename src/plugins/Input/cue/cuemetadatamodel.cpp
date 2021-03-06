/***************************************************************************
 *   Copyright (C) 2009-2020 by Ilya Kotov                                 *
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
#include "cuefile.h"
#include "cuemetadatamodel.h"

CUEMetaDataModel::CUEMetaDataModel(const QString &url) : MetaDataModel(true)
{
    m_cueFile = new CueFile(url);
    if (m_cueFile->count() == 0)
    {
        qWarning("CUEMetaDataModel: invalid cue file");
        return;
    }
    int track = url.section("#", -1).toInt();
    m_path = m_cueFile->dataFilePath(track);
}

CUEMetaDataModel::~CUEMetaDataModel()
{
    delete m_cueFile;
}

QList<MetaDataItem> CUEMetaDataModel::extraProperties() const
{
    QList<MetaDataItem> ep;
    MetaDataModel *model = MetaDataManager::instance()->createMetaDataModel(m_path, true);
    if(model)
    {
        ep = model->extraProperties();
        delete model;
    }
    return ep;
}

QString CUEMetaDataModel::coverPath() const
{
    return MetaDataManager::instance()->findCoverFile(m_path);
}
