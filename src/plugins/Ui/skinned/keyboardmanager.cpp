/***************************************************************************
 *   Copyright (C) 2006-2009 by Ilya Kotov                                 *
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


#include <QKeyEvent>
#include <qmmpui/playlistmodel.h>
#include "playlist.h"
#include "listwidget.h"
#include "keyboardmanager.h"
#include "mainwindow.h"


KeyboardManager::KeyboardManager (PlayList* pl)
{
    m_playlist = pl;
}

bool KeyboardManager::handleKeyPress (QKeyEvent* ke)
{
    bool handled = true;
    switch (ke->key())
    {
        case Qt::Key_Up:
            keyUp (ke);
            break;
        case Qt::Key_Down:
            keyDown (ke);
            break;
        case Qt::Key_PageUp:
            keyPgUp (ke);
            break;
        case Qt::Key_PageDown:
            keyPgDown (ke);
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            keyEnter (ke);
            break;
        case Qt::Key_Home:
            keyHome(ke);
            break;
         case Qt::Key_End:
            keyEnd(ke);
            break;
        default:
            handled = false;
    }
    return handled;
}

bool KeyboardManager::handleKeyRelease (QKeyEvent*)
{
    return false;
}

void KeyboardManager::keyUp (QKeyEvent * ke)
{
    QList<int> rows = m_playlist->listWidget()->model()->selectedRows();
    ListWidget* list_widget = m_playlist->listWidget();

    if (rows.count() > 0)
    {
       if(rows.first() == 0 && rows.count() == 1)
          return;

        if (! (ke->modifiers() & Qt::ShiftModifier || ke->modifiers() & Qt::AltModifier))
        {
            m_playlist->listWidget()->model()->clearSelection();
            list_widget->setAnchorRow(-1);
        }

        bool select_top = false;
        int first_visible = list_widget->firstVisibleRow();
        int last_visible = list_widget->visibleRows() + first_visible - 1;
        foreach (int i, rows)
        {
            if (i > last_visible || i < first_visible)
            {
                select_top = true;
                break;
            }
        }

        if (!select_top || ke->modifiers() & Qt::ShiftModifier || ke->modifiers() & Qt::AltModifier)
        {
            if (ke->modifiers() == Qt::AltModifier)
            {
                if(rows.first() == 0)
                    return;
                m_playlist->listWidget()->model()->moveItems (rows.first(),rows.first() - 1);
                list_widget->setAnchorRow (list_widget->anchorRow() - 1);
            }
            else
            {
                if (rows.last() > list_widget->anchorRow() && ke->modifiers() & Qt::ShiftModifier)
                {
                    m_playlist->listWidget()->model()->setSelected (rows.last(),false);
                }
                else if (rows.first() > 0)
                {
                    m_playlist->listWidget()->model()->setSelected (rows.first() - 1,true);
                }
                else
                {
                    m_playlist->listWidget()->model()->setSelected (rows.first(),true);
                    if(list_widget->anchorRow() == -1)
                     list_widget->setAnchorRow(rows.first());
                }

                if (! (ke->modifiers() & Qt::ShiftModifier) && rows.first() > 0)
                    list_widget->setAnchorRow (rows.first() - 1);
            }
        }
        else
        {
            m_playlist->listWidget()->model()->setSelected (list_widget->firstVisibleRow(),true);
            list_widget->setAnchorRow(list_widget->firstVisibleRow());
        }

        rows = m_playlist->listWidget()->model()->selectedRows();

        if (rows.first()  < list_widget->firstVisibleRow() && list_widget->firstVisibleRow() > 0)
        {
          int r = rows.last() > list_widget->anchorRow() ? rows.last(): rows.first();
          if(ke->modifiers() & Qt::ShiftModifier && (r >= list_widget->firstVisibleRow()))
              ;
            else
                list_widget->scroll (list_widget->firstVisibleRow() - 1);
        }
    }
    else
    {
       //if(list_widget->getAnchorRow() == -1)
          list_widget->setAnchorRow(list_widget->firstVisibleRow());
        m_playlist->listWidget()->model()->setSelected (list_widget->firstVisibleRow(),true);
    }
}

void KeyboardManager::keyDown (QKeyEvent * ke)
{
    QList<int> rows = m_playlist->listWidget()->model()->selectedRows();
    ListWidget* list_widget = m_playlist->listWidget();
    //qWarning("count: %d",rows.count());
    if (rows.count() > 0)
    {
        if (! (ke->modifiers() & Qt::ShiftModifier || ke->modifiers() & Qt::AltModifier))
        {
            m_playlist->listWidget()->model()->clearSelection();
            list_widget->setAnchorRow(-1);
        }

        bool select_top = false;
        int first_visible = list_widget->firstVisibleRow();
        int last_visible = list_widget->visibleRows() + first_visible - 1;
        foreach (int i, rows)
        {
            if (i > last_visible || i < first_visible)
            {
                select_top = true;
                break;
            }
        }

        if (!select_top || ke->modifiers() & Qt::ShiftModifier || ke->modifiers() & Qt::AltModifier)
        {
            if (ke->modifiers() == Qt::AltModifier)
            {
                if(rows.last() == m_playlist->listWidget()->model()->count() - 1)
                    return;
                m_playlist->listWidget()->model()->moveItems (rows.last(),rows.last() + 1);
                list_widget->setAnchorRow (list_widget->anchorRow() + 1);
            }
            else
            {
               //qWarning("list_widget %d",list_widget->getAnchorRow());
                //qWarning("model count: %d rows.last(): %d",m_playlist->listWidget()->model()->count(),rows.last());
                if (rows.first() < list_widget->anchorRow()  && ke->modifiers() & Qt::ShiftModifier)
                    m_playlist->listWidget()->model()->setSelected (rows.first(),false);
                else if (rows.last() < m_playlist->listWidget()->model()->count() - 1)
                {
                    m_playlist->listWidget()->model()->setSelected (rows.last() + 1,true);
                }
                else
                {
                    m_playlist->listWidget()->model()->setSelected (rows.last(),true);
                    if(list_widget->anchorRow() == -1)
                       list_widget->setAnchorRow(rows.last());
                }

                if (! (ke->modifiers() & Qt::ShiftModifier) && rows.last() < m_playlist->listWidget()->model()->count() - 1)
                    list_widget->setAnchorRow (rows.last() + 1);
            }
        }
        else
        {
            m_playlist->listWidget()->model()->setSelected (list_widget->firstVisibleRow(),true);
            list_widget->setAnchorRow(list_widget->firstVisibleRow());
        }

        rows = m_playlist->listWidget()->model()->selectedRows();

        if (!rows.isEmpty() && rows.last() >= list_widget->visibleRows() + list_widget->firstVisibleRow())
        {
            int r = rows.first() < list_widget->anchorRow() ? rows.first(): rows.last();
            if(ke->modifiers() & Qt::ShiftModifier &&
               (r < list_widget->firstVisibleRow() + list_widget->visibleRows()))
              ;
            else
                list_widget->scroll (list_widget->firstVisibleRow() + 1);
        }
    }
    else
    {
        m_playlist->listWidget()->model()->setSelected (list_widget->firstVisibleRow(),true);
        //if(list_widget->getAnchorRow() == -1)
           list_widget->setAnchorRow(list_widget->firstVisibleRow());
    }
}

void KeyboardManager::keyPgUp (QKeyEvent *)
{
    ListWidget* list_widget = m_playlist->listWidget();
    int page_size = list_widget->visibleRows();
    int offset= (list_widget->firstVisibleRow()-page_size >= 0) ?list_widget->firstVisibleRow()-page_size:0;
    list_widget->scroll (offset);
}

void KeyboardManager::keyPgDown (QKeyEvent *)
{
    ListWidget* list_widget = m_playlist->listWidget();
    int page_size = list_widget->visibleRows();
    int offset = (list_widget->firstVisibleRow() +page_size < m_playlist->listWidget()->model()->count()) ?
                 list_widget->firstVisibleRow() +page_size:m_playlist->listWidget()->model()->count() - 1;
    list_widget->scroll (offset);
}

void KeyboardManager::keyEnter (QKeyEvent *)
{
    QList<int> rows = m_playlist->listWidget()->model()->selectedRows();
    MainWindow* mw = qobject_cast<MainWindow*> (m_playlist->parentWidget());
    if (mw && rows.count() > 0)
    {
        m_playlist->listWidget()->model()->setCurrent (rows.first());
        mw->replay();
    }
}

void KeyboardManager::keyHome(QKeyEvent *ke)
{
    ListWidget* list_widget = m_playlist->listWidget();
    m_playlist->listWidget()->scroll (0);
    if(ke->modifiers() & Qt::ShiftModifier)
       for(int i = 0; i <= list_widget->anchorRow(); ++i)
           m_playlist->listWidget()->model()->setSelected (i,true);
}

void KeyboardManager::keyEnd(QKeyEvent *ke)
{
   ListWidget* list_widget = m_playlist->listWidget();
   int page_size = list_widget->visibleRows();
   int scroll_to = m_playlist->listWidget()->model()->count() - page_size;
   if(scroll_to >= 0)
       list_widget->scroll(scroll_to);
   if(ke->modifiers() & Qt::ShiftModifier)
       for(int i = list_widget->anchorRow(); i < m_playlist->listWidget()->model()->count(); ++i)
           m_playlist->listWidget()->model()->setSelected (i,true);
}
