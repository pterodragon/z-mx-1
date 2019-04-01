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



#ifndef BASICTEXTWIDGET_H
#define BASICTEXTWIDGET_H

#include "QTextEdit"
class BasicTextWidget : public QTextEdit
{
public:
    BasicTextWidget(QWidget* a_parent = nullptr,
                    QWidget* a_mainWindow = nullptr,
                    std::function<QSize()> a_sizeHintFunction = nullptr);
    ~BasicTextWidget() override;

    virtual QSize sizeHint() const override;

    void setSizeHintFunction(std::function<QSize()>) noexcept;
    const QWidget* getMainWindow() const noexcept {return m_mainWindow;}


private:
    QWidget* m_mainWindow;
    std::function<QSize()> m_sizeHintFunction;

};

#endif // BASICTEXTWIDGET_H
