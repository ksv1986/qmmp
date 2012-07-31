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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QMap>
#include <qmmp/qmmp.h>

class FileInfo;
class QSettings;

/** @brief The PlayListItem class provides an item for use with the PlayListModel class.
 * @author Ilya Kotov <forkotov02@hotmail.ru>
 */
class PlayListItem : public QMap <Qmmp::MetaData, QString>
{
public:
    /*!
     * Current state of playlist item.
     * FREE - instance is free and may be deleted
     * EDITING - instance is currently busy in some kind of operation(tags editing etc.)
     * and can't be deleted at the moment. Set flag SCHEDULED_FOR_DELETION for it
     * instead of delete operator call.
     */
    enum FLAGS
    {
        FREE = 0,              /*!< instance is free and may be deleted */
        EDITING,               /*!< instance is currently busy */
        SCHEDULED_FOR_DELETION /*!< instance is sheduled for deletion */
    };
    /*!
     * Constructs an empty plalist item.
     */
    PlayListItem();
    /*!
     * Constructs a new PlayListItem that is a copy of the given \b item
     */
    PlayListItem(const PlayListItem &item);
    /*!
     * Constructs plalist item with given metadata.
     * @param info Media file information.
     */
    PlayListItem(FileInfo *info);
    /*!
     * Object destructor.
     */
    ~PlayListItem();
    /*!
     * Sets item selection flag to \b select
     * @param select State of selection (\b true select, \b false unselect)
     */
    void setSelected(bool select);
    /*!
     * Return \b true if item is selected, otherwise returns \b false.
     */
    bool isSelected() const;
    /*!
     * It is used by PlayListModel class.
     */
    void setCurrent(bool yes);
    /*!
     * Returns \b true if the item is the current item; otherwise returns returns \b false.
     */
    bool isCurrent() const;
    /*!
     * Returns current state of the playlist item.
     */
    FLAGS flag() const;
    /*!
     * Sets state of the playlist item.
     */
    void setFlag(FLAGS);
    /*!
     * Returns item short title.
     */
    const QString text();
    /*!
     * Direct access to the item short title.
     * @param title New short title.
     */
    void setText(const QString &title);
    /*!
     * Returns song length in seconds.
     */
    qint64 length() const;
    /*!
     * Sets length in seconds.
     */
    void setLength(qint64 length);
    /*!
     * Same as url()
     */
    const QString url() const;
    /*!
     * Updates current metadata.
     * @param metaData Map with metadata values.
     */
    void updateMetaData(const QMap <Qmmp::MetaData, QString> &metaData);
    /*!
     * Gets new metadata from file (works for local files only).
     */
    void updateTags();

private:
    void readMetadata();
    QString m_title;
    FileInfo *m_info;
    bool m_selected;
    bool m_current;
    FLAGS m_flag;
    qint64 m_length;
};

#endif
