# Feature Plan — <feature name>

> Copy to `specs/YYYY-MM-DD-<feature-name>/plan.md`. Derived from
> `requirements.md`. Numbered groups, dependency order, each ending in something
> observable. Specify behavior and consequential design; leave routine
> implementation to repository convention.

## Group 1 — Foundation

1. First independently verifiable task.
2. Second task.

## Group 2 — Behavior

3. Implement the wire slice (protocol helpers, transport client, retry policy —
   whichever this feature actually touches), with unit tests.

## Group 3 — Contract and documentation

4. Update the surfaces the change makes stale: `README.md` flags/behavior,
   `CHANGELOG.md` under `[Unreleased]`, proto comments if profiles moved.

## Group 4 — Verification

5. Add or update focused unit tests.
6. Run the checks declared in `validation.md`, including the conformance track
   where applicable.

> Commit by task group for higher-risk work; keep the branch uncommitted until
> full validation for small features.
