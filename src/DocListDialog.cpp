#include "DocListDialog.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include "Common.h"
#include "CLIHandler.h"
#include "ShareDocumentDialog.h"
#include <QFileDialog>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMimeDatabase>
#include <QMimeType>
#include <QLabel>
#include <QWidget>

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
    tableWidget->setColumnCount(6);
    QStringList headers;
    headers << "ID" << "标题" << "描述" << "大小" << "创建时间" << "操作";
    tableWidget->setHorizontalHeaderLabels(headers);
    tableWidget->setRowCount(docs.size());
    for (int i = 0; i < docs.size(); ++i) {
        const Document& doc = docs[i];
        tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(doc.id)));
        tableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(doc.title)));
        tableWidget->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(doc.description)));

        // 将文件大小转换为MB显示
        double sizeInMB = static_cast<double>(doc.file_size) / (1024.0 * 1024.0);
        QString sizeText;
        if (sizeInMB < 0.01) {
            sizeText = QString::number(doc.file_size) + " B";
        } else if (sizeInMB < 1.0) {
            double sizeInKB = static_cast<double>(doc.file_size) / 1024.0;
            sizeText = QString::number(sizeInKB, 'f', 1) + " KB";
        } else {
            sizeText = QString::number(sizeInMB, 'f', 2) + " MB";
        }
        tableWidget->setItem(i, 3, new QTableWidgetItem(sizeText));
        tableWidget->setItem(i, 4, new QTableWidgetItem(QString::fromUtf8(Utils::formatTimestamp(doc.created_at))));

        // 创建操作按钮容器
        QWidget* operationWidget = new QWidget();
        QHBoxLayout* operationLayout = new QHBoxLayout(operationWidget);
        operationLayout->setContentsMargins(2, 2, 2, 2);
        operationLayout->setSpacing(2);

        // 删除按钮
        QPushButton* btnDel = new QPushButton("删除");
        btnDel->setMaximumWidth(50);
        btnDel->setProperty("docId", doc.id);
        connect(btnDel, &QPushButton::clicked, this, &DocListDialog::onDeleteDocClicked);

        // 下载按钮
        QString ext;
        if (!doc.content_type.empty()) {
            ext = "." + QString::fromUtf8(doc.content_type);
        } else {
            ext = "";
        }
        QString suggestedName = QString::fromUtf8(doc.title) + ext;
        QPushButton* btnDownload = new QPushButton("下载");
        btnDownload->setMaximumWidth(50);
        btnDownload->setProperty("minioKey", QString::fromUtf8(doc.minio_key));
        btnDownload->setProperty("suggestedName", suggestedName);
        connect(btnDownload, &QPushButton::clicked, this, &DocListDialog::onDownloadDocClicked);

        // 编辑按钮
        QPushButton* btnEdit = new QPushButton("编辑");
        btnEdit->setMaximumWidth(50);
        btnEdit->setProperty("docId", doc.id);
        btnEdit->setProperty("title", QString::fromUtf8(doc.title));
        btnEdit->setProperty("desc", QString::fromUtf8(doc.description));
        btnEdit->setProperty("minioKey", QString::fromUtf8(doc.minio_key));
        connect(btnEdit, &QPushButton::clicked, this, &DocListDialog::onEditDocClicked);

        // 分享按钮
        QPushButton* btnShare = new QPushButton("分享");
        btnShare->setMaximumWidth(50);
        btnShare->setProperty("docId", doc.id);
        btnShare->setProperty("title", QString::fromUtf8(doc.title));
        btnShare->setProperty("description", QString::fromUtf8(doc.description));
        btnShare->setProperty("filePath", QString::fromUtf8(doc.file_path));
        btnShare->setProperty("minioKey", QString::fromUtf8(doc.minio_key));
        btnShare->setProperty("ownerId", doc.owner_id);
        btnShare->setProperty("fileSize", static_cast<qulonglong>(doc.file_size));
        btnShare->setProperty("contentType", QString::fromUtf8(doc.content_type));
        connect(btnShare, &QPushButton::clicked, this, &DocListDialog::onShareDocClicked);

        // 将按钮添加到布局
        operationLayout->addWidget(btnDel);
        operationLayout->addWidget(btnDownload);
        operationLayout->addWidget(btnEdit);
        operationLayout->addWidget(btnShare);
        operationLayout->addStretch();

        tableWidget->setCellWidget(i, 5, operationWidget);
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
        std::vector<std::string> args = { "adddoc", title.toUtf8().constData(), desc.toUtf8().constData(), file.toUtf8().constData() };
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
        docs = g_cliHandler->getSearchedDocsForUI(userId, keyword.toUtf8().constData());
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
    std::vector<std::string> args = { "download", minioKey.toUtf8().constData(), savePath.toUtf8().constData() };
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
        QString newFile = editFile->text().trimmed();

        if (newTitle.isEmpty()) {
            QMessageBox::warning(&dlg, "提示", "标题不能为空！");
            return;
        }

        // 构建参数列表，如果有新文件则包含文件路径
        std::vector<std::string> args = { "updatedoc", std::to_string(docId), newTitle.toUtf8().constData(), newDesc.toUtf8().constData() };
        if (!newFile.isEmpty()) {
            args.push_back(newFile.toUtf8().constData());
        }

        bool ok = g_cliHandler->handleUpdateDocument(args);
        if (ok) {
            QString message = "文档信息已更新！";
            if (!newFile.isEmpty()) {
                message += "\n新文件已上传。";
            }
            QMessageBox::information(&dlg, "成功", message);
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
    bool ok = g_cliHandler->handleExportDocsExcel(username, filePath.toUtf8().constData());
    if (ok) {
        QMessageBox::information(this, "导出成功", "文档已成功导出到:\n" + filePath);
    } else {
        QMessageBox::warning(this, "导出失败", "导出过程中出现错误！");
    }
}

void DocListDialog::onShareDocClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    // 从按钮属性中获取文档信息
    Document doc;
    doc.id = btn->property("docId").toInt();
    doc.title = btn->property("title").toString().toUtf8().constData();
    doc.description = btn->property("description").toString().toUtf8().constData();
    doc.file_path = btn->property("filePath").toString().toUtf8().constData();
    doc.minio_key = btn->property("minioKey").toString().toUtf8().constData();
    doc.owner_id = btn->property("ownerId").toInt();
    doc.file_size = btn->property("fileSize").toULongLong();
    doc.content_type = btn->property("contentType").toString().toUtf8().constData();

    // 检查是否是文档所有者
    auto currentUserResult = g_cliHandler->getCurrentUserForUI();
    if (!currentUserResult.first) {
        QMessageBox::warning(this, "分享失败", "未登录，无法分享文档！");
        return;
    }

    if (currentUserResult.second.id != doc.owner_id) {
        QMessageBox::warning(this, "分享失败", "您不是此文档的所有者，无法分享！");
        return;
    }

    // 打开分享对话框
    ShareDocumentDialog shareDialog(doc, this);
    shareDialog.exec();
}