# Fresh Include Surface

The previous implementation is preserved under `src_legacy/`. Do not delete or
modify it casually; it is the reference quarry for the rewrite.

The first migrated pillars are:

- `piaabo`, the quiet utility layer.
- `wikimyei`, the worker code, with representation and inference copied mostly
  as they existed in `src_legacy`.
- `ujcamei`, meaning input or source, migrated from
  `src_legacy/include/source`.
- `jkimyei`, the training policy and setup room.
- `kikijyeba`, meaning brain, introduced as the future place for intent and
  coordination.

The old runtime ontology is still paused. In particular, this tree does not
assume that graph contracts or training belong under a specific named pillar
until that ownership is decided carefully.

Camahjucunu is not migrated as a whole. Its legacy pieces are being placed into
the fresh rooms that own them: Ujcamei for input/data, Jkimyei for training
specs, Wikimyei for worker design, and Kikijyeba for lineage/memory.
