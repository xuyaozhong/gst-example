#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <png.h>
#include <zbar.h>

zbar_image_scanner_t *scanner = NULL;

char *rawdata = NULL;
char *Y800data = NULL;
int width = 640, height = 480;

#define INIT 0
#define FETCH 1
#define FETCHED 2
#define DECODE 3
#define READY 4
int barcode_status = INIT;

pthread_t pbarcode;

/* Structure to contain all our information, so we can pass it to callbacks */
guint64 num_samples = 0;


typedef struct _CustomData {
	GstElement *pipeline, *app_source, *tee, *audio_queue, *audio_convert1, *audio_resample, *audio_sink;
	GstElement *video_queue, *audio_convert2, *visual, *video_convert, *video_sink;
	GstElement *app_queue, *app_sink;

	guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
	gfloat a, b, c, d;	 /* For waveform generation */

	guint sourceid;		/* To control the GSource */

	GMainLoop *main_loop;  /* GLib's Main Loop */
} CustomData;


void dumptofile(GstMapInfo *info, guint64 id)
{
	if(info)
	{
		FILE *wb_ptr;
		char filename[100];
		if(barcode_status ==READY)
		{
//		sprintf(filename,"/tmp/h264SampleFrames/frame-%04d.yuv",id);;
//		g_print ("%s\n", filename);
//		wb_ptr = fopen(filename,"wb");  // w for write, b for binary

//		fwrite(info->data,info->size,1 ,wb_ptr); // write 10 bytes from our buffer
//		fclose(wb_ptr);
		barcode_status = FETCH;
		memcpy(rawdata, info->data, info->size);
		g_print("rawdata size = %d\n",info->size);
		barcode_status = FETCHED;
		}
	}
}


GstFlowReturn on_new_sample(GstElement* sink, gpointer data, guint64 id)
{
	GstSample *sample;
	GstBuffer* buffer;
	gboolean isDroppable, delta;
	GstSegment* segment;
	GstClockTime buf_pts;
	GstMapInfo info;
	/* Retrieve the buffer */
	//g_signal_emit_by_name (sink, "pull-sample", &sample);
	sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));

	if (sample) {
//		g_print ("%s %d\n", __FUNCTION__,id);
		buffer = gst_sample_get_buffer(sample);
		isDroppable = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_CORRUPTED) || GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY) ||
				(GST_BUFFER_FLAGS(buffer) == GST_BUFFER_FLAG_DISCONT) ||
				(GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) ||
				// drop if buffer contains header only and has invalid timestamp
				!GST_BUFFER_PTS_IS_VALID(buffer);
	
		if (!isDroppable) {
			delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
			//frame.flags = delta ? FRAME_FLAG_NONE : FRAME_FLAG_KEY_FRAME;

			// convert from segment timestamp to running time in live mode.
			
			segment = gst_sample_get_segment(sample);
			buf_pts = gst_segment_to_running_time(segment, GST_FORMAT_TIME, buffer->pts);
			if (!GST_CLOCK_TIME_IS_VALID(buf_pts)) {
				g_print ("Frame contains invalid PTS dropping the frame.\n");
			}

			info.data = NULL;
			if (!(gst_buffer_map(buffer, &info, GST_MAP_READ))) {
				g_print ("Frame contains invalid PTS dropping the frame.\n");
			}
		        //frame.trackId = trackid;
       			//frame.duration = 0;
   			//frame.version = FRAME_CURRENT_VERSION;
		        //frame.size = (UINT32) info.size;
 			//frame.frameData = (PBYTE) info.data;
			if(id < 10000)
			{
				dumptofile(&info , id);
			}
			if (info.data != NULL) {
				gst_buffer_unmap(buffer, &info);
			}
		}
		else
		{
			g_print ("Drop frame\n");
		}
		gst_sample_unref (sample);
		return GST_FLOW_OK;
	}
//	g_print ("sample failed %s %d\n", __FUNCTION__,id);

	return GST_FLOW_ERROR;
}

GstFlowReturn on_new_sample_video(GstElement* sink, gpointer data)
{
	GstFlowReturn ret = on_new_sample(sink, data, num_samples);
	num_samples++;
	return ret;
}


void barcodethread ()
{
	g_print ("run barcodethread\n");
	while ( 1 )
	{
		barcode_status = READY;
		sleep(2);
		if(barcode_status == FETCHED){
			g_print ("rawdata to Y800data\n");
     			Y800data = malloc(width * height ) ;

			for(int i = 0; i< 640*480 - 1; i++)
			{
				Y800data[i] = rawdata[i*2];		
			}
			g_print ("rawdata to Y800data finished\n");
			barcode_status = DECODE;
		}
		if(barcode_status == DECODE){
			g_print (" do DECODE\n");
		    	/* create a reader */
		   	 scanner = zbar_image_scanner_create();

		  	  /* configure the reader */
    			zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

			zbar_image_t *image = zbar_image_create();
			zbar_image_set_format(image, *(int*)"Y800");
			zbar_image_set_size(image, width, height);
			zbar_image_set_data(image, Y800data, 640 * 480, zbar_image_free_data);
			/* scan the image for barcodes */
			int n = zbar_scan_image(scanner, image);
    			/* extract results */
		   	const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
		   	 for(; symbol; symbol = zbar_symbol_next(symbol)) {
		       	 /* do something useful with results */
		        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
		        const char *data = zbar_symbol_get_data(symbol);
		        printf("decoded %s symbol \"%s\"\n",
 		              zbar_get_symbol_name(typ), data);
		    }

		    /* clean up */
		    zbar_image_destroy(image);
		    zbar_image_scanner_destroy(scanner);

		barcode_status = READY;
		}			

	}
}

main (int argc, char *argv[])
{

    /* obtain image data */
    void *raw = NULL;

     rawdata = malloc(width * height *2) ;
     

	CustomData data;
	GstElement *appsinkVideo = NULL, *appsinkAudio = NULL, *pipeline = NULL;
	GstBus* bus;
	GstMessage* msg;
	GError* error = NULL;
	gst_init(&argc, &argv);
	//"videotestsrc is-live=TRUE ! queue ! videoconvert ! video/x-raw,width=1280,height=720,framerate=30/1 ! " 
	//"v4l2src device=/dev/video0 ! video/x-raw, width=640, height=480, framerate=30/1 ! queue ! videoconvert ! "
	pipeline = gst_parse_launch(
	//	"v4l2src ! video/x-raw, width=640, height=480, framerate=30/1 ! queue ! videoconvert ! "
	//	"x264enc bframes=0 speed-preset=veryfast bitrate=512 byte-stream=TRUE tune=zerolatency ! " 
	//	"video/x-h264,stream-format=byte-stream,alignment=au,profile=baseline ! appsink sync=TRUE emit-signals=TRUE  name=appsink-video",
	"v4l2src ! video/x-raw, width=640, height=480, framerate=30/1 ! "
	"appsink sync=TRUE emit-signals=TRUE  name=appsink-video",
			&error);
	if (!pipeline) {
		g_print ("Parse error: %s\n", error->message);
		exit (1);
	}

	appsinkVideo = gst_bin_get_by_name(GST_BIN(pipeline), "appsink-video");
	appsinkAudio = gst_bin_get_by_name(GST_BIN(pipeline), "appsink-audio");

	system("mkdir -p /tmp/h264SampleFrames/");

	g_signal_connect(appsinkVideo, "new-sample", G_CALLBACK(on_new_sample_video), (gpointer) &data);

	pthread_create(&pbarcode,NULL,barcodethread,NULL);

	GstFlowReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_print("Unable to set the pipeline to the playing state.\n");
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
