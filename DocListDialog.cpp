#include "DocListDialog.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include "Common.h"
#include "CLIHandler.h"
#include <QFileDialog>
#include <QLineEdit>
#include <QHBoxLayout>

extern CLIHandler* g_cliHandler;

DocListDialog::DocListDialog(const std::vector<Document>& docs, QWidget* parent)
    : QDialog(parent), currentDocs(docs)
{
    setWindowTitle("我的文档列表");
    resize(800, 400);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QPushButton* btnAddDoc = new QPushButton("上传文档", this);
    layout->addWidget(btnAddDoc);
    connect(btnAddDoc, &QPushButton::clicked, this, &DocListDialog::onAddDocClicked);
    // 搜索区域
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLineEdit* editSearchDoc = new QLineEdit(this);
    editSearchDoc->setPlaceholderText("输入文档标题或描述关键词");
    QPushButton* btnSearchDoc = new QPushButton("搜索", this);
    searchLayout->addWidget(editSearchDoc);
    searchLayout->addWidget(btnSearchDoc);
    layout->addLayout(searchLayout);
    connect(btnSearchDoc, &QPushButton::clicked, this, &DocListDialog::onSearchDocClicked);
    tableWidget = new QTableWidget(this);
    layout->addWidget(tableWidget);
    setupTable(docs);
    // 保存指针用于槽函数
    editSearchDoc->setObjectName("editSearchDoc");
}

void DocListDialog::refreshDocs(const std::vector<Document>& docs)
{
    currentDocs = docs;
    setupTable(docs);
}

void DocListDialog::setupTable(const std::vector<Document>& docs)
{
    tableWidget->clear();
    tableWidget->setColumnCount(5);
    QStringList headers;
    headers << "ID" << "标题" << "大小" << "创建时间" << "操作";
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->setRowCount(docs.size());
    for (int i = 0; i < docs.size(); ++i) {
        const Document& doc = docs[i];
        tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(doc.id)));
        tableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(doc.title)));
        tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(doc.file_size) + " bytes"));
        tableWidget->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(Utils::formatTimestamp(doc.created_at))));
        QPushButton* btnDel = new QPushButton("删除");
        btnDel->setProperty("docId", doc.id);
        connect(btnDel, &QPushButton::clicked, this, &DocListDialog::onDeleteDocClicked);
        tableWidget->setCellWidget(i, 4, btnDel);
    }
    tableWidget->resizeColumnsToContents();
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void DocListDialog::onDeleteDocClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int docId = btn->property("docId").toInt();
    if (QMessageBox::question(this, "确认删除", QString("确定要删除文档ID %1 吗？").arg(docId)) != QMessageBox::Yes) {
        return;
    }
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    std::vector<std::string> args = {"deletedoc", std::to_string(docId)};
    bool ok = g_cliHandler->handleDeleteDocument(args);
    if (ok) {
        QMessageBox::information(this, "删除文档", "删除成功！");
        // 刷新文档列表
        auto userResult = g_cliHandler->getCurrentUserForUI();
        if (userResult.first) {
            int userId = userResult.second.id;
            std::vector<Document> docs = g_cliHandler->getUserDocsForUI(userId);
            refreshDocs(docs);
        }
    } else {
        QMessageBox::warning(this, "删除文档", "删除失败，文档不存在或无权限");
    }
}

void DocListDialog::onAddDocClicked()
{
    QDialog dlg(this);
    dlg.setWindowTitle("上传文档");
    QVBoxLayout* vlayout = new QVBoxLayout(&dlg);
    QLineEdit* editTitle = new QLineEdit(&dlg);
    editTitle->setPlaceholderText("文档标题");
    QLineEdit* editDesc = new QLineEdit(&dlg);
    editDesc->setPlaceholderText("文档描述");
    QLineEdit* editFile = new QLineEdit(&dlg);
    editFile->setPlaceholderText("选择本地文件");
    QPushButton* btnChoose = new QPushButton("选择文件", &dlg);
    QPushButton* btnUpload = new QPushButton("上传", &dlg);
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(editFile);
    fileLayout->addWidget(btnChoose);
    vlayout->addWidget(editTitle);
    vlayout->addWidget(editDesc);
    vlayout->addLayout(fileLayout);
    vlayout->addWidget(btnUpload);
    connect(btnChoose, &QPushButton::clicked, [&]() {
        QString file = QFileDialog::getOpenFileName(&dlg, "选择文件");
        if (!file.isEmpty()) editFile->setText(file);
    });
    connect(btnUpload, &QPushButton::clicked, [&]() {
        QString title = editTitle->text().trimmed();
        QString desc = editDesc->text().trimmed();
        QString file = editFile->text().trimmed();
        if (title.isEmpty() || file.isEmpty()) {
            QMessageBox::warning(&dlg, "提示", "标题和文件不能为空！");
            return;
        }
        std::vector<std::string> args = { "adddoc", title.toStdString(), desc.toStdString(), file.toStdString() };
        if (g_cliHandler->handleAddDocument(args)) {
            QMessageBox::information(&dlg, "成功", "上传成功！");
            dlg.accept();
            // 刷新文档列表
            auto userResult = g_cliHandler->getCurrentUserForUI();
            if (userResult.first) {
                int userId = userResult.second.id;
                std::vector<Document> docs = g_cliHandler->getUserDocsForUI(userId);
                refreshDocs(docs);
            }
        } else {
            QMessageBox::warning(&dlg, "失败", "上传失败，请检查文件路径或权限！");
        }
    });
    dlg.exec();
}

void DocListDialog::onSearchDocClicked()
{
    QLineEdit* editSearchDoc = findChild<QLineEdit*>("editSearchDoc");
    if (!editSearchDoc) return;
    QString keyword = editSearchDoc->text().trimmed();
    auto userResult = g_cliHandler->getCurrentUserForUI();
    if (!userResult.first) return;
    int userId = userResult.second.id;
    std::vector<Document> docs;
    if (keyword.isEmpty()) {
        docs = g_cliHandler->getUserDocsForUI(userId);
    } else {
        docs = g_cliHandler->getSearchedDocsForUI(userId, keyword.toStdString());
    }
    refreshDocs(docs);
} 
