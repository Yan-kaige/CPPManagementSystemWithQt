#pragma once
#include <QDialog>
#include <QTableWidget>
#include "Common.h"
#include <vector>

class DocListDialog : public QDialog {
    Q_OBJECT
public:
    DocListDialog(const std::vector<Document>& docs, QWidget* parent = nullptr);
    void refreshDocs(const std::vector<Document>& docs);
private slots:
    void onDeleteDocClicked();
    void onAddDocClicked();
    void onSearchDocClicked();
    void onDownloadDocClicked();
    void onEditDocClicked();
private:
    QTableWidget* tableWidget;
    std::vector<Document> currentDocs;
    void setupTable(const std::vector<Document>& docs);
}; 