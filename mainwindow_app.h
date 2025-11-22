#ifndef MAINWINDOW_APP_H
#define MAINWINDOW_APP_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QGroupBox>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>

// DigitalPersona Library
#include <digitalpersona.h>

// Local database manager
#include "database_manager.h"

class MainWindowApp : public QMainWindow {
    Q_OBJECT

public:
    MainWindowApp(QWidget *parent = nullptr);
    ~MainWindowApp();

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
    void onEnrollmentProgress(int current, int total, QString message);

    // DigitalPersona Library instance
    FingerprintManager* m_fpManager;
    
    // Local database manager
    DatabaseManager* m_dbManager;
    
    // Enrollment state
    bool m_enrollmentInProgress;
    int m_enrollmentSampleCount;
    QString m_enrollmentUserName;
    QString m_enrollmentUserEmail;

    // UI components
    QLabel* m_statusLabel;
    QLabel* m_readerStatusLabel;
    
    QGroupBox* m_readerGroup;
    QPushButton* m_btnInitialize;
    
    QGroupBox* m_enrollGroup;
    QLineEdit* m_editEnrollName;
    QLineEdit* m_editEnrollEmail;
    QPushButton* m_btnStartEnroll;
    QPushButton* m_btnCaptureEnroll;
    QProgressBar* m_enrollProgress;
    QLabel* m_enrollStatusLabel;
    QLabel* m_enrollImagePreview;
    
    QGroupBox* m_verifyGroup;
    QPushButton* m_btnStartVerify;
    QPushButton* m_btnCaptureVerify;
    QLabel* m_verifyResultLabel;
    QLabel* m_verifyScoreLabel;
    
    QGroupBox* m_userListGroup;
    QListWidget* m_userList;
    QPushButton* m_btnRefreshList;
    QPushButton* m_btnDeleteUser;
    QLabel* m_userCountLabel;
    
    QTextEdit* m_logText;
    QPushButton* m_btnClearLog;
};

#endif // MAINWINDOW_APP_H

