#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    GstElement *pipeline = gst_parse_launch(
        "v4l2src device=/dev/video0 ! "
        "video/x-raw,format=GRAY16_LE,width=640,height=512,framerate=9/1 ! "
        "appsink name=sink", NULL);

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_usleep(1000000); // 1 second
    for (int i = 0; i < 4; i++) {
        GstSample *flush_sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if (flush_sample) gst_sample_unref(flush_sample);
        g_print("Flushed frame %d\n", i + 1);
    }

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    if (!sample) {
        g_printerr("Failed to capture sample\n");
        return -1;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        GstClockTime pts = GST_BUFFER_PTS(buffer);
        g_print("Frame PTS: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(pts));
        FILE *out = fopen("capture.raw", "wb");
        fwrite(map.data, 1, map.size, out);
        fclose(out);
        g_print("Frame captured to capture.raw (%zu bytes)\n", map.size);
        gst_buffer_unmap(buffer, &map);
    }
    g_usleep(10000000); // 10 second
    sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    if (!sample) {
        g_printerr("Failed to capture sample\n");
        return -1;
    }

    buffer = gst_sample_get_buffer(sample);
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        GstClockTime pts = GST_BUFFER_PTS(buffer);
        g_print("Frame PTS: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(pts));
        FILE *out = fopen("capture2.raw", "wb");
        fwrite(map.data, 1, map.size, out);
        fclose(out);
        g_print("Frame captured to capture.raw (%zu bytes)\n", map.size);
        gst_buffer_unmap(buffer, &map);
    }
    gst_sample_unref(sample);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);

    return 0;
}
