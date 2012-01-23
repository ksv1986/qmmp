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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef FLACMETADATAMODEL_H
#define FLACMETADATAMODEL_H

#include <taglib/flacfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/xiphcomment.h>
#include <qmmp/metadatamodel.h>

class FLACMetaDataModel : public MetaDataModel
{
Q_OBJECT
public:
    FLACMetaDataModel(const QString &path, QObject *parent);
    ~FLACMetaDataModel();
    QHash<QString, QString> audioProperties();
    QList<TagModel* > tags();
    QPixmap cover();
    QString coverPath();

private:
    QString m_path;
    QList<TagModel* > m_tags;
};

class VorbisCommentModel : public TagModel
{
public:
    VorbisCommentModel(const QString &path);
    ~VorbisCommentModel();
    const QString name();
    const QString value(Qmmp::MetaData key);
    void setValue(Qmmp::MetaData key, const QString &value);
    void save();

private:
    TagLib::FLAC::File *m_file;
    TagLib::Ogg::FLAC::File *m_ogg_file;
    TagLib::Ogg::XiphComment *m_tag;
};

#endif // FLACMETADATAMODEL_H
