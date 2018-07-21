/***************************************************************************
 *   Copyright (C) 2011-2017 by Ilya Kotov                                 *
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

#include <QtPlugin>
#include <QLocale>
#include <qmmp/soundcore.h>
#include <qmmpui/playlistmanager.h>
#include <qmmpui/metadataformatter.h>
#include <qmmpui/mediaplayer.h>
#include <qmmpui/qmmpuisettings.h>
#include <qmmpui/commandlinemanager.h>
#include "playlistoption.h"

CommandLineProperties PlayListOption::properties() const
{
    CommandLineProperties properties;
    properties.shortName = "PlayListOption";
    properties.helpString << QString("--pl-help") + "||" + tr("Show playlist manipulation commands");
    return properties;
}

bool PlayListOption::identify(const QString & str) const
{
    return  str == QString("--pl-help") ||
            str == QString("--pl-list") ||
            str == QString("--pl-dump") ||
            str == QString("--pl-play") ||
            str == QString("--pl-clear") ||
            str == QString("--pl-repeat-toggle") ||
            str == QString("--pl-shuffle-toggle") ||
            str == QString("--pl-state");
}

QString PlayListOption::executeCommand(const QString& opt_str, const QStringList &args)
{
    Q_UNUSED(args);
    QString out;
    PlayListManager *pl_manager = PlayListManager::instance();
    MediaPlayer *player = MediaPlayer::instance();
    QmmpUiSettings *ui_settings = QmmpUiSettings::instance();

    if(opt_str == "--pl-help")
    {
        QStringList list = QStringList()
                << QString("--pl-list") + "||" + tr("List all available playlists")
                << QString("--pl-dump <id>") + "||" + tr("Show playlist content")
                << QString("--pl-play <id> <track>") + "||" + tr("Play track <track> in playlist <id>")
                << QString("--pl-clear <id>") + "||" + tr("Clear playlist")
                << QString("--pl-repeat-toggle") + "||" + tr("Toggle playlist repeat")
                << QString("--pl-shuffle-toggle") + "||" + tr("Toggle playlist shuffle")
                << QString("--pl-state") + "||" + tr("Show playlist options");

        foreach (QString line, list)
            out += CommandLineManager::formatHelpString(line) + "\n";
    }
    else if(opt_str == "--pl-list")
    {
        QStringList names = pl_manager->playListNames();
        for(int i = 0; i <  names.count(); ++i)
        {
            if(i == pl_manager->currentPlayListIndex())
                out += QString("%1. %2 [*]\n").arg(i+1).arg(names.at(i));
            else
                out += QString("%1. %2\n").arg(i+1).arg(names.at(i));
        }
    }
    else if(opt_str == "--pl-dump")
    {
        MetaDataFormatter formatter("%p%if(%p&%t, - ,)%t%if(%p,,%if(%t,,%f))%if(%l, - %l,)");
        int id = args.isEmpty() ? pl_manager->currentPlayListIndex() : args.at(0).toInt() - 1;
        PlayListModel *model = pl_manager->playListAt(id);
        if(!model)
            return tr("Invalid playlist ID") + "\n";
        for(int i = 0; i < model->count(); ++i)
        {
            PlayListTrack *track = model->track(i);
            if(!track)
                continue;
            out += QString("%1. %2").arg(model->indexOfTrack(i) + 1).arg(formatter.format(track));
            if(i == model->currentIndex())
                out += " [*]";
            out += "\n";
        }
    }
    else if(opt_str == "--pl-play")
    {
        if(args.count() > 2 || args.isEmpty())
            return tr("Invalid number of arguments") + "\n";

        int pl_id = (args.count() == 1) ? pl_manager->currentPlayListIndex() : args.at(0).toInt() - 1;
        int track_number = (args.count() == 1) ? args.at(0).toInt() - 1 : args.at(1).toInt() - 1;
        PlayListModel *model = pl_manager->playListAt(pl_id);
        if(!model)
            return tr("Invalid playlist ID") + "\n";

        PlayListTrack *track = model->findTrack(track_number);
        if(!track)
            return tr("Invalid track ID") + "\n";
        player->stop();
        pl_manager->activatePlayList(model);
        pl_manager->selectPlayList(model);
        model->setCurrent(track);
        player->play();
    }
    else if(opt_str == "--pl-clear")
    {
        int id = args.isEmpty() ? pl_manager->currentPlayListIndex() : args.at(0).toInt() - 1;
        PlayListModel *model = pl_manager->playListAt(id);
        if(!model)
            return tr("Invalid playlist ID") + "\n";
        model->clear();
    }
    else if(opt_str == "--pl-repeat-toggle")
    {
        ui_settings->setRepeatableList(!ui_settings->isRepeatableList());
    }
    else if(opt_str == "--pl-shuffle-toggle")
    {
        ui_settings->setShuffle(!ui_settings->isShuffle());
    }
    else if(opt_str == "--pl-state")
    {
        out += "SHUFFLE:             " + boolToText(ui_settings->isShuffle()) + "\n";
        out += "REPEAT PLAYLIST:     " + boolToText(ui_settings->isRepeatableList()) + "\n";
        out += "REPEAT TRACK:        " + boolToText(ui_settings->isRepeatableTrack()) + "\n";
        out += "NO PLAYLIST ADVANCE: " + boolToText(ui_settings->isNoPlayListAdvance()) + "\n";
    }
    return out;
}

QString PlayListOption::translation() const
{
    return QLatin1String(":/playlist_plugin_");
}

QString PlayListOption::boolToText(bool enabled)
{
    return QString(enabled ? "[+]" : "[-]");
}
