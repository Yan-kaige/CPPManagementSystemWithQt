#include "ChangePasswordDialog.h"
#include <QMessageBox>
#include <QRegularExpression>



ChangePasswordDialog::ChangePasswordDialog(QWidget *parent)
    : QDialog(parent)
    , lineEditOldPassword(nullptr)
    , lineEditNewPassword(nullptr)
    , lineEditConfirmPassword(nullptr)
    , btnOk(nullptr)
    , btnCancel(nullptr)
    , labelError(nullptr)
{
    setupUI();
    setWindowTitle("修改密码");
    setModal(true);
    resize(350, 200);
}

ChangePasswordDialog::~ChangePasswordDialog()
{
}

void ChangePasswordDialog::setupUI()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建表单布局
    QFormLayout *formLayout = new QFormLayout();
    
    // 创建输入控件
    lineEditOldPassword = new QLineEdit(this);
    lineEditOldPassword->setEchoMode(QLineEdit::Password);
    lineEditOldPassword->setPlaceholderText("请输入原密码");
    
    lineEditNewPassword = new QLineEdit(this);
    lineEditNewPassword->setEchoMode(QLineEdit::Password);
    lineEditNewPassword->setPlaceholderText("请输入新密码");
    
    lineEditConfirmPassword = new QLineEdit(this);
    lineEditConfirmPassword->setEchoMode(QLineEdit::Password);
    lineEditConfirmPassword->setPlaceholderText("请再次输入新密码");
    
    // 添加到表单布局
    formLayout->addRow("原密码：", lineEditOldPassword);
    formLayout->addRow("新密码：", lineEditNewPassword);
    formLayout->addRow("确认密码：", lineEditConfirmPassword);
    
    // 创建错误提示标签
    labelError = new QLabel(this);
    labelError->setStyleSheet("color: red;");
    labelError->setWordWrap(true);
    labelError->hide();
    
    // 创建按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    btnOk = new QPushButton("确定", this);
    btnCancel = new QPushButton("取消", this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnOk);
    buttonLayout->addWidget(btnCancel);
    
    // 添加到主布局
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(labelError);
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号槽
    connect(btnOk, &QPushButton::clicked, this, &ChangePasswordDialog::onOkClicked);
    connect(btnCancel, &QPushButton::clicked, this, &ChangePasswordDialog::onCancelClicked);
    connect(lineEditOldPassword, &QLineEdit::textChanged, this, &ChangePasswordDialog::onTextChanged);
    connect(lineEditNewPassword, &QLineEdit::textChanged, this, &ChangePasswordDialog::onTextChanged);
    connect(lineEditConfirmPassword, &QLineEdit::textChanged, this, &ChangePasswordDialog::onTextChanged);
    
    // 设置默认按钮
    btnOk->setDefault(true);
    
    // 设置焦点
    lineEditOldPassword->setFocus();
}

QString ChangePasswordDialog::getOldPassword() const
{
    return lineEditOldPassword->text();
}

QString ChangePasswordDialog::getNewPassword() const
{
    return lineEditNewPassword->text();
}

QString ChangePasswordDialog::getConfirmPassword() const
{
    return lineEditConfirmPassword->text();
}

void ChangePasswordDialog::onOkClicked()
{
    if (validateInput()) {
        accept();
    }
}

void ChangePasswordDialog::onCancelClicked()
{
    reject();
}

void ChangePasswordDialog::onTextChanged()
{
    labelError->hide();
}

bool ChangePasswordDialog::validateInput()
{
    QString oldPass = lineEditOldPassword->text();
    QString newPass = lineEditNewPassword->text();
    QString confirmPass = lineEditConfirmPassword->text();
    
    if (oldPass.isEmpty()) {
        labelError->setText("请输入原密码");
        labelError->show();
        lineEditOldPassword->setFocus();
        return false;
    }
    
    if (newPass.isEmpty()) {
        labelError->setText("请输入新密码");
        labelError->show();
        lineEditNewPassword->setFocus();
        return false;
    }
    
    if (newPass.length() < 6) {
        labelError->setText("新密码长度不能少于6位");
        labelError->show();
        lineEditNewPassword->setFocus();
        return false;
    }
    
    if (confirmPass.isEmpty()) {
        labelError->setText("请确认新密码");
        labelError->show();
        lineEditConfirmPassword->setFocus();
        return false;
    }
    
    if (newPass != confirmPass) {
        labelError->setText("两次输入的新密码不一致");
        labelError->show();
        lineEditConfirmPassword->setFocus();
        return false;
    }
    
    if (oldPass == newPass) {
        labelError->setText("新密码不能与原密码相同");
        labelError->show();
        lineEditNewPassword->setFocus();
        return false;
    }
    
    return true;
}
