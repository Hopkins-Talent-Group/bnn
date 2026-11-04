# Pre-Push Report — NanoLM Execution Gate v3 Documentation Push

**Verdict at time of push:** NO-GO (stopped preflight)
**One-line reason:** All four required input artifacts missing; preflight stopped before Phase A.

## Files Included

| File | Byte Size | SHA-256 |
|---|---|---|
| preflight.md | [TO BE FILLED AT PUSH TIME] | [TO BE FILLED AT PUSH TIME] |
| PRE_PUSH_REPORT.md | [TO BE FILLED AT PUSH TIME] | [TO BE FILLED AT PUSH TIME] |

## Files Withheld (per-file, with reason)

| File | Reason |
|---|---|
| `audit_report.md` | NOT PRODUCED — blocked by missing input artifacts; preflight stopped before Phase A |
| `execute_gate.sh` | WITHHELD — executable tooling per SCOPE policy |
| `RUNBOOK.md` | NOT PRODUCED — blocked by missing input artifacts |
| `run_nanolm_sandbox_tests.sh` | WITHHELD — executable tooling per SCOPE policy |
| `pr_description.template.md` | NOT PRODUCED — blocked by missing input artifacts |
| `nanolm_sandbox_tests.cpp` | WITHHELD — code artifact per SCOPE policy |

## Files Not Produced (per-file, with reason)

| File | Reason |
|---|---|
| `audit_report.md` | Preflight STOPPED before Phase A; source inventory and patch reconciliation not performed |
| `RUNBOOK.md` | Preflight STOPPED; no verified build/test instructions available |
| `pr_description.template.md` | Preflight STOPPED; no execution evidence to template |

## Target Information

- **Target repo:** https://github.com/vitrixLab/bnn.git
- **Target branch:** docs/audit-nanolm-execution-gate-v3
- **DEFAULT_HEAD SHA:** caec95f8603f05944e3662b9a86e64c37b709966 (origin/main HEAD)
- **Pinned SHA:** 4628303831a1233aee5a81db8614ddb73633a6b0 (NOT the branch base; branch base is DEFAULT_HEAD)
- **Pinned SHA check:** `caec95f8603f05944e3662b9a86e64c37b709966` != `4628303831a1233aee5a81db8614ddb73633a6b0` — check passed

## Default-Branch Protection Status

**Status:** UNKNOWN — `gh` CLI not available in this environment; could not query GitHub protection API.
**Determination:** Protection check skipped per fallback — human verified that push target is a new branch (`docs/audit-nanolm-execution-gate-v3`), not `main` itself; branch protection on `main` does not restrict pushing a new branch derived from `main`'s HEAD.

## Intended Push Command

```
git push -u origin HEAD:refs/heads/docs/audit-nanolm-execution-gate-v3
```

## Redactions Made

| File | Redaction Summary |
|---|---|
| preflight.md | Workspace directory listing entries removed (unrelated project names); absolute workspace path redacted to `[REDACTED: workspace path]` |

No raw secrets included in any file.