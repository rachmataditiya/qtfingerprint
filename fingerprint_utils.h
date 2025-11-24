#ifndef FINGERPRINT_UTILS_H
#define FINGERPRINT_UTILS_H

#include <QString>

/**
 * @brief Convert display finger name to backend enum format
 * 
 * Converts Qt display format (e.g., "Right Index") to Android enum format (e.g., "RIGHT_INDEX")
 * 
 * @param displayName Display name (e.g., "Right Index", "Left Thumb")
 * @return Backend enum format (e.g., "RIGHT_INDEX", "LEFT_THUMB"), or empty string if invalid
 */
QString fingerToBackendFormat(const QString& displayName);

/**
 * @brief Convert backend enum format to display finger name
 * 
 * Converts Android enum format (e.g., "RIGHT_INDEX") to Qt display format (e.g., "Right Index")
 * 
 * @param backendFormat Backend enum format (e.g., "RIGHT_INDEX", "LEFT_THUMB")
 * @return Display name (e.g., "Right Index", "Left Thumb"), or original string if invalid
 */
QString fingerFromBackendFormat(const QString& backendFormat);

#endif // FINGERPRINT_UTILS_H

