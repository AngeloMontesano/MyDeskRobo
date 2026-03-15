---
name: feedback_style
description: Coding and collaboration preferences the user has expressed
type: feedback
---

Don't summarize what was just done at the end of a response — the user can read the diff.

**Why:** User finds trailing summaries redundant.
**How to apply:** End responses after the last tool call or with one short sentence max.

---

Don't over-engineer. Only change exactly what was asked.

**Why:** User works iteratively and wants minimal diffs to review.
**How to apply:** No refactoring, no extra comments, no new abstractions unless explicitly requested.

---

Read the file before editing, always.

**Why:** Edits fail if the file hasn't been read in the session; also prevents wrong-context edits.
**How to apply:** Always Read → then Edit, never guess offsets.

---

When a feature "wirkt nicht" (doesn't work), delete it rather than keep patching.

**Why:** User deleted SKEPTICAL after multiple failed attempts. Sunk-cost fixes waste time.
**How to apply:** After 2 failed attempts at the same bug, propose deletion/replacement instead.
