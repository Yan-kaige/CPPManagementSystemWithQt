/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayoutTop;
    QSpacerItem *horizontalSpacerTop;
    QLabel *labelCurrentUser;
    QPushButton *btnChangePasswordDialog;
    QPushButton *btnLogout;
    QPushButton *btnQuit;
    QTabWidget *tabWidgetAuth;
    QWidget *tabLogin;
    QVBoxLayout *verticalLayoutLogin;
    QHBoxLayout *horizontalLayoutLoginUser;
    QLabel *labelUsername;
    QLineEdit *lineEditUsername;
    QHBoxLayout *horizontalLayoutLoginPass;
    QLabel *labelPassword;
    QLineEdit *lineEditPassword;
    QPushButton *btnLogin;
    QWidget *tabRegister;
    QVBoxLayout *verticalLayoutRegister;
    QHBoxLayout *horizontalLayoutRegisterUser;
    QLabel *labelRegisterUsername;
    QLineEdit *lineEditRegisterUsername;
    QHBoxLayout *horizontalLayoutRegisterPass;
    QLabel *labelRegisterPassword;
    QLineEdit *lineEditRegisterPassword;
    QHBoxLayout *horizontalLayoutRegisterEmail;
    QLabel *labelRegisterEmail;
    QLineEdit *lineEditRegisterEmail;
    QPushButton *btnRegisterUser;
    QPushButton *btnViewDocs;
    QGroupBox *groupBoxUsers;
    QVBoxLayout *verticalLayoutUsers;
    QHBoxLayout *horizontalLayoutExportUser;
    QSpacerItem *horizontalSpacerExportUser;
    QPushButton *btnExportUsers;
    QPushButton *btnDownloadUserTemplate;
    QPushButton *btnImportUsers;
    QHBoxLayout *horizontalLayoutSearchUser;
    QLineEdit *lineEditSearchUser;
    QPushButton *btnSearchUser;
    QTableWidget *tableWidgetUsers;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayoutTop = new QHBoxLayout();
        horizontalLayoutTop->setObjectName("horizontalLayoutTop");
        horizontalSpacerTop = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayoutTop->addItem(horizontalSpacerTop);

        labelCurrentUser = new QLabel(centralwidget);
        labelCurrentUser->setObjectName("labelCurrentUser");
        labelCurrentUser->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        horizontalLayoutTop->addWidget(labelCurrentUser);

        btnChangePasswordDialog = new QPushButton(centralwidget);
        btnChangePasswordDialog->setObjectName("btnChangePasswordDialog");

        horizontalLayoutTop->addWidget(btnChangePasswordDialog);

        btnLogout = new QPushButton(centralwidget);
        btnLogout->setObjectName("btnLogout");

        horizontalLayoutTop->addWidget(btnLogout);

        btnQuit = new QPushButton(centralwidget);
        btnQuit->setObjectName("btnQuit");

        horizontalLayoutTop->addWidget(btnQuit);


        verticalLayout->addLayout(horizontalLayoutTop);

        tabWidgetAuth = new QTabWidget(centralwidget);
        tabWidgetAuth->setObjectName("tabWidgetAuth");
        tabWidgetAuth->setTabPosition(QTabWidget::North);
        tabWidgetAuth->setTabShape(QTabWidget::Rounded);
        tabLogin = new QWidget();
        tabLogin->setObjectName("tabLogin");
        verticalLayoutLogin = new QVBoxLayout(tabLogin);
        verticalLayoutLogin->setObjectName("verticalLayoutLogin");
        horizontalLayoutLoginUser = new QHBoxLayout();
        horizontalLayoutLoginUser->setObjectName("horizontalLayoutLoginUser");
        labelUsername = new QLabel(tabLogin);
        labelUsername->setObjectName("labelUsername");

        horizontalLayoutLoginUser->addWidget(labelUsername);

        lineEditUsername = new QLineEdit(tabLogin);
        lineEditUsername->setObjectName("lineEditUsername");

        horizontalLayoutLoginUser->addWidget(lineEditUsername);


        verticalLayoutLogin->addLayout(horizontalLayoutLoginUser);

        horizontalLayoutLoginPass = new QHBoxLayout();
        horizontalLayoutLoginPass->setObjectName("horizontalLayoutLoginPass");
        labelPassword = new QLabel(tabLogin);
        labelPassword->setObjectName("labelPassword");

        horizontalLayoutLoginPass->addWidget(labelPassword);

        lineEditPassword = new QLineEdit(tabLogin);
        lineEditPassword->setObjectName("lineEditPassword");
        lineEditPassword->setEchoMode(QLineEdit::Password);

        horizontalLayoutLoginPass->addWidget(lineEditPassword);


        verticalLayoutLogin->addLayout(horizontalLayoutLoginPass);

        btnLogin = new QPushButton(tabLogin);
        btnLogin->setObjectName("btnLogin");
        btnLogin->setMinimumHeight(36);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(btnLogin->sizePolicy().hasHeightForWidth());
        btnLogin->setSizePolicy(sizePolicy);

        verticalLayoutLogin->addWidget(btnLogin);

        tabWidgetAuth->addTab(tabLogin, QString());
        tabRegister = new QWidget();
        tabRegister->setObjectName("tabRegister");
        verticalLayoutRegister = new QVBoxLayout(tabRegister);
        verticalLayoutRegister->setObjectName("verticalLayoutRegister");
        horizontalLayoutRegisterUser = new QHBoxLayout();
        horizontalLayoutRegisterUser->setObjectName("horizontalLayoutRegisterUser");
        labelRegisterUsername = new QLabel(tabRegister);
        labelRegisterUsername->setObjectName("labelRegisterUsername");

        horizontalLayoutRegisterUser->addWidget(labelRegisterUsername);

        lineEditRegisterUsername = new QLineEdit(tabRegister);
        lineEditRegisterUsername->setObjectName("lineEditRegisterUsername");

        horizontalLayoutRegisterUser->addWidget(lineEditRegisterUsername);


        verticalLayoutRegister->addLayout(horizontalLayoutRegisterUser);

        horizontalLayoutRegisterPass = new QHBoxLayout();
        horizontalLayoutRegisterPass->setObjectName("horizontalLayoutRegisterPass");
        labelRegisterPassword = new QLabel(tabRegister);
        labelRegisterPassword->setObjectName("labelRegisterPassword");

        horizontalLayoutRegisterPass->addWidget(labelRegisterPassword);

        lineEditRegisterPassword = new QLineEdit(tabRegister);
        lineEditRegisterPassword->setObjectName("lineEditRegisterPassword");
        lineEditRegisterPassword->setEchoMode(QLineEdit::Password);

        horizontalLayoutRegisterPass->addWidget(lineEditRegisterPassword);


        verticalLayoutRegister->addLayout(horizontalLayoutRegisterPass);

        horizontalLayoutRegisterEmail = new QHBoxLayout();
        horizontalLayoutRegisterEmail->setObjectName("horizontalLayoutRegisterEmail");
        labelRegisterEmail = new QLabel(tabRegister);
        labelRegisterEmail->setObjectName("labelRegisterEmail");

        horizontalLayoutRegisterEmail->addWidget(labelRegisterEmail);

        lineEditRegisterEmail = new QLineEdit(tabRegister);
        lineEditRegisterEmail->setObjectName("lineEditRegisterEmail");

        horizontalLayoutRegisterEmail->addWidget(lineEditRegisterEmail);


        verticalLayoutRegister->addLayout(horizontalLayoutRegisterEmail);

        btnRegisterUser = new QPushButton(tabRegister);
        btnRegisterUser->setObjectName("btnRegisterUser");
        btnRegisterUser->setMinimumHeight(36);
        sizePolicy.setHeightForWidth(btnRegisterUser->sizePolicy().hasHeightForWidth());
        btnRegisterUser->setSizePolicy(sizePolicy);

        verticalLayoutRegister->addWidget(btnRegisterUser);

        tabWidgetAuth->addTab(tabRegister, QString());

        verticalLayout->addWidget(tabWidgetAuth);

        btnViewDocs = new QPushButton(centralwidget);
        btnViewDocs->setObjectName("btnViewDocs");
        btnViewDocs->setVisible(false);

        verticalLayout->addWidget(btnViewDocs);

        groupBoxUsers = new QGroupBox(centralwidget);
        groupBoxUsers->setObjectName("groupBoxUsers");
        verticalLayoutUsers = new QVBoxLayout(groupBoxUsers);
        verticalLayoutUsers->setObjectName("verticalLayoutUsers");
        horizontalLayoutExportUser = new QHBoxLayout();
        horizontalLayoutExportUser->setObjectName("horizontalLayoutExportUser");
        horizontalSpacerExportUser = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayoutExportUser->addItem(horizontalSpacerExportUser);

        btnExportUsers = new QPushButton(groupBoxUsers);
        btnExportUsers->setObjectName("btnExportUsers");

        horizontalLayoutExportUser->addWidget(btnExportUsers);

        btnDownloadUserTemplate = new QPushButton(groupBoxUsers);
        btnDownloadUserTemplate->setObjectName("btnDownloadUserTemplate");

        horizontalLayoutExportUser->addWidget(btnDownloadUserTemplate);

        btnImportUsers = new QPushButton(groupBoxUsers);
        btnImportUsers->setObjectName("btnImportUsers");

        horizontalLayoutExportUser->addWidget(btnImportUsers);


        verticalLayoutUsers->addLayout(horizontalLayoutExportUser);

        horizontalLayoutSearchUser = new QHBoxLayout();
        horizontalLayoutSearchUser->setObjectName("horizontalLayoutSearchUser");
        lineEditSearchUser = new QLineEdit(groupBoxUsers);
        lineEditSearchUser->setObjectName("lineEditSearchUser");

        horizontalLayoutSearchUser->addWidget(lineEditSearchUser);

        btnSearchUser = new QPushButton(groupBoxUsers);
        btnSearchUser->setObjectName("btnSearchUser");

        horizontalLayoutSearchUser->addWidget(btnSearchUser);


        verticalLayoutUsers->addLayout(horizontalLayoutSearchUser);

        tableWidgetUsers = new QTableWidget(groupBoxUsers);
        if (tableWidgetUsers->columnCount() < 1)
            tableWidgetUsers->setColumnCount(1);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        tableWidgetUsers->setHorizontalHeaderItem(0, __qtablewidgetitem);
        tableWidgetUsers->setObjectName("tableWidgetUsers");
        tableWidgetUsers->setColumnCount(1);
        tableWidgetUsers->setRowCount(0);

        verticalLayoutUsers->addWidget(tableWidgetUsers);


        verticalLayout->addWidget(groupBoxUsers);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        labelCurrentUser->setText(QCoreApplication::translate("MainWindow", "\346\234\252\347\231\273\345\275\225", nullptr));
        btnChangePasswordDialog->setText(QCoreApplication::translate("MainWindow", "\344\277\256\346\224\271\345\257\206\347\240\201", nullptr));
        btnLogout->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\207\272", nullptr));
        btnQuit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        labelUsername->setText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215\357\274\232", nullptr));
        labelPassword->setText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201\357\274\232", nullptr));
        btnLogin->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        tabWidgetAuth->setTabText(tabWidgetAuth->indexOf(tabLogin), QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        labelRegisterUsername->setText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215\357\274\232", nullptr));
        labelRegisterPassword->setText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201\357\274\232", nullptr));
        labelRegisterEmail->setText(QCoreApplication::translate("MainWindow", "\351\202\256\347\256\261\357\274\232", nullptr));
        btnRegisterUser->setText(QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214", nullptr));
        tabWidgetAuth->setTabText(tabWidgetAuth->indexOf(tabRegister), QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214", nullptr));
        btnViewDocs->setText(QCoreApplication::translate("MainWindow", "\346\237\245\347\234\213\346\226\207\346\241\243", nullptr));
        groupBoxUsers->setTitle(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\210\227\350\241\250", nullptr));
        btnExportUsers->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272\347\224\250\346\210\267", nullptr));
        btnDownloadUserTemplate->setText(QCoreApplication::translate("MainWindow", "\344\270\213\350\275\275\345\257\274\345\205\245\346\250\241\346\235\277", nullptr));
        btnImportUsers->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\205\245\347\224\250\346\210\267", nullptr));
        lineEditSearchUser->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\276\223\345\205\245\347\224\250\346\210\267\345\220\215\346\210\226\351\202\256\347\256\261\345\205\263\351\224\256\350\257\215", nullptr));
        btnSearchUser->setText(QCoreApplication::translate("MainWindow", "\346\220\234\347\264\242", nullptr));
        QTableWidgetItem *___qtablewidgetitem = tableWidgetUsers->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
