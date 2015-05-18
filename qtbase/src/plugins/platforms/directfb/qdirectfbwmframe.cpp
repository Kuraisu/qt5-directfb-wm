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

#include "qdirectfbwmframe.h"
//#include "qdirectfbwindow.h"
//#include "qdirectfbbackingstore.h"
//#include "qdirectfbinput.h"
#include "qdirectfbscreen.h"

#include <qpa/qwindowsysteminterface.h>

#include <directfb.h>

#define WD_TITLE_HEIGHT 24
#define WD_BORDER_SIZE 4

#define WD_BUTTON_WIDTH 32
#define WD_BUTTON_HEIGHT 16

QT_BEGIN_NAMESPACE

QDirectFbWMFrame::QDirectFbWMFrame(QDirectFbWindow *win)
    : m_target(win),
    m_isMoving(false)
{
}

void QDirectFbWMFrame::createWMFrame(QDirectFbInput *inputHandler)
{
    Q_ASSERT(!m_dfbWindow.data());

    m_inputHandler = inputHandler;

    DFBDisplayLayerConfig layerConfig;
    IDirectFBDisplayLayer *layer;

    layer = toDfbScreen(m_target->window())->dfbLayer();
    layer->GetConfiguration(layer, &layerConfig);

    m_geom.setRect(m_target->window()->x() - WD_BORDER_SIZE,
                   m_target->window()->y() - WD_TITLE_HEIGHT,
                   qMax(1, m_target->window()->width()) + WD_BORDER_SIZE * 2,
                   qMax(1, m_target->window()->height()) + WD_BORDER_SIZE + WD_TITLE_HEIGHT);

    DFBWindowDescription description;
    memset(&description, 0, sizeof(DFBWindowDescription));
    description.flags = DFBWindowDescriptionFlags(DWDESC_WIDTH|DWDESC_HEIGHT|DWDESC_POSX|DWDESC_POSY|DWDESC_SURFACE_CAPS
                                                  |DWDESC_OPTIONS
                                                  |DWDESC_CAPS);
    description.width = m_geom.width();
    description.height = m_geom.height();
    description.posx = m_geom.x();
    description.posy = m_geom.y();

    if (layerConfig.surface_caps & DSCAPS_PREMULTIPLIED)
        description.surface_caps = DSCAPS_PREMULTIPLIED;
    description.pixelformat = layerConfig.pixelformat;

    description.parent_id = m_target->winId();

    description.options = DFBWindowOptions(DWOP_ALPHACHANNEL|DWOP_OPAQUE_REGION|DWOP_SHAPED|DWOP_KEEP_STACKING|DWOP_KEEP_UNDER);
    description.caps = DFBWindowCapabilities(DWCAPS_DOUBLEBUFFER|DWCAPS_ALPHACHANNEL);

    DFBResult result = layer->CreateWindow(layer, &description, m_dfbWindow.outPtr());
    if (result != DFB_OK)
        DirectFBError("QDirectFbWMFrame: failed to create wmframe window", result);

    m_dfbWindow->DisableEvents(m_dfbWindow.data(), (DFBWindowEventType)(DWET_GOTFOCUS | DWET_LOSTFOCUS));

    m_dfbWindow->SetOpacity(m_dfbWindow.data(), 0xff);
    m_inputHandler->addWMFrame(m_dfbWindow.data(), this);

    m_dfbWindow->PutBelow(m_dfbWindow.data(), m_target->dfbWindow());

    /*
    IDirectFBSurface *surface;
    result = m_dfbWindow->GetSurface(m_dfbWindow.data(), &surface);
    if (result != DFB_OK)
        DirectFBError("QDirectFbWMFrame: failed to get surface from window", result);
    */
    QSize size(description.width, description.height);
    m_blitter = new QDirectFbBlitter(size, dfbSurface());

    updateVisibility();
    updateGeometry();
}

QDirectFbWMFrame::~QDirectFbWMFrame()
{
    m_inputHandler->removeWMFrame(m_dfbWindow.data());
    m_dfbWindow->Destroy(m_dfbWindow.data());
}

void QDirectFbWMFrame::handleMouseEvent(long timestamp, QPoint localPos, QPoint globalPos, Qt::MouseButtons buttons)
{
    if (! m_isMoving && ((buttons & Qt::LeftButton))) {
        QRect rect(m_geom.width() - WD_BUTTON_WIDTH - 8, 0, WD_BUTTON_WIDTH, WD_BUTTON_HEIGHT); // Close button rect

        if (rect.contains(localPos)) {
            m_target->window()->close();
        }
        else {
            startMove(globalPos, localPos);
        }

        //startMove(globalPos, localPos);
    }
    else if (m_isMoving && ((buttons & Qt::LeftButton))) {
        updateMove(globalPos);
    }
    else if (m_isMoving) {
        endMove(globalPos);
    }
}

void QDirectFbWMFrame::startMove(QPoint globalPos, QPoint localPos)
{
    m_isMoving = true;
    m_dragStartPos = globalPos;
    m_dragStartLocalPos = localPos;

    m_dfbWindow->GrabPointer(m_dfbWindow.data());
}

void QDirectFbWMFrame::endMove(QPoint globalPos)
{
    Q_UNUSED(globalPos);
    m_isMoving = false;

    m_dfbWindow->UngrabPointer(m_dfbWindow.data());

    QRect new_geom(m_geom.topLeft() + QPoint(WD_BORDER_SIZE, WD_TITLE_HEIGHT), m_target->window()->size());
    m_target->setGeometry(new_geom);
}

void QDirectFbWMFrame::updateMove(QPoint globalPos)
{
    QPoint frame_new_pos;
    QPoint window_new_pos;

    frame_new_pos = globalPos - m_dragStartLocalPos;
    window_new_pos = frame_new_pos + QPoint(WD_BORDER_SIZE, WD_TITLE_HEIGHT);

    QRect new_geom(window_new_pos, m_target->window()->size());
    //m_target->setGeometry(new_geom);

    m_geom.moveTo(frame_new_pos.x(), frame_new_pos.y());

    m_dfbWindow->MoveTo(m_dfbWindow.data(), frame_new_pos.x(), frame_new_pos.y());
    m_target->dfbWindow()->MoveTo(m_target->dfbWindow(), window_new_pos.x(), window_new_pos.y());
    QWindowSystemInterface::handleGeometryChange(m_target->window(), new_geom);

    updateStacking();
}

WId QDirectFbWMFrame::winId() const
{
    DFBWindowID id;
    m_dfbWindow->GetID(m_dfbWindow.data(), &id);
    return WId(id);
}

IDirectFBWindow *QDirectFbWMFrame::dfbWindow() const
{
    return m_dfbWindow.data();
}

IDirectFBSurface *QDirectFbWMFrame::dfbSurface()
{
    if (!m_dfbSurface) {
        DFBResult res = m_dfbWindow->GetSurface(m_dfbWindow.data(), m_dfbSurface.outPtr());
        if (res != DFB_OK)
            DirectFBError(QDFB_PRETTY, res);
    }

    return m_dfbSurface.data();
}

void QDirectFbWMFrame::renderTitle()
{
    QFont font;
    font.setPointSize(14);
    font.setWeight(QFont::Bold);
    QFontMetrics font_m(font);

    QString title = m_target->window()->title();

    if (title.isEmpty())
        return;

    qreal title_width = font_m.width(title);
    qreal font_height = font_m.height();
    qreal font_ascent = font_m.ascent();

    QPixmap pixmap(title_width, font_height + 1);
    QPainter painter(&pixmap);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(QPen(QColor("#554444")));
    painter.setBrush(QBrush(QColor("#554444")));
    painter.setFont(font);

    painter.drawText(QPoint(0, font_ascent), title);

    painter.setPen(QPen(QColor("#ffcbcb")));
    painter.setBrush(QBrush(QColor("#ffcbcb")));
    painter.setFont(font);

    painter.drawText(QPoint(0, font_ascent + 1), title);

    int posx = m_geom.width() / 2 - title_width / 2;
    int posy = WD_TITLE_HEIGHT / 2 - font_height / 2;
    QRectF rect(posx, posy, title_width, font_height + 1);
    QRectF sub_rect(0, 0, title_width, font_height + 1);

    m_blitter->drawPixmap(rect, pixmap, sub_rect);
}

void QDirectFbWMFrame::renderButtons()
{
    QPixmap pixmap(WD_BUTTON_WIDTH + 2, WD_BUTTON_HEIGHT + 1);
    QPainter painter(&pixmap);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor("#554444"), 2));
    //painter.setBrush(QBrush(QColor("#554444")));
    painter.drawRoundedRect(1, -4, WD_BUTTON_WIDTH, WD_BUTTON_HEIGHT + 4, 4, 4);

    static const int mid_x = (WD_BUTTON_WIDTH + 2) / 2;
    static const int mid_y = WD_BUTTON_HEIGHT / 2;
    static const QPoint points[12] = {
        QPoint(mid_x - 6, mid_y - 5),
        QPoint(mid_x - 3, mid_y - 5),
        QPoint(mid_x, mid_y - 2),
        QPoint(mid_x + 3, mid_y - 5),
        QPoint(mid_x + 6, mid_y - 5),
        QPoint(mid_x + 2, mid_y),
        QPoint(mid_x + 6, mid_y + 5),
        QPoint(mid_x + 3, mid_y + 5),
        QPoint(mid_x, mid_y + 2),
        QPoint(mid_x - 3, mid_y + 5),
        QPoint(mid_x - 6, mid_y + 5),
        QPoint(mid_x - 2, mid_y)
    };

    QPointF real_points[12];

    for(int i = 0; i < 12; i++) {
        real_points[i] = QPointF(points[i].x() - 0.5f, points[i].y() + 0.5f);
    }

    painter.setPen(QPen(QColor("#554444"), 1));
    painter.setBrush(QBrush(QColor("#554444")));
    painter.drawPolygon(real_points, 12);

    int posx = m_geom.width() - WD_BUTTON_WIDTH - 1 - 8;
    int posy = 0;
    QRectF rect(posx, posy, WD_BUTTON_WIDTH + 2, WD_BUTTON_HEIGHT + 1);
    QRectF sub_rect(0, 0, WD_BUTTON_WIDTH + 2, WD_BUTTON_HEIGHT + 1);

    m_blitter->drawPixmap(rect, pixmap, sub_rect);
}

void QDirectFbWMFrame::updateVisibility()
{
    if (! m_target->window()->isVisible()) {
        m_dfbWindow->SetOpacity(m_dfbWindow.data(), 0);
    } else {
        m_dfbWindow->SetOpacity(m_dfbWindow.data(), 0xff);
    }
}

void QDirectFbWMFrame::updatePosition()
{
    m_geom.moveTo(m_target->window()->x() - WD_BORDER_SIZE,
                  m_target->window()->y() - WD_TITLE_HEIGHT);

    m_dfbWindow->MoveTo(m_dfbWindow.data(), m_geom.x(), m_geom.y());
}

void QDirectFbWMFrame::updateGeometry()
{
    m_geom.setRect(m_target->window()->x() - WD_BORDER_SIZE,
                   m_target->window()->y() - WD_TITLE_HEIGHT,
                   qMax(1, m_target->window()->width()) + WD_BORDER_SIZE * 2,
                   qMax(1, m_target->window()->height()) + WD_BORDER_SIZE + WD_TITLE_HEIGHT);

    m_dfbWindow->SetBounds(m_dfbWindow.data(), m_geom.x(), m_geom.y(), m_geom.width(), m_geom.height());

    renderFrame();
}


void QDirectFbWMFrame::updateStacking()
{
    m_dfbWindow->PutBelow(m_dfbWindow.data(), m_target->dfbWindow());
}

void QDirectFbWMFrame::renderFrame()
{
    QSize size(m_geom.width(), m_geom.height());

    m_blitter->fillRect(QRectF(QPoint(), size), QColor("#8f7272"));
    m_blitter->fillRect(QRectF(0, 0, m_geom.width(), 1), QColor("#554444"));
    m_blitter->fillRect(QRectF(0, m_geom.height() - 1, m_geom.width(), 1), QColor("#554444"));
    m_blitter->fillRect(QRectF(0, 0, 1, m_geom.height()), QColor("#554444"));
    m_blitter->fillRect(QRectF(m_geom.width() - 1, 0, 1, m_geom.height()), QColor("#554444"));
    renderTitle();
    renderButtons();
    m_dfbSurface->Flip(m_dfbSurface.data(), NULL, DSFLIP_NONE);
}

QT_END_NAMESPACE
