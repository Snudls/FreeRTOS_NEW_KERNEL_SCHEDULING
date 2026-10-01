FROM debian:bookworm-slim

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      curl \
      bash \
      tini \
      make \
      git \
      gcc-arm-none-eabi \
      libnewlib-arm-none-eabi \
      gcc-arm-linux-gnueabi \
      qemu-user \
      qemu-system-arm \
      gdb-multiarch \
 && curl -L --output /usr/local/bin/gitlab-runner https://gitlab-runner-downloads.s3.amazonaws.com/latest/binaries/gitlab-runner-linux-amd64 \
 && chmod +x /usr/local/bin/gitlab-runner \
 && useradd --comment 'GitLab Runner' --create-home gitlab-runner --shell /bin/bash \
 && gitlab-runner install --user=gitlab-runner --working-directory=/home/gitlab-runner \
 && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["bash"]