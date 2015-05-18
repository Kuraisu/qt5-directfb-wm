/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDIRECTFBWMFRAME_H
#define QDIRECTFBWMFRAME_H

//#include <qpa/qplatformwindow.h>

//#include "qdirectfbconvenience.h"
//#include "qdirectfbinput.h"

#include "qdirectfbwindow.h"
#include "qdirectfbblitter.h"

QT_BEGIN_NAMESPACE

class QDirectFbWMFrame : public QObject
{
public:
    QDirectFbWMFrame(QDirectFbWindow *win);
    ~QDirectFbWMFrame();

    void createWMFrame(QDirectFbInput *inputHandler);

    void handleMouseEvent(long timestamp, QPoint localPos, QPoint globalPos, Qt::MouseButtons buttons);

    WId winId() const;

    IDirectFBWindow *dfbWindow() const;

    // helper to get access to DirectFB types
    IDirectFBSurface *dfbSurface();

    void updateVisibility();
    void updatePosition();
    void updateGeometry();
    void updateSurface();
    void updateStacking();
    void renderFrame();
    void renderButtons();

protected:
    QDirectFBPointer<IDirectFBSurface> m_dfbSurface;
    QDirectFBPointer<IDirectFBWindow> m_dfbWindow;
    QDirectFbWindow *m_target;
    QDirectFbBlitter *m_blitter;
    QDirectFbInput *m_inputHandler;

    QRect m_geom;
    bool m_isMoving;
    QPoint m_dragStartPos;
    QPoint m_dragStartLocalPos;

    void renderTitle();
    void startMove(QPoint globalPos, QPoint localPos);
    void updateMove(QPoint globalPos);
    void endMove(QPoint globalPos);
};

QT_END_NAMESPACE

#endif // QDIRECTFBWMFRAME_H
