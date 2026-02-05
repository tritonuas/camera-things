#include "simple-cam.hpp"
#include <unistd.h>
#include <loguru/loguru.hpp>

using namespace libcamera;

namespace RPICam {


    //static void requestComplete(Request *request);

    /*
     * --------------------------------------------------------------------
     * Handles and processes the request
     * Heavy processing should not be done in this function and instead
     * done in a different thread
     */

    /*
     * Processes the camera frame
     * Option to 
     * Sends image to OBC
     * Save image to disk
     * Process image metadata (the important ones) TODO
     * Save image metadata (TODO)
     * Print image petadata
     */
    void saveData(Request *request) {

        //TODO: determine if this mutex is no longer needed
        mutex.lock();

        /*
         * Saves the settings of the image (for locking the camera controls)
         */
        if (save_settings) {
            copy_controls(&controlList_copy, &request->metadata(), 0);
            save_settings = 0;
        }


        /*
         * Prints all the metadata of the image, if enabled
         */
        if (print_metadata || 1) {
            const ControlList &requestMetadata = request->metadata();
            for (const auto &ctrl : requestMetadata) {
                const ControlId *id = controls::controls.at(ctrl.first);
                const ControlValue &value = ctrl.second;
                LOG_F(INFO, "\t%s = %s", id->name().c_str(), value.toString().c_str());
            }
        }

        /*
         * Processes the buffers
         * Note the "planes" are the images, contained in "buffers", contained
         * in "requests"
         */
        const Request::BufferMap &buffers = request->buffers();
        for (auto bufferPair : buffers) {

            /*
             * determines if image is being requested
             * also will send GPS+attitude data if requested
             */
            send_count_mutex.lock();
            // If we have a pending request, mark send_current as true
            if (send_count >= 1) {
                send_current = 1;
                // Decrement count only once per frame 

                // Ideally, we check if this is plane 0 before decrementing, or decrement outside loop since its buggy af 
                send_count--; 
            }
            send_count_mutex.unlock();

            if (mavlink_enabled) {
                Mavlink::send_both_messages();
            }

            // (Unused) Stream *stream = bufferPair.first;
            FrameBuffer *buffer = bufferPair.second;

            /*
             * prints the metadata, if enabled
             */
            if (debug) {
                const FrameMetadata &metadata = buffer->metadata();
                LOG_F(INFO, " seq: %06d timestamp: %lu bytesused: ", metadata.sequence, metadata.timestamp);
                unsigned int nplane = 0;
                for (const FrameMetadata::Plane &plane : metadata.planes()) {
                    LOG_F(INFO, "%d", plane.bytesused);
                    if (++nplane < metadata.planes().size()) {
                        LOG_F(INFO, "/");
                    }
                }

                LOG_F(INFO, "Buffer planes: %lu", buffer->planes().size());
                for (size_t i = 0; i < buffer->planes().size(); ++i) {
                    const FrameBuffer::Plane &plane = buffer->planes()[i];
                    LOG_F(INFO, "Plane %lu: fd=%d, length=%u, offset=%u", 
                        i, plane.fd.get(), plane.length, plane.offset);
                }
            }


            /*
             * Prepares to send the images
             * YUV420 format means there are 3 planes that the camera spits
             * the output into
             * maps a pointer to each of these regions, which is then 
             * sent/saved 
             */
            for (size_t i = 0; i < buffer->planes().size(); ++i) {
                const FrameBuffer::Plane &plane = buffer->planes()[i];

                // Get plane data
                int fd = plane.fd.get();
                size_t length = plane.length;
                off_t offset = plane.offset;

                // --- Page-Alignment Fix ---
                // We must mmap at a page-aligned offset
                long page_size = sysconf(_SC_PAGE_SIZE);
                off_t mmap_offset = (offset / page_size) * page_size;
                size_t mmap_length = length + (offset - mmap_offset);

                // Map the memory
                void *mappedMemory = mmap(nullptr,
                        mmap_length,  // Use new calculated length
                        PROT_READ,    // Read-only
                        MAP_SHARED,
                        fd,
                        mmap_offset); // Use new calculated offset

                if (mappedMemory == MAP_FAILED) {
                    LOG_F(ERROR, "mmap failed: %s", strerror(errno));
                    // DO NOT RETURN. Just stop processing planes for this buffer.
                    // The request will still be re-queued later.
                    break; 
                }

                // Get the pointer to the *actual* data start
                unsigned char *data = static_cast<unsigned char *>(mappedMemory) + (offset - mmap_offset);


                if (send_current || save_to_file) {
                    if (save_to_file) {
                        // ... (your file saving logic)
                        // Note: use 'data' and 'length' here, not 'mappedMemory' or 'mmap_length'
                        std::ofstream file("out/output_plane" + std::to_string(image_counter) + ".raw", std::ios::binary);
                        file.write(reinterpret_cast<const char *>(data), length);
                        file.close();

                        if (debug) {
                            LOG_F(INFO, "Saved: output_plane%d.raw", image_counter);
                            image_counter++;
                        }
                    }
                    if (send_to_obc && send_current) { 
                        if (debug) {
                            LOG_F(1, "Sending to OBC: %lu bytes", length);
                        }
                        OBCPort::send_image(data, length);
                    }
                }

                // Unmap using the original mapped address and mmap_length
                munmap(mappedMemory, mmap_length);
            }
            send_current = 0;
        }

        if (debug) {
            LOG_F(INFO, "Request Finished: %s", request->toString().c_str());
        }

        /* Re-queue the Request to the camera. */
        request->reuse(Request::ReuseBuffers);

        /*
         * If the controls are locked, copy the controls into the new request
         */
        if (settings_locked) {
            ControlList &controls = request->controls();
            copy_controls(&controls, &controlList_copy, 0);
        }

        camera->queueRequest(request);
        mutex.unlock();
    }

    /*
     * Enqueues the fucntion into the sending queue
     */
    void processRequest(Request *request) {
        if (debug) {
            LOG_F(INFO, "Request Queued: %s", request->toString().c_str());
        }

        funQ.push_back_function(saveData, request);
    }



    /*
     * Honestly I have no idea why this and the above function are here, they
     * essentially enqueue a function that enqueues another function, but it works
     * and I am too scared to touch it as this was copy pasted from the default 
     * libcamera template and I have no idea what that loop thingy does but I have 
     * a tiny bit of idea what the function queue thing that "I" (deekseek)
     * wrote does
     *
     * So if someone smarter than me wants to fix this bit, then fix it.
     */
    void requestComplete(Request *request) {
        if (request->status() == Request::RequestCancelled) {
            return;
        }

        loop.callLater(std::bind(&processRequest, request));
    }

    /*
     * Camera Naming.
     * TODO: see if this should (can) be removed
     */
    std::string cameraName(Camera *camera) {
        const ControlList &props = camera->properties();
        std::string name;

        const auto &location = props.get(properties::Location);
        if (location) {
            switch (*location) {
                case properties::CameraLocationFront:
                    name = "Internal front camera";
                    break;
                case properties::CameraLocationBack:
                    name = "Internal back camera";
                    break;
                case properties::CameraLocationExternal:
                    name = "External camera";
                    const auto &model = props.get(properties::Model);
                    if (model)
                        name = " '" + *model + "'";
                    break;
            }
        }

        name += " (" + camera->id() + ")";

        return name;
    }

    /*
     * Called when an image is sent
     * Counts number of images requested incase takng pictures lags behind
     */
    void send_next_image() {
        send_count_mutex.lock();
        send_count++;
        send_count_mutex.unlock();
    }

    /**
     * Creates the camera and configs it
     */
    void start() {

        /*
         * Create a Camera Manager.
         */
        std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
        cm->start();

        /*
         * Just as a test, generate names of the Cameras registered in the
         * system, and list them.
         */
        for (auto const &camera : cm->cameras())
            LOG_F(INFO, " - %s", cameraName(camera.get()).c_str());

        /*
         * Get the camera thing
         */
        if (cm->cameras().empty()) {
            LOG_F(INFO, "No cameras were identified on the system.");
            cm->stop();
            return;
        }

        std::string cameraId = cm->cameras()[0]->id();
        camera = cm->get(cameraId);
        camera->acquire();


        /*
         * Config the camera
         */
        std::unique_ptr<CameraConfiguration> config =
            camera->generateConfiguration( { StreamRole::Viewfinder } );

        /*
         * Config the stream
         * TODO: make it work with global shutter camera
         */
        StreamConfiguration &streamConfig = config->at(0);
        streamConfig.size = { 1456, 1088 };
        LOG_F(INFO, "Default Raw configuration is: %s", streamConfig.toString().c_str());
        streamConfig.bufferCount = BUFFER_COUNT;

        streamConfig.pixelFormat = streamConfig.pixelFormat.fromString("YUV420");
        //streamConfig.pixelFormat = streamConfig.pixelFormat.fromString("RGB888");

        LOG_F(INFO, "pixelFormat: %s", streamConfig.pixelFormat.toString().c_str());


        //SensorConfiguration sensorConfig;

        /*
         * Each StreamConfiguration parameter which is part of a
         * CameraConfiguration can be independently modified by the
         * application.
         *
         * In order to validate the modified parameter, the CameraConfiguration
         * should be validated -before- the CameraConfiguration gets applied
         * to the Camera.
         *
         * The CameraConfiguration validation process adjusts each
         * StreamConfiguration to a valid value.
         */

        /*
         * The Camera configuration procedure fails with invalid parameters.
         */

        /*
         * Validating a CameraConfiguration -before- applying it will adjust it
         * to a valid configuration which is as close as possible to the one
         * requested.
         */
        config->validate();
        LOG_F(INFO, "Validated viewfinder configuration is: %s", streamConfig.toString().c_str());

        /*
         * Once we have a validated configuration, we can apply it to the
         * Camera.
         */
        camera->configure(config.get());

        /*
         * --------------------------------------------------------------------
         * Buffer Allocation
         *
         * Now that a camera has been configured, it knows all about its
         * Streams sizes and formats. The captured images need to be stored in
         * framebuffers which can either be provided by the application to the
         * library, or allocated in the Camera and exposed to the application
         * by libcamera.
         *
         * An application may decide to allocate framebuffers from elsewhere,
         * for example in memory allocated by the display driver that will
         * render the captured frames. The application will provide them to
         * libcamera by constructing FrameBuffer instances to capture images
         * directly into.
         *
         * Alternatively libcamera can help the application by exporting
         * buffers allocated in the Camera using a FrameBufferAllocator
         * instance and referencing a configured Camera to determine the
         * appropriate buffer size and types to create.
         */

        FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);
        for (StreamConfiguration &cfg : *config) {
            int ret = allocator->allocate(cfg.stream());
            if (ret < 0) {
                LOG_F(ERROR, "Can't allocate buffers");
                return;
                //return EXIT_FAILURE;
            }

            size_t allocated = allocator->buffers(cfg.stream()).size();
            LOG_F(INFO, "Allocated %lu buffers for stream", allocated);
        }

        /*
         * --------------------------------------------------------------------
         * Frame Capture
         *
         * libcamera frames capture model is based on the 'Request' concept.
         * For each frame a Request has to be queued to the Camera.
         *
         * A Request refers to (at least one) Stream for which a Buffer that
         * will be filled with image data shall be added to the Request.
         *
         * A Request is associated with a list of Controls, which are tunable
         * parameters (similar to v4l2_controls) that have to be applied to
         * the image.
         *
         * Once a request completes, all its buffers will contain image data
         * that applications can access and for each of them a list of metadata
         * properties that reports the capture parameters applied to the image.
         */




        Stream *stream = streamConfig.stream();
        const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);

        //Array of the requests
        const std::unique_ptr<Request> request_arr[buffers.size()];

        for (unsigned int i = 0; i < buffers.size(); ++i) {
            std::unique_ptr<Request> request = camera->createRequest();
            if (!request)
            {
                LOG_F(ERROR, "Can't create request");
                return;

            }

            const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
            int ret = request->addBuffer(stream, buffer.get());
            if (ret < 0)
            {
                LOG_F(ERROR, "Can't set buffer for request");
                return;
            }


            requests.push_back(std::move(request));
        }

        /*
         * --------------------------------------------------------------------
         * Signal&Slots
         *
         * libcamera uses a Signal&Slot based system to connect events to
         * callback operations meant to handle them, inspired by the QT graphic
         * toolkit.
         *
         * Signals are events 'emitted' by a class instance.
         * Slots are callbacks that can be 'connected' to a Signal.
         *
         * A Camera exposes Signals, to report the completion of a Request and
         * the completion of a Buffer part of a Request to support partial
         * Request completions.
         *
         * In order to receive the notification for request completions,
         * applications shall connecte a Slot to the Camera 'requestCompleted'
         * Signal before the camera is started.
         */
        camera->requestCompleted.connect(requestComplete);

        send_count = 0;

        /*
         * --------------------------------------------------------------------
         * Start Capture
         *
         * For each delivered frame, the Slot connected to the
         * Camera::requestCompleted Signal is called.
         */
        camera->start();
        for (std::unique_ptr<Request> &request : requests) {
            LOG_F(INFO, "Added Request: %s", request->toString().c_str());
            camera->queueRequest(request.get());
        }

        /*
         * Run an EventLoop
         */
        loop.timeout(TIMEOUT_SEC);
        //int ret = loop.exec();
        std::thread cameraThread(&EventLoop::exec, &loop);
        std::vector<std::unique_ptr<Request>> requests;
        //cameraThread.detach();
        cameraThread.join();

        /*
         * Clean Up
         */
        camera->stop();
        allocator->free(stream);
        delete allocator;
        camera->release();
        camera.reset();
        cm->stop();

    }

    void stop_taking_pictures() {
        loop.exit(0);
    }

    /*
     * Call to lock the camera controls to that of the next processed image
     */
    void lock_settings() {
        save_settings = 1;
        settings_locked = 1;

    }

    /*
     * Unlock the camera controls
     */
    void unlock_settings() {
        save_settings = 0;
        settings_locked = 0;
    }

    /**
     * Makes a deep copy of the metadata (controls)
     * @ all set to 1 if everything should be copied, 0 to only copy inputs
     * Controls can be added to a request on a per frame basis.
     * ControlList &controls = request->controls(); 
     * controls.set(controls::AnalogueGain, 0.5);
     */
    void copy_controls(ControlList *list_out, ControlList *list_in, int all) {
        list_out->clear();
        int skip = 0;
        for (const auto &ctrl : *list_in) {
            const ControlId *id = controls::controls.at(ctrl.first);
            const ControlValue &value = ctrl.second;


            if (all || id->isInput()) {
                for (const uint invalid_id : invalid_controls) {
                    if (invalid_id == id->id()) {
                        skip = 1;
                    }
                }
                if (!skip) {
                    list_out->set(id->id(), ControlValue(value));
                }
                skip = 0;
                if (print_metadata) {
                    LOG_F(INFO, "COPYING: \t\t\t%s ID: %d = %s", id->name().c_str(), id->id(), value.toString().c_str());
                }
            }
        }

    }
};
