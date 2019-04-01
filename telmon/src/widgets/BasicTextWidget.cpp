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


#include "BasicTextWidget.h"

BasicTextWidget::BasicTextWidget(QWidget* a_parent,
                                 QWidget* a_mainWindow,
                                 std::function<QSize()> a_sizeHintFunction):
    QTextEdit(a_parent),
    m_mainWindow(a_mainWindow),
    m_sizeHintFunction(a_sizeHintFunction)
{
    // couldnt assign it in constructor
    if (nullptr == m_sizeHintFunction)
    {
       // set default behavior
       m_sizeHintFunction = [this]() {return QTextEdit::sizeHint();};
    }

    setReadOnly(true);
    viewport()->setCursor(Qt::ArrowCursor);
    this->setContextMenuPolicy(Qt::NoContextMenu); // set custom menu policy
}


BasicTextWidget::~BasicTextWidget()
{

}


QSize BasicTextWidget::sizeHint() const
{
    return m_sizeHintFunction();
//    if (!m_mainWindow) { return QTextEdit::sizeHint(); }

//    const auto l_size = static_cast<QWidget*>(this->parent())->size();

//    // after playing with it, for below values the tree widget looks good on strartup
//    return  QSize(l_size.width() * 60 / 100, l_size.height() * 90 / 100);
}


void BasicTextWidget::setSizeHintFunction(std::function<QSize()> a_func) noexcept
{
    m_sizeHintFunction = a_func;
}




