import numpy as np
import av
import threading
import sys
import time
import signal
import os
import gi
gi.require_version("Gst", "1.0")
from gi.repository import Gst, GLib
def gstream(folder, frame_limit, dual):
  
    # Initialize GStreamer
    Gst.init(None)
    time.sleep(3)
    print("GStreamer initializing...")
    
    # GStreamer pipelines to capture video frames and portentially display with Wayland
    single_pipeline_description = """
    v4l2src device=/dev/video0 ! videoconvert !
    video/x-h265, framerate=60/1, width=640, height=512, format=GRAY_16LE !
    appsink name=sink 
    """
    
    dual_pipeline_description = """
    v4l2src device=/dev/video0 ! videoconvert ! 
    video/x-raw, format=I420, width=640, height=512 ! tee name=t
    t. ! queue max-size-buffers=10 ! waylandsink fullscreen=true
    t. ! queue max-size-buffers=10 ! appsink name=sink 
    """
    
    # Create GStreamer pipeline
    if dual.lower() == "true" or dual.lower() == "t":
        pipeline = Gst.parse_launch(dual_pipeline_description)
    else:
        pipeline = Gst.parse_launch(single_pipeline_description)
    appsink = pipeline.get_by_name("sink")
    
    # Configure appsink properties
    appsink.set_property("emit-signals", True)
    appsink.set_property("sync", False)
    appsink.set_property("max-buffers", 1)
    appsink.set_property("drop", True)
    
    # Start the pipeline
    pipeline.set_state(Gst.State.PLAYING)
    print("pipeline started\n")
    # Set variables
    folder = folder
    os.makedirs(folder, exist_ok=True)
    global frame_count
    frame_count = 0
    stop_event = threading.Event()
    global main_loop
    main_loop = None
    frame_buff = []
    
    def on_new_sample(sink):
    
        global frame_count
        sample = sink.emit("pull-sample")
        
        if not sample:
            return Gst.FlowReturn.ERROR
            
        buf = sample.get_buffer()
        caps = sample.get_caps()
        structure = caps.get_structure(0)
        width = structure.get_value("width")
        height = structure.get_value("height")
        
        # Extract the buffer data into a numpy array
        success, map_info = buf.map(Gst.MapFlags.READ)
        if not success:
            return Gst.FlowReturn.ERROR
            
        # Convert buffer to numpy array
        frame_data = np.frombuffer(map_info.data, np.uint8).reshape(
            (height * 3 // 2, width)
        )
        buf.unmap(map_info)
        
        # Process the frame
        frame_buff.append(frame_data)
        frame_count += 1
        
        if frame_count >= frame_limit:
            stop_event.set()
            pipeline.set_state(Gst.State.NULL)
            end_time = time.perf_counter()
            if main_loop:
                main_loop.quit()
                
        return Gst.FlowReturn.OK
        
    # Connect the appsink to the new-sample signal
    appsink.connect("new-sample", on_new_sample)
    
    # Start the pipeline
    pipeline.set_state(Gst.State.PLAYING)
    start_time = time.perf_counter()
    print("pipeline 2 started")
    # Convert from I420 to h264
    def write_video_pyav(frames, output_file):
        container = av.open(output_file, mode="w")
        stream = container.add_stream("h264", rate=60)
        stream.pix_fmt = "yuv420p"
        stream.bit_rate = 4000000
        for frame in frames:
            frame_av = av.VideoFrame.from_ndarray(frame, format="yuv420p")
            packet = stream.encode(frame_av)
            if packet:
                container.mux(packet)
        container.close()
        
    # Run GStreamer's main loop in a separate thread
    def gstreamer_loop():
    
        global main_loop
        main_loop = GLib.MainLoop()
        
        try:
            main_loop.run()
            
        except Exception as e:
            print(f"Error in GStreamer loop: {e}")
            
        finally:
            print("Gstreamer loop stopped")
            
    # Run GStreamer main loop in separate thread
    gstreamer_thread = threading.Thread(target=gstreamer_loop)
    gstreamer_thread.start()
    
    # Graceful shutdown handler
    def signal_handler(sig, frame):
        """Handles termination signals (Ctr+C) and stops the pipeline safely."""
        
        print("\nStopping pipeline gracefully...")
        
        stop_event.set()
        pipeline.set_state(Gst.State.NULL)
        
        if main_loop:
            main_loop.quit()
            
        gstreamer_thread.join()
        print("Pipeline stopped successfully. Exiting...")
        
    # Register the signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    # Keep the main thread alive until stopped
    try:
        while not stop_event.is_set():
            pass
    except KeyboardInterrupt:
        signal_handler(None, None)
    finally:
        end_time = time.perf_counter()
        save_path = os.path.join(folder, "output.mp4")
        
        write_video_pyav(frame_buff, save_path)
        print(f"Saved {frame_count} frames as a video.")
        
        execution_time = end_time - start_time
        print(f"Execution time: {execution_time} seconds")
        os._exit(0)
        
if __name__ == "__main__":
    args = sys.argv[1:]
    f = args[0]  # Folder
    f_l = args[1]  # Frame limit
    d = args[2]  # Dual output (True or False)
    gstream(f, int(f_l), d)
