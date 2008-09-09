/***************************************************************************
 *   Copyright (C) 2007-2008 by Ilya Kotov                                 *
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
#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QMainWindow>
#include <QPoint>

#include "pixmapwidget.h"
#include "playlist.h"
#include "mainwindow.h"

/**
	@author Ilya Kotov <forkotov02@hotmail.ru>
*/
class MainWindow;
class QMouseEvent;

class Skin;
class Button;
class SymbolDisplay;
class TitleBarControl;
class ShadedVisual;

class TitleBar : public PixmapWidget
{
Q_OBJECT
public:
    TitleBar(QWidget *parent = 0);

    ~TitleBar();

    void setActive(bool);
    //void setInfo(const OutputState &st);

public slots:
    void setTime(qint64 time);

private slots:
    void updateSkin();
    void showMainMenu();
    void shade();

private:
    Skin *m_skin;
    QPoint m_pos;
    MainWindow *m_mw;
    Button *m_menu;
    Button *m_minimize;
    Button *m_shade;
    Button *m_shade2;
    Button *m_close;
    SymbolDisplay *m_currentTime;
    QString formatTime (int);
    bool m_shaded;
    bool m_align;
    TitleBarControl *m_control;
    ShadedVisual *m_visual;
    void updateMask();

protected:
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
};



#endif
