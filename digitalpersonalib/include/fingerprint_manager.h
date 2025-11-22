#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include "digitalpersona_global.h"
#include <QString>
#include <QByteArray>
#include <QVector>

// Forward declarations to avoid including GLib in header
typedef struct _FpContext FpContext;
typedef struct _FpDevice FpDevice;
typedef struct _FpPrint FpPrint;

class DIGITALPERSONA_EXPORT FingerprintManager {
public:
    struct EnrollmentData {
        int userId;
        QString userName;
        QByteArray templateData;
    };

    FingerprintManager();
    ~FingerprintManager();

    // Initialize fingerprint reader
    bool initialize();
    void cleanup();

    // Reader operations
    bool openReader();
    void closeReader();
    bool isReaderOpen() const { return m_device != nullptr; }
    
    // Enrollment operations
    bool startEnrollment();
    int addEnrollmentSample(QString& message, int& quality);
    bool createEnrollmentTemplate(QByteArray& templateData);
    void cancelEnrollment();
    
    // Verification operations
    bool verifyFingerprint(const QByteArray& templateData, int& score);
    
    // Utility
    QString getLastError() const { return m_lastError; }
    int getReaderCount();

private:
    FpContext* m_context;
    FpDevice* m_device;
    FpPrint* m_enrollPrint;
    QString m_lastError;
    int m_enrollmentCount;
    bool m_enrollmentInProgress;
    
    void setError(const QString& error);
    bool captureAndMatch(FpPrint* storedPrint, bool& matched, int& score);
};

#endif // FINGERPRINT_MANAGER_H

