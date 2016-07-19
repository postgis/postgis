DOCKER=docker
REGISTRY=docker.kbt.io
NAME=$(REGISTRY)/postgis/build-test

all: trisquel

push: trisquel-push

trisquel:
	$(DOCKER) build -t $(NAME):trisquel - < Dockerfile.trisquel

trisquel-push:
	$(DOCKER) push $(NAME):trisquel
