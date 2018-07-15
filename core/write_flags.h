#pragma once

namespace scada {

class WriteFlags {
 public:
	enum Flag { SELECT = 0x0001, PARAM = 0x0002 };

	WriteFlags() : flags_(0) { }
	explicit WriteFlags(unsigned flags) : flags_(flags) { }

	unsigned raw() const { return flags_; }
	bool get(Flag flag) const { return (flags_&flag) != 0; }
	bool select() const { return get(SELECT); }
  bool param() const { return get(PARAM); }

	WriteFlags& set(Flag flag) { flags_ |= flag; return *this; }
	WriteFlags& set_select() { return set(SELECT); }
  WriteFlags& set_param() { return set(PARAM); }

 private:
	unsigned	flags_;
};

} // namespace scada