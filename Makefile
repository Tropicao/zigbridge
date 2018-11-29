.PHONY: build

build:
	docker build -t zigbridge .

run:
	docker run -it -v /dev/ttyACM0:/dev/ttyACM0 --privileged -p 5818:5818 zigbridge ./builddir/zigbridge