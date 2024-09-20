# Cuwacnu 
## The libtorch implementation

## Install the requirements
### Install docker
go to (https://docker.com)

### Download libtorch
Go to (https://pytorch.org/) you'll find the Download setting, select the latest stable, linux, Libtorch, OpenSSL, Libcurl C++, CUDA 12.1 (these instructions consider you have a GPU) 

For the most resent c++ standar:    (https://download.pytorch.org/libtorch/cu121/libtorch-cxx11-abi-shared-with-deps-2.1.2%2Bcu121.zip)
for old c++ standar:            (https://download.pytorch.org/libtorch/cu121/libtorch-shared-with-deps-2.1.2%2Bcu121.zip)
what ever you select please extract libtorch into <external/libtorch> folder 

### Download CUDNN
go to (https://developer.nvidia.com/rdp/cudnn-download) set up an account with nvidia 
download cudnn for debian 11 (https://developer.nvidia.com/downloads/compute/cudnn/secure/8.9.7/local_installers/12.x/cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb/)

put the file <cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb> in <external/cudnn>

# Set up the Docker 

### Pull the debian 11 (bullseye) base
```cmd
docker pull debian:11
```

### Start the container
```cmd
docker run --name unnamed_tao --gpus all -it -v $PWD//data:/data -v $PWD//external:/external -v $PWD//src:/src debian:11
```

# Configure the Container
For now on, the commands will be inside the docker linux container: 

### Install some initial requirements
```bash
apt-get update && apt-get install -y --no-install-recommends build-essential gnupg2 curl ca-certificates valgrind libssl-dev \
rm -rf /var/lib/apt/lists/*

apt install locales --no-install-recommends
# (default to) en_US.UTF-8
```

### Install curl dev
Try this
```bash
apt install -y --no-install-recommends curl libcurl4-openssl-dev
# (alternative) install from source, see instruction bellow
```
We require curl 7.86.0 or later since we use curl for websocket.

Websocket in CURL is experiemtal and for this we require to install it from source and explicitly define websocket support. 
Try installing from soruce:
```bash
# (change the number to the latest)
cd /external
curl -LO https://curl.se/download/curl-8.9.1.tar.gz 
apt remove curl libcurl4 libcurl3-gnutls
tar -xzf curl-8.9.1.tar.gz 
cd curl-8.9.1
./configure --with-openssl --enable-websockets
make
make install
ldconfig
source ~/.bashrc
# (Faced the problem of having multiple curl versions installed, so be aware of this while trying to reproduce.)
```

Be sure to do "curl --version" and verify that websocket (WS, WSS) is listed under "protocols". 


### Add the NVIDIA package repositories
```bash
curl -fsSL https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/3bf863cc.pub | gpg --dearmor -o /usr/share/keyrings/nvidia-cuda-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/nvidia-cuda-archive-keyring.gpg] https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/ /" > /etc/apt/sources.list.d/cuda.list
```

### Install CUDA 12.1
```bash
apt-get update && apt-get install -y --no-install-recommends cuda-toolkit-12-1 libcudnn8
rm -rf /var/lib/apt/lists/*
echo 'export CUDA_VERSION=12.1' >> ~/.bashrc
echo 'export PATH=/usr/local/cuda-$CUDA_VERSION/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-$CUDA_VERSION/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
. ~/.bashrc
```

### Valdiate the CUDA installation 
```bash
nvcc --version
```

### Install cuDNN 8 runtime
```bash
cd /external/cudnn/
dpkg -i cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb
cp /var/cudnn-local-repo-debian11-8.9.7.29/cudnn-local-9EAC560A-keyring.gpg /usr/share/keyrings/
apt-get update && apt-get install -y --no-install-recommends libcudnn8 libcudnn8-dev
rm -rf /var/lib/apt/lists/*
echo 'export CUDNN_VERSION=8' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH' >> ~/.bashrc
. ~/.bashrc
```
### Valdiate the cuDNN installation 
```bash
dpkg -l | grep cudnn
```

# Compile the source code
```bash
cd /src
make main
```

# Utils
```bash
nvidia-smi
```

# Genearte valgrind supression files
```bash
valgrind --gen-suppressions=all --leak-check=full --track-origins=yes ./your_program
```
