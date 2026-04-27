# Documentation Editing Guidance

The files under `doc/` are an active manuscript, not just generated reference
material.  Treat user-authored prose as draft thinking in progress.

- Do not make opportunistic or unsolicited content changes to manuscript files.
- When the user asks for a specific documentation edit, change only the
  specified passage or notation unless the user explicitly authorizes a broader
  cleanup.
- If a nearby issue is discovered, report it in the conversation or leave it
  untouched unless the user asks for that fix.
- When reviewing manuscript text and noticing a wording problem, ambiguity,
  inconsistency, or likely mistake, do not rewrite the content directly.  Either
  report it in the conversation or add a LaTeX comment near the issue, such as
  `% TODO: ...`, so the user can review it before any prose is changed.
- Be pedantic and explicit about every change made under `doc/`.  After any
  edit, report the touched file path, the exact kind of change, and whether any
  prose, equation, notation, section title, footnote, citation, or symbol-table
  entry was modified.
- Never let documentation changes be silent.  If no manuscript text changed,
  say that explicitly; if manuscript text did change, summarize the changed
  passage in plain language.
- Do not delete user-written text casually, even when reorganizing sections.
- Prefer preserving, moving, or lightly revising an idea over removing it.
- If text seems redundant, under construction, or in the wrong section, relocate
  it or leave a clear TODO-style note instead of erasing it.
- Only remove substantial prose when the user explicitly asks for deletion, or
  when the same idea is preserved elsewhere in a clearly equivalent form.
- When restructuring numbered section files, keep the conceptual content visible
  and explain where it moved.
