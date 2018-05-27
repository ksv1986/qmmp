/***************************************************************************
 *   Copyright (C) 2009-2018 by Ilya Kotov                                 *
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
#include "cueparser.h"
#include "cuemetadatamodel.h"

CUEMetaDataModel::CUEMetaDataModel(const QString &url, QObject *parent) : MetaDataModel(parent)
{
    m_parser = new CUEParser(url, TrackInfo::AllParts);
    if (m_parser->count() == 0)
    {
        qWarning("CUEMetaDataModel: invalid cue file");
        return;
    }
    int track = url.section("#", -1).toInt();
    m_path = m_parser->filePath(track);
}

CUEMetaDataModel::~CUEMetaDataModel()
{
    delete m_parser;
}

QHash<QString, QString> CUEMetaDataModel::audioProperties()
{
    QHash <QString, QString> ap;
    MetaDataModel *model = MetaDataManager::instance()->createMetaDataModel(m_path);
    if(model)
    {
        ap = model->audioProperties();
        model->deleteLater();
    }
    return ap;
}

QString CUEMetaDataModel::coverPath()
{
    return MetaDataManager::instance()->findCoverFile(m_path);
}
