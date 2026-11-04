# Binary-Skill (Zero-FPU Second-Brain Engine)

Memory-mappable binary representation of the tracked skill knowledge base. The standalone runtime is implemented in C++17 and has no Python runtime dependency. It is a data-access experiment, not an integrated neural-network inference engine.

## Architecture

`binary-skill` compiles skill records into a binary format (`VSKB0001`). The repository does not currently provide a matched, reproducible comparison against a Markdown/file-based baseline, so this architecture description is not a measured speedup claim:

- **Header (20 bytes)**: Magic identifier (`VSKB`), format version (1), skill count (52), string pool offset & size.
- **Record Table (52 × 128 bytes)**: Fixed-width cache-aligned records containing null-terminated names, category IDs, capability bitmasks, cryptographic SHA-256 hashes, and string pool pointer offsets.
- **String Pool (504 KB)**: Contiguous UTF-8 bytes for descriptions and bodies.

## Memory-Mapped Zero-FPU Querying

The reader supports memory mapping where available. After a successful open, indexed `record(index)` access is constant-time. Opening a file also validates the layout by traversing records, and keyword search scans records and their text, so those operations are not O(1). No latency or speedup guarantee is made by this description.

## Compiling and Running the C++ CLI

### Using GCC / Clang (Linux / macOS / WSL)
```bash
g++ -std=c++17 -O2 main.cpp -o binary_skill_cli
./binary_skill_cli secondbrain.bin info
./binary_skill_cli secondbrain.bin list
./binary_skill_cli secondbrain.bin search "nvidia"
```

### Using MSVC (Windows)
```cmd
cl /EHsc /std:c++17 /O2 main.cpp /Febinary_skill_cli.exe
binary_skill_cli.exe secondbrain.bin info
```

### Self-Test Compiler
To test the pure C++ compiler roundtrip:
```bash
./binary_skill_cli test-compile
```

## Distilled Categories

- `1: NVIDIA_STACK` (claude-mem, headroom-ai, claude-code-setup, omniroute, task-observer)
- `2: CORE_ENGINEERING` (workflow-authoring, requirements-clarification, create-skill, git, ...)
- `3: ANTI_SLOP` (antislop, antislop-code, antislop-ui, antislop-human, ...)
- `4: DESIGN_FRONTEND` (ui-ux-pro-max, ui-styling, design-system, banner-design, ...)
- `5: DATABASE_BACKEND` (prisma-cli, prisma-client-api, prisma-postgres, ...)
- `6: MULTIMODAL_CREATIVE` (threejs, remotion-best-practices, whisper-beat-finder, ...)


## Benchmark evidence status

The tracked `binary-skill/benchmark_results.json` is historical evidence unless its provenance explicitly identifies the measured commit, source cleanliness, build configuration, and environment. CI benchmark runs are smoke tests for correctness and harness execution; their timings are informational, not a performance gate. The current benchmark does not measure a workload-equivalent Markdown corpus baseline, so it does not establish a comparative speedup.