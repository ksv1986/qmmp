/***************************************************************************
 *   Copyright (C) 2008-2020 by Ilya Kotov                                 *
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

#include <QEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QWindow>
#include <QTextDocumentFragment>
#include <qmmp/soundcore.h>

#include "qmmptrayicon.h"
#ifdef QMMP_WS_X11
#include "statusiconpopupwidget.h"
#endif


QmmpTrayIcon::QmmpTrayIcon(QObject *parent)
        : QSystemTrayIcon(parent)
{}


QmmpTrayIcon::~QmmpTrayIcon()
{
}

void QmmpTrayIcon::setToolTip(const QString &tip)
{
#ifdef QMMP_WS_X11
    if(hasToolTipEvent())
    {
        m_message = tip;
        if(m_popupWidget)
            showToolTip();
    }
    else
    {
        QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(tip);
        QSystemTrayIcon::setToolTip(fragment.toPlainText());
    }
#else
    QSystemTrayIcon::setToolTip(tip);
#endif
}

#ifdef QMMP_WS_X11
bool QmmpTrayIcon::event(QEvent *e)
{
    if (e->type() == QEvent::Wheel )
    {
        wheelEvent((QWheelEvent *) e);
        e->accept();
        return true;
    }
    if (e->type() == QEvent::ToolTip)
    {
        showToolTip();
        e->accept();
        return true;
    }
    return QSystemTrayIcon::event(e);
}

bool QmmpTrayIcon::hasToolTipEvent()
{
    //checking for XEmbed system tray implementation
    //only this implementation is able to send QHelpEvent
    for(const QWindow *w : qApp->allWindows())
    {
        if(w->objectName() == "QSystemTrayIconSysWindow")
            return true;
    }
    return false;
}

void QmmpTrayIcon::wheelEvent(QWheelEvent *e)
{
    SoundCore::instance()->changeVolume(e->delta()/20);
}

void QmmpTrayIcon::showToolTip()
{
    if(m_popupWidget.isNull())
    {
        m_popupWidget = new StatusIconPopupWidget();
    }
    m_popupWidget->showInfo(geometry().x(),geometry().y(), m_message);
}
#endif
