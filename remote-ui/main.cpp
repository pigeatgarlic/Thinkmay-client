#include "remoteui.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RemoteUI w;
    w.show();
    return a.exec();
}
