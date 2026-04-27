# Cuwacunu Algorithm Document

This directory contains the formal Cuwacunu algorithm manuscript.  The source is
plain LaTeX and is intended to grow with the runtime as the model, DSL, catalog,
and compliance layers become more mathematical.

## Build

From the repository root:

```bash
make doc
```

For a faster one-pass draft build while editing:

```bash
make doc_fast
```

or directly:

```bash
make -C doc
```

The publishable PDF is written to:

```text
doc/publish/cuwacunu-algorithm.pdf
```

The fast draft PDF is written separately to:

```text
doc/publish/cuwacunu-algorithm-fast.pdf
```

## Secure LaTeX Policy

The document is built through `doc/scripts/build-doc.sh`, not by invoking an
arbitrary TeX command from the shell.  The wrapper:

- uses `pdflatex` only,
- passes `-no-shell-escape`,
- sets paranoid TeX file IO with `openin_any=p` and `openout_any=p`,
- compiles only the repository-owned `algorithm.tex`,
- writes build products under `doc/build/`,
- copies the release artifact into `doc/publish/`.

Do not add packages that require shell escape, such as `minted`.  Prefer static
LaTeX constructs, `amsmath`, and project-owned source snippets.

## Dependencies

`setup.sh` installs the required LaTeX packages with:

```bash
apt install -y --no-install-recommends texlive-latex-base texlive-latex-recommended texlive-fonts-recommended
```

No Python tooling is required.
