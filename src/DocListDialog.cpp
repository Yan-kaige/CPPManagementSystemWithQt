#include "DocListDialog.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include "Common.h"


DocListDialog::DocListDialog(const std::vector<Document>& docs, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("我的文档列表");
    resize(700, 400);
    QVBoxLayout* layout = new QVBoxLayout(this);
    tableWidget = new QTableWidget(this);
    layout->addWidget(tableWidget);
    setupTable(docs);
}

void DocListDialog::setupTable(const std::vector<Document>& docs)
{
    tableWidget->setColumnCount(4);
    QStringList headers;
    headers << "ID" << "标题" << "大小" << "创建时间";
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->setRowCount(docs.size());
    for (int i = 0; i < docs.size(); ++i) {
        const Document& doc = docs[i];
        tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(doc.id)));
        tableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(doc.title)));
        tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(doc.file_size) + " bytes"));
        tableWidget->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(Utils::formatTimestamp(doc.created_at))));
    }
    tableWidget->resizeColumnsToContents();
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
} 
