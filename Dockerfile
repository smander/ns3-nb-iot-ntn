FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ python3 python3-dev \
    git libgsl-dev libsqlite3-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ns3
COPY . /ns3/

RUN CXXFLAGS="-Wno-reorder" python3 waf configure --enable-examples -d optimized
RUN python3 waf build -j$(nproc)

VOLUME /output
ENV LD_LIBRARY_PATH=/ns3/build/lib
ENTRYPOINT ["/bin/bash", "-c"]
CMD ["cd /ns3 && ./build/src/lte/examples/nsns-3.32-nb-iot-ntn-ids-scenario-optimized --simTime=300s --output=/output/dataset.csv"]
