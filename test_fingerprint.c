
#include <glib.h>
#include <glib-object.h>
#include <fprint.h>
#include <stdio.h>
#include <stdlib.h>

static void on_enroll_progress(FpDevice *device, gint completed_stages, FpPrint *print, gpointer user_data, GError *error) {
    if (error) {
        printf("Enrollment error: %s\n", error->message);
        return;
    }
    printf("Enrollment progress: Stage %d completed\n", completed_stages);
}

int main(int argc, char *argv[]) {
    printf("Initializing...\n");
    g_type_init();

    FpContext *ctx = fp_context_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return 1;
    }

    GPtrArray *devices = fp_context_get_devices(ctx);
    if (!devices || devices->len == 0) {
        fprintf(stderr, "No devices found\n");
        g_object_unref(ctx);
        return 1;
    }

    printf("Found %d devices\n", devices->len);
    FpDevice *dev = FP_DEVICE(g_ptr_array_index(devices, 0));
    printf("Using device: %s\n", fp_device_get_name(dev));

    GError *error = NULL;
    if (!fp_device_open_sync(dev, NULL, &error)) {
        fprintf(stderr, "Failed to open device: %s\n", error->message);
        g_error_free(error);
        g_object_unref(ctx);
        return 1;
    }
    printf("Device opened successfully\n");

    printf("Starting enrollment... please scan finger 5 times\n");
    
    FpPrint *print_template = fp_print_new(dev);
    FpPrint *enrolled_print = fp_device_enroll_sync(dev, print_template, NULL, on_enroll_progress, NULL, &error);

    if (error) {
        fprintf(stderr, "Enrollment failed: %s\n", error->message);
        g_error_free(error);
    } else {
        printf("Enrollment complete!\n");
        if (enrolled_print) g_object_unref(enrolled_print);
    }

    g_object_unref(print_template);

    printf("Closing device...\n");
    if (!fp_device_close_sync(dev, NULL, &error)) {
         fprintf(stderr, "Failed to close device: %s\n", error->message);
         g_error_free(error);
    } else {
         printf("Device closed.\n");
    }

    g_object_unref(ctx);
    printf("Done.\n");

    return 0;
}
