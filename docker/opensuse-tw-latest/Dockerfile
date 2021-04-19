FROM opensuse/tumbleweed

RUN mkdir -p /scripts
COPY get-dependencies_linux.sh /scripts
RUN chmod +x /scripts/get-dependencies_linux.sh
RUN /scripts/get-dependencies_linux.sh opensuse-tumbleweed
WORKDIR /code
CMD ["sh"]
