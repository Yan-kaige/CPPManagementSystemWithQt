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
#include <QMimeDatabase>
#include <QMimeType>
#include <QLabel> // Added for QLabel

extern CLIHandler* g_cliHandler;

DocListDialog::DocListDialog(const std::vector<Document>& docs, QWidget* parent)
    : QDialog(parent), currentDocs(docs)
{
    setWindowTitle("我的文档列表");
    resize(800, 400);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* topLayout = new QHBoxLayout();
    QPushButton* btnAddDoc = new QPushButton("上传文档", this);
    QPushButton* btnExportDocs = new QPushButton("导出文档", this);
    topLayout->addWidget(btnAddDoc);
    topLayout->addStretch();
    topLayout->addWidget(btnExportDocs);
    layout->addLayout(topLayout);
    connect(btnAddDoc, &QPushButton::clicked, this, &DocListDialog::onAddDocClicked);
    connect(btnExportDocs, &QPushButton::clicked, this, &DocListDialog::onExportDocsClicked);
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
    tableWidget->setColumnCount(7);
    QStringList headers;
    headers << "ID" << "标题" << "大小" << "创建时间" << "操作" << "下载" << "编辑";
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
        // 下载按钮
        QString ext;
        QMimeDatabase mimeDb;
        QMimeType mime = mimeDb.mimeTypeForName(QString::fromStdString(doc.content_type));
        if (mime.isValid() && !mime.suffixes().isEmpty()) {
            ext = "." + mime.suffixes().first();
        } else {
            ext = "";
        }
        QString suggestedName = QString::fromStdString(doc.title) + ext;
        QPushButton* btnDownload = new QPushButton("下载");
        btnDownload->setProperty("minioKey", QString::fromStdString(doc.minio_key));
        btnDownload->setProperty("suggestedName", suggestedName);
        connect(btnDownload, &QPushButton::clicked, this, &DocListDialog::onDownloadDocClicked);
        tableWidget->setCellWidget(i, 5, btnDownload);
        // 编辑按钮
        QPushButton* btnEdit = new QPushButton("编辑");
        btnEdit->setProperty("docId", doc.id);
        btnEdit->setProperty("title", QString::fromStdString(doc.title));
        btnEdit->setProperty("desc", QString::fromStdString(doc.description));
        btnEdit->setProperty("minioKey", QString::fromStdString(doc.minio_key));
        connect(btnEdit, &QPushButton::clicked, this, &DocListDialog::onEditDocClicked);
        tableWidget->setCellWidget(i, 6, btnEdit);
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

void DocListDialog::onDownloadDocClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString minioKey = btn->property("minioKey").toString();
    QString suggestedName = btn->property("suggestedName").toString();
    QString savePath = QFileDialog::getSaveFileName(this, "保存文件", suggestedName);
    if (savePath.isEmpty()) return;
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    std::vector<std::string> args = { "download", minioKey.toStdString(), savePath.toStdString() };
    bool ok = g_cliHandler->handleDownloadFile(args);
    if (ok) {
        QMessageBox::information(this, "下载文件", "下载成功！");
    } else {
        QMessageBox::warning(this, "下载文件", "下载失败，请检查 MinIO 配置或权限");
    }
}

void DocListDialog::onEditDocClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int docId = btn->property("docId").toInt();
    QString oldTitle = btn->property("title").toString();
    QString oldDesc = btn->property("desc").toString();
    QString oldMinioKey = btn->property("minioKey").toString();
    QDialog dlg(this);
    dlg.setWindowTitle("编辑文档");
    QVBoxLayout* vlayout = new QVBoxLayout(&dlg);
    QLineEdit* editTitle = new QLineEdit(oldTitle, &dlg);
    QLineEdit* editDesc = new QLineEdit(oldDesc, &dlg);
    QLineEdit* editFile = new QLineEdit(&dlg);
    editFile->setPlaceholderText("如需重新上传文件请选择");
    QPushButton* btnChoose = new QPushButton("选择新文件", &dlg);
    QPushButton* btnUpdate = new QPushButton("保存修改", &dlg);
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(editFile);
    fileLayout->addWidget(btnChoose);
    vlayout->addWidget(new QLabel("标题：", &dlg));
    vlayout->addWidget(editTitle);
    vlayout->addWidget(new QLabel("描述：", &dlg));
    vlayout->addWidget(editDesc);
    vlayout->addLayout(fileLayout);
    vlayout->addWidget(btnUpdate);
    connect(btnChoose, &QPushButton::clicked, [&]() {
        QString file = QFileDialog::getOpenFileName(&dlg, "选择新文件");
        if (!file.isEmpty()) editFile->setText(file);
    });
    connect(btnUpdate, &QPushButton::clicked, [&]() {
        QString newTitle = editTitle->text().trimmed();
        QString newDesc = editDesc->text().trimmed();
        if (newTitle.isEmpty()) {
            QMessageBox::warning(&dlg, "提示", "标题不能为空！");
            return;
        }
        std::vector<std::string> args = { "updatedoc", std::to_string(docId), newTitle.toStdString(), newDesc.toStdString() };
        bool ok = g_cliHandler->handleUpdateDocument(args);
        if (ok) {
            QMessageBox::information(&dlg, "成功", "文档信息已更新！");
            dlg.accept();
            // 刷新文档列表
            auto userResult = g_cliHandler->getCurrentUserForUI();
            if (userResult.first) {
                int userId = userResult.second.id;
                std::vector<Document> docs = g_cliHandler->getUserDocsForUI(userId);
                refreshDocs(docs);
            }
        } else {
            QMessageBox::warning(&dlg, "失败", "文档更新失败，请检查输入或权限！");
        }
    });
    dlg.exec();
}

void DocListDialog::onExportDocsClicked()
{
    // 1. 选择保存路径
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出文档",
        "我的文档.csv",
        "CSV文件 (*.csv);;所有文件 (*.*)"
    );
    if (filePath.isEmpty()) {
        return; // 用户取消
    }

    // 2. 获取当前登录用户名
    auto userPair = g_cliHandler->getCurrentUserForUI();
    if (!userPair.first) {
        QMessageBox::warning(this, "导出失败", "未登录，无法导出文档！");
        return;
    }
    std::string username = userPair.second.username;

    // 3. 调用导出
    bool ok = g_cliHandler->handleExportDocsExcel(username, filePath.toStdString());
    if (ok) {
        QMessageBox::information(this, "导出成功", "文档已成功导出到:\n" + filePath);
    } else {
        QMessageBox::warning(this, "导出失败", "导出过程中出现错误！");
    }
}