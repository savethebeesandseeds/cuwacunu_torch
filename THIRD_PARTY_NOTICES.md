# Third-Party Notices

This project includes original work and third-party components.

`LICENSE` applies to original code and content in this repository, unless a file or folder states otherwise.
Third-party components keep their own licenses and copyright notices.
Nothing in this repository re-licenses third-party work under MIT.

## Components currently present

`external/curl-8.9.1`
- Upstream project: cURL
- License: cURL license (see `external/curl-8.9.1/COPYING`)

`external/libtorch` and `external/libtorch-upd`
- Upstream project: PyTorch / LibTorch binary distribution
- License: as provided by upstream PyTorch distribution and notices

`external/cudnn`
- Upstream project: NVIDIA cuDNN packages (downloaded separately)
- License: NVIDIA cuDNN license terms from NVIDIA distribution

`/usr` system dependencies (linked at build time)
- OpenSSL (`libcrypto`, `libssl`)
  - Upstream project: OpenSSL
  - License: OpenSSL License / Apache-style license terms from the OpenSSL distribution
- libcurl
  - Upstream project: cURL (system package when not using bundled curl)
  - License: cURL license
- CUDA toolkit/runtime (`/usr/local/cuda-12.1`): `libcudnn`, `libcuda`, `libcudart`, `lnvToolsExt`
  - Upstream project: NVIDIA
  - License: CUDA and cuDNN license terms from NVIDIA distributions
- ncursesw (`-lncursesw`)
  - Upstream project: ncurses
  - License: ncurses license terms from the ncurses distribution

`external/libtorch-upd`
- Upstream project: PyTorch / LibTorch binary distribution
- License: as provided by upstream PyTorch distribution and notices

## Redistribution note

If you redistribute this repository with third-party binaries or source bundles, include each component's original license text and notices.

## Tooling and AI assistance

Build and maintenance assistance in this repository can include tooling/AI workflows using OpenAI Codex.
- OpenAI Codex: tooling/assistance credit, no third-party source code is included from this service in shipped artifacts.
