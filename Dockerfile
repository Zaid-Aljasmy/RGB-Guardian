FROM debian:bookworm

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Update system and install all required packages
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        g++ \
        make \
        libsdl2-dev \
        libsdl2-mixer-dev \
        libsdl2-image-dev \
        libsdl2-ttf-dev \
        && rm -rf /var/lib/apt/lists/*

# Working directory inside the container
WORKDIR /app

# Copy the project into the container
COPY . .

# Build using Makefile
RUN make

# Run the game
CMD ["./rgb_guardian"]

