/**
 * libfprint-wrapper.c
 * C wrapper for libfprint functions to be used from Rust
 * 
 * This wrapper provides a simple C API that hides the complexity
 * of GLib/GObject lifecycle management.
 */

#include "wrapper.h"
#include <fprint.h>
#include <fpi-print.h>
#include <fpi-image.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>  // For offsetof if needed

// Forward declare the internal structure (matches fp-print-private.h)
// We can't include fp-print-private.h directly because it includes <nbis.h>
// which may not be available in our build environment
struct _FpPrint
{
    void* parent_instance;  // GInitiallyUnowned
    int type;  // FpiPrintType
    char* driver;
    char* device_id;
    int device_stored;  // gboolean
    void* image;  // FpImage*
    int finger;  // FpFinger
    char* username;
    char* description;
    void* enroll_date;  // GDate*
    void* data;  // GVariant*
    GPtrArray* prints;  // GPtrArray of xyt_struct* - THIS IS WHAT WE NEED
};

// Forward declare xyt_struct (from nbis/bozorth.h)
// Use ifndef guard to prevent redefinition
#ifndef _XYT_STRUCT_DEFINED
#define _XYT_STRUCT_DEFINED
struct xyt_struct {
    int nrows;
    int xcol[200];
    int ycol[200];
    int thetacol[200];
};
#endif

// Forward declare bozorth functions (from nbis)
// These are exported by libfprint-2.2.dylib
extern int bozorth_probe_init(struct xyt_struct *pstruct);
extern int bozorth_to_gallery(int probe_len, struct xyt_struct *pstruct, struct xyt_struct *gstruct);

// Note: We can't access bozorth functions directly without including nbis headers
// So we use binary search to find the maximum threshold that still matches

// Global initialization flag
static gboolean g_initialized = FALSE;

// Structure for minutiae detection callback
typedef struct {
    gboolean success;
    GError* error;
    GMainLoop* loop;
} MinutiaeDetectData;

// Callback function for async minutiae detection
static void minutiae_detect_cb(GObject* source_object, GAsyncResult* res, gpointer user_data) {
    MinutiaeDetectData* data = (MinutiaeDetectData*)user_data;
    FpImage* img = FP_IMAGE(source_object);
    
    data->success = fp_image_detect_minutiae_finish(img, res, &data->error);
    g_main_loop_quit(data->loop);
}

/**
 * Initialize libfprint (one-time call)
 */
static int ensure_initialized(void) {
    if (!g_initialized) {
        // GLib is auto-initialized, but we can ensure it here if needed
        g_initialized = TRUE;
    }
    return LIBFPRINT_WRAPPER_SUCCESS;
}

/**
 * Allocate error message string
 */
static char* alloc_error(const char* message) {
    if (!message) return NULL;
    size_t len = strlen(message) + 1;
    char* err = malloc(len);
    if (err) {
        memcpy(err, message, len);
    }
    return err;
}

/**
 * Deserialize FP3 template data into FpPrint object
 */
int libfprint_deserialize_template(
    const uint8_t* data,
    size_t len,
    FpPrintHandle* print_out,
    char** error_out)
{
    if (!data || len == 0 || !print_out) {
        if (error_out) *error_out = alloc_error("Invalid arguments");
        return LIBFPRINT_WRAPPER_ERROR_INVALID_ARG;
    }

    if (ensure_initialized() != LIBFPRINT_WRAPPER_SUCCESS) {
        if (error_out) *error_out = alloc_error("Failed to initialize libfprint");
        return LIBFPRINT_WRAPPER_ERROR_INVALID_ARG;
    }

    GError* error = NULL;
    FpPrint* print = fp_print_deserialize((const guchar*)data, (gsize)len, &error);

    if (error) {
        if (error_out) *error_out = alloc_error(error->message);
        g_error_free(error);
        return LIBFPRINT_WRAPPER_ERROR_PARSE_FAILED;
    }

    if (!print) {
        if (error_out) *error_out = alloc_error("Deserialization returned NULL");
        return LIBFPRINT_WRAPPER_ERROR_PARSE_FAILED;
    }

    *print_out = (FpPrintHandle)print;
    return LIBFPRINT_WRAPPER_SUCCESS;
}

/**
 * Create FpPrint from raw fingerprint image
 * 
 * Steps:
 * 1. Create FpImage from raw data
 * 2. Copy image data to FpImage (using private struct access)
 * 3. Set ppmm (pixels per mm) - default 19.685 (~500 ppi)
 * 4. Detect minutiae synchronously
 * 5. Create FpPrint (without device, using g_object_new)
 * 6. Set type to NBIS and add minutiae from image
 */
int libfprint_create_print_from_image(
    const uint8_t* image_data,
    size_t image_len,
    uint32_t width,
    uint32_t height,
    FpPrintHandle* print_out,
    char** error_out)
{
    if (!image_data || image_len == 0 || !print_out) {
        if (error_out) *error_out = alloc_error("Invalid arguments");
        return LIBFPRINT_WRAPPER_ERROR_INVALID_ARG;
    }

    if (ensure_initialized() != LIBFPRINT_WRAPPER_SUCCESS) {
        if (error_out) *error_out = alloc_error("Failed to initialize libfprint");
        return LIBFPRINT_WRAPPER_ERROR_INVALID_ARG;
    }

    // Validate image size
    size_t expected_size = (size_t)width * height;
    if (image_len != expected_size) {
        if (error_out) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Image size mismatch: expected %zu bytes (%ux%u), got %zu bytes",
                     expected_size, width, height, image_len);
            *error_out = alloc_error(buf);
        }
        return LIBFPRINT_WRAPPER_ERROR_INVALID_ARG;
    }

    // Create FpImage
    FpImage* image = fp_image_new((gint)width, (gint)height);
    if (!image) {
        if (error_out) *error_out = alloc_error("Failed to create FpImage");
        return LIBFPRINT_WRAPPER_ERROR_OUT_OF_MEMORY;
    }

    // Access private struct to copy image data
    // Note: FpImage struct is partially public in fpi-image.h
    // The 'data' field exists in struct _FpImage but is marked private
    // We access it directly here for the wrapper
    struct _FpImage* img = (struct _FpImage*)image;
    if (img->data) {
        memcpy(img->data, image_data, image_len);
    } else {
        g_object_unref(image);
        if (error_out) *error_out = alloc_error("FpImage data buffer is NULL");
        return LIBFPRINT_WRAPPER_ERROR_OUT_OF_MEMORY;
    }

    // Set ppmm (pixels per millimeter) - default is ~19.685 (500 ppi)
    img->ppmm = 19.685;

    // Detect minutiae synchronously using GMainLoop
    MinutiaeDetectData detect_data_storage;
    MinutiaeDetectData* detect_data = &detect_data_storage;
    detect_data->success = FALSE;
    detect_data->error = NULL;
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    detect_data->loop = loop;

    // Setup callback for async minutiae detection
    fp_image_detect_minutiae(
        image,
        NULL, // cancellable
        minutiae_detect_cb,
        detect_data
    );

    // Run main loop until detection completes
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    gboolean detection_success = detect_data->success;
    GError* detect_error = detect_data->error;

    if (!detection_success) {
        if (detect_error) {
            if (error_out) *error_out = alloc_error(detect_error->message);
            g_error_free(detect_error);
        } else {
            if (error_out) *error_out = alloc_error("Failed to detect minutiae in image");
        }
        g_object_unref(image);
        return LIBFPRINT_WRAPPER_ERROR_PARSE_FAILED;
    }

    // Create FpPrint without device (using g_object_new directly)
    // We need driver and device_id - use dummy values for matching
    FpPrint* print = g_object_new(
        FP_TYPE_PRINT,
        "driver", "virtual",
        "device-id", "virtual-image",
        NULL
    );
    
    if (!print) {
        g_object_unref(image);
        if (error_out) *error_out = alloc_error("Failed to create FpPrint");
        return LIBFPRINT_WRAPPER_ERROR_OUT_OF_MEMORY;
    }

    // Set print type to NBIS
    fpi_print_set_type(print, FPI_PRINT_NBIS);

    // Add minutiae from image to print
    GError* error = NULL;
    if (!fpi_print_add_from_image(print, image, &error)) {
        if (error) {
            if (error_out) *error_out = alloc_error(error->message);
            g_error_free(error);
        } else {
            if (error_out) *error_out = alloc_error("Failed to add minutiae to print");
        }
        g_object_unref(print);
        g_object_unref(image);
        return LIBFPRINT_WRAPPER_ERROR_PARSE_FAILED;
    }

    // Image is now owned by print, so we don't need to unref it
    *print_out = (FpPrintHandle)print;
    return LIBFPRINT_WRAPPER_SUCCESS;
}

/**
 * Match two FpPrint objects using bozorth3 algorithm
 * Returns actual bozorth3 score, not just match/no-match
 */
int libfprint_match_prints(
    FpPrintHandle template_print,
    FpPrintHandle probe_print,
    int threshold,
    int* score_out,
    bool* matched_out,
    char** error_out)
{
    if (!template_print || !probe_print || !score_out || !matched_out) {
        if (error_out) *error_out = alloc_error("Invalid arguments");
        return LIBFPRINT_WRAPPER_ERROR_INVALID_ARG;
    }

    FpPrint* template = (FpPrint*)template_print;
    FpPrint* probe = (FpPrint*)probe_print;

    // First try using the official API to see if there's a match with requested threshold
    // This will also check types and validate prints
    GError* error = NULL;
    FpiMatchResult result = fpi_print_bz3_match(template, probe, threshold, &error);

    if (error) {
        if (error_out) *error_out = alloc_error(error->message);
        g_error_free(error);
        *matched_out = false;
        *score_out = 0;
        return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
    }

    // Determine match/no-match based on requested threshold
    *matched_out = (result == FPI_MATCH_SUCCESS);
    
    // Get actual bozorth3 score by calling bozorth_to_gallery directly
    // This is what fpi_print_bz3_match does internally but doesn't return
    // Access internal prints array using GObject property "fpi-prints"
    // NO FALLBACK - if we can't access, return error
    
    // Use GObject property to access prints field (more reliable than struct casting)
    GPtrArray* template_prints = NULL;
    GPtrArray* probe_prints = NULL;
    
    g_object_get(template, "fpi-prints", &template_prints, NULL);
    g_object_get(probe, "fpi-prints", &probe_prints, NULL);
    
    if (!template_prints || !probe_prints) {
        if (error_out) *error_out = alloc_error("Failed to get prints field via GObject property fpi-prints");
        return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
    }
    
    if (probe_prints->len != 1) {
        if (error_out) *error_out = alloc_error("Probe print must contain exactly one print");
        return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
    }
    
    if (template_prints->len == 0) {
        if (error_out) *error_out = alloc_error("Template print contains no prints");
        return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
    }
    
    // Get probe print struct (same as fpi_print_bz3_match line 234-235)
    struct xyt_struct* pstruct = (struct xyt_struct*)g_ptr_array_index(probe_prints, 0);
    if (!pstruct) {
        if (error_out) *error_out = alloc_error("Failed to get probe print struct");
        return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
    }
    
    int probe_len = bozorth_probe_init(pstruct);
    if (probe_len <= 0) {
        if (error_out) *error_out = alloc_error("bozorth_probe_init failed");
        return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
    }
    
    // Find best score across all template prints (same as fpi_print_bz3_match lines 237-246)
    int best_score = 0;
    int i;
    for (i = 0; i < template_prints->len; i++) {
        struct xyt_struct* gstruct = (struct xyt_struct*)g_ptr_array_index(template_prints, i);
        if (!gstruct) {
            if (error_out) *error_out = alloc_error("Failed to get template print struct");
            return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
        }
        
        int score = bozorth_to_gallery(probe_len, pstruct, gstruct);
        // Debug: log score for each comparison
        // Note: We can't use printf directly, but we can add a comment here
        // The score will be logged by Rust side
        
        if (score > best_score) {
            best_score = score;
        }
    }
    
    // Validate that we got a reasonable score
    // If best_score is still 0 or very low, something might be wrong
    if (best_score == 0) {
        if (error_out) *error_out = alloc_error("bozorth_to_gallery returned 0 for all comparisons - possible data issue");
        return LIBFPRINT_WRAPPER_ERROR_MATCH_FAILED;
    }
    
    *score_out = best_score;
    return LIBFPRINT_WRAPPER_SUCCESS;
}

/**
 * Free FpPrint handle
 */
void libfprint_free_print(FpPrintHandle print) {
    if (print) {
        g_object_unref((FpPrint*)print);
    }
}

/**
 * Free error message string
 */
void libfprint_free_error(char* error) {
    if (error) {
        free(error);
    }
}

