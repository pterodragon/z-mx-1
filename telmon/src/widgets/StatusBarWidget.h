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


#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include <QStatusBar>
class QLabel;

/**
 * @brief The StatusBarWidget class
 * encapsualtes https://doc.qt.io/qt-5/qstatusbar.html#details
 * main functions
 */
class StatusBarWidget : public QStatusBar
{
public:
    StatusBarWidget(QWidget *a_parent);
    virtual ~StatusBarWidget() override final;


    /**
     * @brief briefly occupies most of the status bar. Used to explain tool tip texts or menu entries, for example.
     * @param a_msg
     * @param a_timeout
     */
    void setTemporaryMsg(const QString& a_msg, int a_timeout = 0) noexcept;
    void clearTemporaryMsg() noexcept;


    /**
     * @brief  occupies part of the status bar and may be hidden by temporary messages.
     *  Used to display the page and line number in a word processor, for example.
     * @param a_msg
     */
    void setNormalMsg(const QString& a_msg) noexcept;
    void clearNormalMsg() noexcept;

    /**
     * @brief is never hidden. Used for important mode indications, for example,
     * some applications put a Caps Lock indicator in the status bar.
     * @param a_msg
     */
    void setPermanentMsg(const QString& a_msg) noexcept;
    void clearPermanentMsg() noexcept;


protected:
    QLabel* m_normalMsgWidget;
    QLabel* m_permanentMsgWidget;
};

#endif // STATUSBARWIDGET_H
