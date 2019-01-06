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
#ifndef LYRICSWINDOW_H
#define LYRICSWINDOW_H

#include <QWidget>

#include "ui_lyricswindow.h"

class QNetworkAccessManager;
class QNetworkReply;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class LyricsWindow : public QWidget
{
Q_OBJECT
public:
    LyricsWindow(const QString &artist, const QString &title, QWidget *parent = nullptr);

    ~LyricsWindow();

private slots:
    void showText(QNetworkReply *reply);
    void on_searchPushButton_clicked();
    QString cacheFilePath() const;
    bool loadFromCache();
    void saveToCache(const QString &text);

private:
    Ui::LyricsWindow m_ui;
    QNetworkAccessManager *m_http;
    QNetworkReply *m_requestReply;
    QString m_artist, m_title;
    QString m_cachePath;
};

#endif
