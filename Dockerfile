# Stage 1: Build Environment
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .
COPY include/ ./include/
COPY src/ ./src/
COPY tests/ ./tests/
COPY benchmarks/ ./benchmarks/

# Build parallel Release
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel

# Run tests to ensure a broken build is never deployed
RUN cd build && ctest --output-on-failure

# Stage 2: Minimal Runtime Environment
FROM debian:bookworm-slim

RUN useradd -m -s /bin/bash appuser
WORKDIR /app

# Copy binary and configuration (Linux CMake outputs to build/HttpServer)
COPY --from=builder /app/build/HttpServer /app/HttpServer
COPY config.json /app/config.json

RUN chown -R appuser:appuser /app
USER appuser

EXPOSE 8080

CMD ["./HttpServer"]
