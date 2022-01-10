#include "MainWindow.h"
#include <QVBoxLayout>
#include "./ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 3D Viewer and some controls on top of it
    parent_page_ = new ParentPage();
    QVBoxLayout* aLayout = new QVBoxLayout (parent_page_);
    aLayout->setDirection (QBoxLayout::BottomToTop);
    aLayout->setAlignment (Qt::AlignBottom);

    setCentralWidget (parent_page_);

}

MainWindow::~MainWindow()
{
    delete ui;
}



