# Bit-Balance Ratio and Entropy Normalization as Diagnostic Metrics for Knowledge Distillation in 1-Bit Binary Neural Networks

**Authors:** [Author Names / VitrixLab Research]
**Affiliation:** VitrixLab, Hopkins Talent Group
**Date:** October 31, 2026

---

## Abstract

Binary Neural Networks constrain weights and activations to `{+1,-1}`, replacing floating-point matrix multiplication with XNOR and popcount logic. Distilling full-precision teachers into 1-bit students is lossy and prone to silent structural degradation that validation accuracy alone does not expose. This paper defines a **bit-balance ratio** — the ratio of minority to majority bit counts — and a parity-normalized Shannon entropy, both corrected so that decision thresholds are well-defined for odd tensor lengths. For prime-length tensors, a rank-aware shape gate distinguishes byte-aligned streams (where primality indicates truncation or mispacking) from genuinely bit-packed 1-D tensors (where primality is admissible). Hamming distance provides an order-sensitive comparison between checkpoints. The method is a supplementary structural monitor: it cannot assess semantic correctness and is not a substitute for validation benchmarks. A self-contained C++20 reference implementation, an ASCII status grammar, and a normative notation-to-ASCII-to-identifier mapping are provided to support reproducible reimplementation. No model-quality, speedup, or production-readiness claim is made.

**Keywords:** binary neural networks, knowledge distillation, quantization diagnostics, structural monitoring, reproducibility

---

## 1. Introduction

Binary Neural Networks constrain weights and activations to `{+1,-1}`, replacing floating-point matrix multiplication with XNOR and popcount logic (Rastegari et al., 2016; Courbariaux et al., 2016). The parent `vitrixlab/bnn` repository targets edge inference using the BIN1 representation; companion work on INT4 weights with dynamic INT8 activations is out of scope here.

During knowledge distillation (Hinton et al., 2015), a full-precision teacher is compressed into a 1-bit student (`nanoLM`). Because the student's representational capacity is limited, information loss can be rapid and irreversible. Validation accuracy alone provides little visibility into how 1-bit capacity is being allocated, or where structural integrity begins to fail.

The contributions of this paper are three:

1. **A parity-corrected pair of metrics.** Raw balance and raw entropy are biased for odd $N$; normalization by parity-corrected maxima removes the bias and makes fixed thresholds well-defined across even and odd lengths.
2. **A rank-aware primality gate.** Prime $N$ is neither universally an error (a legitimate bit-packed 1-D tensor has prime length) nor silently admissible (a claimed 2-D tensor of prime length cannot exist). The gate distinguishes the two cases.
3. **An ASCII status grammar.** The diagnostic emits a closed set of tokens under a defined grammar, so downstream consumers parse by exact token rather than by substring matching.

The method is a *structural* diagnostic. Section 6 states its limits explicitly.

### 1.1. Provenance

Consistent with the parent repository's acceptance rule (correctness, integrity, numerical accuracy, reproducibility, provenance, evaluated jointly), the reference implementation in §4.3 was compiled with:

| Item | Value |
|---|---|
| Standard | C++20 (`<bit>`, `std::popcount`) |
| Compiler | GCC 13.2, Clang 17 (both tested) |
| Flags | `-O2 -Wall -Wextra -pedantic -Wconversion` |
| Platform | x86-64, Linux 6.6 |

The repository currently builds with `-std=c++17`; §4.4 provides a C++17 backport of the two non-portable facilities (`std::popcount`, `__uint128_t`). Any integration claim must be verified against the exact toolchain in use.

---

## 2. Theoretical Framework

### 2.1. Bit-Balance Ratio and Entropy

Let $N$ be the tensor length in bits, $N_1$ the number of ones, and $N_0 = N - N_1$ the number of zeros. The raw bit-balance ratio is

$$
R = \frac{\max(N_1, N_0)}{\min(N_1, N_0)} \in [1, \infty), \qquad N_1, N_0 > 0.
$$

Fixing the orientation (majority over minority) makes $R \ge 1$ and $R = 1$ denote perfect balance. The alternative $N_1/N_0$ is a reciprocal whose orientation depends on which sign happens to be dominant; fixing the orientation removes that ambiguity throughout the paper and in code.

Information capacity is Shannon entropy in bits:

$$
H = -p \log_2 p - (1-p) \log_2(1-p), \qquad p = \frac{N_1}{N}.
$$

$R$ and $H$ are both order-insensitive: `1010` and `1100` produce identical values despite different structure. They describe capacity utilization, not correctness.

### 2.2. Checkpoint Distance

Comparing two checkpoints (teacher binarized vs. student distilled) requires an order-sensitive measure:

$$
D_H(A, B) = \operatorname{Popcount}(A \oplus B).
$$

This is distinct from BNN inference, which computes an XNOR-popcount inner product. The two operators are complements — $\operatorname{Popcount}(\mathrm{XNOR}) = N - \operatorname{Popcount}(\mathrm{XOR})$ — and serve different purposes. The diagnostic uses XOR; inference uses XNOR; prose and code must not conflate them.

### 2.3. Parity Normalization

For odd $N$, neither $R = 1$ nor $H = 1$ is attainable. The best-balanced split is $N_1 = \lceil N/2 \rceil$, $N_0 = \lfloor N/2 \rfloor$, so raw $R$ and $H$ understate balance even for a perfectly randomized tensor. Naive thresholds therefore yield false positives on healthy small odd-length layers.

The parity-corrected metrics are:

$$
\rho = \frac{\min(N_1, N_0)}{\lfloor N/2 \rfloor} \in [0, 1], \qquad N \ge 2,
$$

$$
\tilde H = \frac{H}{H_{\max}(N)} \in [0, 1], \qquad H_{\max}(N) = -\frac{\lceil N/2 \rceil}{N} \log_2 \frac{\lceil N/2 \rceil}{N} - \frac{\lfloor N/2 \rfloor}{N} \log_2 \frac{\lfloor N/2 \rfloor}{N}.
$$

$H_{\max}(N) = 1$ iff $N$ is even; for odd $N$, $H_{\max}(N) < 1$. A value $\tilde H = 1$ indicates the tensor has reached the maximum information capacity permitted by its length; $\rho = 1$ indicates the maximum achievable balance. Both metrics are undefined at $N \in \{0, 1\}$ and are handled by explicit degenerate-case flags (§2.4).

**The invariance is precise:** the *metrics* are $N$-invariant, not the thresholds. A fixed $\tau_\rho = 0.1$ means "at most 10% of the achievable balance for this $N$," which is well-defined for any $N \ge 2$. Threshold sensitivity is an empirical question (§5), not a theoretical one, and is not asserted here.

### 2.4. Diagnostic Output

The diagnostic returns $(\rho, \tilde H, D_H, \mathcal{F})$, where $\mathcal{F}$ is a set of flags from a closed enumeration. Because collapse and skew can co-occur — a collapsed layer is usually also skewed — the decision is a set of independent predicates, not an `if / else if / else` chain. Multiple flags may be set.

| Flag | Severity | Predicate |
|---|---|---|
| `SHAPE_LEN` | Err | $N \ne \prod_i d_i$ |
| `SHAPE_PRIME_ND` | Err | $N$ prime and rank $\ge 2$ |
| `SHAPE_PRIME_1D` | Info | $N$ prime and rank $= 1$ |
| `ALIGN_BYTE` | Err | explicit $N_{\text{bits}}$ leaves nonzero padding |
| `DEGEN_EMPTY` | Err | $N = 0$ |
| `DEGEN_SINGLETON` | Err | $N = 1$ |
| `COLLAPSE` | Err | $\tilde H < \tau_H$ |
| `SKEW` | Warn | $\rho < \tau_\rho$ |
| `DRIFT` | Info | $D_H > \tau_D \cdot N$ |
| `HEALTHY` | Ok | no `Err` or `Warn` flag set |

`DRIFT` is **advisory only**: it is emitted as `Info` and does not participate in the aggregation that decides `HEALTHY`. Callers who wish to treat drift as a warning do so at the boundary; the diagnostic itself does not.

**Serialization grammar.** Flags are declared as `enum class` values and emitted as ASCII tokens, one per line, in the form `SEV ":" CODE EOL` where `SEV ::= "OK" | "INFO" | "WARN" | "ERR"` and `CODE` is one of the identifiers above. The grammar is intentionally minimal: no delimiter is overloaded, no severity is inferred from a substring of another token, and dispatch is by exact token rather than by `find("ERROR")`. String serialization occurs only at the output boundary; internal consumers use the enum.

---

## 3. Structural Interpretation

### 3.1. Byte-Aligned Streams

In standard `.bin` serialization, tensors occupy whole bytes, so $N = 8\lvert T \rvert$ and is never prime for $\lvert T \rvert \ge 1$. A prime $N$ in this path is a structural anomaly: a truncated read, a header mismatch, or an endianness error. The `SHAPE_LEN` check catches this case directly when the claimed dimensions are compared against $N$.

The `ALIGN_BYTE` check addresses the complementary case: when the caller supplies an explicit `N_bits` that is not a multiple of 8, the padding bits of the final byte must be zero. Nonzero padding indicates an unreported bit-length or a bit-packing error.

### 3.2. Bit-Packed Tensors

In memory-optimized runtimes, weights may be packed without byte padding, so $N$ may legitimately be prime (e.g., $N = 7, 11, 13$). Prime factorization then yields only the trivial $1 \times N$. A prime $N$ is admissible at rank 1 and fatal at rank $\ge 2$:

- $N$ prime, $\text{rank} = 1$ → informational flag `SHAPE_PRIME_1D`; diagnostics continue.
- $N$ prime, $\text{rank} \ge 2$ → error flag `SHAPE_PRIME_ND`; diagnostics halt.

This gate is not a heuristic: a tensor claimed as a 2-D array of prime bit-length has no integer factorization into two factors greater than 1, and the claim is therefore structurally impossible.

### 3.3. Interpreting the Flags

- **`SKEW`:** $\rho$ decreasing across epochs indicates the layer is allocating capacity toward one sign. Warning only; a skewed layer can still be correct.
- **`COLLAPSE`:** $\tilde H \to 0$ indicates the layer no longer distributes information. Distillation temperature or learning rate typically requires adjustment.
- **`DRIFT`:** $D_H$ between teacher and student checkpoints increasing over time. Advisory; the tolerable drift depends on training schedule and is not part of the diagnostic's decision rule.

These readings are interpretive, not diagnostic of correctness. Structure is necessary but not sufficient for accuracy (§6).

---

## 4. Diagnostic Pipeline

### 4.1. Preconditions

The implementation enforces:

1. `reference.size() >= tensor.size()`.
2. If `N_bits` is supplied, `8*(tensor.size()-1) < N_bits <= 8*tensor.size()`.
3. If `N_bits < 8*tensor.size()`, the padding bits of the final byte are zero (`ALIGN_BYTE`).
4. $N \ge 2$ for $\rho$ and $\tilde H$ to be defined.

Violations produce structured errors, not undefined behavior.

### 4.2. Algorithm

```text
Algorithm 1: Parity-Aware Bit-Balance Diagnostic
Input : T, R_ref, expected_dims = (d_1..d_k) in bits, optional N_bits
        tau_H, tau_rho, tau_D
Output: DiagnosticResult { rho, h_norm, D_H, flags }

 1. N <- N_bits if given else 8 * len(T)
 2. if N != prod(d_i):            add(SHAPE_LEN);        return
 3. if N == 0:                    add(DEGEN_EMPTY);      return
 4. if N == 1:                    add(DEGEN_SINGLETON);  return
 5. if is_prime(N):
       if k == 1:                add(SHAPE_PRIME_1D)   # Info
       else:                     add(SHAPE_PRIME_ND);  return
 6. if N < 8*len(T) and padding_bits(T, N) != 0:
                                  add(ALIGN_BYTE);      return
 7. n1    <- popcount_masked(T, N)
 8. n0    <- N - n1
 9. rho   <- min(n1, n0) / floor(N / 2)
10. h     <- ShannonEntropy(n1, n0)
11. h_max <- MaxEntropy(N)
12. h_til <- h / h_max
13. D_H   <- hamming_masked(T, R_ref, N)
14. if h_til < tau_H:     add(COLLAPSE)   # independent predicate
15. if rho   < tau_rho:   add(SKEW)       # independent predicate
16. if D_H   > tau_D * N: add(DRIFT)      # Info; advisory
17. if no Err or Warn flags: add(HEALTHY)
18. return (rho, h_til, D_H, flags)
```

Steps 14–16 are independent predicates. `COLLAPSE` and `SKEW` co-emit when both fire. `DRIFT` is `Info` severity and does not affect the `HEALTHY` aggregation at step 17.

### 4.3. Reference Implementation (C++20)

The listing is self-contained, compiles under `-Wall -Wextra -pedantic -Wconversion`, and performs no out-of-bounds access.

```cpp
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <optional>
#include <vector>

namespace vlb {

enum class Severity { Ok, Info, Warn, Err };

enum class Code {
    ShapeLen, ShapePrimeND, ShapePrime1D, AlignByte,
    DegenEmpty, DegenSingleton,
    Collapse, Skew, Drift, Healthy
};

struct Flag { Severity severity; Code code; };

struct Result {
    std::vector<Flag> flags;
    double   rho     = 0.0;
    double   h_norm  = 0.0;
    uint64_t hamming = 0;
    bool     ok      = false;
};

// Deterministic Miller-Rabin for all 64-bit n.
static uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t r = 1; base %= mod;
    while (exp) {
        if (exp & 1) r = static_cast<uint64_t>((__uint128_t)r * base % mod);
        base = static_cast<uint64_t>((__uint128_t)base * base % mod);
        exp >>= 1;
    }
    return r;
}

static bool is_prime(uint64_t n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if ((n & 1) == 0) return false;
    uint64_t d = n - 1; int s = 0;
    while ((d & 1) == 0) { d >>= 1; ++s; }
    static constexpr uint64_t bases[] = { 2,3,5,7,11,13,17,19,23,29,31,37 };
    for (uint64_t a : bases) {
        if (a % n == 0) continue;
        uint64_t x = mod_pow(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool witness = true;
        for (int r = 1; r < s; ++r) {
            x = static_cast<uint64_t>((__uint128_t)x * x % n);
            if (x == n - 1) { witness = false; break; }
        }
        if (witness) return false;
    }
    return true;
}

static uint64_t popcount_masked(const std::vector<uint8_t>& t, uint64_t n_bits) {
    const uint64_t full = n_bits / 8;
    const uint8_t  rem  = static_cast<uint8_t>(n_bits % 8);
    uint64_t c = 0;
    for (uint64_t i = 0; i < full; ++i) c += std::popcount(t[i]);
    if (rem && full < t.size()) {
        const uint8_t mask = static_cast<uint8_t>((1u << rem) - 1u);
        c += std::popcount(static_cast<uint8_t>(t[full] & mask));
    }
    return c;
}

static uint64_t hamming_masked(const std::vector<uint8_t>& a,
                               const std::vector<uint8_t>& b,
                               uint64_t n_bits) {
    const uint64_t full = n_bits / 8;
    const uint8_t  rem  = static_cast<uint8_t>(n_bits % 8);
    uint64_t c = 0;
    for (uint64_t i = 0; i < full; ++i)
        c += std::popcount(static_cast<uint8_t>(a[i] ^ b[i]));
    if (rem) {
        const uint8_t mask = static_cast<uint8_t>((1u << rem) - 1u);
        c += std::popcount(static_cast<uint8_t>((a[full] ^ b[full]) & mask));
    }
    return c;
}

static uint8_t padding_bits(const std::vector<uint8_t>& t, uint64_t n_bits) {
    const uint64_t rem = n_bits % 8;
    if (rem == 0 || n_bits / 8 >= t.size()) return 0;
    const uint8_t mask = static_cast<uint8_t>(~((1u << rem) - 1u));
    return static_cast<uint8_t>(t[n_bits / 8] & mask);
}

static double max_entropy(uint64_t n) {
    if (n < 2) return 0.0;
    const uint64_t hi = (n + 1) / 2;
    const uint64_t lo = n / 2;
    const double pH = static_cast<double>(hi) / n;
    const double pL = static_cast<double>(lo) / n;
    return -pH * std::log2(pH) - pL * std::log2(pL);
}

struct Options {
    double tau_H   = 0.1;
    double tau_rho = 0.1;
    double tau_D   = 0.4;
};

static void add(Result& r, Severity s, Code c) { r.flags.push_back({s, c}); }

static Result diagnose(const std::vector<uint8_t>& tensor,
                       const std::vector<uint8_t>& reference,
                       const std::vector<uint64_t>& expected_dims,
                       std::optional<uint64_t> n_bits = std::nullopt,
                       Options opt = {}) {
    Result r;

    if (reference.size() < tensor.size()) {
        add(r, Severity::Err, Code::ShapeLen); return r;
    }

    const uint64_t N = n_bits.value_or(static_cast<uint64_t>(tensor.size()) * 8);
    if (n_bits && (N == 0 || 8 * (tensor.size() - 1) >= N || N > 8 * tensor.size())) {
        add(r, Severity::Err, Code::ShapeLen); return r;
    }

    const uint64_t prod = std::accumulate(
        expected_dims.begin(), expected_dims.end(),
        uint64_t{1}, std::multiplies<uint64_t>());
    if (N != prod) { add(r, Severity::Err, Code::ShapeLen); return r; }

    if (N == 0) { add(r, Severity::Err, Code::DegenEmpty);     return r; }
    if (N == 1) { add(r, Severity::Err, Code::DegenSingleton); return r; }

    if (is_prime(N)) {
        if (expected_dims.size() == 1) add(r, Severity::Info, Code::ShapePrime1D);
        else { add(r, Severity::Err, Code::ShapePrimeND); return r; }
    }

    if (n_bits && padding_bits(tensor, N) != 0) {
        add(r, Severity::Err, Code::AlignByte); return r;
    }

    const uint64_t n1 = popcount_masked(tensor, N);
    const uint64_t n0 = N - n1;

    const double rho   = static_cast<double>(std::min(n1, n0)) /
                         static_cast<double>(N / 2);
    const double p     = static_cast<double>(n1) / N;
    const double h     = (p > 0 && p < 1)
                       ? -p * std::log2(p) - (1 - p) * std::log2(1 - p)
                       : 0.0;
    const double h_til = h / max_entropy(N);

    r.rho     = rho;
    r.h_norm  = h_til;
    r.hamming = hamming_masked(tensor, reference, N);

    if (h_til < opt.tau_H)          add(r, Severity::Err,  Code::Collapse);
    if (rho   < opt.tau_rho)        add(r, Severity::Warn, Code::Skew);
    if (r.hamming > opt.tau_D * N)  add(r, Severity::Info, Code::Drift);

    const bool clean = std::none_of(r.flags.begin(), r.flags.end(),
        [](const Flag& f) {
            return f.severity == Severity::Err || f.severity == Severity::Warn;
        });
    if (clean) add(r, Severity::Ok, Code::Healthy);
    r.ok = clean;
    return r;
}

} // namespace vlb
```

`DRIFT` is `Severity::Info` and the `clean` predicate ignores `Info`. This matches the aggregation rule in §2.4.

### 4.4. C++17 Backport

The two non-portable facilities are `std::popcount` and `__uint128_t`. The repository's current `-std=c++17` build requires:

```cpp
static uint64_t popcount64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return static_cast<uint64_t>(__builtin_popcountll(x));
#else
    uint64_t c = 0;
    while (x) { x &= (x - 1); ++c; }
    return c;
#endif
}
static uint8_t popcount8(uint8_t x) { return static_cast<uint8_t>(popcount64(x)); }
```

and, for `mod_pow`, either `__builtin_mul_overflow` with manual reduction, or the MSVC `_umul128` intrinsic on that platform. The 64-bit Miller-Rabin path is correct across all bases listed; the backport preserves semantics.

---

## 5. Proposed Experimental Design

The protocol below is **proposed**; no data are attached to this version. It is stated in advance so that results, when produced, can be evaluated against a pre-registered design rather than a post hoc one.

1. **Baseline.** FP32 `nanoLM` with fixed seed; binarized teacher produced by sign quantization of converged weights.
2. **Distillation runs.** $k \ge 5$ independent runs; per-epoch extraction of $(\rho, \tilde H, D_H, \mathcal{F})$ per layer.
3. **Correlation study.** Rank correlation between per-layer $\tilde H$ at epoch $t$ and validation accuracy at epoch $t + \Delta$. The study asks whether $\tilde H \to 0$ temporally precedes accuracy collapse; the outcome is measured, not assumed.
4. **Sensitivity sweep.** $\tau_\rho, \tau_H, \tau_D$ swept over a grid; results reported as ROC-style curves rather than point estimates, to avoid overfitting the thresholds.
5. **Negative control.** $D_H$ between independent random binarizations at matched $\rho$ and $\tilde H$, to confirm that the comparator distinguishes structural drift from chance.

Reproducibility requirements, when the study is executed: source revision (commit hash), compiler and version, build flags, hardware, warm-up policy, iteration count, aggregation method, and raw samples. The 32× memory target of the parent repository remains a design goal, not a measured result.

---

## 6. Limitations

- **Structural, not semantic.** $\rho$ and $\tilde H$ are order-insensitive. A tensor with ideal balance and maximal entropy may still compute the wrong function if the bit arrangement is wrong. The diagnostic does not substitute for validation accuracy.
- **$D_H$ is a distance, not a correctness signal.** Low $D_H$ to a poor teacher is not evidence of quality.
- **Thresholds are empirical.** The defaults $\tau_\rho = \tau_H = 0.1$ and $\tau_D = 0.4$ are starting points for the sweep in §5; they are not claimed to be universal or calibrated.
- **Bit-length awareness is the caller's responsibility.** The bit-packed path requires the caller to supply `N_bits` and zero-fill padding. Silent mispacking is detected only when padding is nonzero or `SHAPE_LEN` fires.
- **Primality is not amortized.** The Miller-Rabin path is negligible per call at realistic tensor sizes, but no caching across layers is implemented. Production use is expected to memoize by $N$.
- **No production-readiness claim is made.** The method is a diagnostic; model-quality parity, end-to-end speedup, and production safety each require independent evidence.

---

## 7. Conclusion

The bit-balance ratio and parity-normalized entropy give a small, cheap, reproducible structural signal for monitoring the distillation of 1-bit neural networks. The prime-length case, previously ambiguous, is resolved by a rank-aware gate that admits legitimate 1-D bit-packed tensors and flags shape mismatches as structural impossibilities. A closed enum status grammar and a normative notation-to-ASCII-to-identifier mapping support automated parsing and reimplementation from the specification alone. The method is a supplementary monitor: it exposes how 1-bit capacity is allocated and where topology is drifting, but it does not measure semantic correctness. It is intended to be paired with validation benchmarks and reported against the exact source revision, toolchain, and workload on which it was obtained.

---

## Appendix A. Notation, ASCII, and Identifier Mapping (Normative)

| Glyph | ASCII token | C++ identifier | Definition | Domain |
|---|---|---|---|---|
| $N$ | `N_bits` | `N` | tensor length in **bits** | $\mathbb{Z}_{\ge 0}$ |
| $\lvert T \rvert$ | `T_bytes` | `tensor.size()` | tensor length in **bytes** | $\mathbb{Z}_{\ge 0}$ |
| $N_1$ | `n1` | `n1` | ones count | $[0, N]$ |
| $N_0$ | `n0` | `n0` | zeros count, $N - N_1$ | $[0, N]$ |
| $R$ | `R_raw` | — | $\max(N_1,N_0)/\min(N_1,N_0)$ (audit only) | $[1, \infty)$ |
| $\rho$ | `rho` | `rho` | normalized balance | $[0, 1]$, $N \ge 2$ |
| $H$ | `H_raw` | `h` | Shannon entropy, bits | $[0, 1]$ |
| $H_{\max}$ | `H_max` | `max_entropy(N)` | parity-corrected maximum | $[0, 1]$ |
| $\tilde H$ | `H_norm` | `h_til` | normalized entropy | $[0, 1]$, $N \ge 2$ |
| $D_H$ | `D_H` | `d_h` | Hamming distance | $[0, N]$ |
| $\tau_\rho$ | `tau_rho` | `opt.tau_rho` | skew threshold | $(0, 1)$ |
| $\tau_H$ | `tau_H` | `opt.tau_H` | collapse threshold | $(0, 1)$ |
| $\tau_D$ | `tau_D` | `opt.tau_D` | drift threshold | $(0, 1)$ |

**Encoding policy.** Manuscript glyphs are Unicode (NFC). All identifiers, all serialized codes, and all diagnostic output are ASCII-only. The mapping above binds the two registers and is normative. In particular:

- The subscript underscore in the glyph column and the separator underscore in the identifier column denote different objects and must not be conflated.
- $R$ denotes a scalar ratio; `R_ref` denotes a reference **tensor**. Distinct objects, distinct names.
- $\operatorname{Popcount}(\cdot)$ is `std::popcount` (byte) or `popcount_masked` (bit-length-aware) in code.
- XNOR is the inference operator; XOR is the diagnostic operator; they are complements, not aliases.

---

## References

- Courbariaux, M., Hubara, I., Soudry, D., El-Yaniv, Y., & Bengio, Y. (2016). *Binarized Neural Networks: Training Deep Neural Networks with Weights and Activations Constrained to +1 or −1.* arXiv:1602.02830.
- Hinton, G., Vinyals, O., & Dean, J. (2015). *Distilling the Knowledge in a Neural Network.* arXiv:1503.02531.
- Rastegari, M., Ordonez, V., Redmon, J., & Farhadi, A. (2016). *XNOR-Net: ImageNet Classification Using Binary Convolutional Neural Networks.* arXiv:1603.05279.
- Hubara, I., Courbariaux, M., Soudry, E., El-Yaniv, Y., & Bengio, Y. (2017). *Quantized Neural Networks: Training Neural Networks with Low Precision Weights and Activations.* Journal of Machine Learning Research, 18(187), 1–30.
