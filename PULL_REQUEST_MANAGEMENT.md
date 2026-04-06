# Pull Request Note

This fork does not use a formal pull request management process.
There is no promised review queue, approval threshold, merge cadence or label taxonomy here.

If a pull request exists, treat it as an optional input channel rather than as the center of the workflow.

## What Matters If A PR Is Considered

- the change fits the current personal direction of the fork
- the patch is small enough to review quickly
- visible NT 5.x behavior is preserved or improved
- the change reduces instability, not just internal discomfort
- the author explains what was tested

## What Usually Does Not Fit

- broad rewrites done mainly for architectural purity
- process-heavy PRs that assume a large maintainer team
- compatibility expansion that pulls the fork away from its current `0.5.x` baseline
- patches that leave visible regressions behind because the internals look cleaner

## Practical Expectation

A PR may be merged, ignored, postponed or superseded by local work depending on the fork owner's current priorities.
That is normal for this repository.

For direction and validation policy, see [TECHNICAL_DIRECTION.md](TECHNICAL_DIRECTION.md) and [STABILITY_VALIDATION.md](STABILITY_VALIDATION.md).
