/********************************************************************************
** Form generated from reading UI file 'remoteui.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REMOTEUI_H
#define UI_REMOTEUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_RemoteUI
{
public:
    QWidget *centralwidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *RemoteUI)
    {
        if (RemoteUI->objectName().isEmpty())
            RemoteUI->setObjectName(QString::fromUtf8("RemoteUI"));
        RemoteUI->resize(800, 600);
        centralwidget = new QWidget(RemoteUI);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        RemoteUI->setCentralWidget(centralwidget);
        menubar = new QMenuBar(RemoteUI);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        RemoteUI->setMenuBar(menubar);
        statusbar = new QStatusBar(RemoteUI);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        RemoteUI->setStatusBar(statusbar);

        retranslateUi(RemoteUI);

        QMetaObject::connectSlotsByName(RemoteUI);
    } // setupUi

    void retranslateUi(QMainWindow *RemoteUI)
    {
        RemoteUI->setWindowTitle(QApplication::translate("RemoteUI", "RemoteUI", nullptr));
    } // retranslateUi

};

namespace Ui {
    class RemoteUI: public Ui_RemoteUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REMOTEUI_H
