#include "fingerprint_utils.h"

QString fingerToBackendFormat(const QString& displayName)
{
    // Map display names to backend enum format (matching Android Finger.kt)
    if (displayName == "Left Thumb") return "LEFT_THUMB";
    if (displayName == "Left Index") return "LEFT_INDEX";
    if (displayName == "Left Middle") return "LEFT_MIDDLE";
    if (displayName == "Left Ring") return "LEFT_RING";
    if (displayName == "Left Pinky") return "LEFT_PINKY";
    if (displayName == "Right Thumb") return "RIGHT_THUMB";
    if (displayName == "Right Index") return "RIGHT_INDEX";
    if (displayName == "Right Middle") return "RIGHT_MIDDLE";
    if (displayName == "Right Ring") return "RIGHT_RING";
    if (displayName == "Right Pinky") return "RIGHT_PINKY";
    
    // If already in backend format, return as-is
    if (displayName.contains("_")) {
        return displayName.toUpper();
    }
    
    return QString(); // Invalid format
}

QString fingerFromBackendFormat(const QString& backendFormat)
{
    QString upper = backendFormat.toUpper();
    
    // Map backend enum format to display names
    if (upper == "LEFT_THUMB") return "Left Thumb";
    if (upper == "LEFT_INDEX") return "Left Index";
    if (upper == "LEFT_MIDDLE") return "Left Middle";
    if (upper == "LEFT_RING") return "Left Ring";
    if (upper == "LEFT_PINKY") return "Left Pinky";
    if (upper == "RIGHT_THUMB") return "Right Thumb";
    if (upper == "RIGHT_INDEX") return "Right Index";
    if (upper == "RIGHT_MIDDLE") return "Right Middle";
    if (upper == "RIGHT_RING") return "Right Ring";
    if (upper == "RIGHT_PINKY") return "Right Pinky";
    
    // If already in display format, return as-is
    return backendFormat;
}

