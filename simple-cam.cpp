#include <iomanip>
#include <iostream>
#include <memory>
#include <sys/mman.h> // For mmap
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>


#include <libcamera/libcamera.h>

#include "event_loop.h"

#define TIMEOUT_SEC 3

using namespace libcamera;
static std::shared_ptr<Camera> camera;
static EventLoop loop;


static int j = 0;

/*
 * --------------------------------------------------------------------
 * Handles and processes the request
 * Heavy processing should not be done in this function and instead
 * done in a different thread
 */

static void processRequest(Request *request);

static void requestComplete(Request *request) {
	if (request->status() == Request::RequestCancelled) {
		return;
    }

	loop.callLater(std::bind(&processRequest, request));
}

static void processRequest(Request *request) {
	std::cout << std::endl
		  << "Request completed: " << request->toString() << std::endl;

	/*
     * meta data stuff
	 */
	const ControlList &requestMetadata = request->metadata();
	for (const auto &ctrl : requestMetadata) {
		const ControlId *id = controls::controls.at(ctrl.first);
		const ControlValue &value = ctrl.second;

		std::cout << "\t" << id->name() << " = " << value.toString()
			  << std::endl;
	}

	/*
     * Buffer info
	 */
	const Request::BufferMap &buffers = request->buffers();
	for (auto bufferPair : buffers) {
		// (Unused) Stream *stream = bufferPair.first;
		FrameBuffer *buffer = bufferPair.second;
		const FrameMetadata &metadata = buffer->metadata();
		std::cout << "\n";
		std::cout << buffer;

		/* Print some information about the buffer which has completed. */
		std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence
			  << " timestamp: " << metadata.timestamp
			  << " bytesused: ";

		unsigned int nplane = 0;
		for (const FrameMetadata::Plane &plane : metadata.planes())
		{
			std::cout << plane.bytesused;
			if (++nplane < metadata.planes().size()) {
				std::cout << "/";
            }
		}

		/*
         * Get image data
		 */

        for (size_t i = 0; i < buffer->planes().size(); ++i) {
            const FrameBuffer::Plane &plane = buffer->planes()[i];
            int fd = plane.fd.get();   // File descriptor for the plane
            size_t length = plane.length; // Length of the plane data
            off_t offset = plane.offset;  // Offset within the file descriptor

            // Map the plane memory
            void *mappedMemory = mmap(nullptr,
                    length,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd, offset);
            if (mappedMemory == MAP_FAILED) {
                perror("mmap failed");
                return;
            }

            std::cout << "Buffer planes: " << buffer->planes().size() << std::endl;
            for (size_t i = 0; i < buffer->planes().size(); ++i) {
                const FrameBuffer::Plane &plane = buffer->planes()[i];
                std::cout << "Plane " << i << ": fd=" << plane.fd.get()
                 << ", length=" << plane.length
                 << ", offset=" << plane.offset << std::endl;
            }	
            // Save the mapped memory to a file
            std::ofstream file("out/output_plane" + std::to_string(j) + ".raw", std::ios::binary);
            std::cout << length;
            file.write(static_cast<char *>(mappedMemory), length);
            file.close();

            std::cout << "Plane " << j << " saved to output_plane" << j << ".raw" << std::endl;
            j++;


            // Unmap the memory
            munmap(mappedMemory, length);
	    }


    }

    /* Re-queue the Request to the camera. */
    request->reuse(Request::ReuseBuffers);
    camera->queueRequest(request);
}

/*
 * Camera Naming.
 */
std::string cameraName(Camera *camera)
{
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

int main()
{
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
		std::cout << " - " << cameraName(camera.get()) << std::endl;

	/*
     * Get the camera thing
	 */
	if (cm->cameras().empty()) {
		std::cout << "No cameras were identified on the system."
			  << std::endl;
		cm->stop();
		return EXIT_FAILURE;
	}

	std::string cameraId = cm->cameras()[0]->id();
	camera = cm->get(cameraId);
	camera->acquire();


	/*
     * Config the camera
	 */
	std::unique_ptr<CameraConfiguration> config =
		camera->generateConfiguration( { StreamRole::Raw, StreamRole::Raw } );

	/*
     * Config the stream
	 */
	StreamConfiguration &streamConfig = config->at(0);
	std::cout << "Default Raw configuration is: "
		  << streamConfig.toString() << std::endl;
	streamConfig.bufferCount = 3;

	streamConfig.pixelFormat = streamConfig.pixelFormat.fromString("YUV420");
	
	std::cout << "pixelFormat" << streamConfig.pixelFormat.toString();


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
	std::cout << "Validated viewfinder configuration is: "
		  << streamConfig.toString() << std::endl;

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
			std::cerr << "Can't allocate buffers" << std::endl;
			return EXIT_FAILURE;
		}

		size_t allocated = allocator->buffers(cfg.stream()).size();
		std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
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
	std::vector<std::unique_ptr<Request>> requests;
	for (unsigned int i = 0; i < buffers.size(); ++i) {
		std::unique_ptr<Request> request = camera->createRequest();
		if (!request)
		{
			std::cerr << "Can't create request" << std::endl;
			return EXIT_FAILURE;
		}

		const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
		int ret = request->addBuffer(stream, buffer.get());
		if (ret < 0)
		{
			std::cerr << "Can't set buffer for request"
				  << std::endl;
			return EXIT_FAILURE;
		}

		/*
		 * Controls can be added to a request on a per frame basis.
		 */
		//ControlList &controls = request->controls();
		//controls.set(controls::AnalogueGain, 0.5);

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

	/*
	 * --------------------------------------------------------------------
	 * Start Capture
	 *
	 * In order to capture frames the Camera has to be started and
	 * Request queued to it. Enough Request to fill the Camera pipeline
	 * depth have to be queued before the Camera start delivering frames.
	 *
	 * For each delivered frame, the Slot connected to the
	 * Camera::requestCompleted Signal is called.
	 */
	camera->start();
	for (std::unique_ptr<Request> &request : requests) {

		camera->queueRequest(request.get());
	}

	/*
	 * --------------------------------------------------------------------
	 * Run an EventLoop
	 *
	 * In order to dispatch events received from the video devices, such
	 * as buffer completions, an event loop has to be run.
	 */
	loop.timeout(TIMEOUT_SEC);
	int ret = loop.exec();
	std::cout << "Capture ran for " << TIMEOUT_SEC << " seconds and "
		  << "stopped with exit status: " << ret << std::endl;

	/*
	 * --------------------------------------------------------------------
	 * Clean Up
	 *
	 * Stop the Camera, release resources and stop the CameraManager.
	 * libcamera has now released all resources it owned.
	 */
	camera->stop();
	allocator->free(stream);
	delete allocator;
	camera->release();
	camera.reset();
	cm->stop();

	return EXIT_SUCCESS;
}

