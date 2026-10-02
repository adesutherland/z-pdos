# TSO31 bridge architecture

[ABI.md](ABI.md) describes the entry, service vector, callbacks, register and
character contracts. [SERVICES.md](SERVICES.md) records the public native
interface facts used for explicit service calls. src/ owns the maintained
entry assembler, C caller and ABI declarations. The exact original reference
is frozen in archive/; tests own normalized original statement expectations
and independently expected object bytes. This does not supply the native
PDPCLIB prerequisites or establish complete guest execution.
