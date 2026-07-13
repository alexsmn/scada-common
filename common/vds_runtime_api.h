#pragma once

#include <stdint.h>

#ifdef _WIN32
#define TC_VDS_RUNTIME_CALL __cdecl
#else
#define TC_VDS_RUNTIME_CALL
#endif

#ifdef TC_VDS_RUNTIME_IMPLEMENTATION
#ifdef _WIN32
#define TC_VDS_RUNTIME_EXPORT extern "C" __declspec(dllexport)
#else
#define TC_VDS_RUNTIME_EXPORT extern "C" __attribute__((visibility("default")))
#endif
#else
#define TC_VDS_RUNTIME_EXPORT extern "C"
#endif

enum TcVdsRuntimeDocumentKind : int32_t {
  TC_VDS_RUNTIME_DOCUMENT_KIND_AUTO = 0,
  TC_VDS_RUNTIME_DOCUMENT_KIND_VDS = 1,
  TC_VDS_RUNTIME_DOCUMENT_KIND_SDE = 2,
  TC_VDS_RUNTIME_DOCUMENT_KIND_XSDE = 3,
};

struct TcVdsRuntimeRect {
  double x;
  double y;
  double width;
  double height;
};

struct TcVdsRuntimeError {
  int32_t code;
  char message[512];
};

struct TcVdsRuntimeDocumentInfo {
  const char* title;
  const char* path;
  TcVdsRuntimeRect bounds;
};

struct TcVdsRuntimeShapeInfo {
  int32_t id;
  TcVdsRuntimeRect bounds;
  const char* name;
  const char* text;
  const char* data_source;
};

// Equipment-state colour palette for single-line / mimic displays (backlog
// 2.7). The host (client) injects the resolved `sl_*` design-token colours
// once via `set_state_palette`; the runtime maps each data source's semantic
// equipment state (decoded from the value/quality passed to
// `invalidate_data_source`) onto these colours at render time. This keeps the
// colour decision inside the renderer — the host passes tokens + raw state,
// never per-shape colours. Colours are packed 0x00RRGGBB (the high byte is
// ignored). See client/docs/ux/design-language.md §2 for the semantics; the
// energized colour is a restrained amber, never alarm-red.
struct TcVdsRuntimeStatePalette {
  uint32_t sl_live;       // energized primary conductor / live busbar
  uint32_t sl_energized;  // de-energized / idle conductor (neutral)
  uint32_t sl_closed;     // switching device closed / in service
  uint32_t sl_open;       // switching device open (neutral, not an alarm)
  uint32_t bad;           // bad-quality telemetry
  uint32_t uncertain;     // uncertain / intermediate state
};

using TcVdsRuntimeDocument = struct TcVdsRuntimeDocumentOpaque*;

struct TcVdsRuntimeApi {
  uint32_t abi_version;
  uint32_t struct_size;

  TcVdsRuntimeDocument(TC_VDS_RUNTIME_CALL* open_document)(
      const char* utf8_path,
      int32_t kind,
      TcVdsRuntimeError* error);
  void(TC_VDS_RUNTIME_CALL* close_document)(TcVdsRuntimeDocument document);

  int32_t(TC_VDS_RUNTIME_CALL* get_document_info)(
      TcVdsRuntimeDocument document,
      TcVdsRuntimeDocumentInfo* info,
      TcVdsRuntimeError* error);

  int32_t(TC_VDS_RUNTIME_CALL* render_bgra)(TcVdsRuntimeDocument document,
                                            uint8_t* pixels,
                                            int32_t width,
                                            int32_t height,
                                            int32_t stride,
                                            TcVdsRuntimeError* error);

  int32_t(TC_VDS_RUNTIME_CALL* hit_test)(TcVdsRuntimeDocument document,
                                         double page_x,
                                         double page_y,
                                         TcVdsRuntimeShapeInfo* info,
                                         TcVdsRuntimeError* error);

  // Records the latest value + quality for a data source (a shape identified by
  // its Name()). The runtime stores them and, when a state palette has been
  // injected, recolours the bound shape on the next render — closed/open carry
  // both shape and colour, energized/de-energized recolour the conductor, and
  // bad quality marks the symbol rather than freezing it. `value` is a UTF-8
  // keyword ("closed"/"open"/"on"/"off"/"energized"/"dead"/… — see
  // ParseEquipmentState in designer/core/single_line_state.h); `quality` is an
  // OPC UA StatusCode whose severity bits pick good/uncertain/bad.
  // `invalidated_bounds` receives the region needing repaint.
  int32_t(TC_VDS_RUNTIME_CALL* invalidate_data_source)(
      TcVdsRuntimeDocument document,
      const char* data_source,
      const char* value,
      int32_t quality,
      TcVdsRuntimeRect* invalidated_bounds,
      TcVdsRuntimeError* error);

  // ── ABI version 2 additions ───────────────────────────────────────────────
  // Appended so an ABI-1 caller (which stops at `invalidate_data_source`) still
  // sees a byte-compatible prefix; gate access on `struct_size`/`abi_version`.

  // Injects the equipment-state colour palette used to recolour shapes bound to
  // live telemetry. Pass null to clear it (revert to painting the authored
  // document verbatim). Returns non-zero on success.
  int32_t(TC_VDS_RUNTIME_CALL* set_state_palette)(
      TcVdsRuntimeDocument document,
      const TcVdsRuntimeStatePalette* palette,
      TcVdsRuntimeError* error);
};

// ABI 2 appended `set_state_palette` and the state-palette semantics for
// `invalidate_data_source`. The layout of every ABI-1 member is unchanged, so
// callers negotiate compatibility through `abi_version`/`struct_size`.
constexpr uint32_t TC_VDS_RUNTIME_ABI_VERSION = 2;

TC_VDS_RUNTIME_EXPORT const TcVdsRuntimeApi* TC_VDS_RUNTIME_CALL
TcVdsRuntimeGetApi(uint32_t requested_abi_version);
