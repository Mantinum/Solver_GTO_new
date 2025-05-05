FROM ubuntu:22.04

# Set frontend to noninteractive to avoid prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    g++-12 \
    cmake \
    python3-dev \
    libpybind11-dev \
    libspdlog-dev \
    catch2 \
    clang-format \
    git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Default command (can be overridden)
# Example: docker run <image> cmake -B build && cmake --build build -j
# Or simply: docker run <image> bash
CMD ["bash"] 