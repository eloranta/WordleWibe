#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QCoreApplication>
#include <QFile>
#include <QPlainTextEdit>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadStrings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadStrings()
{
    const QString filePath = QCoreApplication::applicationDirPath() + "/wordle_strings.txt";
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->stringsView->setPlainText(tr("Could not open:\n%1").arg(filePath));
        statusBar()->showMessage(tr("wordle_strings.txt not found in exe directory"));
        return;
    }

    ui->stringsView->setPlainText(QString::fromUtf8(file.readAll()));
    statusBar()->showMessage(tr("Loaded wordle_strings.txt from exe directory"));
}
