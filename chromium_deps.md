# Chromium-Base Dependencies

Remaining chromium-base headers used in the common module.

## Threading
- `base/threading/thread_checker.h` — `node_service/fetch_queue.h`, `node_service/v1/node_fetch_status_queue.h`, `node_service/v1/node_fetch_status_tracker.h`

## Utilities
- `base/md5.h` — `common/file_system.h`

## Windows COM
- `base/win/scoped_variant.h` — `opc/variant_converter.h`, `vidicon/services/vidicon_monitored_data_point.cpp`
- `base/win/scoped_bstr.h` — `vidicon/services/vidicon_session.cpp`
