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

  int32_t(TC_VDS_RUNTIME_CALL* render_bgra)(
      TcVdsRuntimeDocument document,
      uint8_t* pixels,
      int32_t width,
      int32_t height,
      int32_t stride,
      TcVdsRuntimeError* error);

  int32_t(TC_VDS_RUNTIME_CALL* hit_test)(
      TcVdsRuntimeDocument document,
      double page_x,
      double page_y,
      TcVdsRuntimeShapeInfo* info,
      TcVdsRuntimeError* error);

  int32_t(TC_VDS_RUNTIME_CALL* invalidate_data_source)(
      TcVdsRuntimeDocument document,
      const char* data_source,
      const char* value,
      int32_t quality,
      TcVdsRuntimeRect* invalidated_bounds,
      TcVdsRuntimeError* error);
};

constexpr uint32_t TC_VDS_RUNTIME_ABI_VERSION = 1;

TC_VDS_RUNTIME_EXPORT const TcVdsRuntimeApi* TC_VDS_RUNTIME_CALL
TcVdsRuntimeGetApi(uint32_t requested_abi_version);
