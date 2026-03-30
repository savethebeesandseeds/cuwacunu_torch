# Repo Hooks

This repo ships local git hooks for the first TSODAO public/private guard.

Enable them with:

```bash
make -C /cuwacunu/src/main/tools -j12 install-tsodao
bash /cuwacunu/src/scripts/install_git_hooks.sh
```

What they do:
- `pre-commit` reads `src/config/instructions/defaults/tsodao.dsl`, refreshes
  and stages the current hidden archive when `visibility_mode=recipient` is
  configured, and plaintext hidden content is newer than the archive.
- `pre-commit` rejects staged hidden plaintext files other than the configured
  public keep path.
- `pre-push` rejects pushes that would publish hidden plaintext from the commit
  range being pushed.
- both hooks call `/cuwacunu/.build/tools/tsodao` directly and fail fast with a
  build/install hint if the binary is missing.
