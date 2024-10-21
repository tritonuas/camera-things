#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/libcamera.h>

static std::shared_ptr<libcamera::Camera> camera;


using namespace libcamera;
using namespace std::chrono_literals;


int main()
{
	std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
	cm->start();
	std::cout << "test";
	for (auto const &camera : cm->cameras())
		std::cout << camera->id() << std::endl;

	auto cameras = cm->cameras();
	if (cameras.empty()) {
	    std::cout << "No cameras were identified on the system."
		      << std::endl;
	    cm->stop();
	    return EXIT_FAILURE;
	}

	std::string cameraId = cameras[0]->id();

	auto camera = cm->get(cameraId);
	/*
	 * Note that `camera` may not compare equal to `cameras[0]`.
	 * In fact, it might simply be a `nullptr`, as the particular
	 * device might have disappeared (and reappeared) in the meantime.
	 */
	camera->acquire();
	std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );
	StreamConfiguration &streamConfig = config->at(0);
	std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

	streamConfig.size.width = 4056;
	streamConfig.size.height = 3040;
	streamConfig.bufferCount = 2;


	config->validate();
	std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;

	camera->configure(config.get());


	
	FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);

	for (StreamConfiguration &cfg : *config) {
	    int ret = allocator->allocate(cfg.stream());
	    if (ret < 0) {
		std::cerr << "Can't allocate buffers" << std::endl;
		return -ENOMEM;
	    }

	    size_t allocated = allocator->buffers(cfg.stream()).size();
	    std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
	}

	Stream *stream = streamConfig.stream();
	const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);
	std::vector<std::unique_ptr<Request>> requests;


	for (unsigned int i = 0; i < buffers.size(); ++i) {
	    std::unique_ptr<Request> request = camera->createRequest();
	    if (!request)
	    {
		std::cerr << "Can't create request" << std::endl;
		return -ENOMEM;
	    }

	    const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
	    int ret = request->addBuffer(stream, buffer.get());
	    if (ret < 0)
	    {
		std::cerr << "Can't set buffer for request"
		      << std::endl;
		return ret;
	    }

	    requests.push_back(std::move(request));
	}
	std::cout.flush();


}


