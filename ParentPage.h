#ifndef PARENTPAGE_H
#define PARENTPAGE_H

#include <QWidget>
#include "OcctQtViewer.h"

namespace Ui {
class ParentPage;
}

class ParentPage : public QWidget
{
    Q_OBJECT

public:
    explicit ParentPage(QWidget *parent = nullptr);
    ~ParentPage();
    OcctQtViewer *myViewer;


private:
    Ui::ParentPage *ui;

public slots:
    void onCloseOcctWidget();
    void onShowOcctWidget();
    void onDeleteOcctWidget();
    void onCloseParentPage();
};

#endif // PARENTPAGE_H
