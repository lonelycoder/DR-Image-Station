#include "exitwidget.h"
#include "ui_exitwidget.h"

#include "mainwindow.h"

#include "Windows.h"

ExitWidget::ExitWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExitWidget)
{
    ui->setupUi(this);

    connect(ui->lockStationButton, SIGNAL(clicked()), this, SLOT(onLockStation()));
    connect(ui->exitStationButton, SIGNAL(clicked()), this, SLOT(onExitStation()));
    connect(ui->shutDownButton, SIGNAL(clicked()), this, SLOT(onShutDown()));
    connect(ui->rebootButton, SIGNAL(clicked()), this, SLOT(onReboot()));
}

ExitWidget::~ExitWidget()
{
    delete ui;
}

void ExitWidget::onLockStation()
{
    while (!mainWindow->lockStation());
}

void ExitWidget::onExitStation()
{
    const_cast<MainWindow*>(mainWindow)->close();
}

void ExitWidget::onShutDown()
{
    if (mainWindow->savelyClose()) {
        // 提升进程权限
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;
        // 打开当前进程令牌
        OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

        // 查询SE_SHUTDOWN_NAME权限ID
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        // 提升至SE_SHUTDOWN_NAME权限
        AdjustTokenPrivileges(hToken, false, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

        ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
    }
}

void ExitWidget::onReboot()
{
    if (mainWindow->savelyClose()) {
        // 提升进程权限
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;
        // 打开当前进程令牌
        OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

        // 查询SE_SHUTDOWN_NAME权限ID
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        // 提升至SE_SHUTDOWN_NAME权限
        AdjustTokenPrivileges(hToken, false, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

        ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
    }
}
