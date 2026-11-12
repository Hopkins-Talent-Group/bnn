# AGY-CLI Review Packet: Binary-Skill Benchmark & Integrity Audit

**Target repository:** `vitrixLab/bnn`  
**Review target:** `binary-skill/` and root `evidence.md`  
**Baseline:** `main` at review preparation time; the comparison report was added by commit `74e8ee60a8c7db34f4f75b2cd31c276dc47acf6e`.  
**Disposition:** **HOLD — claims not yet certified.** This is a static source/documentation review, not a claim that benchmarks or tests were run.

## 1. Decision summary

The binary format may be useful as a compact, single-file skill container. The current repository does **not** provide enough reproducible evidence to certify the reported latency, speedup, token, memory-mapping, or integrity claims. Keep the format as an experiment; do not use the current `evidence.md` as a benchmark certificate.

| Claim in current docs | Source-level finding | AGY-CLI disposition |
|---|---|---|
| “Memory-mapped” / “zero-copy” reader | `binary-skill/binary_skill.hpp`: `BinarySkillReader::open()` reads the entire file into `std::vector<uint8_t>` with `std::ifstream::read`. This is a buffered full-file load, not OS `mmap` / `MapViewOfFile`. Returned `string_view` values may avoid copying individual strings after load; that is a narrower claim. | **REJECT wording as implemented.** Say “whole-file buffered load with views into the buffer” unless a mapping implementation is added. |
| O(1) name/category/keyword lookup | `find_by_name`, `find_by_category`, and `search_keyword` iterate through records. Direct `record(index)` is constant-time after validation, but name/category/keyword queries are linear scans. Keyword work also scales with text inspected. | **PARTIAL.** O(1) applies to indexed record-by-index access only. |
| `2.917 µs` keyword search / `>300x` speedup | `evidence.md` provides no reproducible benchmark source, invocation, machine metadata, raw samples, cache-state protocol, query corpus, repetitions, or correctness parity results supporting these numbers. | **UNVERIFIED.** Remove or label as historical/unverified until measured. |
| Raw corpus `11,889,847` bytes vs binary `510,896` bytes; `95.7%` reduction | The binary artifact is listed by GitHub as 510,896 bytes. The raw-corpus byte total is a report claim; its input manifest and repeatable measurement command are not supplied alongside the result. | **PARTIAL.** Binary file size is corroborated by GitHub metadata; raw side and derived ratio need a reproducible manifest/run. |
| “0 tokens” / “100% token elimination” | A local binary file is not itself an LLM prompt, but text retrieved from it and sent to a model still consumes context tokens. Storage representation does not establish end-to-end token savings. | **REJECT as stated.** Measure tokens for an identical task/query and report retrieved text and model context separately. |
| “Zero CPU cycles” / “zero FPU operations” | Address calculation and reader/search logic execute CPU instructions. No CPU-cycle or instruction-counter evidence is included; keyword comparisons and text scanning are not “zero CPU.” | **REJECT as stated.** Replace with a narrowly defined statement, or measure counters under a documented workload. |
| SHA-256 integrity / “instant tamper detection” | `sha256_hex(index)` formats the 32 bytes stored in the record. It does not recompute the digest from the source or verify a whole-file checksum. The `verify` command must not be described as cryptographic verification unless it actually recomputes and compares hashes. | **NOT CERTIFIED.** Define the hash input bytes and add recomputation plus tamper tests. |
| Structural bounds safety | `open()` uses arithmetic such as `sizeof(BinaryHeader) + skill_count * sizeof(SkillRecord)` and `string_table_offset + string_table_bytes`; description/body checks use offset-plus-length. These expressions can overflow before comparison. Layout/overlap constraints are not fully established by these checks. | **HOLD for untrusted input.** Use checked arithmetic and validate every region before pointer/view construction. |
| Portable file format | The reader interprets packed native structs directly. Packing fixes structure size for this build but does not itself define byte order or portable field decoding. | **LIMITED.** Document the supported ABI or define an explicit endian-stable serialized layout. |
| CLI self-test proves roundtrip | `test-compile` checks count and first name, and writes `test_roundtrip.bin`; it does not comprehensively compare all records/strings/hashes and does not clean up the output file. | **SMOKE TEST ONLY.** Expand parity assertions and write artifacts to a temp path with cleanup. |

## 2. Evidence anchors

Review these files at the baseline commit:

- [Reader and binary structs](https://github.com/vitrixLab/bnn/blob/main/binary-skill/binary_skill.hpp)
  - `BinarySkillReader::open`: buffered whole-file read into `buffer_`.
  - `record(index)`: direct record-table indexing.
  - `find_by_name`, `find_by_category`, `search_keyword`: scans.
  - `sha256_hex`: renders the stored digest bytes; no recomputation in this method.
  - `description` / `body`: offset-length validation uses addition.
- [CLI](https://github.com/vitrixLab/bnn/blob/main/binary-skill/main.cpp)
  - Help/info text currently repeats “zero-copy”, “zero-FPU”, “sub-millisecond” and “memory-mapped” language.
  - `test-compile` is a small round-trip smoke test.
- [Binary-skill README](https://github.com/vitrixLab/bnn/blob/main/binary-skill/README.md)
  - Describes memory mapping and microsecond keyword lookup, but the reader implementation does not establish those claims.
- [Current comparison report](https://github.com/vitrixLab/bnn/blob/main/evidence.md)
  - Contains precise benchmark/token/compute claims without a reproducible evidence bundle.

**Artifact metadata:** GitHub reports `binary-skill/secondbrain.bin` at 510,896 bytes. This metadata is not a successful runtime validation, source-corpus comparison, or benchmark.

## 3. Required benchmark protocol

Do not compare unlike workloads. Run the same fixed corpus and query set against the raw-file implementation and binary implementation.

### 3.1 Record the environment

Save to each JSON result:

- Git commit SHA and binary artifact SHA-256.
- OS/distribution, kernel, CPU model, RAM, storage type, filesystem.
- Compiler and version, exact flags, build type, dependency versions.
- Corpus manifest (relative path, byte size, SHA-256 for each source file) and binary size/hash.
- Query-set file and its SHA-256.
- Cache protocol and whether the run is warm, cold, or uncontrolled.

### 3.2 Separate operations

Measure these independently; do not combine them into one “search latency” number:

1. Process startup and file open/load.
2. Indexed record-by-index access.
3. Exact-name lookup.
4. Category lookup.
5. Keyword search across name, description, and body.
6. End-to-end retrieval: query → matching records → returned text.
7. Memory: peak RSS and, if mapping is implemented, mapped virtual size and resident pages.
8. Storage: raw corpus bytes, binary bytes, and build/compile time.

### 3.3 Fair run procedure

- Use the exact same source corpus and normalized query cases.
- First assert result parity (same skill IDs and same returned content) before timing.
- Build optimized release binaries with identical compiler/optimization settings where applicable.
- Do at least 30 independent repetitions per scenario; retain every sample. Report median, p95, min/max, and sample count. Do not report only a best run.
- Report warm-cache results separately. A cold-cache claim requires a documented, reproducible cache-reset protocol; if privileged cache control is unavailable, label the result “cold cache not controlled.”
- Include no-match, first-match, last-match, common-term, rare-term, and long-body queries. Avoid selecting only favorable queries.
- Ensure benchmark work cannot be optimized away; consume and validate results.
- Publish raw JSON/CSV samples, query fixture, corpus manifest, build command, and script in the repository.
- Never substitute estimates or illustrative numbers for measured results.

### 3.4 Minimal machine-readable result schema

Each measurement row should include:

```json
{
  "schema_version": 1,
  "commit": "<git-sha>",
  "implementation": "binary-buffered",
  "operation": "exact_name_lookup",
  "cache_state": "warm",
  "iteration": 1,
  "elapsed_ns": 0,
  "result_count": 0,
  "correctness": "pass",
  "corpus_manifest_sha256": "<sha256>",
  "query_set_sha256": "<sha256>",
  "environment": {
    "os": "<os>",
    "cpu": "<cpu>",
    "compiler": "<compiler>",
    "flags": "-O2"
  }
}
```

The zero values above are schema examples, **not measurements**. Keep raw samples and compute summary statistics from them.

## 4. Correctness and safety gates before performance certification

1. **Hash semantics:** Specify whether each digest covers source file bytes, normalized content, or a binary region. Recompute SHA-256 and compare expected values. Add one-bit corruption tests that must fail.
2. **Checked arithmetic:** For every offset/length, check bounds without overflowing (e.g. validate `offset <= pool_size` and `length <= pool_size - offset` after validating the first condition).
3. **Layout validation:** Check header size/version, record-table end, string-pool start/end, non-overlap, exact file-size policy, record count limits, and every record reference before use.
4. **Malformed corpus tests:** empty file, truncated header/table/pool, invalid magic/version, oversized count, overflow-sized offsets/lengths, invalid names, overlapping/out-of-range regions, corrupted digest.
5. **Sanitizers:** Run ASan and UBSan builds against valid and malformed fixtures; preserve exact commands and logs.
6. **Round-trip parity:** Compile fixture skills, reopen output, and compare all names, categories, flags, descriptions, bodies, and hashes—not just record count and first name.
7. **Portability:** Either define and decode a fixed byte order explicitly or state the supported ABI and test it. Do not infer portability solely from `#pragma pack`.

## 5. Suggested AGY-CLI review checklist

- [ ] Confirm the current branch and baseline commit before editing.
- [ ] Classify each performance statement as **measured**, **derived from measured values**, **estimated**, or **unsupported**.
- [ ] Correct “mmap/zero-copy/O(1)/zero CPU/zero tokens” wording to match actual code.
- [ ] Add a corpus manifest and one-command reproducible size report.
- [ ] Add correctness parity and malformed-input tests before benchmark runs.
- [ ] Add benchmark harness with JSONL/CSV raw output and summary statistics.
- [ ] Run release build, unit tests, ASan/UBSan, then benchmarks.
- [ ] Attach logs and raw data to the PR; do not mark gates PASS on planned commands.
- [ ] Re-review the report after the implementation and data are available.

## 6. Current RAV status

| Gate | Status | Reason |
|---|---|---|
| Reader implementation inspected | **PASS (static inspection only)** | The code path and search methods were reviewed. |
| Existing performance claims independently reproduced | **UNKNOWN / NOT RUN** | No raw benchmark output or reproducible harness was provided. |
| SHA-256 verification proven | **NOT CERTIFIED** | Stored digest display is not digest recomputation. |
| Malformed-input safety proven | **NOT CERTIFIED** | Arithmetic/layout validation gaps need tests and hardening. |
| Raw-vs-binary storage delta independently reproduced | **PARTIAL** | Binary size is available from repository metadata; raw input manifest/measurement is missing. |
| Runtime speedup or token savings certified | **NOT AUTHORIZED** | No controlled end-to-end benchmark with parity evidence. |

**Rule:** Evidence → analysis → derived evidence does not itself grant authorization. Do not merge implementation changes or promote performance claims based on this review packet alone.
