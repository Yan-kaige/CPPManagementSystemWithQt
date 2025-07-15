#pragma once
#include <QDialog>
#include <QTableWidget>
#include "Common.h"
#include <vector>

class DocListDialog : public QDialog {
    Q_OBJECT
public:
    DocListDialog(const std::vector<Document>& docs, QWidget* parent = nullptr);
private:
    QTableWidget* tableWidget;
    void setupTable(const std::vector<Document>& docs);
}; 