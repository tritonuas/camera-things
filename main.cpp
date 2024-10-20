#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/libcamera.h>

static std::shared_ptr<libcamera::Camera> camera;

/*
using namespace libcamera;
using namespace std::chrono_literals;

*/
int main()
{
	std::cout << "test";
	/*
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
	*/
	/*
	 * Note that `camera` may not compare equal to `cameras[0]`.
	 * In fact, it might simply be a `nullptr`, as the particular
	 * device might have disappeared (and reappeared) in the meantime.
	 */
	/*
	camera->acquire();
	std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );
	StreamConfiguration &streamConfig = config->at(0);
	std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

	std::cout.flush();
	*/
}


