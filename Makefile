IMAGE_NAME = camera-things-dev
PLATFORM = linux/arm64

.PHONY: docker-build docker-run clean

build-docker:
	docker build --platform $(PLATFORM) -t $(IMAGE_NAME) .


run-docker: docker-build
	docker run --rm -it --platform $(PLATFORM) \
		-v "$(PWD):/workspace" \
		$(IMAGE_NAME) \
		/bin/bash

clean:
	rm -rf build 