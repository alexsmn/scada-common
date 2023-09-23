#pragma once

enum FormatFlags {
  FORMAT_QUALITY = 0x0001,
  FORMAT_STATUS = 0x0002,  // manual & locked modifiers
  FORMAT_UNITS = 0x0004,
  FORMAT_COLOR = 0x0008,

  FORMAT_DEFAULT = FORMAT_QUALITY | FORMAT_STATUS | FORMAT_UNITS
};

struct ValueFormat {
  int flags = FORMAT_DEFAULT;
};
