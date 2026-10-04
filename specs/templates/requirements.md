# Feature Requirements — <feature name>

> Copy this file to `specs/YYYY-MM-DD-<feature-name>/requirements.md` and delete
> the quoted guidance. Write this *before* `plan.md` and `validation.md`.

Roadmap phase: <link to the phase in ../roadmap.md>
Branch: `feat/<name>` (from `develop`, PR back to `develop`)

## Goal

What wire-visible outcome does this feature create? One paragraph.

## In Scope

- Observable behavior included in this branch. Name the transports, the
  `IClient` surface, and the proto profile (`OIP` / `OIP_REPOSITORY`) actually
  covered.

## Out of Scope

- Related work deliberately deferred, and where it goes instead (a roadmap
  phase, a sibling repo, an issue). Consumer adapters, backends, and server
  behavior belong elsewhere — say so explicitly.

## Decisions

- Choice and its rationale. Record the ones where the obvious implementation
  would violate [`../tech-stack.md`](../tech-stack.md) or
  [`../mission.md`](../mission.md), or where the runtime must move first.

## Constraints and Context

- Constitution rules that apply (raw-bytes contract, transport parity,
  backward compatibility of `IClient`).
- Cross-repo sequencing: wire-contract changes land in
  `neuriplo-kserve-runtime` first unless purely additive/optional here. Does
  this need a runtime change, or an `neuriplo-infer` adapter follow-up?
- Contract surfaces touched: `IClient` signatures, proto files, retry/backoff
  behavior, binary-extension handling.
- KServe spec section governing the wire behavior, when one exists.

## Open Questions

- Anything that could materially change the feature and has not been answered.
  Surface these before writing the plan, not after implementing.
