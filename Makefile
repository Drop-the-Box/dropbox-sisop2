ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(ARGS):;@:)
MAKEFLAGS += --silent
$(eval TEST_TYPE := $(shell [[ -z "$(ARGS)" ]] && echo "null" || echo "$(ARGS)"))

# HELP COMMANDS
.PHONY: help
help: ### show this help
	@echo 'usage: make [target] [option]'
	@echo ''
	@echo 'Available targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | sed 's/:.*##/#/' | column -t -c 2 -s '#'

.PHONY: init
init:  ### Install dependencies and start application
	@ which docker || (echo "Please, install docker\n"\
	" -> MacOS: https://docs.docker.com/docker-for-mac/install/ \n"\
	" -> Ubuntu: https://docs.docker.com/install/linux/docker-ce/ubuntu/ \n"\
	" -> Fedora: https://docs.docker.com/install/linux/docker-ce/fedora/ "; exit 1;)
	@ make run

.PHONY: build
build-image:  ### builds the base image 
	@ docker build -t dropthebox -f dropthebox.dockerfile .

.PHONY: run-server 
run-server: build-image ### build and run the server app
	@ docker run -it --rm dropthebox sh -c "g++ src/server/main.cpp -o ./dropthebox-server && ./dropthebox-server"

.PHONY: run-client
run-client: build-image   ### build and run the client app
	@ docker run -it --rm dropthebox sh -c "g++ src/client/main.cpp -o ./dropthebox-client && ./dropthebox-client"
