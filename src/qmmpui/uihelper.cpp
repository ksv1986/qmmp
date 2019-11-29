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

#include <QDialog>
#include <QMenu>
#include <QWidget>
#include <QAction>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <qmmp/soundcore.h>
#include <qmmp/metadatamanager.h>
#include "filedialog.h"
#include "playlistparser.h"
#include "playlistmanager.h"
#include "qmmpuisettings.h"
#include "general.h"
#include "generalfactory.h"
#include "jumptotrackdialog_p.h"
#include "aboutdialog_p.h"
#include "addurldialog_p.h"
#include "mediaplayer.h"
#include "uihelper.h"

UiHelper *UiHelper::m_instance = nullptr;

UiHelper::UiHelper(QObject *parent)
        : QObject(parent)
{
    m_instance = this;
    m_jumpDialog = nullptr;
    General::create(parent);
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    m_lastDir = settings.value("General/last_dir", QDir::homePath()).toString(); //last directory
}

UiHelper::~UiHelper()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("General/last_dir",m_lastDir);
}

bool UiHelper::visibilityControl()
{
    for(const GeneralFactory *factory : General::enabledFactories())
    {
        if (factory->properties().visibilityControl)
            return true;
    }
    return false;
}

void UiHelper::addAction(QAction *action, MenuType type)
{
    connect(action, SIGNAL(destroyed (QObject *)), SLOT(removeAction(QObject*)));

    if(!m_menus[type].actions.contains(action))
    {
        m_menus[type].actions.append(action);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        action->setShortcutVisibleInContextMenu(true);
#endif
    }
    if(m_menus[type].menu && !m_menus[type].menu->actions().contains(action))
    {
        if(m_menus[type].before)
            m_menus[type].menu->insertAction(m_menus[type].before, action);
        else
            m_menus[type].menu->addAction(action);
        m_menus[type].menu->menuAction()->setVisible(!m_menus[type].autoHide || !m_menus[type].actions.isEmpty());
    }
}

void UiHelper::removeAction(QAction *action)
{
    for(MenuType type : m_menus.keys())
    {
        m_menus[type].actions.removeAll(action);
        if(m_menus[type].menu)
        {
            m_menus[type].menu->removeAction(action);
            m_menus[type].menu->menuAction()->setVisible(!m_menus[type].autoHide || !m_menus[type].actions.isEmpty());
        }
    }
}

QList<QAction *> UiHelper::actions(MenuType type)
{
    return m_menus[type].actions;
}

QMenu *UiHelper::createMenu(MenuType type, const QString &title, bool autoHide, QWidget *parent)
{
    if(m_menus[type].menu)
    {
        m_menus[type].menu->setTitle(title);
    }
    else
    {
        m_menus[type].menu = new QMenu(title, parent);
        m_menus[type].menu->addActions(m_menus[type].actions);
    }
    m_menus[type].autoHide = autoHide;
    m_menus[type].menu->menuAction()->setVisible(!autoHide || !m_menus[type].actions.isEmpty());
    return m_menus[type].menu;
}

void UiHelper::registerMenu(UiHelper::MenuType type, QMenu *menu, bool autoHide, QAction *before)
{
    m_menus[type].menu = menu;
    m_menus[type].before = before;
    m_menus[type].autoHide = autoHide;
    if(before)
        m_menus[type].menu->insertActions(before, m_menus[type].actions);
    else
        m_menus[type].menu->addActions(m_menus[type].actions);
     m_menus[type].menu->menuAction()->setVisible(!autoHide || !m_menus[type].actions.isEmpty());
}

void UiHelper::addFiles(QWidget *parent, PlayListModel *model)
{
    QStringList filters;
    filters << tr("All Supported Bitstreams")+" (" +
            MetaDataManager::instance()->nameFilters().join (" ") +")";
    filters << MetaDataManager::instance()->filters();
    m_model = model;
    FileDialog::popup(parent, FileDialog::PlayDirsFiles, &m_lastDir,
                      this, SLOT(addSelectedFiles(QStringList,bool)),
                      tr("Select one or more files to open"), filters.join(";;"));
}

void UiHelper::playFiles(QWidget *parent, PlayListModel *model)
{
    QStringList filters;
    filters << tr("All Supported Bitstreams")+" (" +
            MetaDataManager::instance()->nameFilters().join (" ") +")";
    filters << MetaDataManager::instance()->filters();
    m_model = model;
    FileDialog::popup(parent, FileDialog::AddDirsFiles, &m_lastDir,
                      this, SLOT(playSelectedFiles(QStringList)),
                      tr("Select one or more files to play"), filters.join(";;"));

}

void UiHelper::addDirectory(QWidget *parent, PlayListModel *model)
{
    FileDialog::popup(parent, FileDialog::AddDirs, &m_lastDir,
                      model, SLOT(add(QStringList)),
                      tr("Choose a directory"));
}

void UiHelper::addUrl(QWidget *parent, PlayListModel *model)
{
    AddUrlDialog::popup(parent, model);
}

void UiHelper::loadPlayList(QWidget *parent, PlayListModel *model)
{
    if(PlayListParser::nameFilters().isEmpty())
    {
        qWarning("UiHelper: There is no registered playlist parsers");
        return;
    }

    QString mask = tr("Playlist Files") + " (" + PlayListParser::nameFilters().join(" ") + ")";
    //TODO use nonmodal dialog and multiplier playlists
    QString f_path = FileDialog::getOpenFileName(parent, tr("Open Playlist"), m_lastDir, mask);
    if (!f_path.isEmpty())
    {
        if(QmmpUiSettings::instance()->clearPreviousPlayList())
        {
            model->clear();
            model->setName(QFileInfo(f_path).baseName());
        }
        model->loadPlaylist(f_path);
        m_lastDir = QFileInfo(f_path).absoluteDir().path();
    }
}

void UiHelper::savePlayList(QWidget *parent, PlayListModel *model)
{
    QStringList nameFilters = PlayListParser::nameFilters();

    if(nameFilters.isEmpty())
    {
        qWarning("UiHelper: There is no registered playlist parsers");
        return;
    }

    QStringList filters;
    filters << tr("Playlist Files") + " (" + nameFilters.join(" ") + ")";
    filters << PlayListParser::filters();
    QString selectedFilter = filters.at(1);
    QString f_name = FileDialog::getSaveFileName(parent, tr("Save Playlist"), m_lastDir + "/" +
                                                 model->name(), filters.join(";;"), &selectedFilter);

    if(f_name.isEmpty())
        return;

    if(!PlayListParser::isPlayList(f_name)) //append selected extension
    {
        QStringList selectedFilters = selectedFilter.section("(", 1).remove(")").split(" ");
        if(selectedFilters.isEmpty())
            return;

        QString ext = selectedFilters.first().remove("*"); //use first extension
        f_name.append(ext);

        QFileInfo info(f_name);

        if(info.exists())
        {
            if (QMessageBox::question(parent, tr("Save Playlist"),  tr("%1 already exists.\nDo you want to replace it?")
                                      .arg(info.fileName()), QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
            {
                return;
            }
        }
    }

    if (!f_name.isEmpty())
    {
        model->savePlaylist(f_name);
        m_lastDir = QFileInfo(f_name).absoluteDir().path();
    }
}

void UiHelper::jumpToTrack(QWidget *parent, PlayListModel *model)
{
    if(!m_jumpDialog)
    {
        m_jumpDialog = new JumpToTrackDialog(model, parent);
    }
    if(m_jumpDialog->isHidden())
    {
        m_jumpDialog->show();
        m_jumpDialog->refresh();
    }
    m_jumpDialog->raise();
}

static QString getDirPath(const TrackInfo *info)
{
    if (!info)
        return QString();

    const QString& url = info->path();
    if (!url.contains("://")) //local file
        return QFileInfo(url).absolutePath();

    if (url.contains(":///")) //pseudo-protocol
    {
        QString path = QUrl(url).path();
        path.replace(QString(QUrl::toPercentEncoding("#")), "#");
        path.replace(QString(QUrl::toPercentEncoding("?")), "?");
        path.replace(QString(QUrl::toPercentEncoding("%")), "%");
        return QFileInfo(path).absolutePath();
    }
    return QString();
}

void UiHelper::openFileLocation(const TrackInfo *info) const
{
    QString dir_path = getDirPath(info);
    if (dir_path.isEmpty())
        return;

    QDesktopServices::openUrl(QUrl::fromLocalFile(dir_path));
}

void UiHelper::about(QWidget *parent)
{
    AboutDialog *dialog = new AboutDialog(parent);
    dialog->exec();
    dialog->deleteLater();
}

void UiHelper::toggleVisibility()
{
    emit toggleVisibilityCalled();
}

void UiHelper::showMainWindow()
{
    emit showMainWindowCalled();
}

void UiHelper::exit()
{
    //send non-spontaneous close event
    //for all windows
    for(QWidget *widget : qApp->topLevelWidgets())
        widget->close();

    qApp->closeAllWindows();
    qApp->quit();
}

UiHelper* UiHelper::instance()
{
    return m_instance;
}

void UiHelper::removeAction(QObject *action)
{
    for(MenuType type : m_menus.keys())
    {
        for(QList<QAction *>::iterator it = m_menus[type].actions.begin(); it != m_menus[type].actions.end(); ++it)
        {
            if(*it == action)
            {
                m_menus[type].actions.erase(it);
                m_menus[type].menu->menuAction()->setVisible(!m_menus[type].autoHide || !m_menus[type].actions.isEmpty());
                break;
            }
        }
    }
}

void UiHelper::addSelectedFiles(const QStringList &files, bool play)
{
    if(files.isEmpty() || !PlayListManager::instance()->playLists().contains(m_model))
        return;
    if(play)
        playSelectedFiles(files);
    else
        m_model->add(files);
}

void UiHelper::playSelectedFiles(const QStringList &files)
{
    if(files.isEmpty() || !PlayListManager::instance()->playLists().contains(m_model))
        return;
    m_model->clear();
    PlayListManager::instance()->activatePlayList(m_model);
    connect(m_model, SIGNAL(trackAdded(PlayListTrack*)), MediaPlayer::instance(), SLOT(play()));
    connect(m_model, SIGNAL(trackAdded(PlayListTrack*)), SLOT(disconnectPl()));
    m_model->add(files);
}

void UiHelper::disconnectPl()
{
    PlayListModel *model = qobject_cast<PlayListModel*>(sender());
    if(model)
    {
        disconnect(model, SIGNAL(trackAdded(PlayListTrack*)), MediaPlayer::instance(), SLOT(play()));
        disconnect(model, SIGNAL(trackAdded(PlayListTrack*)), this, SLOT(disconnectPl()));
    }
}
