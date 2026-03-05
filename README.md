# Cuwacnu
This is an Artificial Intelligence Agent dedicated to managing investment portfolios.

## Project Note
What we are building here is a semantic transformer that optimizes one's market position with respect to abstract values.  
An example intent is that money should be placed in instruments that follow an arrow of love, nature, and survival toward the edge of honor.

This is an unlikely portfolio optimizer: it enforces values, while allowing financial entities to define those values as abstract ideas.  
Inside, a powerful optimization engine still maximizes profit, with those values acting as constraints.

Be aware of the `tsodao`: the abstract dictionary is curated so people cannot define absolute-evil values.

## Reproducible Setup (Docker + `setup.sh`)
The canonical environment setup is now scripted in [`setup.sh`](./setup.sh).  
The script is designed for reproducibility and expects third-party artifacts in `external/`.

### 1. Host prerequisites
Install Docker: https://docker.com

### 2. Stage third-party artifacts in `external/`
Some dependencies require manual download because of vendor terms:

1. Download `libtorch` from https://pytorch.org/ and extract into:
   `external/libtorch`
2. Download the cuDNN Debian package (Debian 11, CUDA 12.x family) from NVIDIA:
   https://developer.nvidia.com/rdp/cudnn-download
3. Place the `.deb` file in:
   `external/cudnn/`

Default expected cuDNN filename:
`cudnn-local-repo-debian11-8.9.7.29_1.0-1_amd64.deb`

### 3. Start a Debian container
```bash
docker pull debian:latest
docker run --name cuwacunu-dev --gpus all -it -v "$PWD":/cuwacunu debian:latest /bin/bash
```

### 4. Run setup inside the container
```bash
cd /cuwacunu
./setup.sh
```

For detailed command output:
```bash
./setup.sh --verbose
```

### 5. Useful flags
```bash
./setup.sh --skip-build
./setup.sh --skip-cuda
./setup.sh --skip-cudnn
./setup.sh --with-curl-source
./setup.sh --dry-run
```

Default build target is `modules`. Override it with:
```bash
MAKE_TARGETS="piaabo camahjucunu" ./setup.sh --verbose
```

### 6. Post-setup checks
```bash
source ~/.bashrc
nvcc --version
nvidia-smi
```

## License and Attribution
This project's original code is open source under the MIT license (`LICENSE`).

For feature requests and ideas, open a GitHub Discussion in this repository.

Third-party components remain under their original licenses and notices.
See `THIRD_PARTY_NOTICES.md` for details and redistribution guidance.
