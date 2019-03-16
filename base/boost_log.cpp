#include "base/boost_log.h"

#include "base/format.h"
#include "base/strings/string16.h"

#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/utility/functional/save_result.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace {

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", BoostLogSeverity)

// Our attribute value visitor
struct StringFormatter {
  typedef std::string result_type;

  template <class T>
  result_type operator()(const T& value) const {
    return Format(value);
  }
};

std::string ToString(const boost::log::attribute_value& attr) {
  // NOTE: It's not clear why long is required at least under Windows.
  using Types = boost::mpl::vector<bool, int16_t, uint16_t, int32_t, uint32_t,
                                   int64_t, uint64_t, long, float, double,
                                   std::string, base::string16>;

  StringFormatter::result_type result;
  boost::log::visit<Types>(attr,
                           boost::log::save_result(StringFormatter{}, result));

  return result;
}

const char* ToString(BoostLogSeverity severity) {
  const char* strs[] = {"Debug", "Info", "Warning", "Error", "Critical"};
  static_assert(std::size(strs) == static_cast<size_t>(BoostLogSeverity::Count),
                "No severities");
  return strs[static_cast<size_t>(severity)];
}

void FormatLogRecord(const boost::log::record_view& record,
                     bool console,
                     boost::log::formatting_ostream& stream) {
  auto severity = BoostLogSeverity::Info;
  std::string channel;
  std::string line_id;
  using TimeStamp = boost::log::attributes::local_clock::value_type;
  TimeStamp timestamp;
  std::string message;

  std::vector<std::pair<const boost::log::attribute_name*, std::string>> attrs;
  attrs.reserve(record.attribute_values().size());

  for (auto& attr : record.attribute_values()) {
    if (attr.first == boost::log::aux::default_attribute_names::severity()) {
      severity = attr.second.extract_or_default(BoostLogSeverity::Info);
      continue;
    } else if (attr.first ==
               boost::log::aux::default_attribute_names::timestamp()) {
      timestamp = attr.second.extract_or_default(TimeStamp());
      continue;
    }

    auto string_value = ToString(attr.second);
    if (!string_value.empty()) {
      if (attr.first == boost::log::aux::default_attribute_names::channel())
        channel = std::move(string_value);
      else if (attr.first ==
               boost::log::aux::default_attribute_names::line_id())
        line_id = std::move(string_value);
      else if (attr.first ==
               boost::log::aux::default_attribute_names::message())
        message = std::move(string_value);
      else
        attrs.emplace_back(&attr.first, std::move(string_value));
    }
  }

  stream << "#" << line_id << " ";

  if (!console)
    stream << "[" << boost::posix_time::to_simple_string(timestamp) << "] ";

  stream << "[" << ToString(severity) << "] ";

  if (!channel.empty())
    stream << channel << ": ";

  stream << message;

  for (auto& attr : attrs)
    stream << " | " << *attr.first << " = " << attr.second;
}

template <bool kConsole>
void FormatLogRecordT(const boost::log::record_view& record,
                      boost::log::formatting_ostream& stream) {
  FormatLogRecord(record, kConsole, stream);
}

}  // namespace

void InitBoostLogging(const std::filesystem::path& path, bool console) {
  boost::log::add_common_attributes();

  if (!path.empty()) {
    try {
      auto new_path = path;
      auto suffix = "_%Y-%m-%d_%H-%M-%S-%N" + new_path.extension().string();
      new_path.replace_extension();
      new_path += suffix;
      auto sink = boost::log::add_file_log(
          boost::log::keywords::file_name = new_path,
          boost::log::keywords::rotation_size = 10 * 1024 * 1024,
          boost::log::keywords::max_size = 100 * 1024 * 1024,
          boost::log::keywords::time_based_rotation =
              boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
          boost::log::keywords::format =
              "%TimeStamp% (%LineID%) <%Severity%>: %Message%");
      sink->set_formatter(&FormatLogRecordT<false>);
      sink->locked_backend()->auto_flush();

    } catch (const boost::filesystem::filesystem_error&) {
    }
  }

  if (console) {
    auto sink = boost::log::add_console_log();
    boost::log::core::get()->add_sink(sink);
    sink->set_formatter(&FormatLogRecordT<true>);
    sink->set_filter(severity >= BoostLogSeverity::Info);
  }
}
