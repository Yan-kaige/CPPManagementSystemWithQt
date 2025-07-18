#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QMessageBox>
#include <QHeaderView>
#include "Common.h"
#include <vector>

class ShareDocumentDialog : public QDialog {
    Q_OBJECT

public:
    ShareDocumentDialog(const Document& document, QWidget* parent = nullptr);

private slots:
    void onSearchUserClicked();
    void onShareClicked();
    void onCancelClicked();

private:
    void setupUI();
    void searchUsers(const QString& keyword);
    void updateUserTable(const std::vector<User>& users);

    Document currentDocument;
    QLineEdit* searchLineEdit;
    QPushButton* searchButton;
    QTableWidget* userTableWidget;
    QPushButton* shareButton;
    QPushButton* cancelButton;
    QLabel* documentInfoLabel;
    
    std::vector<User> currentUsers;
    int selectedUserId;
};
