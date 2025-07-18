#ifndef CHANGEPASSWORDDIALOG_H
#define CHANGEPASSWORDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

class ChangePasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePasswordDialog(QWidget *parent = nullptr);
    ~ChangePasswordDialog();

    QString getOldPassword() const;
    QString getNewPassword() const;
    QString getConfirmPassword() const;

private slots:
    void onOkClicked();
    void onCancelClicked();
    void onTextChanged();

private:
    void setupUI();
    bool validateInput();

    QLineEdit *lineEditOldPassword;
    QLineEdit *lineEditNewPassword;
    QLineEdit *lineEditConfirmPassword;
    QPushButton *btnOk;
    QPushButton *btnCancel;
    QLabel *labelError;
};

#endif // CHANGEPASSWORDDIALOG_H
