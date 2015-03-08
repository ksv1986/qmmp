/***************************************************************************
 *   Copyright (C) 2015 by Ilya Kotov                                      *
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

#ifndef PLAYLISTHEADER_H
#define PLAYLISTHEADER_H

#include <QWidget>

class QFontMetrics;
class QFont;
class QMouseEvent;
class QMenu;
class PlayListModel;
class PlayList;
class Skin;
class ColumnManager;

/**
    @author Ilya Kotov <forkotov02@hotmail.ru>
*/
class PlayListHeader : public QWidget
{
    Q_OBJECT
public:
    explicit PlayListHeader(QWidget *parent = 0);
    ~PlayListHeader();

    void readSettings();
    void setNumberWidth(int width);
    void updateColumns();

private slots:
    void updateSkin();

private:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    void loadColors();

    PlayListModel *m_model;
    Skin *m_skin;
    QFontMetrics *m_metrics;
    QFont m_font;
    bool m_scrollable;
    QColor m_normal, m_normal_bg, m_selected_bg, m_current;
    bool m_show_number;
    bool m_align_numbres;
    int m_number_width;
    ColumnManager *m_manager;
    QList <QRect> m_rects;
    QStringList m_names;
    QPoint m_pressed_pos;
    int m_pressed_column;
    int m_old_size;
    QPoint m_mouse_pos;
    int m_press_offset;

    enum
    {
        NO_TASK = -1,
        RESIZE,
        MOVE
    } m_task;


};

#endif // PLAYLISTHEADER_H
