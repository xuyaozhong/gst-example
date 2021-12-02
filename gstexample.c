#include <stdlib.h>
#include <stdint.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline, *app_source, *tee, *audio_queue, *audio_convert1, *audio_resample, *audio_sink;
  GstElement *video_queue, *audio_convert2, *visual, *video_convert, *video_sink;
  GstElement *app_queue, *app_sink;

  guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
  gfloat a, b, c, d;     /* For waveform generation */

  guint sourceid;        /* To control the GSource */

  GMainLoop *main_loop;  /* GLib's Main Loop */
} CustomData;


GstFlowReturn on_new_sample(GstElement* sink, gpointer data, uint64_t trackid)
{
    GstBuffer* buffer;
    gboolean  isDroppable, delta;
    GstFlowReturn ret = GST_FLOW_OK;
    GstSample* sample = NULL;
    GstMapInfo info;
    GstSegment* segment;
    GstClockTime buf_pts;
    printf("on_new_sample \n");
    system("echo 1  >> /tmp/log.log");
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    buffer = gst_sample_get_buffer(sample);
    isDroppable = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_CORRUPTED) || GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY) ||
        (GST_BUFFER_FLAGS(buffer) == GST_BUFFER_FLAG_DISCONT) ||
        (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) ||
        // drop if buffer contains header only and has invalid timestamp
        !GST_BUFFER_PTS_IS_VALID(buffer);



}

GstFlowReturn on_new_sample_video(GstElement* sink, gpointer data)
{
    printf("on_new_sample_video \n");
//    return on_new_sample(sink, data, 1);
    GstSample *sample;

    /* Retrieve the buffer */
    g_signal_emit_by_name (sink, "pull-sample", &sample);
    if (sample) {
    /* The only thing we do in this example is print a * to indicate a received buffer */
    g_print ("*");
    gst_sample_unref (sample);
    return GST_FLOW_OK;
  }

  return GST_FLOW_ERROR;
}




main (int argc, char *argv[])
{
	CustomData data;
	GstElement *appsinkVideo = NULL, *appsinkAudio = NULL, *pipeline = NULL;
	GstBus* bus;
	GstMessage* msg;
	GError* error = NULL;
	gst_init(&argc, &argv);
                     //"videotestsrc is-live=TRUE ! queue ! videoconvert ! video/x-raw,width=1280,height=720,framerate=30/1 ! " 
		     //"v4l2src device=/dev/video0 ! video/x-raw, width=640, height=480, framerate=30/1 ! queue ! videoconvert ! "
	pipeline = gst_parse_launch(
                     "videotestsrc is-live=TRUE ! queue ! videoconvert ! video/x-raw,width=1280,height=720,framerate=30/1 ! " 
                     "x264enc bframes=0 speed-preset=veryfast bitrate=512 byte-stream=TRUE tune=zerolatency ! " 
                    "video/x-h264,stream-format=byte-stream,alignment=au,profile=baseline ! appsink sync=TRUE emit-signals=TRUE  name=appsink-video",
                    &error);
	if (!pipeline) {
		g_print ("Parse error: %s\n", error->message);
		exit (1);
	}

	appsinkVideo = gst_bin_get_by_name(GST_BIN(pipeline), "appsink-video");
	appsinkAudio = gst_bin_get_by_name(GST_BIN(pipeline), "appsink-audio");

	g_signal_connect(appsinkVideo, "new-sample", G_CALLBACK(on_new_sample_video), (gpointer) &data);

	GstFlowReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		printf("Unable to set the pipeline to the playing state.\n");
		return;
	}
        g_assert(ret == GST_STATE_CHANGE_ASYNC);
	/* block until error or EOS */


	data.main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (data.main_loop);


	bus = gst_element_get_bus(pipeline);
	msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
	if (msg != NULL) {
		 gst_message_unref(msg);
	}
	gst_object_unref(bus);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	return;
}
