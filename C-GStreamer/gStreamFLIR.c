#define _GNU_SOURCE
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>
#include <pigpio.h>
#include <pthread.h>
#include <time.h>
#define DEFAULT_SAMPLE_RATE 1
#define OUTPUT_FILE_NAME "output.txt"
GstElement *appsink = NULL;
FILE *fd;
long long trigger_counter = 0;
double time_in_seconds = 0;
void aFunction(int gpio, int level, uint32_t tick) {
    /* only record low to high edges */
    if (level == 1) {
        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (!sample) {
            g_printerr("Failed to capture sample\n");
            return;
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            char filename[20];
            snprintf(filename, sizeof(filename), "capture%lld.txt", trigger_counter);
            GstClockTime pts = GST_BUFFER_PTS(buffer);
            double pts_seconds = (double)pts / GST_SECOND;
            fprintf(fd, "%f \n", time_in_seconds + pts_seconds);
            fflush(fd);
            FILE *out = fopen("capture.raw", "wb");
            fwrite(map.data, 1, map.size, out);
            fclose(out);
            g_print("Frame captured to capture.raw (%zu bytes)\n", map.size);
            gst_buffer_unmap(buffer, &map);
        }
        gst_sample_unref(sample);
        trigger_counter += 1;
        
    }
    return;
  }
int main(int argc, char *argv[]) {
    pthread_t thread = pthread_self(); 
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);

    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        return 1;
    } else {
        printf("CPU affinity set to core 2\n");
    }

    struct sched_param param;
    param.sched_priority = 99;

    if (pthread_setschedparam(thread, SCHED_FIFO, &param) != 0) {
        perror("pthread_setschedparam");
        return 1;
    } else {
        printf("Thread priority set to 99 (SCHED_FIFO)\n");
    }

    gst_init(&argc, &argv);

    GstElement *pipeline = gst_parse_launch(
        "v4l2src device=/dev/video0 ! "
        "video/x-raw,format=GRAY16_LE,width=640,height=512,framerate=9/1 ! "
        "appsink name=sink", NULL);

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    g_object_set(G_OBJECT(appsink),
             "drop", TRUE,
             "max-buffers", 1,
             "sync", FALSE,
             NULL);
    struct timespec ts;

    // Get current time with clock_gettime
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("clock_gettime");
        return -1;
    }

    // Convert to decimal seconds
    time_in_seconds = ts.tv_sec + ts.tv_nsec / 1e9;


    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_usleep(10000000); // 1 second
    for (int i = 0; i < 4; i++) {
        GstSample *flush_sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (flush_sample) gst_sample_unref(flush_sample);
        g_print("Flushed frame %d\n", i + 1);
    }
    fd = fopen(OUTPUT_FILE_NAME, "w");
    if (fd == NULL){
        perror("Failed to open file\n");
    }
    fflush(fd);

    gpioTerminate();
    gpioCfgClock(DEFAULT_SAMPLE_RATE, 1, 1);
    if (gpioInitialise() < 0) {
    return;
    }
    int mode;
    gpioWaveClear();
    gpioSetMode(17, PI_INPUT); // for gpio pin 4 (broadcom numbered)
    gpioSetAlertFunc(17, aFunction);  // for GPIO pin 4
    while(1){ // maybe not needed depending on how the call back works (if it creates a seperate thread for the callback on gpio or not)
        pause();
    }
    // g_usleep(10000000); // 10 second
    // sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    // if (!sample) {
    //     g_printerr("Failed to capture sample\n");
    //     return -1;
    // }

    // buffer = gst_sample_get_buffer(sample);
    // if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    //     GstClockTime pts = GST_BUFFER_PTS(buffer);
    //     g_print("Frame PTS: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(pts));
    //     FILE *out = fopen("capture2.raw", "wb");
    //     fwrite(map.data, 1, map.size, out);
    //     fclose(out);
    //     g_print("Frame captured to capture.raw (%zu bytes)\n", map.size);
    //     gst_buffer_unmap(buffer, &map);
    // }
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
    gpioTerminate();
    return 0;
}
