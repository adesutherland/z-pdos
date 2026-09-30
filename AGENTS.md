# z/PDOS development guide

This repository is the home of z/PDOS and Mainframe Classic Tools. Keep each
component's implementation, user guide, architecture and tests together.

- Read the component guide and inspect Git status before editing. Preserve
  unrelated changes. Do not commit, push or publish without session authority.
- New assembler code and machine descriptions are original project material.
  Do not copy or translate inherited assembler/emulator implementations or
  tables. Use cited architecture facts and independently expected test vectors.
- Keep private research, guest assets, credentials, downloaded manuals and
  native reference listings/objects outside this repository. Preserve the actual
  licence of each component; a new MIT grant does not relicense inherited work.
- Use portable C89/C90 for the bootstrap assembler core. Host services cross
  explicit interfaces. No mandatory POSIX, native 64-bit integer, cREXX or
  dynamic plugin loader. Retained development scripts use cREXX.
- Internal text is UTF-8; the first parser accepts ASCII. Binary objects are
  octets, and target character encoding is explicit. Do not assume the host C
  execution character set is ASCII.
- Separate real architecture profiles, community extensions and counterfactual
  designs. Do not infer a new ISA or ABI from an experiment.
- Implementers run appropriate unit and integration QA; coordinators review
  delivery independently. Reuse unchanged qualification evidence. Host tests
  do not establish guest hosting, linking or complete OS rebuilding.
- Use GPT-6.1 Sol Extra High for coordination and complex work; GPT-6.1 Sol High
  is suitable for defined implementation and QA tasks.
