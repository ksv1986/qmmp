/***************************************************************************
 *   Copyright (C) 2010-2020 by Ilya Kotov                                 *
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

#include <QSettings>
#include <QAction>
#include <QIcon>
#include <QFile>
#include <QApplication>
#include <QStyle>
#include <QWidgetAction>
#include <qmmp/qmmp.h>
#include "actionmanager.h"

ActionManager *ActionManager::m_instance = nullptr;

ActionManager::ActionManager(QObject *parent) :
    QObject(parent)
{
    m_instance = this;
    m_settings = new QSettings(Qmmp::configFile(), QSettings::IniFormat);
    m_settings->beginGroup("SimpleUiShortcuts");

    m_actions = {
        //playback
        { PLAY, createAction(tr("&Play"), "play", tr("X"), "media-playback-start") },
        { PAUSE, createAction(tr("&Pause"), "pause", tr("C"), "media-playback-pause") },
        { STOP, createAction(tr("&Stop"), "stop", tr("V"), "media-playback-stop") },
        { PREVIOUS, createAction(tr("&Previous"), "previous", tr("Z"), "media-skip-backward") },
        { NEXT, createAction(tr("&Next"), "next", tr("B"), "media-skip-forward") },
        { PLAY_PAUSE, createAction(tr("&Play/Pause"), "play_pause", tr("Space"), "media-playback-start") },
        { JUMP, createAction(tr("&Jump to Track"), "jump", tr("J"), "go-up") },
        { EJECT, createAction(tr("&Play Files"), "eject", tr("E"), "media-eject") },
        { RECORD, createAction2(tr("&Record"), "record", "", "media-record") },
        { REPEAT_ALL, createAction2(tr("&Repeat Playlist"), "repeate_playlist", tr("R"), "media-playlist-repeat") },
        { REPEAT_TRACK, createAction2(tr("&Repeat Track"), "repeate_track", tr("Ctrl+R")) },
        { SHUFFLE, createAction2(tr("&Shuffle"), "shuffle", tr("S"), "media-playlist-shuffle") },
        { NO_PL_ADVANCE, createAction2(tr("&No Playlist Advance"), "no_playlist_advance", tr("Ctrl+N")) },
        { TRANSIT_BETWEEN_PLAYLISTS, createAction2(tr("&Transit between playlists"), "transit_between_playlists") },
        { STOP_AFTER_SELECTED, createAction(tr("&Stop After Selected"), "stop_after_selected", tr("Ctrl+S")) },
        { CLEAR_QUEUE, createAction(tr("&Clear Queue"), "clear_queue", tr("Alt+Q")) },
        //view
        { WM_ALLWAYS_ON_TOP, createAction2(tr("Always on Top"), "always_on_top") },
        { WM_STICKY, createAction2(tr("Put on All Workspaces"), "sticky") },
        { UI_ANALYZER, nullptr }, //external action
        { UI_FILEBROWSER, nullptr }, //external action
        { UI_COVER, nullptr }, //external action
        { UI_PLAYLIST_BROWSER, nullptr }, //external action
        { UI_WAVEFORM_SEEKBAR, nullptr }, //external action
        { UI_SHOW_TABS, createAction2(tr("Show Tabs"), "show_tabs") },
        { UI_SHOW_TITLEBARS, createAction2(tr("Show Title Bars"), "show_titlebars") },
        { UI_BLOCK_TOOLBARS, createAction2(tr("Block Toolbars"), "block_toolbars") },
        //volume
        { VOL_ENC, createAction(tr("Volume &+"), "vol_enc", tr("0")) },
        { VOL_DEC, createAction(tr("Volume &-"), "vol_dec", tr("9")) },
        { VOL_MUTE, createAction2(tr("&Mute"), "vol_mute", tr("M")) },
        //playlist
        { PL_ADD_FILE, createAction(tr("&Add File"), "add_file", tr("F"), "audio-x-generic") },
        { PL_ADD_DIRECTORY, createAction(tr("&Add Directory"), "add_dir", tr("D"), "folder") },
        { PL_ADD_URL, createAction(tr("&Add Url"), "add_url", tr("U"), "network-server") },
        { PL_REMOVE_SELECTED, createAction(tr("&Remove Selected"), "remove_selected", tr("Del"), "edit-delete") },
        { PL_REMOVE_ALL, createAction(tr("&Remove All"), "remove_all", "", "edit-clear") },
        { PL_REMOVE_UNSELECTED, createAction(tr("&Remove Unselected"), "remove_unselected", "", "edit-delete") },
        { PL_REMOVE_INVALID, createAction(tr("Remove unavailable files"), "remove_invalid", "", "dialog-error") },
        { PL_REMOVE_DUPLICATES, createAction(tr("Remove duplicates"), "remove_duplicates") },
        { PL_REFRESH, createAction(tr("Refresh"), "refresh", "F5", "view-refresh") },
        { PL_ENQUEUE, createAction(tr("&Queue Toggle"), "enqueue", tr("Q")) },
        { PL_INVERT_SELECTION, createAction(tr("Invert Selection"), "invert_selection") },
        { PL_CLEAR_SELECTION, createAction(tr("&Select None"), "clear_selection") },
        { PL_SELECT_ALL, createAction(tr("&Select All"), "select_all", tr("Ctrl+A"), "edit-select-all") },
        { PL_SHOW_INFO, createAction(tr("&View Track Details"), "show_info", tr("Alt+I"), "dialog-information") },
        { PL_NEW, createAction(tr("&New List"), "new_pl", tr("Ctrl+T"), "document-new") },
        { PL_CLOSE, createAction(tr("&Delete List"), "close_pl", tr("Ctrl+W"), "window-close") },
        { PL_LOAD, createAction(tr("&Load List"), "load_pl", tr("O"), "document-open") },
        { PL_SAVE, createAction(tr("&Save List"), "save_pl", tr("Shift+S"), "document-save-as") },
        { PL_RENAME, createAction(tr("&Rename List"), "pl_rename", tr("F2")) },
        { PL_SELECT_NEXT, createAction(tr("&Select Next Playlist"), "next_pl", tr("Ctrl+PgDown"), "go-next") },
        { PL_SELECT_PREVIOUS, createAction(tr("&Select Previous Playlist"), "prev_pl", tr("Ctrl+PgUp"), "go-previous") },
        { PL_GROUP_TRACKS, createAction2(tr("&Group Tracks"), "group_tracks", tr("Ctrl+G")) },
        { PL_SHOW_HEADER, createAction2(tr("&Show Column Headers"), "show_header", tr("Ctrl+H")) },
        { PL_FILE_LOCATION, createAction(QApplication::translate("DetailsDialog", "Open the directory containing this file", nullptr), "file_location", "") },
        //other
        { EQUALIZER, createAction(tr("&Equalizer"), "equalizer", tr("Ctrl+E")) },
        { SETTINGS, createAction(tr("&Settings"), "show_settings", tr("Ctrl+P"), "configure") },
        { ABOUT_UI, createAction(tr("&About Ui"), "about_ui") },
        { ABOUT, createAction(tr("&About"), "about") },
        { ABOUT_QT, createAction(tr("&About Qt"), "about_qt") },
        { QUIT, createAction(tr("&Exit"), "exit", tr("Ctrl+Q"), "application-exit") }
    };
    m_settings->endGroup();
    readStates();
    delete m_settings;
    m_settings = nullptr;
    m_actions[ABOUT]->setIcon(qApp->windowIcon());
}

ActionManager::~ActionManager()
{
    saveStates();
    m_instance = nullptr;
}

QAction *ActionManager::action(int type)
{
    return m_actions[type];
}

QAction *ActionManager::use(int type, const QObject *receiver, const char *member)
{
    QAction *act = m_actions[type];
    connect(act,SIGNAL(triggered(bool)), receiver, member);
    return act;
}

QList<QAction *> ActionManager::actions()
{
    return m_actions.values();
}

ActionManager* ActionManager::instance()
{
    return m_instance;
}

QAction *ActionManager::createAction(QString name, QString confKey, QString key, QString iconName)
{
    QAction *action = new QAction(name, this);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    action->setShortcutVisibleInContextMenu(true);
#endif
    action->setShortcut(m_settings->value(confKey, key).toString());
    action->setObjectName(confKey);
    action->setProperty("defaultShortcut", key);
    if(iconName.isEmpty())
        return action;
    if(QFile::exists(iconName))
        action->setIcon(QIcon(iconName));
    else if(QIcon::hasThemeIcon(iconName))
        action->setIcon(QIcon::fromTheme(iconName));
    else if(QFile::exists(QString(":/qsui/")+iconName+".png"))
        action->setIcon(QIcon(QString(":/qsui/")+iconName+".png"));
    return action;
}

QAction *ActionManager::createAction2(QString name, QString confKey, QString key, QString iconName)
{
    QAction *action = createAction(name, confKey, key);
    action->setCheckable(true);
    action->setIconVisibleInMenu(false);
    if(iconName.isEmpty())
        return action;
    if(QFile::exists(iconName))
        action->setIcon(QIcon(iconName));
    else if(QIcon::hasThemeIcon(iconName))
        action->setIcon(QIcon::fromTheme(iconName));
    else if(QFile::exists(QString(":/qsui/")+iconName+".png"))
        action->setIcon(QIcon(QString(":/qsui/")+iconName+".png"));
    return action;
}

void ActionManager::readStates()
{
    m_settings->beginGroup("Simple");
    m_actions[PL_SHOW_HEADER]->setChecked(m_settings->value("pl_show_header", true).toBool());
    m_settings->endGroup();
}

void ActionManager::saveStates()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("Simple");
    settings.setValue("pl_show_header", m_actions[PL_SHOW_HEADER]->isChecked());
    settings.endGroup();
}

void ActionManager::saveActions()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    for(const QAction *action : m_actions.values())
    {
        settings.setValue(QString("SimpleUiShortcuts/")+action->objectName(), action->shortcut());
    }
}

void ActionManager::resetShortcuts()
{
    for(QAction *action : m_actions.values())
    {
        action->setShortcut(action->property("defaultShortcut").toString());
    }
}

void ActionManager::registerAction(int id, QAction *action, QString confKey, QString key)
{
    if(m_actions.value(id))
        qFatal("ActionManager: invalid action id");

    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("SimpleUiShortcuts");
    action->setShortcut(settings.value(confKey, key).toString());
    action->setProperty("defaultShortcut", key);
    action->setObjectName(confKey);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    action->setShortcutVisibleInContextMenu(true);
#endif
    m_actions[id] = action;
    settings.endGroup();
}

void ActionManager::registerWidget(int id, QWidget *w, QString text, QString name)
{
    if(m_actions.value(id))
        qFatal("ActionManager: invalid action id");
    QWidgetAction *action = new QWidgetAction(this);
    action->setText(text);
    action->setObjectName(name);
    action->setDefaultWidget(w);
    m_actions[id] = action;
}

QToolBar *ActionManager::createToolBar(ActionManager::ToolBarInfo info, QWidget *parent)
{
    QToolBar *toolBar = new QToolBar(info.title, parent);
    updateToolBar(toolBar, info);
    toolBar->setProperty("uid", info.uid);
    toolBar->setIconSize(info.iconSize);
    toolBar->setObjectName("Toolbar" + info.uid);
    return toolBar;
}

void ActionManager::updateToolBar(QToolBar *toolBar, ActionManager::ToolBarInfo info)
{
    toolBar->clear();
    toolBar->setIconSize(info.iconSize);
    for(const QString &name : qAsConst(info.actionNames))
    {
        if(name == "separator")
        {
            toolBar->addSeparator()->setObjectName("separator");
            continue;
        }
        QAction *action = findChild<QAction *>(name);
        if(action)
        {
            action->setVisible(true);
            toolBar->addAction(action);
        }
    }
}

ActionManager::ToolBarInfo ActionManager::defaultToolBar() const
{
    const QList<Type> idList = {
        PL_ADD_FILE, PL_ADD_DIRECTORY, PREVIOUS, PLAY, PAUSE, STOP, NEXT, EJECT,
        UI_SEPARATOR, UI_POS_SLIDER, UI_SEPARATOR, UI_VOL_SLIDER, VOL_MUTE
    };
    QStringList names;
    for(const Type &id : qAsConst(idList))
    {
        if(id == UI_SEPARATOR)
        {
            names << "separator";
            continue;
        }
        names << m_actions.value(id)->objectName();
    }
    ActionManager::ToolBarInfo info;
    info.title = tr("Toolbar");
    info.actionNames = names;
    info.uid = "{68363a0b-f2cd-462a-87ca-e3089db21561}";
    return info;
}

QList<ActionManager::ToolBarInfo> ActionManager::readToolBarSettings() const
{
    QList<ToolBarInfo> list;
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    int iconSize = settings.value("Simple/toolbar_icon_size", -1).toInt();
    if(iconSize <= 0)
        iconSize = qApp->style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    int size = settings.beginReadArray("SimpleUiToolbars");
    for(int i = 0; i < size; ++i)
    {
        ToolBarInfo info;
        settings.setArrayIndex(i);
        info.title = settings.value("title").toString();
        info.actionNames = settings.value("actions").toStringList();
        info.uid = settings.value("uid").toString();
        info.iconSize = QSize(iconSize, iconSize);
        list.append(info);
    }
    settings.endArray();
    if(list.isEmpty())
    {
        list << defaultToolBar();
        list.last().iconSize = QSize(iconSize, iconSize);
    }
    return list;
}

void ActionManager::writeToolBarSettings(QList<ActionManager::ToolBarInfo> l)
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginWriteArray("SimpleUiToolbars");
    for(int i = 0; i < l.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("title", l[i].title);
        settings.setValue("actions", l[i].actionNames);
        settings.setValue("uid", l[i].uid);
    }
    settings.endArray();
}
