# Cuwacnu 
This is a Artificial Intelligence Agent, dedicated to managing investment portfolios. 

This code was build by heart, out of the nights where the spirit calls in and the hands do, that was is needed.

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

## Set up the Docker 

### Pull the debian (latest) base
```cmd
docker pull debian:latest
```

### Start the container
```cmd
docker run --name any_name --gpus all -it -v $PWD//:/cuwacunu debian:latest
```

## Configure the Container
For now on, the commands will be inside the docker linux container: 

```bash
mkdir /cuwacunu/external
mkdir /cuwacunu/data
```

### Install some initial requirements
```bash
apt update
apt upgrade -y
apt install -y --no-install-recommends ca-certificates
apt install -y --no-install-recommends build-essential
apt install -y --no-install-recommends build-essential --fix-missing
apt install -y --no-install-recommends libssl-dev
apt install -y --no-install-recommends gnupg2
apt install -y --no-install-recommends valgrind
apt install -y --no-install-recommends locales

rm -rf /var/lib/apt/lists/*

# (default to) en_US.UTF-8
```

## Install CURL dev
Try this
```bash
# Install form apt repositories
apt install -y --no-install-recommends curl
apt install -y --no-install-recommends libcurl4-openssl-dev
```
We require curl 7.86.0 or later since we use curl for websocket.

```bash
which curl
curl --version
```
Verify that websocket (ws wss) is listed under protocols.

If this not the case, you shuld need to install libcurl from source: Websocket in CURL is experiemtal and for this we might require to install it from source and explicitly define websocket support. 

Try installing from soruce:
```bash
# (alternative) install from source, see instruction bellow
# (change the number to the latest)
cd /cuwacunu/external
curl -LO https://curl.se/download/curl-8.9.1.tar.gz 
tar -xzf curl-8.9.1.tar.gz 

apt remove -y curl
apt remove -y libcurl4-openssl-dev
apt remove -y libcurl4
apt remove -y libcurl3-gnutls

cd /cuwacunu/external/curl-8.9.1
./configure --with-openssl --enable-websockets
make
make install

echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
ldconfig
```

Validate
```bash
which curl
curl --version
```
Be sure to check the websocket (ws wss) are listed under protocols. 


## Add the NVIDIA package repositories
```bash
curl -fsSL https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/3bf863cc.pub | gpg --dearmor -o /usr/share/keyrings/nvidia-cuda-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/nvidia-cuda-archive-keyring.gpg] https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/ /" > /etc/apt/sources.list.d/cuda.list
# (update) again
apt update
```

### Install CUDA 12.1
```bash
apt install -y --no-install-recommends cuda-toolkit-12-1
apt install -y --no-install-recommends cuda-toolkit-12-1 --fix-missing

rm -rf /var/lib/apt/lists/*
echo 'export CUDA_VERSION=12.1' >> ~/.bashrc
echo 'export PATH=/usr/local/cuda-$CUDA_VERSION/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-$CUDA_VERSION/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
. ~/.bashrc
```

Valdiate the CUDA installation 
```bash
nvcc --version
```

### Install cuDNN 8 runtime
```bash
cd /cuwacunu/external/cudnn/
dpkg -i cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb
cp /var/cudnn-local-repo-debian11-8.9.7.29/cudnn-local-9EAC560A-keyring.gpg /usr/share/keyrings/

apt update
apt install -y --no-install-recommends libcudnn8
apt install -y --no-install-recommends libcudnn8 --fix-missing
apt install -y --no-install-recommends libcudnn8-dev
apt install -y --no-install-recommends libcudnn8-dev --fix-missing

rm -rf /var/lib/apt/lists/*
echo 'export CUDNN_VERSION=8' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH' >> ~/.bashrc
. ~/.bashrc
```
### Valdiate the cuDNN installation 
```bash
dpkg -l | grep cudnn
```

## Compile the source code
```bash
cd /cuwacunu/src
make piaabo
make camahjucunu
make ...
make main
```

## Utils
```bash
nvidia-smi
```
