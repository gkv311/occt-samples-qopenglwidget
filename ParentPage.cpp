#include "ParentPage.h"
#include "ui_ParentPage.h"

#include <QVBoxLayout>

ParentPage::ParentPage(QWidget *parent) :
    QWidget(parent)
  , ui(new Ui::ParentPage)
  , myViewer(nullptr)
{
    ui->setupUi(this);
    myViewer = new OcctQtViewer(this);
    ui->verticalLayout->layout()->addWidget(myViewer);
}

ParentPage::~ParentPage()
{
    delete ui;
    std::cout<<"[Parent Page] destructor --> delete  ui"<<std::endl;
}

void ParentPage::onCloseOcctWidget()
{
    this->myViewer->close();
    std::cout<<"[Parent Page] onCloseOcctWidget"<<std::endl;
}

void ParentPage::onShowOcctWidget()
{
    this->myViewer->show();
    std::cout<<"[Parent Page] onShowOcctWidget"<<std::endl;
}

void ParentPage::onCloseParentPage()
{
    this->close();
    std::cout<<"[Parent Page] onCloseParentPage"<<std::endl;
}

void ParentPage::onDeleteOcctWidget()
{
    delete this->myViewer;
    this->myViewer = nullptr;
    std::cout<<"[Parent Page] onDeleteOcctWidget"<<std::endl;
}
