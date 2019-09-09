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



#include "StatusBarWidget.h"
#include "QLabel"


StatusBarWidget::StatusBarWidget(QWidget *a_parent):
    QStatusBar(a_parent),
    m_normalMsgWidget(new QLabel(this)),
    m_permanentMsgWidget(new QLabel(this))
{
    clearNormalMsg();
    addWidget(m_normalMsgWidget);

    clearPermanentMsg();
    addPermanentWidget(m_permanentMsgWidget);
}


StatusBarWidget::~StatusBarWidget()
{

}


void StatusBarWidget::setTemporaryMsg(const QString& a_msg, int a_timeout) noexcept
{
    showMessage(tr(qPrintable(a_msg)), a_timeout);
}


void StatusBarWidget::clearTemporaryMsg() noexcept
{
    clearMessage();
}


void StatusBarWidget::setNormalMsg(const QString& a_msg) noexcept
{
    m_normalMsgWidget->setText(a_msg);
}


void StatusBarWidget::clearNormalMsg() noexcept
{
    m_normalMsgWidget->setText(QString());
}



void StatusBarWidget::setPermanentMsg(const QString& a_msg) noexcept
{
    m_permanentMsgWidget->setText(a_msg);
}


void StatusBarWidget::clearPermanentMsg() noexcept
{
    m_permanentMsgWidget->setText(QString());
}


















