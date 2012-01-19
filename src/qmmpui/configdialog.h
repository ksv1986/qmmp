/***************************************************************************
 *   Copyright (C) 2007-2012 by Ilya Kotov                                 *
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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QIcon>


namespace Ui {
    class ConfigDialog;
}

class QListWidgetItem;
class QTreeWidgetItem;


/**
    @author Ilya Kotov <forkotov02@hotmail.ru>
*/
class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    ConfigDialog(QWidget *parent = 0);
    virtual ~ConfigDialog();

    void addPage(const QString &name, QWidget *widget, const QIcon &icon = QIcon());

private slots:
    void on_contentsWidget_currentItemChanged (QListWidgetItem *current, QListWidgetItem *previous);
    void on_preferencesButton_clicked();
    void on_informationButton_clicked();
    void addTitleString(QAction *);
    void saveSettings();
    void updateDialogButton(int);
    void on_fdInformationButton_clicked();
    void on_treeWidget_itemChanged (QTreeWidgetItem *item, int column);
    void on_treeWidget_currentItemChanged (QTreeWidgetItem *current, QTreeWidgetItem *);
    void on_outputComboBox_activated (int index);
    void on_outputPreferencesButton_clicked();
    void on_outputInformationButton_clicked();
    void on_uiComboBox_activated (int index);
    void on_uiInformationButton_clicked();

private:
    void readSettings();
    void findSkins(const QString &path);
    void loadPluginsInfo();
    void createMenus();
    int m_insert_row;
    Ui::ConfigDialog *m_ui;

};

#endif
