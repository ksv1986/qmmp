/***************************************************************************
 *   Copyright (C) 2009-2019 by Ilya Kotov                                 *
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
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QTreeView>
#include <QMessageBox>
#include <QMenu>
#include <QSettings>
#include <QInputDialog>
#include <QGridLayout>
#include <qmmp/soundcore.h>
#include <qmmp/decoder.h>
#include <qmmp/metadatamanager.h>
#include <qmmp/effect.h>
#include <qmmp/qmmpsettings.h>
#include <qmmpui/general.h>
#include <qmmpui/playlistparser.h>
#include <qmmpui/playlistformat.h>
#include <qmmpui/filedialog.h>
#include <qmmpui/playlistmodel.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/uihelper.h>
#include <qmmpui/configdialog.h>
#include <qmmpui/qmmpuisettings.h>
#include "qsuitabbar.h"
#include "toolbareditor.h"
#include "actionmanager.h"
#include "qsuianalyzer.h"
#include "visualmenu.h"
#include "listwidget.h"
#include "positionslider.h"
#include "mainwindow.h"
#include "qsuisettings.h"
#include "hotkeyeditor.h"
#include "filesystembrowser.h"
#include "aboutqsuidialog.h"
#include "keyboardmanager.h"
#include "coverwidget.h"
#include "playlistbrowser.h"
#include "volumeslider.h"
#include "qsuitabwidget.h"
#include "qsuiquicksearch.h"
#include "equalizer.h"

#define KEY_OFFSET 10000

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_ui.setupUi(this);
    m_update = false;
    m_wasMaximized = false;
    m_titleFormatter.setPattern("%if(%p,%p - %t,%t)");
    //qmmp objects
    m_player = MediaPlayer::instance();
    connect(m_player, SIGNAL(playbackFinished()), SLOT(restoreWindowTitle()));
    m_core = SoundCore::instance();
    m_pl_manager = PlayListManager::instance();
    m_uiHelper = UiHelper::instance();
    m_ui_settings = QmmpUiSettings::instance();
    connect(m_uiHelper, SIGNAL(toggleVisibilityCalled()), SLOT(toggleVisibility()));
    connect(m_uiHelper, SIGNAL(showMainWindowCalled()), SLOT(showAndRaise()));
    m_visMenu = new VisualMenu(this); //visual menu
    m_ui.actionVisualization->setMenu(m_visMenu);
    m_pl_menu = new QMenu(this); //playlist menu
    new ActionManager(this); //action manager
    createWidgets(); //widgets
    //status
    connect(m_core, SIGNAL(elapsedChanged(qint64)), SLOT(updatePosition(qint64)));
    connect(m_core, SIGNAL(stateChanged(Qmmp::State)), SLOT(showState(Qmmp::State)));
    connect(m_core, SIGNAL(bitrateChanged(int)), SLOT(updateStatus()));
    connect(m_core, SIGNAL(bufferingProgress(int)), SLOT(showBuffering(int)));
    connect(m_core, SIGNAL(trackInfoChanged()), SLOT(showMetaData()));
    //keyboard manager
    m_key_manager = new KeyboardManager(this);
    //create tabs
    foreach(PlayListModel *model, m_pl_manager->playLists())
    {
        if(m_pl_manager->currentPlayList() != model)
            m_tabWidget->addTab(model->name());
        else
            m_tabWidget->addTab("[" + model->name() + "]");
        connect(model, SIGNAL(nameChanged(QString)), SLOT(updateTabs()));
    }
    m_tabWidget->setCurrentIndex(m_pl_manager->selectedPlayListIndex());
    m_key_manager->setListWidget(m_listWidget);

    m_positionSlider = new PositionSlider(this);
    m_positionSlider->setFocusPolicy(Qt::NoFocus);
    m_positionSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //prepare visualization
    Visual::initialize(this, m_visMenu, SLOT(updateActions()));
    //playlist manager
    connect(m_positionSlider, SIGNAL(sliderReleased()), SLOT(seek()));
    connect(m_pl_manager, SIGNAL(currentPlayListChanged(PlayListModel*,PlayListModel*)),
            SLOT(onCurrentPlayListChanged(PlayListModel*,PlayListModel*)));
    connect(m_pl_manager, SIGNAL(selectedPlayListChanged(PlayListModel*,PlayListModel*)),
            SLOT(updateTabs()));
    connect(m_pl_manager, SIGNAL(playListRemoved(int)), SLOT(removeTab(int)));
    connect(m_pl_manager, SIGNAL(playListAdded(int)), SLOT(addTab(int)));
    connect(m_pl_manager, SIGNAL(selectedPlayListChanged(PlayListModel*,PlayListModel*)),
            m_listWidget, SLOT(setModel(PlayListModel*,PlayListModel*)));
    connect(m_pl_manager->currentPlayList(), SIGNAL(listChanged(int)), SLOT(onListChanged(int)));
    connect(m_tabWidget,SIGNAL(currentChanged(int)), m_pl_manager, SLOT(selectPlayList(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), m_pl_manager, SLOT(removePlayList(int)));
    connect(m_tabWidget, SIGNAL(tabMoved(int,int)), m_pl_manager, SLOT(move(int,int)));
    connect(m_tabWidget, SIGNAL(createPlayListRequested()), m_pl_manager, SLOT(createPlayList()));

    m_tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabWidget, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showTabMenu(QPoint)));
    m_tab_menu = new QMenu(m_tabWidget);
    //status bar
    m_timeLabel = new QLabel(this);
    m_statusLabel = new QLabel(this);
    m_ui.statusbar->addPermanentWidget(m_statusLabel, 0);
    m_ui.statusbar->addPermanentWidget(m_timeLabel, 1);
    m_ui.statusbar->setMinimumWidth(2);
    m_statusLabel->setMinimumWidth(2);
    m_ui.statusbar->setStyleSheet("QStatusBar::item { border: 0px solid black };");
    //volume
    m_volumeSlider = new VolumeSlider(this);
    m_volumeSlider->setFocusPolicy(Qt::NoFocus);
    m_volumeSlider->setFixedWidth(100);
    m_volumeSlider->setRange(0,100);
    SET_ACTION(ActionManager::VOL_MUTE, m_core, SLOT(setMuted(bool)));
    connect(m_volumeSlider, SIGNAL(sliderMoved(int)), m_core, SLOT(setVolume(int)));
    connect(m_core, SIGNAL(volumeChanged(int)), m_volumeSlider, SLOT(setValue(int)));
    connect(m_core, SIGNAL(volumeChanged(int)), SLOT(updateVolumeIcon()));
    connect(m_core, SIGNAL(mutedChanged(bool)), SLOT(updateVolumeIcon()));
    connect(m_core, SIGNAL(mutedChanged(bool)), ACTION(ActionManager::VOL_MUTE), SLOT(setChecked(bool)));
    m_volumeSlider->setValue(m_core->volume());
    updateVolumeIcon();
    //quick search
    m_quickSearch = new QSUIQuickSearch(m_listWidget, this);
    m_quickSearch->setMaximumWidth(250);
    //visualization
    m_analyzer = new QSUiAnalyzer(this);
    m_ui.analyzerDockWidget->setWidget(m_analyzer);
    Visual::add(m_analyzer);
    //filesystem browser
    m_ui.fileSystemDockWidget->setWidget(new FileSystemBrowser(this));
    //cover
    m_ui.coverDockWidget->setWidget(new CoverWidget(this));
    connect(m_listWidget, SIGNAL(anchorChanged(int)), SLOT(anchorChanged(int)));
    //playlists
    m_ui.playlistsDockWidget->setWidget(new PlayListBrowser(m_pl_manager, this));

    createActions();
    readSettings();
    updateStatus();
    restoreWindowTitle();
}

MainWindow::~MainWindow()
{
}

void MainWindow::addDir()
{
    m_uiHelper->addDirectory(this);
}

void MainWindow::addFiles()
{
    m_uiHelper->addFiles(this);
}

void MainWindow::playFiles()
{
    m_uiHelper->playFiles(this);
}

void MainWindow::record(bool enabled)
{
    EffectFactory *fileWriterFactory = Effect::findFactory("filewriter");
    if(fileWriterFactory)
        Effect::setEnabled(fileWriterFactory, enabled);
}

void MainWindow::addUrl()
{
    m_uiHelper->addUrl(this);
}

void MainWindow::updatePosition(qint64 pos)
{
    m_positionSlider->setMaximum(m_core->duration()/1000);
    if(!m_positionSlider->isSliderDown())
        m_positionSlider->setValue(pos/1000);

    QString text = MetaDataFormatter::formatDuration(pos, false);
    if(m_core->duration() > 1000)
    {
        text.append("/");
        text.append(MetaDataFormatter::formatDuration(m_core->duration()));
    }
    m_timeLabel->setText(text);
}

void MainWindow::seek()
{
    m_core->seek(m_positionSlider->value()*1000);
}

CoverWidget* MainWindow::coverWidget() const
{
    return qobject_cast<CoverWidget *>(m_ui.coverDockWidget->widget());
}

namespace {
    QPixmap getCover(QString path)
    {
        return MetaDataManager::instance()->getCover(path);
    }
}

void MainWindow::setCover(QString path)
{
    coverWidget()->setCover(getCover(path));
}

void MainWindow::showState(Qmmp::State state)
{
    if(state != Qmmp::Playing)
    {
        ACTION(ActionManager::PLAY_PAUSE)->setIcon(QIcon::fromTheme("media-playback-start"));
    }

    switch(state)
    {
    case Qmmp::Playing:
    {
        updateStatus();
        m_analyzer->setCover(getCover(m_core->path()));
        if (!QmmpSettings::instance()->coverFollowsSelected())
            setCover(m_core->path());
        ACTION(ActionManager::PLAY_PAUSE)->setIcon(QIcon::fromTheme("media-playback-pause"));
        break;
    }
    case Qmmp::Paused:
        updateStatus();
        break;
    case Qmmp::Stopped:
        updateStatus();
        m_timeLabel->clear();
        m_positionSlider->setValue(0);
        m_analyzer->clearCover();
        if (!QmmpSettings::instance()->coverFollowsSelected())
            coverWidget()->clearCover();
        break;
    }
}

void MainWindow::updateTabs()
{
    for(int i = 0; i < m_pl_manager->count(); ++i)
    {
        PlayListModel *model = m_pl_manager->playListAt(i);
        if(model == m_pl_manager->currentPlayList())
            m_tabWidget->setTabText(i, "[" + model->name() + "]");
        else
            m_tabWidget->setTabText(i, model->name());
    }
    m_tabWidget->setCurrentIndex(m_pl_manager->selectedPlayListIndex());
}

void MainWindow::removePlaylist()
{
    m_pl_manager->removePlayList(m_pl_manager->selectedPlayList());
}

void MainWindow::removePlaylistWithIndex(int index)
{
    m_pl_manager->removePlayList(m_pl_manager->playListAt(index));
}

void MainWindow::addTab(int index)
{
    m_tabWidget->insertTab(index, m_pl_manager->playListAt(index)->name());
    connect(m_pl_manager->playListAt(index), SIGNAL(nameChanged(QString)), SLOT(updateTabs()));
    updateTabs();
}

void MainWindow::removeTab(int index)
{
    m_tabWidget->removeTab(index);
    updateTabs();
}

void MainWindow::renameTab()
{
    bool ok = false;
    QString name = QInputDialog::getText (this,
                                          tr("Rename Playlist"), tr("Playlist name:"),
                                          QLineEdit::Normal,
                                          m_pl_manager->selectedPlayList()->name(), &ok);
    if(ok)
        m_pl_manager->selectedPlayList()->setName(name);
}

void MainWindow::aboutUi()
{
    AboutQSUIDialog dialog(this);
    dialog.exec();
}

void MainWindow::about()
{
    m_uiHelper->about(this);
}

void MainWindow::toggleVisibility()
{
    if(isHidden() || isMinimized())
        showAndRaise();
    else
        hide();
}

void MainWindow::showAndRaise()
{
    show();
    if(m_wasMaximized)
        showMaximized();
    else
        showNormal();
    raise();
    activateWindow();
}

void MainWindow::showSettings()
{
    ConfigDialog *confDialog = new ConfigDialog(this);
    QSUISettings *simpleSettings = new QSUISettings(this);
    confDialog->addPage(tr("Appearance"), simpleSettings, QIcon(":/qsui/qsui_settings.png"));
    confDialog->addPage(tr("Shortcuts"), new HotkeyEditor(this), QIcon(":/qsui/qsui_shortcuts.png"));
    confDialog->exec();
    simpleSettings->writeSettings();
    confDialog->deleteLater();
    readSettings();
    ActionManager::instance()->saveActions();
    m_analyzer->readSettings();
}

void MainWindow::updateVolumeIcon()
{
    int maxVol = m_core->volume();

    QString iconName = "audio-volume-high";
    if(maxVol == 0 || m_core->isMuted())
        iconName = "audio-volume-muted";
    else if(maxVol < 30)
        iconName = "audio-volume-low";
    else if(maxVol < 60)
        iconName = "audio-volume-medium";

    ACTION(ActionManager::VOL_MUTE)->setIcon(QIcon::fromTheme(iconName, QIcon(QString(":/qsui/") + iconName + ".png")));
}

void MainWindow::jumpTo()
{
    m_uiHelper->jumpToTrack(this);
}

void MainWindow::playPause()
{
    if (m_core->state() == Qmmp::Playing)
        m_core->pause();
    else
        m_player->play();
}

void MainWindow::updateStatus()
{
    int tracks = m_pl_manager->currentPlayList()->trackCount();
    qint64 duration = m_pl_manager->currentPlayList()->totalDuration();

    if(m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused)
    {
        AudioParameters ap = m_core->audioParameters();
        m_statusLabel->setText(tr("<b>%1</b>|%2 bit|%3 ch|%4 Hz|tracks: %5|total time: %6|%7 kbps|")
                               .arg(m_core->state() == Qmmp::Playing ? tr("Playing") : tr("Paused"))
                               .arg(ap.validBitsPerSample())
                               .arg(ap.channels())
                               .arg(ap.sampleRate())
                               .arg(tracks)
                               .arg(MetaDataFormatter::formatDuration(duration, false))
                               .arg(m_core->bitrate()));
    }
    else if(m_core->state() == Qmmp::Stopped)
    {
        m_statusLabel->setText(tr("<b>%1</b>|tracks: %2|total time: %3|")
                               .arg(tr("Stopped"))
                               .arg(tracks)
                               .arg(MetaDataFormatter::formatDuration(duration, false)));
    }
    else
        m_statusLabel->clear();
}

void MainWindow::closeEvent(QCloseEvent *)
{
    if(!m_hideOnClose || !m_uiHelper->visibilityControl())
        m_uiHelper->exit();

}

void MainWindow::hideEvent(QHideEvent *)
{
    writeSettings();
    m_wasMaximized = isMaximized();
}

void MainWindow::createWidgets()
{
    m_tabWidget = new QSUiTabWidget(this);
    m_listWidget = m_tabWidget->listWidget();
    m_listWidget->setMenu(m_pl_menu);
    setCentralWidget(m_tabWidget);
    //'new playlist' button
    m_addListButton = new QToolButton(m_tabWidget);
    m_addListButton->setText("+");
    m_addListButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_addListButton->setAutoRaise(true);
    m_addListButton->setIcon(QIcon::fromTheme("list-add"));
    m_addListButton->setToolTip(tr("Add new playlist"));
    connect(m_addListButton, SIGNAL(clicked()), m_pl_manager, SLOT(createPlayList()));
    //playlist menu button
    m_tabListMenuButton = new QToolButton(m_tabWidget);
    m_tabListMenuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_tabListMenuButton->setAutoRaise(true);
    m_tabListMenuButton->setToolTip(tr("Show all tabs"));
    m_tabListMenuButton->setArrowType(Qt::DownArrow);
    m_tabListMenuButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    m_tabListMenuButton->setPopupMode(QToolButton::InstantPopup);
    m_tabListMenuButton->setMenu(m_tabWidget->menu());
}

void MainWindow::createActions()
{
    //prepare checkable actions
    ACTION(ActionManager::REPEAT_ALL)->setChecked(m_ui_settings->isRepeatableList());
    ACTION(ActionManager::REPEAT_TRACK)->setChecked(m_ui_settings->isRepeatableTrack());
    ACTION(ActionManager::SHUFFLE)->setChecked(m_ui_settings->isShuffle());
    ACTION(ActionManager::NO_PL_ADVANCE)->setChecked(m_ui_settings->isNoPlayListAdvance());

    connect(m_ui_settings, SIGNAL(repeatableListChanged(bool)),
            ACTION(ActionManager::REPEAT_ALL), SLOT(setChecked(bool)));
    connect(m_ui_settings, SIGNAL (repeatableTrackChanged(bool)),
            ACTION(ActionManager::REPEAT_TRACK), SLOT(setChecked(bool)));
    connect(m_ui_settings, SIGNAL (noPlayListAdvanceChanged(bool)),
            ACTION(ActionManager::NO_PL_ADVANCE), SLOT(setChecked(bool)));
    connect(m_ui_settings, SIGNAL(shuffleChanged(bool)),
            ACTION(ActionManager::SHUFFLE), SLOT(setChecked(bool)));
    //register external actions
    ActionManager::instance()->registerAction(ActionManager::UI_ANALYZER,
                                              m_ui.analyzerDockWidget->toggleViewAction(),
                                              "analyzer", "");
    ActionManager::instance()->registerAction(ActionManager::UI_FILEBROWSER,
                                              m_ui.fileSystemDockWidget->toggleViewAction(),
                                              "file_browser", tr("Ctrl+0"));
    ActionManager::instance()->registerAction(ActionManager::UI_COVER,
                                              m_ui.coverDockWidget->toggleViewAction(),
                                              "cover", "");
    ActionManager::instance()->registerAction(ActionManager::UI_PLAYLISTBROWSER,
                                              m_ui.playlistsDockWidget->toggleViewAction(),
                                              "playlist_browser", tr("P"));
    ActionManager::instance()->registerWidget(ActionManager::UI_POS_SLIDER, m_positionSlider,
                                              tr("Position"), "position_slider");
    ActionManager::instance()->registerWidget(ActionManager::UI_VOL_SLIDER, m_volumeSlider,
                                              tr("Volume"), "volume_slider");
    ActionManager::instance()->registerWidget(ActionManager::UI_QUICK_SEARCH, m_quickSearch,
                                              tr("Quick Search"), "quick_search");
    //playback
    SET_ACTION(ActionManager::PREVIOUS, m_player, SLOT(previous()));
    SET_ACTION(ActionManager::PLAY, m_player, SLOT(play()));
    SET_ACTION(ActionManager::PAUSE, m_core, SLOT(pause()));
    SET_ACTION(ActionManager::STOP, m_player, SLOT(stop()));
    SET_ACTION(ActionManager::NEXT, m_player, SLOT(next()));
    SET_ACTION(ActionManager::EJECT, this, SLOT(playFiles()));
    SET_ACTION(ActionManager::RECORD, this, SLOT(record(bool)));

    //file menu
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_ADD_FILE, this, SLOT(addFiles())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_ADD_DIRECTORY, this, SLOT(addDir())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_ADD_URL, this, SLOT(addUrl())));
    QAction *sep = m_ui.menuFile->addSeparator();
    UiHelper::instance()->registerMenu(UiHelper::ADD_MENU, m_ui.menuFile, sep);
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_NEW, m_pl_manager, SLOT(createPlayList())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_CLOSE, this, SLOT(removePlaylist())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_RENAME, this, SLOT(renameTab())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_SELECT_NEXT, m_pl_manager,
                                        SLOT(selectNextPlayList())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_SELECT_PREVIOUS, m_pl_manager,
                                        SLOT(selectPreviousPlayList())));
    m_ui.menuFile->addSeparator();
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_LOAD, this, SLOT(loadPlayList())));
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::PL_SAVE, this, SLOT(savePlayList())));
    m_ui.menuFile->addSeparator();
    m_ui.menuFile->addAction(SET_ACTION(ActionManager::QUIT, m_uiHelper, SLOT(exit())));
    //edit menu
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_SELECT_ALL, m_pl_manager, SLOT(selectAll())));
    m_ui.menuEdit->addSeparator();
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_SELECTED, m_listWidget,
                                        SLOT(removeSelected())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_UNSELECTED, m_listWidget,
                                        SLOT(removeUnselected())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_ALL, m_listWidget, SLOT(clear())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_INVALID, m_pl_manager,
                                        SLOT(removeInvalidTracks())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REMOVE_DUPLICATES, m_pl_manager,
                                        SLOT(removeDuplicates())));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_REFRESH, m_pl_manager,
                                        SLOT(refresh())));
    m_ui.menuEdit->addSeparator();
    //view menu
    m_ui.menuView->addAction(SET_ACTION(ActionManager::WM_ALLWAYS_ON_TOP, this, SLOT(readSettings())));
    m_ui.menuView->addSeparator();
    m_ui.menuView->addAction(m_ui.analyzerDockWidget->toggleViewAction());
    m_ui.menuView->addAction(m_ui.fileSystemDockWidget->toggleViewAction());
    m_ui.menuView->addAction(m_ui.coverDockWidget->toggleViewAction());
    m_ui.menuView->addAction(m_ui.playlistsDockWidget->toggleViewAction());
    m_ui.menuView->addSeparator();
    m_ui.menuView->addAction(SET_ACTION(ActionManager::UI_SHOW_TABS, m_tabWidget->tabBar(), SLOT(setVisible(bool))));
    m_ui.menuView->addAction(SET_ACTION(ActionManager::UI_SHOW_TITLEBARS, this, SLOT(setTitleBarsVisible(bool))));
    m_ui.menuView->addAction(ACTION(ActionManager::PL_SHOW_HEADER));
    m_ui.menuView->addSeparator();
    m_ui.menuView->addAction(SET_ACTION(ActionManager::UI_BLOCK_TOOLBARS, this, SLOT(setToolBarsBlocked(bool))));
    m_ui.menuView->addAction(tr("Edit Toolbars"), this, SLOT(editToolBar()));

    QMenu* sort_mode_menu = new QMenu (tr("Sort List"), this);
    sort_mode_menu->setIcon(QIcon::fromTheme("view-sort-ascending"));

    QAction *titleAct = sort_mode_menu->addAction(tr ("By Title"));
    connect(titleAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::TITLE); } );

    QAction *albumAct = sort_mode_menu->addAction(tr ("By Album"));
    connect(albumAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::ALBUM); } );

    QAction *artistAct = sort_mode_menu->addAction(tr ("By Artist"));
    connect(artistAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::ARTIST); } );

    QAction *albumArtistAct = sort_mode_menu->addAction(tr ("By Album Artist"));
    connect(albumArtistAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::ALBUMARTIST); } );

    QAction *nameAct = sort_mode_menu->addAction(tr ("By Filename"));
    connect(nameAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::FILENAME); } );

    QAction *pathnameAct = sort_mode_menu->addAction(tr ("By Path + Filename"));
    connect(pathnameAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::PATH_AND_FILENAME); } );

    QAction *dateAct = sort_mode_menu->addAction(tr ("By Date"));
    connect(dateAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::DATE); } );

    QAction *trackAct = sort_mode_menu->addAction(tr("By Track Number"));
    connect(trackAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::TRACK); } );

    QAction *discAct = sort_mode_menu->addAction(tr("By Disc Number"));
    connect(discAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::DISCNUMBER); } );

    QAction *fileCreationDateAct = sort_mode_menu->addAction(tr("By File Creation Date"));
    connect(fileCreationDateAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::FILE_CREATION_DATE); } );

    QAction *fileModificationDateAct = sort_mode_menu->addAction(tr("By File Modification Date"));
    connect(fileModificationDateAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::FILE_MODIFICATION_DATE); } );

    QAction *groupAct = sort_mode_menu->addAction(tr("By Group"));
    connect(groupAct, &QAction::triggered, [this]{ m_pl_manager->sort(PlayListModel::GROUP); } );

    m_ui.menuEdit->addMenu (sort_mode_menu);

    sort_mode_menu = new QMenu (tr("Sort Selection"), this);
    sort_mode_menu->setIcon(QIcon::fromTheme("view-sort-ascending"));

    titleAct = sort_mode_menu->addAction(tr ("By Title"));
    connect(titleAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::TITLE); });

    albumAct = sort_mode_menu->addAction(tr ("By Album"));
    connect(albumAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::ALBUM); });

    artistAct = sort_mode_menu->addAction(tr ("By Artist"));
    connect(artistAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::ARTIST); });

    albumArtistAct = sort_mode_menu->addAction(tr ("By Album Artist"));
    connect(albumArtistAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::ALBUMARTIST); });

    nameAct = sort_mode_menu->addAction(tr ("By Filename"));
    connect(nameAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::FILENAME); });

    pathnameAct = sort_mode_menu->addAction(tr ("By Path + Filename"));
    connect(pathnameAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::PATH_AND_FILENAME); });

    dateAct = sort_mode_menu->addAction(tr ("By Date"));
    connect(dateAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::DATE); });

    trackAct = sort_mode_menu->addAction(tr ("By Track Number"));
    connect(trackAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::TRACK); });

    discAct = sort_mode_menu->addAction(tr("By Disc Number"));
    connect(discAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::DISCNUMBER); });

    fileCreationDateAct = sort_mode_menu->addAction(tr("By File Creation Date"));
    connect(fileCreationDateAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::FILE_CREATION_DATE); });

    fileModificationDateAct = sort_mode_menu->addAction(tr("By File Modification Date"));
    connect(fileModificationDateAct, &QAction::triggered, [this]{ m_pl_manager->sortSelection(PlayListModel::FILE_MODIFICATION_DATE ); });

    m_ui.menuEdit->addMenu (sort_mode_menu);
    m_ui.menuEdit->addSeparator();
    m_ui.menuEdit->addAction(QIcon::fromTheme("media-playlist-shuffle"), tr("Randomize List"),
                              m_pl_manager, SLOT(randomizeList()));
    m_ui.menuEdit->addAction(QIcon::fromTheme("view-sort-descending"), tr("Reverse List"),
                              m_pl_manager, SLOT(reverseList()));
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::PL_GROUP_TRACKS, m_ui_settings, SLOT(setGroupsEnabled(bool))));
    ACTION(ActionManager::PL_GROUP_TRACKS)->setChecked(m_ui_settings->isGroupsEnabled());
    m_ui.menuEdit->addSeparator();
    m_ui.menuEdit->addAction(SET_ACTION(ActionManager::SETTINGS, this, SLOT(showSettings())));
    //tools
    m_ui.menuTools->addMenu(m_uiHelper->createMenu(UiHelper::TOOLS_MENU, tr("Actions"), this));
    //playback menu
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PLAY));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::STOP));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PAUSE));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::NEXT));
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PREVIOUS));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::PLAY_PAUSE,this,SLOT(playPause())));
    m_ui.menuPlayback->addSeparator();
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::JUMP, this, SLOT(jumpTo())));
    m_ui.menuPlayback->addSeparator();
    m_ui.menuPlayback->addAction(ACTION(ActionManager::PL_ENQUEUE));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::CLEAR_QUEUE, m_pl_manager, SLOT(clearQueue())));
    m_ui.menuPlayback->addSeparator();
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::REPEAT_ALL, m_ui_settings,
                                            SLOT(setRepeatableList(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::REPEAT_TRACK, m_ui_settings,
                                            SLOT(setRepeatableTrack(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::SHUFFLE, m_ui_settings,
                                            SLOT(setShuffle(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::NO_PL_ADVANCE, m_ui_settings,
                                            SLOT(setNoPlayListAdvance(bool))));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::STOP_AFTER_SELECTED, m_pl_manager,
                                            SLOT(stopAfterSelected())));
    m_ui.menuPlayback->addSeparator();
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::VOL_ENC, m_core, SLOT(volumeUp())));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::VOL_DEC, m_core, SLOT(volumeDown())));
    m_ui.menuPlayback->addAction(SET_ACTION(ActionManager::VOL_MUTE, m_core, SLOT(setMuted(bool))));
    connect(m_core, SIGNAL(mutedChanged(bool)), ACTION(ActionManager::VOL_MUTE), SLOT(setChecked(bool)));

    //help menu
    m_ui.menuHelp->addAction(SET_ACTION(ActionManager::ABOUT_UI, this, SLOT(aboutUi())));
    m_ui.menuHelp->addAction(SET_ACTION(ActionManager::ABOUT, this, SLOT(about())));
    m_ui.menuHelp->addAction(SET_ACTION(ActionManager::ABOUT_QT, qApp, SLOT(aboutQt())));
    //playlist menu
    m_pl_menu->addAction(SET_ACTION(ActionManager::PL_SHOW_INFO, m_pl_manager, SLOT(showDetails())));
    m_pl_menu->addAction(SET_ACTION(ActionManager::PL_FILE_LOCATION, this, SLOT(openFileLocation())));
    m_pl_menu->addSeparator();
    m_pl_menu->addAction(ACTION(ActionManager::PL_REMOVE_SELECTED));
    m_pl_menu->addAction(ACTION(ActionManager::PL_REMOVE_ALL));
    m_pl_menu->addAction(ACTION(ActionManager::PL_REMOVE_UNSELECTED));
    m_pl_menu->addMenu(UiHelper::instance()->createMenu(UiHelper::PLAYLIST_MENU,
                                                        tr("Actions"), this));
    m_pl_menu->addSeparator();
    m_pl_menu->addAction(SET_ACTION(ActionManager::PL_ENQUEUE, m_pl_manager, SLOT(addToQueue())));
    //tools menu
    m_ui.menuTools->addAction(SET_ACTION(ActionManager::EQUALIZER, this, SLOT(showEqualizer())));

    //tab menu
    m_tab_menu->addAction(ACTION(ActionManager::PL_LOAD));
    m_tab_menu->addAction(ACTION(ActionManager::PL_SAVE));
    m_tab_menu->addSeparator();
    m_tab_menu->addAction(ACTION(ActionManager::PL_RENAME));
    m_tab_menu->addAction(ACTION(ActionManager::PL_CLOSE));
    //seeking
    QAction* forward = new QAction(this);
    forward->setShortcut(QKeySequence(Qt::Key_Right));
    connect(forward,SIGNAL(triggered(bool)),this,SLOT(forward()));
    QAction* backward = new QAction(this);
    backward->setShortcut(QKeySequence(Qt::Key_Left));
    connect(backward,SIGNAL(triggered(bool)),this,SLOT(backward()));

    addActions(QList<QAction*>() << forward << backward);
    addActions(ActionManager::instance()->actions());
    addActions(m_key_manager->actions());
}

void MainWindow::readSettings()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("Simple");
    m_titleFormatter.setPattern(settings.value("window_title_format","%if(%p,%p - %t,%t)").toString());

    //update toolbars
    QList<ActionManager::ToolBarInfo> toolBarInfoList = ActionManager::instance()->readToolBarSettings();
    QList<QToolBar *> toolBars = findChildren<QToolBar*>();

    //clear toolbars to avoid conflicts
    foreach (QToolBar *toolBar, toolBars)
        toolBar->clear();

    foreach (ActionManager::ToolBarInfo info, toolBarInfoList)
    {
        bool found = false;
        foreach (QToolBar *toolBar, toolBars)
        {
            if(toolBar->property("uid").toString() == info.uid)
            {
                found = true;
                toolBars.removeAll(toolBar);
                ActionManager::instance()->updateToolBar(toolBar, info);
                break;
            }
        }

        if(!found)
        {
            addToolBar(ActionManager::instance()->createToolBar(info, this));
        }
    }

    foreach (QToolBar *toolBar, toolBars)
        toolBar->deleteLater();
    toolBars.clear();
    setToolBarsBlocked(ACTION(ActionManager::UI_BLOCK_TOOLBARS)->isChecked());

    if(m_update)
    {
        m_listWidget->readSettings();
        qobject_cast<FileSystemBrowser *> (m_ui.fileSystemDockWidget->widget())->readSettings();

        if(ACTION(ActionManager::WM_ALLWAYS_ON_TOP)->isChecked())
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        else
            setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

        if(m_core->state() == Qmmp::Playing || m_core->state() == Qmmp::Paused)
            showMetaData();

        m_tabWidget->readSettings();

        show();
    }
    else
    {
        restoreGeometry(settings.value("mw_geometry").toByteArray());
        QByteArray wstate = settings.value("mw_state").toByteArray();
        if(wstate.isEmpty())
        {
            m_ui.fileSystemDockWidget->hide();
            m_ui.coverDockWidget->hide();
            m_ui.playlistsDockWidget->hide();
        }
        else
            restoreState(settings.value("mw_state").toByteArray());
        if(settings.value("always_on_top", false).toBool())
        {
            ACTION(ActionManager::WM_ALLWAYS_ON_TOP)->setChecked(true);
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        }
        show();
        qApp->processEvents();
        if(settings.value("start_hidden").toBool())
            hide();

        bool state = settings.value("show_titlebars", true).toBool();
        ACTION(ActionManager::UI_SHOW_TITLEBARS)->setChecked(state);
        setTitleBarsVisible(state);

        state = settings.value("show_tabs", true).toBool();
        ACTION(ActionManager::UI_SHOW_TABS)->setChecked(state);
        m_tabWidget->setTabsVisible(state);

        state = settings.value("block_toolbars", false).toBool();
        ACTION(ActionManager::UI_BLOCK_TOOLBARS)->setChecked(state);
        setToolBarsBlocked(state);

        m_update = true;
    }

    m_hideOnClose = settings.value("hide_on_close", false).toBool();

    if(settings.value("pl_show_new_pl_button", false).toBool())
    {
        m_tabWidget->setCornerWidget(m_addListButton, Qt::TopLeftCorner);
        m_addListButton->setIconSize(QSize(16, 16));
        m_addListButton->setVisible(true);
    }
    else
    {
        m_addListButton->setVisible(false);
        m_tabWidget->setCornerWidget(nullptr, Qt::TopLeftCorner);
    }
    if(settings.value("pl_show_tab_list_menu", false).toBool())
    {
        m_tabWidget->setCornerWidget(m_tabListMenuButton, Qt::TopRightCorner);
        m_tabListMenuButton->setIconSize(QSize(16, 16));
        m_tabListMenuButton->setVisible(true);
    }
    else
    {
        m_tabListMenuButton->setVisible(false);
        m_tabWidget->setCornerWidget(nullptr, Qt::TopRightCorner);
    }

    settings.endGroup();
    addActions(m_uiHelper->actions(UiHelper::TOOLS_MENU));
    addActions(m_uiHelper->actions(UiHelper::PLAYLIST_MENU));

    //record action
    EffectFactory *fileWriterFactory = Effect::findFactory("filewriter");
    if(fileWriterFactory)
    {
        ACTION(ActionManager::RECORD)->setEnabled(true);
        ACTION(ActionManager::RECORD)->setChecked(Effect::isEnabled(fileWriterFactory));
    }
    else
    {
        ACTION(ActionManager::RECORD)->setEnabled(false);
        ACTION(ActionManager::RECORD)->setChecked(false);
    }
}

void MainWindow::showTabMenu(QPoint pos)
{
    int index = m_tabWidget->tabBar()->tabAt(pos);
    if(index == -1)
        return;

    m_pl_manager->selectPlayList(index);
    m_tab_menu->popup(m_tabWidget->mapToGlobal(pos));
}

void MainWindow::writeSettings()
{
    QSettings settings (Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue("Simple/mw_geometry", saveGeometry());
    settings.setValue("Simple/mw_state", saveState());
    settings.setValue("Simple/always_on_top", ACTION(ActionManager::WM_ALLWAYS_ON_TOP)->isChecked());
    settings.setValue("Simple/show_analyzer", ACTION(ActionManager::UI_ANALYZER)->isChecked());
    settings.setValue("Simple/show_tabs", ACTION(ActionManager::UI_SHOW_TABS)->isChecked());
    settings.setValue("Simple/show_titlebars", ACTION(ActionManager::UI_SHOW_TITLEBARS)->isChecked());
    settings.setValue("Simple/block_toolbars", ACTION(ActionManager::UI_BLOCK_TOOLBARS)->isChecked());
}

void MainWindow::savePlayList()
{
    m_uiHelper->savePlayList(this);
}

void MainWindow::loadPlayList()
{
    m_uiHelper->loadPlayList(this);
}

void MainWindow::showBuffering(int percent)
{
    if(m_core->state() == Qmmp::Buffering)
        m_statusLabel->setText(tr("Buffering: %1%").arg(percent));
}

void MainWindow::showEqualizer()
{
    Equalizer equalizer(this);
    equalizer.exec();
}

void MainWindow::forward()
{
    m_core->seek(m_core->elapsed() + KEY_OFFSET);
}

void MainWindow::backward()
{
    m_core->seek(qMax(qint64(0), m_core->elapsed() - KEY_OFFSET));
}

void MainWindow::showMetaData()
{
    PlayListModel *model = m_pl_manager->currentPlayList();
    PlayListTrack *track = model->currentTrack();
    if(track && track->path() == m_core->trackInfo().path())
    {
        setWindowTitle(m_titleFormatter.format(track));
    }
}

void MainWindow::setTitleBarsVisible(bool visible)
{
    if(visible)
    {
        QWidget *widget = nullptr;
        if((widget = m_ui.analyzerDockWidget->titleBarWidget()))
        {
            m_ui.analyzerDockWidget->setTitleBarWidget(nullptr);
            delete widget;
        }
        if((widget = m_ui.fileSystemDockWidget->titleBarWidget()))
        {
            m_ui.fileSystemDockWidget->setTitleBarWidget(nullptr);
            delete widget;
        }
        if((widget = m_ui.coverDockWidget->titleBarWidget()))
        {
            m_ui.coverDockWidget->setTitleBarWidget(nullptr);
            delete widget;
        }
        if((widget = m_ui.playlistsDockWidget->titleBarWidget()))
        {
            m_ui.playlistsDockWidget->setTitleBarWidget(nullptr);
            delete widget;
        }
    }
    else
    {
        if(!m_ui.analyzerDockWidget->titleBarWidget())
            m_ui.analyzerDockWidget->setTitleBarWidget(new QWidget());

        if(!m_ui.fileSystemDockWidget->titleBarWidget())
            m_ui.fileSystemDockWidget->setTitleBarWidget(new QWidget());

        if(!m_ui.coverDockWidget->titleBarWidget())
            m_ui.coverDockWidget->setTitleBarWidget(new QWidget());

        if(!m_ui.playlistsDockWidget->titleBarWidget())
            m_ui.playlistsDockWidget->setTitleBarWidget(new QWidget());
    }
}

void MainWindow::setToolBarsBlocked(bool blocked)
{
    foreach (QToolBar *t, findChildren<QToolBar *>())
    {
        t->setMovable(!blocked);
    }
}

void MainWindow::editToolBar()
{
    ToolBarEditor *e = new ToolBarEditor(this);
    if(e->exec() == QDialog::Accepted)
    {
        readSettings();
    }
    e->deleteLater();
}

void MainWindow::restoreWindowTitle()
{
    setWindowTitle(tr("Qmmp"));
}

void MainWindow::onListChanged(int flags)
{
    if(flags & PlayListModel::STRUCTURE)
        updateStatus();
}

void MainWindow::onCurrentPlayListChanged(PlayListModel *current, PlayListModel *previous)
{
    updateTabs();
    updateStatus();
    connect(current, SIGNAL(listChanged(int)), SLOT(onListChanged(int)));
    if(previous)
        disconnect(current, SIGNAL(listChanged(int)), this, SLOT(onListChanged(int)));
}

void MainWindow::openFileLocation()
{
    PlayListModel *model = m_listWidget->model();
    UiHelper::instance()->openFileLocation(model->track(m_listWidget->anchorIndex()));
}

void MainWindow::anchorChanged(int index)
{
    if (!QmmpSettings::instance()->coverFollowsSelected())
        return;

    if (m_listWidget->anchorIndex() < 0) {
        setCover(m_core->path());
        return;
    }

    PlayListModel *model = m_listWidget->model();
    PlayListTrack *track = model->track(m_listWidget->anchorIndex());
    setCover(track->path());
}
