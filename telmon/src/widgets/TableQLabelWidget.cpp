//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "TableQLabelWidget.h"
#include "QMenu"
#include "QFontDatabase"

TableQLabelWidget::TableQLabelWidget(QWidget* a_parent):
    QLabel(a_parent),
    m_contextMenu(new QMenu(this))
{
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setObjectName(QString("TableQLabelWidget"));
    setTextFormat(Qt::RichText);

    // set crossplatform monospaced font for nice spaces:
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixedFont.setPixelSize(11);
    setFont(fixedFont);

    setMinimumSize(400, 300);

    createActions();
    initContextMenu();
}


TableQLabelWidget::~TableQLabelWidget()
{
    delete m_contextMenu;
    m_contextMenu = nullptr;
}


void TableQLabelWidget::createActions() noexcept
{
    // contextMenu view/display functionality
    this->setContextMenuPolicy(Qt::CustomContextMenu); // set custom menu policy
    QObject::connect(this, &QLabel::customContextMenuRequested, this, [this](const QPoint a_pos)
    {
        this->m_contextMenu->exec(mapToGlobal(a_pos));
    } );
}


void TableQLabelWidget::initContextMenu() noexcept
{
    m_contextMenu->addAction("Close", [this]() {
        emit closeAction();
    });
}

void TableQLabelWidget::setText(const QString& a_text)
{
    QLabel::setText(a_text);
//    QFontMetrics fm(QFontDatabase::systemFont(QFontDatabase::FixedFont));
//    int width=fm.width(a_text.split("&nbsp;")[0]);
//    qDebug() << "a_text.split()" << a_text.split("&nbsp;")[0]
//             << "width:" << width;
//    setMinimumSize(300, 300);
}





