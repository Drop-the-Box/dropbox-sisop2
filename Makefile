# define standard colors
ifneq (,$(findstring xterm,${TERM}))
	BLACK        := $(shell tput -Txterm setaf 0)
	RED          := $(shell tput -Txterm setaf 1)
	GREEN        := $(shell tput -Txterm setaf 2)
	YELLOW       := $(shell tput -Txterm setaf 3)
	LIGHTPURPLE  := $(shell tput -Txterm setaf 4)
	PURPLE       := $(shell tput -Txterm setaf 5)
	BLUE         := $(shell tput -Txterm setaf 6)
	WHITE        := $(shell tput -Txterm setaf 7)
	RESET := $(shell tput -Txterm sgr0)
else
	BLACK        := ""
	RED          := ""
	GREEN        := ""
	YELLOW       := ""
	LIGHTPURPLE  := ""
	PURPLE       := ""
	BLUE         := ""
	WHITE        := ""
	RESET        := ""
endif

# set target color
TARGET_COLOR := $(BLUE)

ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(ARGS):;@:)
MAKEFLAGS += --silent
$(eval TEST_TYPE := $(shell [[ -z "$(ARGS)" ]] && echo "null" || echo "$(ARGS)"))


# HELP COMMANDS
.PHONY: help
help: ### show this help
	@echo 'Usage: make ${TARGET_COLOR}[target]${RESET} ${RED}[option]${RESET}'
	@echo '--- Example: make ${TARGET_COLOR}run_client${RESET} ${RED}127.0.0.1:9090${RESET}'
	@echo ''
	@echo 'Available targets:'
	@echo ''
	@grep -E '^[a-zA-Z_0-9%-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "${TARGET_COLOR}%-30s${RESET} ${YELLOW}%s${RESET}\n", $$1, $$2}'


.PHONY: init
init:  ### Install dependencies and start application
	@ which docker || (echo "Please, install docker\n"\
	" -> MacOS: https://docs.docker.com/docker-for-mac/install/ \n"\
	" -> Ubuntu: https://docs.docker.com/install/linux/docker-ce/ubuntu/ \n"\
	" -> Fedora: https://docs.docker.com/install/linux/docker-ce/fedora/ "; exit 1;)
	@ -docker network create dtb
	@ make build

.PHONY: build
build-image:  ### builds the base image 
	@ docker-compose build

############ SERVER COMMANDS #############

.PHONY: run-server
run-server: ### build and run the server app
	@ docker-compose up -d server && docker attach dtb-server

.PHONY: run-server-native
run-server-native:  ### build and run the server app on host machine
	@ mkdir -p ./bin
	@ bash -c ./scripts/run_server.sh

.PHONY: kill-server
kill-server:  ### kills the server service
	@ docker-compose kill server

############ CLIENT COMMANDS #############

.PHONY: run-client
run-client:   ### build and run the client app
	@ docker-compose up -d client && docker attach drop-the-box-client-1

.PHONY: run-client-native $(ARGS)
run-client-native:  ### build and run the client app on host machine
	@ mkdir -p ./bin
	@ bash -c ./scripts/run_client.sh $(ARGS)

.PHONY: kill-client
kill-client:  ### kills the client service
	@ docker-compose kill client
