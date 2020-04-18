/***************************************************************************
 *   Copyright (C) 2006-2020 by Ilya Kotov                                 *
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
#include <QScrollBar>
#include <QMimeData>
#include <qmmpui/playlistitem.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/mediaplayer.h>
#include "listwidget.h"
#include "playlistheader.h"
#include "actionmanager.h"
#include "popupwidget.h"

#define INVALID_INDEX -1

ListWidget::ListWidget(PlayListModel *model, QWidget *parent)
        : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    m_popupWidget = nullptr;
    m_ui_settings = QmmpUiSettings::instance();
    m_menu = nullptr;
    m_filterMode = false;
    m_model = model;
    m_timer = new QTimer(this);
    m_timer->setInterval(50);

    m_header = new PlayListHeader(this);
    m_scrollBar = new QScrollBar(Qt::Vertical, this);
    m_hslider = new QScrollBar(Qt::Horizontal, this);
    m_hslider->setPageStep(50);
    m_update = false;
    m_drop_index = INVALID_INDEX;
    m_scroll_direction = NONE;
    m_prev_y = 0;
    m_anchor_index = INVALID_INDEX;
    m_pressed_index = INVALID_INDEX;
    m_first = 0;
    m_row_count = 0;
    m_select_on_release = false;
    m_count = 0;
    m_firstItem = nullptr;

    setAcceptDrops(true);
    setMouseTracking(true);

    readSettings();
    connect(m_ui_settings, SIGNAL(repeatableTrackChanged(bool)), SLOT(updateRepeatIndicator()));
    connect(m_timer, SIGNAL(timeout()), SLOT(autoscroll()));
    connect(m_scrollBar, SIGNAL(valueChanged (int)), SLOT(setViewPosition(int)));
    connect(m_hslider, SIGNAL(valueChanged(int)), m_header, SLOT(scroll(int)));
    connect(m_hslider, SIGNAL(valueChanged(int)), this, SLOT(update()));
    connect(m_model, SIGNAL(scrollToRequest(int)), SLOT(scrollTo(int)));
    connect(m_model, SIGNAL(listChanged(int)), SLOT(updateList(int)));
    connect(m_model, SIGNAL(sortingByColumnFinished(int,bool)), m_header, SLOT(showSortIndicator(int,bool)));
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
    settings.beginGroup("Simple");
    m_show_protocol = settings.value ("pl_show_protocol", false).toBool();
    bool show_popup = settings.value("pl_show_popup", false).toBool();

    m_header->readSettings();
    m_header->setVisible(ACTION(ActionManager::PL_SHOW_HEADER)->isChecked());
    m_header->setGeometry(0,0,width(), m_header->requiredHeight());

    if (m_update)
    {
        m_drawer.readSettings();
        updateList(PlayListModel::STRUCTURE);
        if(m_popupWidget)
        {
            m_popupWidget->deleteLater();
            m_popupWidget = nullptr;
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

void ListWidget::moveAnchorTo(int index)
{
    m_anchor_index = index;
    emit anchorChanged(index);
}

void ListWidget::setAnchorIndex(int index)
{
    moveAnchorTo(index);
    updateList(PlayListModel::SELECTION);
}

QMenu *ListWidget::menu()
{
    return m_menu;
}

void ListWidget::setMenu(QMenu *menu)
{
    m_menu = menu;
}

PlayListModel *ListWidget::model()
{
    Q_ASSERT(m_model);
    return m_model;
}

bool ListWidget::filterMode() const
{
    return m_filterMode;
}

void ListWidget::setModel(PlayListModel *selected, PlayListModel *previous)
{
    if(m_filterMode)
    {
        m_filterMode = false;
        m_first = 0;
        m_filteredItems.clear();
    }

    if(previous)
    {
        previous->setProperty("first_visible", m_first);
        disconnect(previous, nullptr, this, nullptr); //disconnect previous model
        disconnect(previous,nullptr,m_header,nullptr);
    }
    qApp->processEvents();
    m_model = selected;
    m_count = m_model->count();
    m_firstItem = nullptr;

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
    connect (m_model, SIGNAL(scrollToRequest(int)), SLOT(scrollTo(int)));
    connect (m_model, SIGNAL(listChanged(int)), SLOT(updateList(int)));
    connect (m_model, SIGNAL(sortingByColumnFinished(int,bool)), m_header, SLOT(showSortIndicator(int,bool)));
}

void ListWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    m_drawer.fillBackground(&painter, width(), height());
    painter.setLayoutDirection(Qt::LayoutDirectionAuto);
    bool rtl = (layoutDirection() == Qt::RightToLeft);
    int scroll_bar_width = m_scrollBar->isVisibleTo(this) ? m_scrollBar->sizeHint().width() : 0;

    painter.setClipRect(5,0,width() - scroll_bar_width - 9, height());
    painter.translate(rtl ? m_header->offset() : -m_header->offset(), 0);

    for (int i = 0; i < m_rows.size(); ++i )
    {
        m_drawer.drawBackground(&painter, m_rows[i], i);

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
        if(m_filterMode)
        {
            m_filterMode = false;
            m_filteredItems.clear();
            scrollTo(index);
        }

        m_model->setCurrent(index);
        MediaPlayer *player = MediaPlayer::instance();
        player->playListManager()->selectPlayList(m_model);
        player->playListManager()->activatePlayList(m_model);
        player->stop();
        player->play();
        emit doubleClicked();
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
                moveAnchorTo(m_pressed_index);
                m_model->clearSelection();
                m_model->setSelected(index, true);
            }
            if(m_model->isGroup(index) && m_model->selectedTracks().isEmpty())
            {
                moveAnchorTo(m_pressed_index);
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
            moveAnchorTo(m_pressed_index);
            m_model->setSelected(m_pressed_index, prev_anchor_index, true);
        }
        else //ShiftModifier released
        {
            moveAnchorTo(m_pressed_index);
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
    if(e->oldSize().height() < 10)
        updateList(PlayListModel::STRUCTURE | PlayListModel::CURRENT); //recenter to current on first resize
    else
        updateList(PlayListModel::STRUCTURE);
    QWidget::resizeEvent(e);
}

void ListWidget::wheelEvent (QWheelEvent *e)
{
    if(m_hslider->underMouse())
        return;

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

void ListWidget::showEvent(QShowEvent *)
{
    if(!m_rows.isEmpty())
        updateList(PlayListModel::METADATA);
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
    if(e->type() == QEvent::StyleChange)
        readSettings();

    return QWidget::event(e);
}

void ListWidget::updateList(int flags)
{
    m_hslider->setRange(0, m_header->maxScrollValue());
    m_hslider->setValue(m_header->offset());

    if(updateRowCount())
        flags |= PlayListModel::STRUCTURE;

    if(flags & PlayListModel::STRUCTURE && m_filterMode)
    {
        m_filteredItems = m_model->findTracks(m_filterString);
    }

    if(flags & PlayListModel::CURRENT)
        recenterTo(m_model->currentIndex());

    QList<PlayListItem *> items;
    int count = m_filterMode ? m_filteredItems.count() : m_model->count();

    if(flags & PlayListModel::STRUCTURE || flags & PlayListModel::CURRENT)
    {
        m_scrollBar->blockSignals(true);
        if(m_row_count >= count)
        {
            m_first = 0;
            m_scrollBar->setMaximum(0);
            m_scrollBar->setValue(0);
            emit positionChanged(0,0);
        }
        else if(m_first + m_row_count >= count)
        {
            //try to restore first visible first
            if(!m_filterMode && (m_count > 0) &&
                    (m_count != m_model->count()) && m_firstItem)
            {
                restoreFirstVisible();
            }
            if(m_first + m_row_count >= count)
                m_first = qMax(0, count - m_row_count);
            m_scrollBar->setMaximum(count - m_row_count);
            m_scrollBar->setValue(m_first);
            emit positionChanged(m_first, m_first);
        }
        else if(!m_filterMode && (m_count > 0) && (m_count != m_model->count()) &&
                m_firstItem && m_model->item(m_first) != m_firstItem)
        {
            restoreFirstVisible();
            m_scrollBar->setMaximum(count - m_row_count);
            m_scrollBar->setValue(m_first);
            emit positionChanged(m_first, m_model->count() - m_row_count);
        }
        else
        {
            m_scrollBar->setMaximum(count - m_row_count);
            m_scrollBar->setValue(m_first);
            emit positionChanged(m_first, count - m_row_count);
        }
        m_scrollBar->blockSignals(false);

        if(!m_filterMode)
        {
            m_firstItem = m_model->isEmpty() ? nullptr : m_model->item(m_first);
            m_count = m_model->count();
        }
        items = m_filterMode ? m_filteredItems.mid(m_first, m_row_count) : m_model->mid(m_first, m_row_count);

        while(m_rows.count() < qMin(m_row_count, items.count()))
            m_rows << new ListWidgetRow;
        while(m_rows.count() > qMin(m_row_count, items.count()))
            delete m_rows.takeFirst();

        m_scrollBar->setVisible(count > m_row_count);
    }
    else
    {
        items = m_filterMode ? m_filteredItems.mid(m_first, m_row_count) : m_model->mid(m_first, m_row_count);
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

    updateScrollBars();

    int scroll_bar_width = m_scrollBar->isVisibleTo(this) ? m_scrollBar->sizeHint().width() : 0;
    int trackStateColumn = m_header->trackStateColumn();
    int rowWidth = width() + m_header->maxScrollValue() - 10 - scroll_bar_width;
    bool rtl = layoutDirection() == Qt::RightToLeft;
    m_header->setScrollBarWidth(scroll_bar_width);
    m_hslider->setVisible(m_header->maxScrollValue() > 0);

    for(int i = 0; i < items.count(); ++i)
    {
        ListWidgetRow *row = m_rows[i];
        row->autoResize = m_header->hasAutoResizeColumn();
        row->trackStateColumn = trackStateColumn;
        items[i]->isSelected() ? row->flags |= ListWidgetRow::SELECTED :
                row->flags &= ~ListWidgetRow::SELECTED;

        i == (m_anchor_index - m_first) ? row->flags |= ListWidgetRow::ANCHOR :
                row->flags &= ~ListWidgetRow::ANCHOR;

        if(flags == PlayListModel::SELECTION)
            continue;

        if(rtl)
        {
            row->rect = QRect(width() - 5 - rowWidth, (m_header->isVisibleTo(this) ? m_header->height() : 0) + i * m_drawer.rowHeight(),
                              rowWidth, m_drawer.rowHeight() - 1);
        }
        else
        {
            row->rect = QRect(5, (m_header->isVisibleTo(this) ? m_header->height() : 0) + i * m_drawer.rowHeight(),
                              rowWidth, m_drawer.rowHeight() - 1);
        }

        row->titles = items[i]->formattedTitles();
        row->sizes = m_header->sizes();
        row->alignment = m_header->alignment();

        items[i] == m_model->currentTrack() ? row->flags |= ListWidgetRow::CURRENT :
                row->flags &= ~ListWidgetRow::CURRENT;

        if(items[i]->isGroup())
        {
            row->flags |= ListWidgetRow::GROUP;
            row->number = -1;
            row->length.clear();
        }
        else
        {
            row->flags &= ~ListWidgetRow::GROUP;
            row->number = items.at(i)->trackIndex() + 1;
            row->length = items.at(i)->formattedLength();
            row->extraString = getExtraString(items.at(i));
        }
        m_drawer.prepareRow(row);  //elide titles
    }
    update();
}

void ListWidget::autoscroll()
{
    if(m_filterMode)
        return;

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

void ListWidget::scrollTo(int index)
{
    if (m_row_count && !m_filterMode)
    {
        recenterTo(index);
        updateList(PlayListModel::STRUCTURE);
    }
}

void ListWidget::setViewPosition(int sc)
{
    if (m_model->count() <= m_row_count)
        return;
    m_first = sc;
    updateList(PlayListModel::STRUCTURE);
}

void ListWidget::setFilterString(const QString &str)
{
    m_filterString = str;
    if(str.isEmpty())
    {
        m_filteredItems.clear();
        m_filterString.clear();
        m_filterMode = false;
    }
    else
    {
        m_filterMode = true;
    }
    m_first = 0;
    updateList(PlayListModel::STRUCTURE);
}

void ListWidget::clear()
{
    if(m_filterMode)
    {
        m_model->removeTracks(m_filteredItems);
        m_filteredItems.clear();
    }
    else
    {
        m_model->clear();
    }
}

void ListWidget::removeSelected()
{
    if(m_filterMode)
    {
        QList<PlayListItem *> items;
        for(PlayListItem *item : m_filteredItems)
        {
            if(item->isSelected())
                items << item;
        }
        m_model->removeTracks(items);
    }
    else
    {
        m_model->removeSelected();
    }
}

void ListWidget::removeUnselected()
{
    if(m_filterMode)
    {
        QList<PlayListItem *> items;
        for(PlayListItem *item : m_filteredItems)
        {
            if(!item->isSelected())
                items << item;
        }
        m_model->removeTracks(items);
    }
    else
    {
        m_model->removeUnselected();
    }
}

void ListWidget::updateSkin()
{
    m_drawer.loadSystemColors();
    update();
}

void ListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void ListWidget::dropEvent(QDropEvent *event)
{
    if (!m_filterMode && event->mimeData()->hasUrls())
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
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

const QString ListWidget::getExtraString(PlayListItem *item)
{
    if(item->isGroup())
        return QString();

    QString extra_string;
    PlayListTrack *track = static_cast<PlayListTrack *>(item);

    if (m_show_protocol && track->path().contains("://"))
        extra_string = "[" + track->path().split("://").at(0) + "]";

    if (m_model->isQueued(track))
    {
        int index = m_model->queuedIndex(track);
        extra_string += "|"+QString::number(index + 1)+"|";
    }

    if(m_model->currentTrack() == track && m_ui_settings->isRepeatableTrack())
        extra_string += "|R|";
    else if(m_model->isStopAfter(track))
        extra_string += "|S|";

    return extra_string.trimmed(); //remove white space
}

bool ListWidget::updateRowCount()
{
    int h = height();
    if(m_header->isVisibleTo(this))
        h -= m_header->requiredHeight();
    if(m_hslider->isVisibleTo(this))
        h -= m_hslider->height();
    int row_count = qMax(0, h / m_drawer.rowHeight());
    if(m_row_count != row_count)
    {
        m_row_count = row_count;
        return true;
    }
    return false;
}

void ListWidget::restoreFirstVisible()
{
    if(m_first < m_model->count() && m_firstItem == m_model->item(m_first))
        return;

    int delta = m_model->count() - m_count;

    //try to find and restore first visible index
    if(delta > 0)
    {
        int from = qMin(m_model->count() - 1, m_first + 1);
        for(int i = from; i <= qMin(m_model->count() - 1, m_first + delta); ++i)
        {
            if(m_model->item(i) == m_firstItem)
            {
                m_first = i;
                break;
            }
        }
    }
    else
    {
        int from = qMin(m_model->count() - 1, m_first - 1);
        for(int i = from; i >= qMax(0, m_first + delta); --i)
        {
            if(m_model->item(i) == m_firstItem)
            {
                m_first = i;
                break;
            }
        }
    }
}

void ListWidget::updateScrollBars()
{
    bool rtl = layoutDirection() == Qt::RightToLeft;

    int vslider_width = m_scrollBar->isVisibleTo(this) ? m_scrollBar->sizeHint().width() : 0;
    int hslider_height = m_hslider->isVisibleTo(this) ? m_hslider->sizeHint().height() : 0;

    if(rtl)
    {
        m_scrollBar->setGeometry(0, 0, m_scrollBar->sizeHint().width(), height() - hslider_height);
        m_hslider->setGeometry(vslider_width, height() - m_hslider->sizeHint().height(),
                               width() - vslider_width, m_hslider->sizeHint().height());
    }
    else
    {
        m_scrollBar->setGeometry(width() - m_scrollBar->sizeHint().width(), 0,
                                 m_scrollBar->sizeHint().width(), height() - hslider_height);
        m_hslider->setGeometry(0, height() - m_hslider->sizeHint().height(), width() - vslider_width,
                               m_hslider->sizeHint().height());
    }
}

void ListWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(m_filterMode)
        return;

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
            moveAnchorTo(index);
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
        if(index < 0 || !m_model->isTrack(index) || m_popupWidget->url() != m_model->track(index)->path())
            m_popupWidget->deactivate();
    }
}

void ListWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_select_on_release)
    {
        m_model->clearSelection();
        m_model->setSelected(m_pressed_index,true);
        moveAnchorTo(m_pressed_index);
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

    if(m_filterMode)
    {
        for (int i = 0; i < qMin(m_row_count, m_filteredItems.count() - m_first); ++i)
        {
            if ((y >= i * m_drawer.rowHeight()) && (y <= (i+1) * m_drawer.rowHeight()))
                return m_model->indexOf(m_filteredItems.at(m_first + i));
        }
    }
    else
    {
        for (int i = 0; i < qMin(m_row_count, m_model->count() - m_first); ++i)
        {
            if ((y >= i * m_drawer.rowHeight()) && (y <= (i+1) * m_drawer.rowHeight()))
                return m_first + i;
        }
    }
    return INVALID_INDEX;
}

void ListWidget::contextMenuEvent(QContextMenuEvent * event)
{
    if (menu())
        menu()->exec(event->globalPos());
}

void ListWidget::recenterTo(int index)
{
    if (m_row_count && !m_filterMode)
    {
        if (m_first + m_row_count < index + 1)
            m_first = qMin(m_model->count() - m_row_count, index - m_row_count/2);
        else if (m_first > index)
            m_first = qMax (index - m_row_count/2, 0);
    }
}
