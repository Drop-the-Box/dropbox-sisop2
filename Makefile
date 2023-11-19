ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(ARGS):;@:)
MAKEFLAGS += --silent
$(eval TEST_TYPE := $(shell [[ -z "$(ARGS)" ]] && echo "null" || echo "$(ARGS)"))

SERVER_CMD = 'g++ -g src/server/**/*.cpp src/server/main.cpp -o ./server && ./server'
CLIENT_CMD = 'g++ -g src/client/**/*.cpp src/client/main.cpp -o ./client && ./client'

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
	@ make build

.PHONY: build
build-image:  ### builds the base image 
	@ docker build -t dropthebox -f dropthebox.dockerfile .

.PHONY: run-server 
run-server: build-image ### build and run the server app
	@ docker run --name dtb-server -it -p 6999:6999 --rm dropthebox bash -c ${SERVER_CMD}


.PHONY: debug-server
debug-server: build-image ### build and run the server app
	@ docker run --name dtb-server -it -p 6999:6999 --rm dropthebox bash -c "++ -g src/server/**/*.cpp src/server/main.cpp -o ./server && gdb ./server"

.PHONY: run-client
run-client: build-image   ### build and run the client app
	@ docker run -it --rm dropthebox bash -c "g++ -g ${CLIENT_SRC_FILES} -o ./client && ./client"

.PHONY: kill-server
kill-server:
	@ docker kill dtb-server
