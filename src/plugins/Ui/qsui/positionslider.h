/***************************************************************************
 *   Copyright (C) 2011-2016 by Ilya Kotov                                 *
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

#ifndef POSITIONSLIDER_H
#define POSITIONSLIDER_H

#include <QSlider>

class QWheelEvent;
class QMouseEvent;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class PositionSlider : public QSlider
{
    Q_OBJECT
public:
    explicit PositionSlider(QWidget *parent = nullptr);
    virtual ~PositionSlider(){}

protected:
    virtual void mousePressEvent (QMouseEvent *event) override;
    virtual void mouseReleaseEvent (QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private slots:
    void onSliderMoved(int pos);

};

#endif // POSITIONSLIDER_H
