# Encrypted Optim Bundle

The authored `optim/*.dsl` payload is the first hidden surface owned by TSODAO:
it stays local in plaintext while the public repo keeps only an encrypted
archive.

Tracked backup path:
- `../optim.tar.gpg`

Helper workflow:

```bash
bash -lc 'make -C /cuwacunu/src/main/tools -j12 build-tsodao'
/cuwacunu/.build/tools/tsodao status
/cuwacunu/.build/tools/tsodao init
bash /cuwacunu/src/scripts/install_git_hooks.sh
/cuwacunu/.build/tools/tsodao seal --symmetric
/cuwacunu/.build/tools/tsodao seal --recipient YOUR_GPG_KEY
/cuwacunu/.build/tools/tsodao scrub
/cuwacunu/.build/tools/tsodao open
```

Move to another machine:

```bash
# 1. clone this repo at /cuwacunu, or rewrite the absolute paths in
#    src/config/.config and src/config/instructions/defaults/tsodao.dsl
git clone <repo-url> /cuwacunu

# 2. build the TSODAO tool
make -C /cuwacunu/src/main/tools -j12 build-tsodao

# 3. import the existing TSODAO keypair if you need to restore plaintext
gpg --import /path/to/tsodao-private-key.asc

# 4. validate the configured recipient and install the repo hooks
/cuwacunu/.build/tools/tsodao init --validate

# 5. restore the hidden plaintext surface from the tracked archive
/cuwacunu/.build/tools/tsodao sync
```

Migration notes:
- The checked-in TSODAO policy currently uses absolute `/cuwacunu/...` paths.
- A public key is enough to seal to the configured recipient; the private key is required to decrypt `../optim.tar.gpg` and restore plaintext.
- If you copy a working tree that already contains both plaintext `optim/*.dsl` files and `../optim.tar.gpg`, bringing `.runtime/.tsodao/optim.state` with it helps `tsodao sync` avoid ambiguous direction checks.

Notes:
- `tsodao.dsl` is the source of truth for this public/private rule.
- `optim/` is intended to hold the frozen model-bearing bundle only. Objective-
  local wave and campaign policy should live under the consuming objective
  folders.
- `tsodao init` is the preferred setup because it records a GPG recipient in
  `tsodao.dsl` and lets the repo hooks auto-refresh the encrypted archive during
  commit.
- `seal` packs the current plaintext `optim/` files, excluding this README.
- `scrub` removes the plaintext payload after you have created the encrypted bundle.
- `open` restores the bundle back into `optim/` and refuses to overwrite an existing plaintext payload.
