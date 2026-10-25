# BNN — Binary Neural Network Experiments

Experimental C++17 primitives for weight-only INT4 and 1-bit (BIN1) quantization, including packed matrix formats and matrix-vector kernels.

> **Status: research prototype.** These are standalone kernels, not integrated into a model runtime. No end-to-end speedup or model-quality claim is made without benchmarks.

## What's included

- `src/weight_loss.hpp`: row-wise symmetric INT4 quantization/dequantization, INT4 × dynamically quantized INT8 matrix-vector multiply, binary sign packing, and XNOR/popcount-style BIN1 matrix-vector multiply.
- `tests/weight_loss_test.cpp`: assertion-based smoke tests for INT4 packing/decoding, finite outputs, zero weights, a 35-column BIN1 tail mask, and rejection of infinite weights.

## Build and run tests

From the repository root:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/weight_loss_test.cpp -o /tmp/weight_loss_test
/tmp/weight_loss_test
```

Expected success output: `weight_loss tests passed`.

## Important limitations

- INT4 uses symmetric values in [-7, 7] and a per-row scale; this is a simple reference implementation, not a production quantization backend.
- BIN1 keeps a per-row mean-absolute weight scale and uses activation sign bits. Accuracy must be measured against an FP32 reference on representative workloads.
- The tests are smoke tests, not a complete correctness, fuzzing, sanitizer, or performance suite.
- No model integration, training/backpropagation changes, checkpoint conversion, latency benchmark, memory profile, or output-quality evaluation is included.
- Review large-dimension overflow and index-boundary behavior before production use.

## Next verification gates

1. Run the documented compile and test commands on the branch.
2. Add numerical error tests against an FP32 reference, including absolute and relative tolerances.
3. Run AddressSanitizer and UndefinedBehaviorSanitizer.
4. Add representative model integration and benchmark storage, peak memory, latency, and output-quality changes against FP32.
