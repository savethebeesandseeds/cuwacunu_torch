# download libtorch
most resent c++ standar:    (https://download.pytorch.org/libtorch/cu121/libtorch-cxx11-abi-shared-with-deps-2.1.2%2Bcu121.zip)
old c++ standar:            (https://download.pytorch.org/libtorch/cu121/libtorch-shared-with-deps-2.1.2%2Bcu121.zip)
what ever you select please extract libtorch into <external/libtorch> folder 

# download CUDNN
go to (https://developer.nvidia.com/rdp/cudnn-download)
set up an account with nvidia 
download cudnn for debian 11 (https://developer.nvidia.com/downloads/compute/cudnn/secure/8.9.7/local_installers/12.x/cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb/)
put the file <cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb> in <external/cudnn>

# pull the debian 11 (bulleye) docker base
docker pull debian:11

# start the container
docker run --name unnamed_tao --gpus all -it -v %CD%/data:/data -v %CD%/external:/external -v %CD%/src:/src debian

# install some initial requirements
apt-get update && apt-get install -y --no-install-recommends build-essential gnupg2 curl ca-certificates
rm -rf /var/lib/apt/lists/*

# add the NVIDIA package repositories
curl -fsSL https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/3bf863cc.pub | gpg --dearmor -o /usr/share/keyrings/nvidia-cuda-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/nvidia-cuda-archive-keyring.gpg] https://developer.download.nvidia.com/compute/cuda/repos/debian11/x86_64/ /" > /etc/apt/sources.list.d/cuda.list

# install CUDA 12.1
apt-get update && apt-get install -y --no-install-recommends cuda-toolkit-12-1 libcudnn8
rm -rf /var/lib/apt/lists/*
echo 'export CUDA_VERSION=12.1' >> ~/.bashrc
echo 'export PATH=/usr/local/cuda-$CUDA_VERSION/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-$CUDA_VERSION/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# valdiate the CUDA installation 
nvcc --version

# install cuDNN 8 runtime
cp /var/cudnn-local-repo-debian11-8.9.7.29/cudnn-local-9EAC560A-keyring.gpg /usr/share/keyrings/
cd /external/cudnn/
dpkg -i cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb
apt-get update && apt-get install -y --no-install-recommends libcudnn8 libcudnn8-dev
rm -rf /var/lib/apt/lists/*
echo 'export CUDNN_VERSION=8' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# valdiate the cuDNN installation 
dpkg -l | grep cudnn

# compile
cd /src
make main

# other util lines 
nvidia-smi
