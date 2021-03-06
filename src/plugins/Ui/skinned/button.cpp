/***************************************************************************
 *   Copyright (C) 2007-2020 by Ilya Kotov                                 *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   Based on Promoe, an XMMS2 Client                                      *
 *   Copyright (C) 2005-2006 by XMMS2 Team                                 *
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

#include "button.h"
#include "skin.h"
#include <QMouseEvent>

Button::Button (QWidget *parent, uint normal, uint pressed, uint cursor)
        : PixmapWidget (parent)
{
    name_normal = normal;
    name_pressed = pressed;
    name_cursor = cursor;
    m_skin = Skin::instance();
    setON (false);
    setCursor (m_skin->getCursor (name_cursor));
    connect (m_skin, SIGNAL (skinChanged()), this, SLOT (updateSkin()));
}


Button::~Button()
{}

void Button::updateSkin()
{
    setPixmap (m_skin->getButton (name_normal));
    setCursor (m_skin->getCursor (name_cursor));
}

void Button::setON (bool on)
{
    if (on)
        setPixmap (m_skin->getButton (name_pressed));
    else
        setPixmap (m_skin->getButton (name_normal));
}
void Button::mousePressEvent (QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;
    setON (true);
    m_pressed = true;
    QWidget::mousePressEvent(e);
}

void Button::mouseReleaseEvent (QMouseEvent *e)
{
    if (!m_pressed)
        return;
    m_pressed = false;
    if(rect().contains(e->pos()))
    {
        setON (false);
        emit clicked();
    }
}

void Button::mouseMoveEvent (QMouseEvent *e)
{
    setON (m_pressed && rect().contains(e->pos()));
}
