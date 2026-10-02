# TSO31 bridge user guide

This is a synchronous 31-bit TSO-to-ELF entry adapter with selected storage,
terminal and native-file service calls. It is an experimental component, not
a z/PDOS kernel service implementation.

Run host source, ABI-caller and deck checks from the repository root:

```sh
crexx -nokeep tso31-bridge/scripts/build.crexx --args test
```

The standalone build also builds its assembler dependency. Product inputs
are in src/; tests use explicit statement expectations independently of the
frozen original reference archive. [The ABI](../architecture/ABI.md) and
[service facts](../architecture/SERVICES.md) describe entry registers,
non-reentrant callbacks, encoding and unresolved native prerequisites.

scripts/check-target.crexx retains the compile-only interface for an explicitly
selected compatible ELF producer and target-binutils prefixes. Its layout
assertions and host callbacks do not qualify complete linking or guest execution.
