# Sampling views and non-stopping instrumentation investigation

Inspected the current core debugger `setRead` registrations (111 source occurrences,
including the commented MyVision expansion registration). The active bodies fall
into direct arrays/storage reads, byte-swizzled storage, reconstructed OAM bytes,
Atari 5200 side-effect-free bus peeks, and N64 cache/chip views.

- Ordinary storage callbacks do not run a guest instruction or invoke a guest MMIO
  read. MD/PCE/SFC palette/OAM views reconstruct logical bytes from storage; they
  are consistent debugger views, not a promise of a backing-file byte layout.
- `ares/a52/cpu/memory.cpp::readDebugger` uses `peekBus`, which selects const peek
  paths rather than the ordinary CPU read paths. This remains a logical CPU Bus
  debugger region, not a guarantee that unrelated external inputs are latched.
- `ares/n64/rdram/debugger.cpp` exposes DCache separately. It stays unsupported in
  universal enumeration; explicit CPU/cache views use the N64 adapter.
- **Found an observable exception:** the core RDRAM debugger callback calls the
  chip-mapped read. Before identity mapping, `rdram.cpp::Writable::degrade` can
  consume guest random state; homebrew reads can also update debug diagnostics.
  Universal host RDRAM region reads now use the raw `Memory::Writable` storage
  accessor, bypassing mapping/degradation/guest RNG. Single and batched universal
  reads and native snapshots/recorders share that accessor. N64 CPU-view readMany
  rejects `rdram_not_initialized` before callbacks until identity mapping is ready.
  The legacy N64 single-read contract remains unchanged. This is an explicit view
  distinction, not CPU virtual-address translation through raw regions.

The universal atomic guarantee covers supported callback views in one emulation-
thread dispatch. It does not synchronize worker/GPU devices or external inputs.
New debugger callbacks need the same review before promising sampling semantics.

## Non-stopping call/write buffers

The existing N64 hooks cannot honestly provide a zero-slowdown universal API:

- `cpu/debug-control.cpp::updateActive` enables the debug engine for traces,
  breakpoints/watchpoints and call gates. `cpu/cpu.cpp` bypasses the recompiler
  while CPU/RSP debugging is active, and invokes before/after-instruction hooks
  on the interpreter path. Existing call observations therefore do not establish
  recompiler coverage or its register-capture costs.
- CPU watch hooks in `cpu/memory.cpp` see CPU accesses. PI DMA has separate
  provenance callbacks (`pi/dma.cpp`); RSP DMA and direct DMEM/IMEM accesses follow
  different paths. CPU watchpoints alone cannot promise a write map that covers
  DMA, RSP, renderer writes and state restoration.
- The snapshot maps implemented here deliberately report **changed contents**.
  Writing a value and restoring it before the next capture leaves no change.
  Reset/load invalidates maps/candidates via epoch-scoped snapshot handles.
- A future event recorder needs independently gated interpreter and recompiler
  emission, a bounded native ring, explicit overflow counts, owner cleanup, and
  declared pre/post-instruction registers (especially branch delay slots). It also
  needs a workload measurement with instrumentation disabled and enabled. Reusing
  the stopping debug gate would change the executor and is not an acceptable way
  to claim low-cost live recording.

No call/write-buffer API or no-slowdown claim was added. The frame memory recorder
uses frame/submission boundaries independently of those execution hooks.
