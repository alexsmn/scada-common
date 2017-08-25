#pragma once

namespace scada {

class WriteFlags {
 public:
	enum flag_t { SELECT = 0x0001 };

	WriteFlags() : flags_(0) { }
	explicit WriteFlags(unsigned flags) : flags_(flags) { }

	unsigned raw() const { return flags_; }
	bool get(flag_t flag) const { return (flags_&flag) != 0; }
	bool select() const { return get(SELECT); }

	WriteFlags& set(flag_t flag) { flags_ |= flag; return *this; }
	WriteFlags& set_select() { return set(SELECT); }

 private:
	unsigned	flags_;
};

} // namespace scada