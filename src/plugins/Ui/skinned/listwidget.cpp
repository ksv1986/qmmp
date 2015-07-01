/***************************************************************************
 *   Copyright (C) 2006-2015 by Ilya Kotov                                 *
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
#include <QPixmap>
#include <QResizeEvent>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QSettings>
#include <QMenu>
#include <QUrl>
#include <QApplication>
#include <QHelpEvent>
#include <QTimer>
#include <qmmpui/playlistitem.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/playlistmanager.h>
#include "listwidget.h"
#include "playlistheader.h"
#include "actionmanager.h"
#include "skin.h"
#include "popupwidget.h"
#include "playlist.h"

#define INVALID_INDEX -1

ListWidget::ListWidget(QWidget *parent)
        : QWidget(parent)
{
    m_popupWidget = 0;

    m_skin = Skin::instance();
    m_ui_settings = QmmpUiSettings::instance();
    m_menu = new QMenu(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(50);

    m_header = new PlayListHeader(this);
    m_update = false;
    m_drop_index = INVALID_INDEX;
    m_scroll_direction = NONE;
    m_prev_y = 0;
    m_anchor_index = INVALID_INDEX;
    m_pressed_index = INVALID_INDEX;
    m_first = 0;
    m_row_count = 0;
    m_select_on_release = false;

    setAcceptDrops(true);
    setMouseTracking(true);

    readSettings();
    connect(m_skin, SIGNAL(skinChanged()), SLOT(updateSkin()));
    connect(m_ui_settings, SIGNAL(repeatableTrackChanged(bool)), SLOT(updateRepeatIndicator()));
    connect(m_timer, SIGNAL(timeout()), SLOT(autoscroll()));
    SET_ACTION(ActionManager::PL_SHOW_HEADER, this, SLOT(readSettings()));
}

ListWidget::~ListWidget()
{
    qDeleteAll(m_rows);
    m_rows.clear();
}

void ListWidget::readSettings()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("Skinned");
    m_show_protocol = settings.value ("pl_show_protocol", false).toBool();
    bool show_popup = settings.value("pl_show_popup", false).toBool();

    m_header->readSettings();
    m_header->setVisible(ACTION(ActionManager::PL_SHOW_HEADER)->isChecked());
    m_header->setGeometry(0,0,width(), m_header->requiredHeight());

    if (m_update)
    {
        m_drawer.readSettings();
        m_row_count = (height() - (m_header->isVisibleTo(this) ? m_header->height() : 0)) / m_drawer.rowHeight();
        updateList(PlayListModel::STRUCTURE);
        if(m_popupWidget)
        {
            m_popupWidget->deleteLater();
            m_popupWidget = 0;
        }
    }
    else
    {
        m_update = true;
    }

    if(show_popup)
        m_popupWidget = new PlayListPopup::PopupWidget(this);
}

int ListWidget::visibleRows() const
{
    return m_row_count;
}

int ListWidget::firstVisibleIndex() const
{
    return m_first;
}

int ListWidget::anchorIndex() const
{
    return m_anchor_index;
}

void ListWidget::setAnchorIndex(int index)
{
    m_anchor_index = index;
    updateList(PlayListModel::CURRENT);
}

QMenu *ListWidget::menu()
{
    return m_menu;
}

PlayListModel *ListWidget::model()
{
    Q_ASSERT(m_model);
    return m_model;
}

void ListWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    m_drawer.fillBackground(&painter, width(), height());
#if QT_VERSION >= 0x040700
    painter.setLayoutDirection(Qt::LayoutDirectionAuto);
#endif
    bool rtl = (layoutDirection() == Qt::RightToLeft);

    for (int i = 0; i < m_rows.size(); ++i )
    {
        m_drawer.drawBackground(&painter, m_rows[i]);

        if(m_rows[i]->flags & ListWidgetRow::GROUP)
        {
            m_drawer.drawSeparator(&painter, m_rows[i], rtl);
            continue;
        }

        m_drawer.drawTrack(&painter, m_rows[i], rtl);
    }
    //draw drop line
    if(m_drop_index != INVALID_INDEX)
    {
        m_drawer.drawDropLine(&painter, m_drop_index - m_first, width(),
                              m_header->isVisible() ? m_header->height() : 0);
    }
}

void ListWidget::mouseDoubleClickEvent (QMouseEvent *e)
{
    int y = e->y();
    int index = indexAt(y);
    if (INVALID_INDEX != index)
    {
        m_model->setCurrent(index);
        emit selectionChanged();
        update();
    }
}

void ListWidget::mousePressEvent(QMouseEvent *e)
{
    if(m_popupWidget)
        m_popupWidget->hide();

    int index = indexAt(e->y());

    if (INVALID_INDEX != index && m_model->count() > index)
    {
        m_pressed_index = index;
        if(e->button() == Qt::RightButton)
        {
            if(!m_model->isSelected(index))
            {
                m_anchor_index = m_pressed_index;
                m_model->clearSelection();
                m_model->setSelected(index, true);
            }
            if(m_model->isGroup(index) && m_model->selectedTracks().isEmpty())
            {
                m_anchor_index = m_pressed_index;
                PlayListGroup *group = m_model->group(index);
                m_model->setSelected(group->tracks());
            }
            QWidget::mousePressEvent(e);
            return;
        }

        if (m_model->isSelected(index) && (e->modifiers() == Qt::NoModifier))
        {
            m_select_on_release = true;
            QWidget::mousePressEvent(e);
            return;
        }

        if ((Qt::ShiftModifier & e->modifiers()))
        {
            int prev_anchor_index = m_anchor_index;
            m_anchor_index = m_pressed_index;
            m_model->setSelected(m_pressed_index, prev_anchor_index, true);
        }
        else //ShiftModifier released
        {
            m_anchor_index = m_pressed_index;
            if ((Qt::ControlModifier & e->modifiers()))
            {
                m_model->setSelected(index, !m_model->isSelected(index));
            }
            else //ControlModifier released
            {
                m_model->clearSelection();
                m_model->setSelected(index, true);
            }

        }
        update();
    }
    QWidget::mousePressEvent(e);
}

void ListWidget::resizeEvent(QResizeEvent *e)
{
    m_header->setGeometry(0,0,width(), m_header->requiredHeight());
    m_row_count = (e->size().height() - (m_header->isVisibleTo(this) ? m_header->height() : 0)) / m_drawer.rowHeight();
    m_row_count = qMax(m_row_count, 0);
    updateList(PlayListModel::STRUCTURE);
    QWidget::resizeEvent(e);
}

void ListWidget::wheelEvent (QWheelEvent *e)
{
    if (m_model->count() <= m_row_count)
        return;
    if ((m_first == 0 && e->delta() > 0) ||
            ((m_first == m_model->count() - m_row_count) && e->delta() < 0))
        return;
    m_first -= e->delta()/40;  //40*3 TODO: add step to config
    if (m_first < 0)
        m_first = 0;

    if (m_first > m_model->count() - m_row_count)
        m_first = m_model->count() - m_row_count;

    updateList(PlayListModel::STRUCTURE);
}

bool ListWidget::event (QEvent *e)
{
    if(m_popupWidget)
    {
        if(e->type() == QEvent::ToolTip)
        {
            QHelpEvent *helpEvent = (QHelpEvent *) e;
            int index = indexAt(helpEvent->y());
            if(index < 0 || !m_model->isTrack(index))
            {
                m_popupWidget->deactivate();
                return QWidget::event(e);
            }
            e->accept();
            m_popupWidget->prepare(m_model->track(index), helpEvent->globalPos());
            return true;
        }
        else if(e->type() == QEvent::Leave)
            m_popupWidget->deactivate();
    }
    return QWidget::event(e);
}

void ListWidget::updateList(int flags)
{
    if(flags & PlayListModel::CURRENT)
        recenterCurrent();

    QList<PlayListItem *> items;

    if(flags & PlayListModel::STRUCTURE || flags & PlayListModel::CURRENT)
    {
        if(m_row_count >= m_model->count())
        {
            m_first = 0;
            emit positionChanged(0,0);
        }
        else if(m_first + m_row_count >= m_model->count())
        {
            m_first = qMax(0, m_model->count() - m_row_count);
            emit positionChanged(m_first, m_first);
        }
        else
            emit positionChanged(m_first, m_model->count() - m_row_count);

        items = m_model->mid(m_first, m_row_count);

        while(m_rows.count() < qMin(m_row_count, items.count()))
            m_rows << new ListWidgetRow;
        while(m_rows.count() > qMin(m_row_count, items.count()))
            delete m_rows.takeFirst();
    }
    else
    {
        items = m_model->mid(m_first, m_row_count);
    }

    if(flags & PlayListModel::STRUCTURE)
        m_header->hideSortIndicator();

    if(flags & PlayListModel::STRUCTURE || flags & PlayListModel::METADATA)
    {
        //song numbers width
        m_drawer.calculateNumberWidth(m_model->trackCount());
        m_drawer.setSingleColumnMode(m_model->columnCount() == 1);
        m_header->setNumberWidth(m_drawer.numberWidth());
    }

    int prev_number = 0;
    int trackStateColumn = m_header->trackStateColumn();

    for(int i = 0; i < items.count(); ++i)
    {
        ListWidgetRow *row = m_rows[i];
        row->trackStateColumn = trackStateColumn;
        items[i]->isSelected() ? row->flags |= ListWidgetRow::SELECTED :
                row->flags &= ~ListWidgetRow::SELECTED;

        i == (m_anchor_index - m_first) ? row->flags |= ListWidgetRow::ANCHOR :
                row->flags &= ~ListWidgetRow::ANCHOR;

        if(flags == PlayListModel::SELECTION)
            continue;

        row->rect = QRect(5, (m_header->isVisibleTo(this) ? m_header->height() : 0) + i * m_drawer.rowHeight(),
                          width() - 10, m_drawer.rowHeight() - 1);
        row->titles = items[i]->formattedTitles();
        row->sizes = m_header->sizes();

        (m_first + i) == m_model->currentIndex() ? row->flags |= ListWidgetRow::CURRENT :
                row->flags &= ~ListWidgetRow::CURRENT;

        if(items[i]->isGroup())
        {
            row->flags |= ListWidgetRow::GROUP;
            row->number = 0;
            row->length.clear();
        }
        else
        {
            row->flags &= ~ListWidgetRow::GROUP;
            //optimization: reduces number of PlaListModel::numberOfTrack(int) calls
            if(!prev_number)
            {
                row->number = m_model->indexOfTrack(m_first+i) + 1;
                prev_number = row->number;
            }
            else
            {
                row->number = ++prev_number;
            }
            row->length = items[i]->formattedLength();
            row->extraString = getExtraString(m_first + i);
        }
        m_drawer.prepareRow(row);  //elide titles
    }
    update();
}

void ListWidget::autoscroll()
{
    SimpleSelection sel = m_model->getSelection(m_pressed_index);
    if ((sel.m_top == 0 && m_scroll_direction == TOP && sel.count() > 1) ||
        (sel.m_bottom == m_model->count() - 1 && m_scroll_direction == DOWN && sel.count() > 1))
        return;

    if(m_scroll_direction == DOWN)
    {
        int row = m_first + m_row_count;
        (m_first + m_row_count < m_model->count()) ? m_first ++ : m_first;
        m_model->moveItems(m_pressed_index,row);
        m_pressed_index = row;
    }
    else if(m_scroll_direction == TOP && m_first > 0)
    {
        m_first--;
        m_model->moveItems(m_pressed_index, m_first);
        m_pressed_index = m_first;
    }
}

void ListWidget::updateRepeatIndicator()
{
    updateList(PlayListModel::CURRENT | PlayListModel::STRUCTURE);
}

void ListWidget::scrollToCurrent()
{
    updateList(PlayListModel::CURRENT | PlayListModel::STRUCTURE);
}

void ListWidget::setModel(PlayListModel *selected, PlayListModel *previous)
{
    if(previous)
    {
        previous->setProperty("first_visible", m_first);
        disconnect(previous, 0, this, 0); //disconnect previous model
        disconnect(previous,0,m_header,0);
    }
    qApp->processEvents();
    m_model = selected;

    if(m_model->property("first_visible").isValid())
    {
        m_first = m_model->property("first_visible").toInt();
        updateList(PlayListModel::STRUCTURE);
    }
    else
    {
        m_first = 0;
        updateList(PlayListModel::STRUCTURE | PlayListModel::CURRENT);
    }
    connect (m_model, SIGNAL(currentVisibleRequest()), SLOT(scrollToCurrent()));
    connect (m_model, SIGNAL(listChanged(int)), SLOT(updateList(int)));
    connect (m_model, SIGNAL(sortingByColumnFinished(int,bool)), m_header, SLOT(showSortIndicator(int,bool)));
}

void ListWidget::scroll(int sc)
{
    if (m_model->count() <= m_row_count)
        return;
    m_first = sc; //*(m_model->count() - m_rows)/99;
    updateList(PlayListModel::STRUCTURE);
}

void ListWidget::updateSkin()
{
    m_drawer.loadColors();
    update();
}

void ListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}


void ListWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        QList<QUrl> list_urls = event->mimeData()->urls();
        event->acceptProposedAction();
        QApplication::restoreOverrideCursor();

        int index = indexAt(event->pos().y());
        if(index == INVALID_INDEX)
        {
            index = qMin(m_first + m_row_count, m_model->count());
        }
        m_model->insert(index, list_urls);
    }
    m_drop_index = INVALID_INDEX;
}

void ListWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    m_drop_index = INVALID_INDEX;
    update();
}

void ListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    int index = indexAt(event->pos().y());
    if(index == INVALID_INDEX)
        index = qMin(m_first + m_row_count, m_model->count());
    if(index != m_drop_index)
    {
        m_drop_index = index;
        update();
    }
}

const QString ListWidget::getExtraString(int i)
{
    QString extra_string;

    PlayListTrack *track = m_model->track(i);
    if(!track)
        return extra_string;

    if (m_show_protocol && track->url().contains("://"))
        extra_string = "[" + track->url().split("://").at(0) + "]";

    if (m_model->isQueued(track))
    {
        int index = m_model->queuedIndex(track);
        extra_string += "|"+QString::number(index + 1)+"|";
    }

    if(m_model->currentIndex() == i && m_ui_settings->isRepeatableTrack())
        extra_string += "|R|";
    else if(m_model->isStopAfter(track))
        extra_string += "|S|";

    return extra_string.trimmed(); //remove white space
}

void ListWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons() == Qt::LeftButton)
    {
        if (m_prev_y > e->y())
            m_scroll_direction = TOP;
        else if (m_prev_y < e->y())
            m_scroll_direction = DOWN;
        else
            m_scroll_direction = NONE;

        if(e->y() < 0 || e->y() > height())
        {
            if(!m_timer->isActive())
                m_timer->start();
            return;
        }
        m_timer->stop();

        int index = indexAt(e->y());

        if (INVALID_INDEX != index)
        {
            m_anchor_index = index;
            SimpleSelection sel = m_model->getSelection(m_pressed_index);
            if(sel.count() > 1 && m_scroll_direction == TOP)
            {
                if(sel.m_top == 0 || sel.m_top == m_first)
                    return;
            }
            else if(sel.count() > 1 && m_scroll_direction == DOWN)
            {
                if(sel.m_bottom == m_model->count() - 1 || sel.m_bottom == m_first + m_row_count)
                    return;
            }
            m_model->moveItems(m_pressed_index,index);

            m_prev_y = e->y();
            m_pressed_index = index;
        }
    }
    else if(m_popupWidget)
    {
        int index = indexAt(e->y());
        if(index < 0 || !m_model->isTrack(index) || m_popupWidget->url() != m_model->track(index)->url())
            m_popupWidget->deactivate();
    }
}

void ListWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_select_on_release)
    {
        m_model->clearSelection();
        m_model->setSelected(m_pressed_index,true);
        m_anchor_index = m_pressed_index;
        m_select_on_release = false;
    }
    m_pressed_index = INVALID_INDEX;
    m_scroll_direction = NONE;
    m_timer->stop();
    QWidget::mouseReleaseEvent(e);
}

int ListWidget::indexAt(int y) const
{
    y -= m_header->isVisible() ? m_header->height() : 0;
    for (int i = 0; i < qMin(m_row_count, m_model->count() - m_first); ++i)
    {
        if ((y >= i * m_drawer.rowHeight()) && (y <= (i+1) * m_drawer.rowHeight()))
            return m_first + i;
    }
    return INVALID_INDEX;
}

void ListWidget::contextMenuEvent(QContextMenuEvent * event)
{
    if (menu())
        menu()->exec(event->globalPos());
}

void ListWidget::recenterCurrent()
{
    if (m_row_count)
    {
        if (m_first + m_row_count < m_model->currentIndex() + 1)
            m_first = qMin(m_model->count() - m_row_count,
                           m_model->currentIndex() - m_row_count/2);
        else if (m_first > m_model->currentIndex())
            m_first = qMax (m_model->currentIndex() - m_row_count/2, 0);
    }
}
