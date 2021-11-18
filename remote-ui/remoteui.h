#ifndef REMOTEUI_H
#define REMOTEUI_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class RemoteUI; }
QT_END_NAMESPACE

class RemoteUI : public QMainWindow
{
    Q_OBJECT

public:
    RemoteUI(QWidget *parent = nullptr);
    ~RemoteUI();

private:
    Ui::RemoteUI *ui;
};
#endif // REMOTEUI_H
