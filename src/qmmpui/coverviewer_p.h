/***************************************************************************
 *   Copyright (C) 2017-2020 by Ilya Kotov                                 *
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
#ifndef COVERVIEWER_H
#define COVERVIEWER_H

#include <QWidget>
#include <QPixmap>

/**
   @internal
   @author Ilya Kotov <forkotov02@ya.ru>
*/

class CoverViewer : public QWidget
{
Q_OBJECT
public:
    CoverViewer(QWidget *parent = nullptr);

    ~CoverViewer();

    void setPixmap(const QPixmap&);
    bool hasPixmap() const;
    const QPixmap &pixmap() const;

public slots:
    void saveAs();
    void load();
    void clear();

private:
    void paintEvent(QPaintEvent *) override;
    QPixmap m_pixmap;
    QString m_lastDir;
};

#endif
