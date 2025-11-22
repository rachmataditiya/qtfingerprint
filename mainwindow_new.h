#ifndef MAINWINDOW_NEW_H
#define MAINWINDOW_NEW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "fingerprint_manager.h"
#include "database_manager.h"

class MainWindowNew : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindowNew(QWidget *parent = nullptr);
    ~MainWindowNew();

private slots:
    void onInitializeClicked();
    void onEnrollClicked();
    void onCaptureEnrollSample();
    void onVerifyClicked();
    void onCaptureVerifySample();
    void onRefreshUserList();
    void onUserSelected(QListWidgetItem* item);
    void onDeleteUserClicked();
    void onClearLog();

private:
    void setupUI();
    void updateStatus(const QString& status, bool isError = false);
    void log(const QString& message);
    void updateUserList();
    void enableEnrollmentControls(bool enable);
    void enableVerificationControls(bool enable);

    // Managers
    FingerprintManager* m_fpManager;
    DatabaseManager* m_dbManager;

    // Enrollment state
    bool m_enrollmentInProgress;
    int m_enrollmentSampleCount;
    QString m_enrollmentUserName;
    QString m_enrollmentUserEmail;

    // UI Components
    // Status
    QLabel* m_statusLabel;
    QTextEdit* m_logText;
    QPushButton* m_btnClearLog;

    // Reader Group
    QGroupBox* m_readerGroup;
    QPushButton* m_btnInitialize;
    QLabel* m_readerStatusLabel;

    // Enrollment Group
    QGroupBox* m_enrollGroup;
    QLineEdit* m_editEnrollName;
    QLineEdit* m_editEnrollEmail;
    QPushButton* m_btnStartEnroll;
    QPushButton* m_btnCaptureEnroll;
    QProgressBar* m_enrollProgress;
    QLabel* m_enrollStatusLabel;

    // Verification Group
    QGroupBox* m_verifyGroup;
    QPushButton* m_btnStartVerify;
    QPushButton* m_btnCaptureVerify;
    QLabel* m_verifyResultLabel;
    QLabel* m_verifyScoreLabel;

    // User List Group
    QGroupBox* m_userListGroup;
    QListWidget* m_userList;
    QPushButton* m_btnRefreshList;
    QPushButton* m_btnDeleteUser;
    QLabel* m_userCountLabel;
};

#endif // MAINWINDOW_NEW_H

