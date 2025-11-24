#ifndef LIBFPRINT_WRAPPER_H
#define LIBFPRINT_WRAPPER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error codes
 */
#define LIBFPRINT_WRAPPER_SUCCESS 0
#define LIBFPRINT_WRAPPER_ERROR_INVALID_ARG -1
#define LIBFPRINT_WRAPPER_ERROR_PARSE_FAILED -2
#define LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED -3
#define LIBFPRINT_WRAPPER_ERROR_OUT_OF_MEMORY -4

/**
 * Opaque pointer to FpPrint object
 */
typedef void* FpPrintHandle;

/**
 * Deserialize FP3 template data into FpPrint object
 * 
 * @param data FP3 template data (must start with "FP3")
 * @param len Length of data in bytes
 * @param print_out Output: FpPrint handle (must be freed with libfprint_free_print)
 * @param error_out Output: Error message (must be freed by caller if not NULL)
 * @return 0 on success, negative error code on failure
 */
int libfprint_deserialize_template(
    const uint8_t* data,
    size_t len,
    FpPrintHandle* print_out,
    char** error_out
);

/**
 * Create FpPrint from raw fingerprint image
 * 
 * @param image_data Raw grayscale image data (8-bit per pixel)
 * @param image_len Length of image data in bytes (should be width * height)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param print_out Output: FpPrint handle (must be freed with libfprint_free_print)
 * @param error_out Output: Error message (must be freed by caller if not NULL)
 * @return 0 on success, negative error code on failure
 */
int libfprint_create_print_from_image(
    const uint8_t* image_data,
    size_t image_len,
    uint32_t width,
    uint32_t height,
    FpPrintHandle* print_out,
    char** error_out
);

/**
 * Match two FpPrint objects using bozorth3 algorithm
 * 
 * @param template_print FpPrint from template (deserialized)
 * @param probe_print FpPrint from probe (newly scanned)
 * @param threshold Bozorth3 threshold (typically 40-60)
 * @param score_out Output: Match score (0-100+)
 * @param matched_out Output: Whether prints matched (true/false)
 * @param error_out Output: Error message (must be freed by caller if not NULL)
 * @return 0 on success, negative error code on failure
 */
int libfprint_match_prints(
    FpPrintHandle template_print,
    FpPrintHandle probe_print,
    int threshold,
    int* score_out,
    bool* matched_out,
    char** error_out
);

/**
 * Free FpPrint handle
 * 
 * @param print FpPrint handle to free (can be NULL)
 */
void libfprint_free_print(FpPrintHandle print);

/**
 * Free error message string
 * 
 * @param error Error message string to free (can be NULL)
 */
void libfprint_free_error(char* error);

#ifdef __cplusplus
}
#endif

#endif // LIBFPRINT_WRAPPER_H

