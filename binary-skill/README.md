# Binary-Skill (Zero-FPU Second-Brain Engine)

Zero-copy memory-mappable binary representation of the 52-skill distilled knowledge base. Built entirely in modern C++17/C++20 with **zero Python dependencies** and zero floating-point model inference / tokenization overhead.

## Architecture

Traditional agent frameworks read hundreds of bloated markdown files per session, wasting millions of FLOPs on tokenization and exhausting context windows. `binary-skill` compiles all skills into a deterministic binary format (`VSKB0001`):

- **Header (20 bytes)**: Magic identifier (`VSKB`), format version (1), skill count (52), string pool offset & size.
- **Record Table (52 × 128 bytes)**: Fixed-width cache-aligned records containing null-terminated names, category IDs, capability bitmasks, cryptographic SHA-256 hashes, and string pool pointer offsets.
- **String Pool (504 KB)**: Contiguous UTF-8 bytes for descriptions and bodies.

## Memory-Mapped Zero-FPU Querying

Any model runtime, C++ service, or local AI daemon can memory-map `secondbrain.bin` and perform $O(1)$ constant-time record access and microsecond keyword lookups without invoking BPE tokenizers or GPU attention matrices.

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
