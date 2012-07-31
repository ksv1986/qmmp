/***************************************************************************
 *   Copyright (C) 2006-2008 by Ilya Kotov                                 *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#ifndef SYMBOLDISPLAY_H
#define SYMBOLDISPLAY_H

#include <QPixmap>

#include "pixmapwidget.h"

/**
   @author Vladimir Kuznetsov <vovanec@gmail.com>
 */

class Skin;

class SymbolDisplay : public PixmapWidget
{
    Q_OBJECT
public:
    SymbolDisplay(QWidget *parent = 0, int digits = 3);

    ~SymbolDisplay();

    void setAlignment(Qt::Alignment a)
    {
        m_alignment = a;
    }
    Qt::Alignment alignment()const
    {
        return m_alignment;
    }

public slots:
    void display(const QString&);
    void display(int);

private slots:
    void draw();

private:
    Skin* m_skin;
    QPixmap m_pixmap;
    int m_digits;
    QString m_text;
    Qt::Alignment m_alignment;
    int m_max;

};

#endif
