# Architecture decision records

One file per decision that is hard to reverse: dependencies, architecture, workflow, style. Numbered in
order, never renumbered. A superseded record stays, with its status changed to `Superseded by NNNN`.

| # | Decision | Status |
| --- | --- | --- |
| [0001](0001-git-workflow.md) | Branches off `develop`, PRs, owner squash-merges | Accepted |
| [0002](0002-style-configs-follow-the-code.md) | Style configs follow the existing code | Accepted |
| [0003](0003-sdl3-and-raw-vulkan.md) | SDL3 for windowing, raw Vulkan for graphics | Accepted |

## Template

```markdown
# NNNN. Title in a few words

- Status: Proposed | Accepted | Superseded by NNNN
- Date: YYYY-MM-DD

## Context

What forces the decision; facts with sources.

## Decision

What we do, in one or two paragraphs.

## Consequences

What becomes easier, what becomes harder, what follow-up work it creates.
```
