# OPC UA

The in-tree native OPC UA stack (binary wire codec, secure channel/crypto,
websocket transport, shared client/server session + runtime) has been extracted
to the standalone **opcuapp** repo (`third_party/opcuapp`). This directory is now
a thin CMake **shim**: `CMakeLists.txt` defines `scada_core_opcua`,
`scada_core_opcua_client`, and `scada_common_opcua_ws` as INTERFACE targets that
link `opcuapp::opcuapp` plus the boundary adapter `scada_opcua_bridge`
(`common/opcua_bridge/`), so existing consumers keep linking the same target
names. opcuapp's include directory is exported `BEFORE` so `#include "opcua/..."`
resolves to opcuapp.

The `.cpp`/`.h` native sources still physically present here are **dead** (no
longer compiled) and are kept only until the cutover is reviewed/committed; they
will be removed in a deliberate cleanup step. Do not add to them — new OPC UA
work belongs in opcuapp (the stack) or `common/opcua_bridge/` (the scada<->opcua
boundary).

For the formal OPC UA Binary type schema source of truth, refer to the OPC
Foundation schema:

- `https://files.opcfoundation.org/schemas/UA/1.04/Opc.Ua.Types.bsd.xml`

That schema should be treated as the external reference when updating binary
message definitions, code generation, or compatibility checks in opcuapp.
