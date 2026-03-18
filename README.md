# Cuwacunu
An artificial intelligence agent for portfolio management.

`Cuwacunu` is an official `WAAJACU TM` project.
Official source identity for this repository is `waajacu.com` and the GitHub
account `savethebeesandseeds`.
See [`TRADEMARKS.md`](./TRADEMARKS.md) for trademark usage guidance.

## Project Note
Cuwacunu is a semantic transformer for portfolio positioning under abstract values.  
Think of it as a lance that cannot be turned toward harm.

The `tsodao` is not the abstract transformer but the eyes on the edge of the lance. 

## Reproducible Setup (Docker + `setup.sh`)
The canonical environment setup is now scripted in [`setup.sh`](./setup.sh).  
The script is designed for reproducibility and expects the staged LibTorch bundle in `.external/`.

### 1. Host prerequisites
Install Docker: https://docker.com

### 2. Stage the LibTorch bundle in `.external/`
The current build is wired for the upgraded LibTorch bundle:

1. Download `libtorch` from https://pytorch.org/ with CUDA 12.4 support.
2. Extract it into:
   `.external/libtorch-upd`

`setup.sh` installs CUDA 12.4 and cuDNN 9 from NVIDIA's Debian 12 network repository, so no separate cuDNN `.deb` needs to be staged.

### 3. Start a Debian container
```bash
docker pull debian:12
docker run --name cuwacunu-dev --gpus all -it --shm-size=1g -v "$PWD":/cuwacunu -w /cuwacunu debian:12 /bin/bash
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
/cuwacunu/.build/tests/test_cuda_probe
```

## HERO MCP Preflight (Codex)

Build MCP binaries:

```bash
make -C /cuwacunu/src/main/hero -j12 all
```

Use minimal MCP args:

```bash
--global-config /cuwacunu/src/config/.config
```

Hashimyei/Lattice catalogs are unencrypted in MCP mode, so no passphrase is
required. For deterministic smoke tests, use temp catalogs under
`/tmp/hero_mcp_smoke/...`.

Detailed HERO MCP guidance lives in `src/main/hero/README.md`.

## License and Attribution
This project's original code is open source under the MIT license (`LICENSE`).
The MIT license covers copyright permissions for the code and documentation.
Brand names, logos, and source identity remain subject to the trademark notice
in [`TRADEMARKS.md`](./TRADEMARKS.md).

For feature requests and ideas, open a GitHub Discussion in this repository.

Third-party components remain under their original licenses and notices.
See `THIRD_PARTY_NOTICES.md` for details and redistribution guidance.
