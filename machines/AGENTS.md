# Machine definition guidance

Read ../AGENTS.md and UPSTREAM.md. Keep real historical IBM hardware,
community extensions and fictional designs distinct. State an instruction
ceiling separately from address width, ABI, object format and OS services.
Document selector status without enabling a feature merely by naming it.
Use cited architecture facts; do not import inherited encoder tables or manuals.
Executable encoding/lowering belongs to its owning component src/. There is
no alternate product source tree here. Component work remains in its own
single doc/BACKLOG.md; coordinate shared target changes explicitly.
