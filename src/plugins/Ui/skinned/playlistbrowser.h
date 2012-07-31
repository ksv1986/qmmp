/***************************************************************************
 *   Copyright (C) 2009 by Ilya Kotov                                      *
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

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include <QDialog>
#include "ui_playlistbrowser.h"

class PlayListManager;

/**
    @author Ilya Kotov <forkotov02@hotmail.ru>
*/
class PlayListBrowser : public QDialog
{
Q_OBJECT
public:
    PlayListBrowser(PlayListManager *manager, QWidget *parent = 0);
    ~PlayListBrowser();

private slots:
    void updateList();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_listWidget_itemChanged(QListWidgetItem *item);
    void on_listWidget_itemPressed (QListWidgetItem *item);
    void rename();
    void on_deleteButton_clicked();
    void on_downButton_clicked();
    void on_upButton_clicked();

private:
    Ui::PlayListBrowser ui;
    PlayListManager *m_pl_manager;
};

#endif // PLAYLISTBROWSER_H
