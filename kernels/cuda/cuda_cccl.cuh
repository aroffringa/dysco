#ifndef DYSCO_CUDA_CCCL_CUH
#define DYSCO_CUDA_CCCL_CUH

/**
 * @file
 * Small collection of CUDA C++ helpers shared by the device kernels.
 *
 * These helpers are deliberately thin wrappers around CCCL (libcu++ / CUB),
 * which the project requires at the CUDA 12 level (either the CCCL bundled
 * with CUDA >= 12, or the standalone CCCL package):
 *
 *   - `cuda::std::span` / `cuda::std::mdspan` for typed, bounds-carrying
 *     device views (the kernels no longer take raw pointer/size pairs),
 *   - `cuda::atomic_ref` (libcu++) for scoped atomic maximum operations,
 *   - `cub::BlockReduce` + `cuda::maximum<>` for order-independent block-wide
 *     reductions,
 *   - `cuda::std` math in place of the global-namespace intrinsics.
 *
 * Keeping the CUDA-specific vocabulary in one place makes the individual
 * kernels express their algorithms rather than their CUDA plumbing, and makes
 * a future HIP/SYCL backend a matter of replacing this header.
 */

#include "../gpu_portability.h"

#include <cuda/atomic>
#include <cuda/functional>
#include <cuda/std/cmath>
#include <cuda/std/cstdint>
#include <cuda/std/mdspan>
#include <cuda/std/span>

namespace dyscostman {
namespace gpu {

/// Row-major two-dimensional device view, backed by `cuda::std::mdspan`.
/// All multi-dimensional arrays in this backend are contiguous and row-major
/// (the default `layout_right`), so a single alias is enough.
template <class T>
using MdSpan2D =
    cuda::std::mdspan<T, cuda::std::dextents<cuda::std::size_t, 2>>;

/// Component measure used by the encoder: `max(|re|, |im|)`, written in the
/// same signed form as `AFTimeBlockEncoder::fitToMaximum` so the non-finite
/// handling matches exactly.
[[nodiscard]] DYSCO_HD DYSCO_INLINE double componentMax(double re,
                                                        double im) noexcept {
  return cuda::std::fmax(cuda::std::fmax(re, im), -cuda::std::fmin(re, im));
}

/// Reinterpret a *non-negative* double as an unsigned 64-bit key.  The IEEE-754
/// bit patterns of non-negative doubles are order-isomorphic to unsigned
/// integers, so an integer `fetch_max` computes a double maximum.
[[nodiscard]] DYSCO_HD DYSCO_INLINE cuda::std::uint64_t
doubleToMaxKey(double value) noexcept {
  // Bit-cast through a union: `cuda::std::bit_cast` is not available on every
  // supported host compiler, and the union is the canonical CUDA idiom.
  union {
    double d;
    cuda::std::uint64_t u;
  } bits;
  bits.d = value;
  return bits.u;
}

[[nodiscard]] DYSCO_HD DYSCO_INLINE double maxKeyToDouble(
    cuda::std::uint64_t key) noexcept {
  union {
    cuda::std::uint64_t u;
    double d;
  } bits;
  bits.u = key;
  return bits.d;
}

/// Block-scoped atomic maximum of non-negative double keys.  On shared memory
/// libcu++ lowers this to the native shared atomic when the architecture
/// supports it and to a CAS loop otherwise.
DYSCO_HD DYSCO_INLINE void atomicMaxKeyBlock(
    cuda::std::uint64_t *address, cuda::std::uint64_t key) noexcept {
  cuda::atomic_ref<cuda::std::uint64_t, cuda::thread_scope_block> ref(*address);
  ref.fetch_max(key, cuda::memory_order_relaxed);
}

/// Device-scoped atomic maximum for a non-negative double.
DYSCO_HD DYSCO_INLINE void atomicMaxNonNegative(double *address,
                                                double value) noexcept {
  cuda::atomic_ref<cuda::std::uint64_t, cuda::thread_scope_device> ref(
      *reinterpret_cast<cuda::std::uint64_t *>(address));
  ref.fetch_max(doubleToMaxKey(value), cuda::memory_order_relaxed);
}

/// Device-scoped atomic OR, used for the K6 progress / early-exit flags.
DYSCO_HD DYSCO_INLINE void atomicOrDevice(int *address, int value) noexcept {
  cuda::atomic_ref<int, cuda::thread_scope_device> ref(*address);
  ref.fetch_or(value, cuda::memory_order_relaxed);
}

}  // namespace gpu
}  // namespace dyscostman

#endif  // DYSCO_CUDA_CCCL_CUH
