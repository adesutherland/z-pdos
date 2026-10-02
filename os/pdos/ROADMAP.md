# z/PDOS roadmap

I want z/PDOS to run the same classic load modules that we qualify on TSO,
without application rebuilds for PDOS. Version 0.1 establishes the maintained
Classic source-to-image route and the qualified 31-bit and 64-bit application
paths. [The qualification record](QUALIFICATION.md) owns the exact results.
Machine instruction ceilings remain consistent with the Classic and ELF SDK
profiles; service compatibility and addressing contracts remain explicit.

| Next work | Acceptance |
| --- | --- |
| AMODE24/RMODE24 application loading | Load code, save areas and parameters below 16 MiB, dispatch in the declared mode, and provide a separate constrained storage budget. Run the unchanged released TSO24 RXVM and its library-free I/O subset, including actual input and stopped output readback. The wider profiles retain their generous heaps. |
| Conditional storage-service errors | Extend the reached successful GETMAIN/FREEMAIN contract with checked invalid requests and truthful failures, without weakening the unchanged-binary qualification. |
| Shared machine selectors | Give Classic compiler and assembler the same named instruction ceilings as the ELF SDK; retain distinct object, ABI and guest-service contracts. |
| Native 64-bit kernel | Treat a full-width C kernel as a separate conversion. The current C32 kernel runs in AMODE31 while support code dispatches AMODE64 applications. |

## Operator interface

The following needs were observed while using the Lab's existing PDOS setup
and running the released applications. Each is a generic OS or command-processor
improvement, rather than a cREXX-specific workaround.

| Need | Acceptance |
| --- | --- |
| Batch-file delivery | Document and diagnose the native raw EBCDIC, hex-15 newline contract. Reject unsupported framing or joined commands clearly; provide a repeatable text-import route. |
| Command length | Report the limit before truncation. Console input currently has an 80-column limit; batch commands must fit the 200-byte buffer, with at most 198 characters before the delimiter. |
| Console input and prompts | Echo a typed line together, keep one prompt and preserve its editable input field. The newly built shell currently echoes individual characters on separate lines. |
| Results and scrollback | Make each application completion code and its command easy to identify. Long output and repeated screen snapshots must not make an older result look current. |
| Diagnostics | Distinguish an expected missing AUTOEXEC/BAT probe from an actionable failure. Announce an unsupported addressing/residence combination before dispatch. |
| Temporary package lifecycle | Support replacing, testing and removing packages while preserving the base OS. Reuse the Lab's stopped-disk checks and leases; qualification must leave one working OS image and no duplicate application installation. |

Maintain the source in its owning component, test general behavior and use
the next real build or unchanged application as the consumer. Git preserves
earlier checkpoints; a second maintained source tree is unnecessary.
