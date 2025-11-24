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
#include <QComboBox>

// DigitalPersona Library
#include <digitalpersona.h>

// Backend client
#include "backend_client.h"
#include <QFutureWatcher>
#include <QCloseEvent>

class MainWindowApp : public QMainWindow {
    Q_OBJECT

public:
    MainWindowApp(QWidget *parent = nullptr);
    ~MainWindowApp();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onInitializeClicked();
    void onEnrollClicked();
    void onCaptureEnrollSample();
    void onCaptureEnrollFinished(); // New slot
    void onVerifyClicked();
    void onIdentifyClicked(); // New slot for identification
    void onCaptureVerifySample();
    void onRefreshUserList();
    void onUserSelected(QListWidgetItem* item);
    void onDeleteUserClicked();
    void onClearLog();
    void onConfigClicked(); // Show backend configuration
    // BackendClient slots
    void onUsersListed(const QVector<User>& users);
    void onUserCreated(int userId);
    void onTemplateStored(int userId, const QString& finger);
    void onTemplateLoaded(const BackendFingerprintTemplate& tmpl);
    void onTemplatesLoaded(const QVector<BackendFingerprintTemplate>& templates);
    void onUserFingersRetrieved(int userId, const QStringList& fingers);
    void onBackendError(const QString& errorMessage);

private:
    void setupUI();
    void updateStatus(const QString& status, bool isError = false);
    void log(const QString& message);
    void updateUserList();
    void enableEnrollmentControls(bool enable);
    void enableVerificationControls(bool enable);
    void onEnrollmentProgress(int current, int total, QString message);
    void processEnrollmentResult(int result);
    void reinitDatabase(); // Helper to re-initialize database

    // DigitalPersona Library instance
    FingerprintManager* m_fpManager;
    
    // Backend client
    BackendClient* m_backendClient;
    
    // Enrollment state
    bool m_enrollmentInProgress;
    int m_enrollmentSampleCount;
    int m_enrollmentUserId; // User ID for enrollment
    QString m_enrollmentUserName;
    QByteArray m_pendingEnrollmentTemplate;
    QString m_pendingEnrollmentFinger;
    
    // Verification state
    int m_verificationUserId;
    QString m_verificationUserName;
    QString m_verificationUserEmail;
    QVector<BackendFingerprintTemplate> m_verificationTemplates; // All templates for verification
    QStringList m_remainingVerificationFingers; // Remaining fingers to load
    bool m_verificationWaitingForFingers; // Flag to track if waiting for getUserFingers response
    
    // Threading
    QFutureWatcher<int> m_enrollWatcher;
    
    // Temp storage for worker thread results
    QString m_tempEnrollMessage;
    QImage m_tempEnrollImage;

    // UI components
    QLabel* m_statusLabel;
    QLabel* m_readerStatusLabel;
    
    QGroupBox* m_readerGroup;
    QPushButton* m_btnInitialize;
    
    QGroupBox* m_enrollGroup;
    QComboBox* m_enrollUserSelect; // User selection dropdown
    QComboBox* m_enrollFingerSelect; // Finger selection dropdown
    QPushButton* m_btnStartEnroll;
    QPushButton* m_btnCaptureEnroll;
    QProgressBar* m_enrollProgress;
    QLabel* m_enrollStatusLabel;
    QLabel* m_enrollImagePreview;
    
    QGroupBox* m_verifyGroup;
    QPushButton* m_btnStartVerify;
    QPushButton* m_btnIdentify; // New button
    QPushButton* m_btnCaptureVerify;
    QLabel* m_verifyResultLabel;
    QLabel* m_verifyScoreLabel;
    
    QGroupBox* m_userListGroup;
    QListWidget* m_userList;
    QPushButton* m_btnRefreshList;
    QPushButton* m_btnDeleteUser;
    QPushButton* m_btnConfig; // Database config button
    QLabel* m_userCountLabel;
    
    QTextEdit* m_logText;
    QPushButton* m_btnClearLog;
};

#endif // MAINWINDOW_APP_H

