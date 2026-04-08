FROM gcc:latest

WORKDIR /app

COPY projects/embedded-linux-qemu-labs-with-templates/toolchain/hello.c .
RUN gcc -o hello hello.c

CMD ["echo", "Hello, World!"]

