# Encrypted Optim Bundle

The authored `optim/*.dsl` payload is the first hidden surface owned by TSODAO:
it stays local in plaintext while the public repo keeps only an encrypted
archive.

Tracked backup path:
- `../optim.tar.gpg`

Helper workflow:

```bash
bash -lc 'make -C /cuwacunu/src/main/tools -j12 install-tsodao'
/cuwacunu/.build/tools/tsodao status
/cuwacunu/.build/tools/tsodao init
bash /cuwacunu/src/scripts/install_git_hooks.sh
/cuwacunu/.build/tools/tsodao seal --symmetric
/cuwacunu/.build/tools/tsodao seal --recipient YOUR_GPG_KEY
/cuwacunu/.build/tools/tsodao scrub
/cuwacunu/.build/tools/tsodao open
```

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
