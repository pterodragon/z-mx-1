Regarding Memory Leaks

command used:
valgrind --leak-check=full --show-leak-kinds=all -v ./TelMon

If results are close to the first scenario (below) 
we can conclude there are no memory leaks


Tested 3 scenarios, files are attached in the end:
# # # 1 # # #
main.cpp - Begin
#include "MainWindowController.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindowController w;
    w.show();

    return a.exec();
}
main.cpp - End

valgrind result
==24194== LEAK SUMMARY:
==24194==    definitely lost: 464 bytes in 2 blocks
==24194==    indirectly lost: 13,045 bytes in 77 blocks
==24194==      possibly lost: 3,416 bytes in 32 blocks
==24194==    still reachable: 1,443,568 bytes in 16,479 blocks
==24194==                       of which reachable via heuristic:
==24194==                         length64           : 4,832 bytes in 80 blocks
==24194==                         newarray           : 2,096 bytes in 51 blocks
==24194==         suppressed: 0 bytes in 0 blocks
==24194== 
==24194== ERROR SUMMARY: 32 errors from 32 contexts (suppressed: 0 from 0)
==24194== ERROR SUMMARY: 32 errors from 32 contexts (suppressed: 0 from 0)

# # # 2 # # #
main.cpp - Begin
#include "MainWindowController.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindowController w;
//    w.show();

    return a.exec();
}
main.cpp - End

valgrind result
==24246== LEAK SUMMARY:
==24246==    definitely lost: 0 bytes in 0 blocks
==24246==    indirectly lost: 0 bytes in 0 blocks
==24246==      possibly lost: 4,344 bytes in 36 blocks
==24246==    still reachable: 1,712,927 bytes in 18,852 blocks
==24246==                       of which reachable via heuristic:
==24246==                         length64           : 4,832 bytes in 80 blocks
==24246==                         newarray           : 2,096 bytes in 51 blocks
==24246==         suppressed: 0 bytes in 0 blocks
==24246== 
==24246== ERROR SUMMARY: 34 errors from 34 contexts (suppressed: 0 from 0)
==24246== ERROR SUMMARY: 34 errors from 34 contexts (suppressed: 0 from 0)

# # # 3 # # # 
#include "MainWindowController.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindowController w;
//    w.show();

//    return a.exec();
    return 0;
}

valgrind result
i==24302== LEAK SUMMARY:
==24302==    definitely lost: 0 bytes in 0 blocks
==24302==    indirectly lost: 0 bytes in 0 blocks
==24302==      possibly lost: 3,416 bytes in 32 blocks
==24302==    still reachable: 1,430,313 bytes in 16,441 blocks
==24302==                       of which reachable via heuristic:
==24302==                         length64           : 4,832 bytes in 80 blocks
==24302==                         newarray           : 2,096 bytes in 51 blocks
==24302==         suppressed: 0 bytes in 0 blocks
==24302== 
==24302== ERROR SUMMARY: 30 errors from 30 contexts (suppressed: 0 from 0)
==24302== ERROR SUMMARY: 30 errors from 30 contexts (suppressed: 0 from 0)


# # # # # # # # # # # # # # # # # # # # # # # #
Files are:
 
MainWindowController.h - Begin
#ifndef MAINWINDOWCONTROLLER_H
#define MAINWINDOWCONTROLLER_H

#include <QMainWindow>

class MainWindowController : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindowController(QWidget *parent = nullptr);

signals:

public slots:
};

#endif // MAINWINDOWCONTROLLER_H
MainWindowController.h - End


MainWindowController.cpp - Begin
#include "MainWindowController.h"

MainWindowController::MainWindowController(QWidget *parent) : QMainWindow(parent)
{

}
MainWindowController.cpp - End


TelMon.pro - Begin
#-------------------------------------------------
#
# Project created by QtCreator 2019-01-29T14:55:08
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TelMon
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



HEADERS += \
    MainWindowController.h

SOURCES += \
    MainWindowController.cpp \
    main.cpp
TelMon.pro - End

