# isi Dockerfile
FROM ubuntu:20.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    fuse3 libfuse3-dev gcc make nano

COPY antink.c /app/antink.c
WORKDIR /app
RUN gcc -Wall antink.c `pkg-config fuse3 --cflags --libs` -o antink

CMD ["./antink", "/antink_mount"]
