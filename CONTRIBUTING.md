# Contributing

## Branch workflow (Gitflow)

This repo follows [Gitflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow).

| Branch | Purpose |
|--------|---------|
| `develop` | Integration branch — all day-to-day work lands here |
| `master` | Release branch — only receives merges from `develop` when cutting a release |
| `feature/*` | Short-lived branches for individual changes |

### Rules

1. **Do not commit directly to `master`.** Open a feature branch from `develop`, merge back into `develop`.
2. **`develop` must never lag behind `master`.** After every release merge into `master`, merge (or fast-forward) `master` back into `develop` if needed so both stay aligned.
3. **Releases** — merge `develop` → `master`, tag (e.g. `v0.1.1`), push branch and tag. Bump the pin in downstream consumers (`neuriplo-infer` `versions.env`).

### Typical flow

```bash
git checkout develop
git pull origin develop
git checkout -b feature/my-change

# … work, commit …

git checkout develop
git pull origin develop
git merge feature/my-change
git push origin develop

# When releasing:
git checkout master
git merge develop
git tag v0.x.y
git push origin master --tags
git checkout develop
git merge master   # keep develop in sync
git push origin develop
```

## Build & test

```bash
cmake -B build -DKSERVE_CLIENT_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

See [README.md](README.md) for CMake options (gRPC, TLS, proto profiles).
