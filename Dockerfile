ARG TARGETPLATFORM
ARG TARGETARCH
# Ensuring GLIBCXX_3.4.29 as available in Lambda provided.al2023
FROM public.ecr.aws/sam/build-provided.al2023:1.132

RUN mkdir src
WORKDIR /src
RUN dnf install cmake3 make gcc libcurl-devel openssl-devel git -y

RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp
RUN git clone https://github.com/awslabs/aws-lambda-cpp.git

RUN cd aws-lambda-cpp && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local && make -j && make install
RUN cd aws-sdk-cpp && mkdir build && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_ONLY="core"
RUN cd aws-sdk-cpp/build && cmake --build . --config Release && cmake --install . --config Release

ARG DUCKDB_VERSION=1.1.3

# Download, for example, libduckdb-linux-aarch64.zip or libduckdb-linux-amd64.zip
RUN curl -L https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/libduckdb-linux-$(uname -m | sed 's/x86_/amd/').zip > /src/libduckdb.zip

RUN mkdir -p /app/build
WORKDIR /app/build
RUN unzip /src/libduckdb.zip && mv libduckdb* /usr/local/lib64/ && mv duckdb.h* /usr/local/include/
RUN echo $TARGETARCH $TARGETPLATFORM
COPY CMakeLists.txt /app/
COPY *.h /app/
COPY *.cpp /app/
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/var/task
RUN make -j
