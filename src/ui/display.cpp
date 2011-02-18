/***************************************************************************
 *   Copyright (C) 2006-2011 by Ilya Kotov                                 *
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

#include <QApplication>
#include <QSettings>
#include <QMenu>
#include <qmmp/soundcore.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/playlistmanager.h>
#include "skin.h"
#include "mainvisual.h"
#include "button.h"
#include "titlebar.h"
#include "positionbar.h"
#include "number.h"
#include "togglebutton.h"
#include "symboldisplay.h"
#include "textscroller.h"
#include "monostereo.h"
#include "playstatus.h"
#include "volumebar.h"
#include "balancebar.h"
#include "mainwindow.h"
#include "timeindicator.h"
#include "actionmanager.h"
#include "display.h"

MainDisplay::MainDisplay (QWidget *parent)
        : PixmapWidget (parent)
{
    m_shaded = false;
    m_skin = Skin::instance();
    setPixmap (m_skin->getMain());
    setCursor(m_skin->getCursor(Skin::CUR_NORMAL));
    m_mw = qobject_cast<MainWindow*>(parent);
    m_titlebar = new TitleBar(this);
    m_titlebar->move(0,0);
    m_titlebar->setActive(true);
    m_previous = new Button (this, Skin::BT_PREVIOUS_N, Skin::BT_PREVIOUS_P, Skin::CUR_NORMAL);
    m_previous->setToolTip(tr("Previous"));
    connect (m_previous, SIGNAL (clicked()), parent, SLOT (previous()));

    m_play = new Button (this, Skin::BT_PLAY_N, Skin::BT_PLAY_P, Skin::CUR_NORMAL);
    m_play->setToolTip(tr("Play"));
    connect (m_play, SIGNAL (clicked()),parent,SLOT (play()));
    m_pause = new Button (this, Skin::BT_PAUSE_N,Skin::BT_PAUSE_P, Skin::CUR_NORMAL);
    m_pause->setToolTip(tr("Pause"));
    connect (m_pause,SIGNAL (clicked()),parent,SLOT (pause()));
    m_stop = new Button (this, Skin::BT_STOP_N,Skin::BT_STOP_P, Skin::CUR_NORMAL);
    m_stop->setToolTip(tr("Stop"));
    connect (m_stop,SIGNAL (clicked()),parent,SLOT (stop()));
    m_next = new Button (this, Skin::BT_NEXT_N,Skin::BT_NEXT_P, Skin::CUR_NORMAL);
    m_next->setToolTip(tr("Next"));
    connect (m_next,SIGNAL (clicked()),parent,SLOT (next()));
    m_eject = new Button (this, Skin::BT_EJECT_N,Skin::BT_EJECT_P, Skin::CUR_NORMAL);
    m_eject->setToolTip(tr("Add file"));
    connect (m_eject,SIGNAL (clicked()),parent,SLOT (addFile()));
    connect (m_skin, SIGNAL (skinChanged()), this, SLOT (updateSkin()));
    m_posbar = new PositionBar (this);
    m_vis = new MainVisual (this);

    m_eqButton = new ToggleButton (this,Skin::BT_EQ_ON_N,Skin::BT_EQ_ON_P,
                                   Skin::BT_EQ_OFF_N,Skin::BT_EQ_OFF_P);
    m_eqButton->setToolTip(tr("Equalizer"));
    m_plButton = new ToggleButton (this,Skin::BT_PL_ON_N,Skin::BT_PL_ON_P,
                                   Skin::BT_PL_OFF_N,Skin::BT_PL_OFF_P);
    m_plButton->setToolTip(tr("Playlist"));

    m_repeatButton = new ToggleButton (this,Skin::REPEAT_ON_N,Skin::REPEAT_ON_P,
                                       Skin::REPEAT_OFF_N,Skin::REPEAT_OFF_P);
    connect(m_repeatButton,SIGNAL(clicked(bool)),this,SIGNAL(repeatableToggled(bool)));
    m_repeatButton->setToolTip(tr("Repeat playlist"));
    m_shuffleButton = new ToggleButton (this,Skin::SHUFFLE_ON_N,Skin::SHUFFLE_ON_P,
                                        Skin::SHUFFLE_OFF_N,Skin::SHUFFLE_OFF_P);
    m_shuffleButton->setToolTip(tr("Shuffle"));
    connect(m_shuffleButton,SIGNAL(clicked(bool)),this,SIGNAL(shuffleToggled(bool)));

    m_kbps = new SymbolDisplay(this,3);
    m_freq = new SymbolDisplay(this,2);
    m_text = new TextScroller (this);
    m_monoster = new MonoStereo (this);
    m_playstatus = new PlayStatus(this);

    m_volumeBar = new VolumeBar(this);
    m_volumeBar->setToolTip(tr("Volume"));
    connect(m_volumeBar, SIGNAL(sliderMoved(int)),SLOT(updateVolume()));
    connect(m_volumeBar, SIGNAL(sliderPressed()),SLOT(updateVolume()));
    connect(m_volumeBar, SIGNAL(sliderReleased()),m_text,SLOT(clear()));

    m_balanceBar = new BalanceBar(this);
    m_balanceBar->setToolTip(tr("Balance"));
    connect(m_balanceBar, SIGNAL(sliderMoved(int)),SLOT(updateVolume()));
    connect(m_balanceBar, SIGNAL(sliderPressed()),SLOT(updateVolume()));
    connect(m_balanceBar, SIGNAL(sliderReleased()),m_text,SLOT(clear()));

    m_timeIndicator = new TimeIndicator(this);
    m_aboutWidget = new QWidget(this);
    m_core = SoundCore::instance();
    connect(m_core, SIGNAL(elapsedChanged(qint64)), SLOT(setTime(qint64)));
    connect(m_core, SIGNAL(bitrateChanged(int)), m_kbps, SLOT(display(int)));
    connect(m_core, SIGNAL(frequencyChanged(quint32)), SLOT(setSampleRate(quint32)));
    connect(m_core, SIGNAL(channelsChanged(int)), m_monoster, SLOT(setChannels(int)));
    connect(m_core, SIGNAL(stateChanged(Qmmp::State)), SLOT(setState(Qmmp::State)));
    connect(m_core, SIGNAL(volumeChanged(int,int)), SLOT(setVolume(int, int)));
    connect(m_core, SIGNAL(elapsedChanged(qint64)),m_titlebar, SLOT(setTime(qint64)));
    PlayListManager *pl_manager = MediaPlayer::instance()->playListManager();
    connect(pl_manager, SIGNAL(repeatableListChanged(bool)), m_repeatButton, SLOT(setON(bool)));
    connect(pl_manager, SIGNAL(shuffleChanged(bool)), m_shuffleButton, SLOT(setON(bool)));
    updatePositions();
    updateMask();
}

MainDisplay::~MainDisplay()
{
    QSettings settings (Qmmp::configFile(), QSettings::IniFormat);
    settings.setValue ("Playlist/visible",m_plButton->isChecked());
    settings.setValue ("Equalizer/visible",m_eqButton->isChecked());
}

void MainDisplay::updatePositions()
{
    int r = m_skin->ratio();
    m_previous->move (r*16, r*88);
    m_play->move (r*39,  r*88);
    m_pause->move (r*62,  r*88);
    m_vis->move(r*24,  r*43);
    m_stop->move (r*85,  r*88);
    m_next->move (r*108, r*88);
    m_eject->move (r*136, r*89);
    m_posbar->move (r*16, r*72);
    m_eqButton->move (r*219, r*58);
    m_plButton->move (r*241, r*58);
    m_repeatButton->move (r*210, r*89);
    m_shuffleButton->move (r*164, r*89);
    m_kbps->move (r*111, r*43);
    m_freq->move (r*156, r*43);
    m_text->resize (r*154, r*15);
    m_text->move (r*109, r*23);
    m_monoster->move (r*212, r*41);
    m_playstatus->move(r*24, r*28);
    m_volumeBar->move(r*107, r*57);
    m_balanceBar->move(r*177, r*57);
    m_timeIndicator->move(r*34, r*26);
    m_aboutWidget->setGeometry(r*247,r*83,r*20,r*25);
}

void MainDisplay::setTime (qint64 t)
{
    m_posbar->setValue (t);
    m_timeIndicator->setTime(t/1000);
}
void MainDisplay::setDuration(qint64 t)
{
    m_posbar->setMax (t);
    m_timeIndicator->setSongDuration(t/1000);
}

void MainDisplay::setState(Qmmp::State state)
{
    switch ((int) state)
    {
    case Qmmp::Playing:
        m_playstatus->setStatus(PlayStatus::PLAY);
        m_timeIndicator->setNeedToShowTime(true);
        setDuration(m_core->totalTime());
        break;
    case Qmmp::Paused:
        m_playstatus->setStatus(PlayStatus::PAUSE);
        break;
    case Qmmp::Stopped:
        m_playstatus->setStatus(PlayStatus::STOP);
        m_monoster->setChannels (0);
        m_timeIndicator->setNeedToShowTime(false);
        m_posbar->setValue (0);
        m_posbar->setMax (0);
        m_titlebar->setTime(-1);
    }
}

void MainDisplay::setVolume(int left, int right)
{
    int maxVol = qMax(left, right);
    m_volumeBar->setValue(maxVol);
    if (maxVol && !m_volumeBar->isPressed())
        m_balanceBar->setValue((right - left) * 100/maxVol);
}

void MainDisplay::updateSkin()
{
    setPixmap (m_skin->getMain());
    m_mw->resize(size());
    setCursor(m_skin->getCursor(Skin::CUR_NORMAL));
    setMinimalMode(m_shaded);
    updatePositions();
}

void MainDisplay::updateMask()
{
    m_mw->clearMask();
    m_mw->setMask(QRegion(0,0,m_mw->width(),m_mw->height()));
    QRegion region = m_skin->getRegion(m_shaded? Skin::WINDOW_SHADE : Skin::NORMAL);
    if (!region.isEmpty())
        m_mw->setMask(region);
}

void MainDisplay::setMinimalMode(bool b)
{
    m_shaded = b;
    int r = m_skin->ratio();

    if(m_shaded)
         m_mw->resize(r*275,r*14);
    else
         m_mw->resize(r*275,r*116);
    updateMask();
}

void MainDisplay::setActive(bool b)
{
    m_titlebar->setActive(b);
}

void MainDisplay::setSampleRate(quint32 rate)
{
    m_freq->display((int) rate/1000);
}
//TODO optimize this connections
void MainDisplay::setEQ (QWidget* w)
{
    m_equlizer = w;
    m_eqButton->setON (m_equlizer->isVisible());
    ACTION(ActionManager::SHOW_EQUALIZER)->setChecked(m_equlizer->isVisible());

    connect (ACTION(ActionManager::SHOW_EQUALIZER), SIGNAL(triggered(bool)),
             m_equlizer, SLOT (setVisible (bool)));
    connect (ACTION(ActionManager::SHOW_EQUALIZER), SIGNAL(triggered(bool)),
             m_eqButton, SLOT (setON (bool)));

    connect (m_eqButton, SIGNAL(clicked(bool)),
             ACTION(ActionManager::SHOW_EQUALIZER), SLOT(setChecked (bool)));
    connect (m_eqButton, SIGNAL(clicked(bool)), m_equlizer, SLOT (setVisible (bool)));
    connect (m_equlizer, SIGNAL(closed ()), m_eqButton, SLOT (click()));
}

void MainDisplay::setPL (QWidget* w)
{
    m_playlist = w;
    m_plButton->setON (m_playlist->isVisible());
    ACTION(ActionManager::SHOW_PLAYLIST)->setChecked(m_playlist->isVisible());

    connect (ACTION(ActionManager::SHOW_PLAYLIST), SIGNAL(triggered(bool)),
             m_playlist, SLOT (setVisible (bool)));
    connect (ACTION(ActionManager::SHOW_PLAYLIST), SIGNAL(triggered(bool)),
             m_plButton, SLOT (setON (bool)));

    connect (m_plButton, SIGNAL(clicked(bool)),
             ACTION(ActionManager::SHOW_PLAYLIST), SLOT(setChecked (bool)));
    connect (m_plButton, SIGNAL (clicked (bool)), m_playlist, SLOT (setVisible (bool)));
    connect (m_playlist, SIGNAL (closed ()), m_plButton, SLOT (click()));
}

bool MainDisplay::isPlaylistVisible() const
{
    return m_plButton->isChecked();
}

bool MainDisplay::isEqualizerVisible() const
{
    return m_eqButton->isChecked();
}

void MainDisplay::updateVolume()
{
    if(sender() == m_volumeBar)
        m_text->setText(tr("Volume: %1%").arg(m_volumeBar->value()));
    if(sender() == m_balanceBar)
    {
        if(m_balanceBar->value() > 0)
            m_text->setText(tr("Balance: %1% right").arg(m_balanceBar->value()));
        else if(m_balanceBar->value() < 0)
            m_text->setText(tr("Balance: %1% left").arg(-m_balanceBar->value()));
        else
            m_text->setText(tr("Balance: center"));
    }
    m_mw->setVolume(m_volumeBar->value(), m_balanceBar->value());
}

void MainDisplay::wheelEvent (QWheelEvent *e)
{
    m_mw->setVolume(m_volumeBar->value()+e->delta()/10, m_balanceBar->value());
}

bool MainDisplay::isRepeatable() const
{
    return m_repeatButton->isChecked();
}

bool MainDisplay::isShuffle() const
{
    return m_shuffleButton->isChecked();
}

void MainDisplay::setIsRepeatable(bool yes)
{
    m_repeatButton->setON(yes);
}

void MainDisplay::setIsShuffle(bool yes)
{
    m_shuffleButton->setON(yes);
}

void MainDisplay::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton)
        m_mw->menu()->exec(e->globalPos());
    else if(e->button() == Qt::LeftButton && m_aboutWidget->underMouse())
        m_mw->about();
    PixmapWidget::mousePressEvent(e);
}

