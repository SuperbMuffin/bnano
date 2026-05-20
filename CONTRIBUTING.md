# Contributing to bnano

Contributions are welcome! Here's everything you need to get started.

## Requirements

- `clang` (used as the compiler)
- `clang-format` 21, installed via pip: `pip install clang-format`
- `bash` (for running tests)

## Building

```bash
make        # build
make run    # build and run
make clean  # remove build artifacts
```

The binary ends up at `build/bnano`.

## Code Style

All C code must be formatted with `clang-format` before submitting — the CI will reject PRs that aren't. A `.clang-format` is checked in at the root, so just run:

```bash
clang-format -i source/*.c include/*.h
```

Key style rules: LLVM base, Allman braces, 2-space indent, 100 column limit, right-aligned pointers.

## Running Tests

```bash
bash tests/run_tests.sh
```

Tests are organized by module under `tests/` (e.g. `tests/rope/`, `tests/buffer/`). If you're adding a new feature, add tests for it in the relevant suite or create a new one.

## CI

PRs and pushes to `main` run three GitHub Actions checks:

- **Build** — compiles the project with `clang` via `make`
- **Clang Format** — runs `clang-format --dry-run -Werror` and fails if anything is unformatted
- **Tests** — runs `tests/run_tests.sh`

All three must pass before a PR can be merged. Run the build and tests locally before opening a PR to avoid round-trips.

## Submitting Changes

1. Fork the repo and create a branch
2. Make your changes
3. Run `clang-format -i source/*.c include/*.h`
4. Run `bash tests/run_tests.sh` and make sure everything passes
5. [Open a PR](https://github.com/SuperbMuffin/bnano/compare) with a short description of what you changed and why

## Reporting Bugs

[Open an issue](https://github.com/SuperbMuffin/bnano/issues/new) with steps to reproduce and your terminal/OS info.
