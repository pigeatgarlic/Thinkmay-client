#include "remoteui.h"
#include "ui_remoteui.h"

RemoteUI::RemoteUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::RemoteUI)
{
    ui->setupUi(this);
}

RemoteUI::~RemoteUI()
{
    delete ui;
}

