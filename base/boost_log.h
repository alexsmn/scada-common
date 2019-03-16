#pragma once

#include <boost/log/common.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <filesystem>

enum class BoostLogSeverity { Debug, Info, Warning, Error, Critical, Count };

using BoostLogger = boost::log::sources::severity_channel_logger_mt<BoostLogSeverity>;

#define LOG_DEBUG(logger) BOOST_LOG_SEV(logger, ::BoostLogSeverity::Debug)
#define LOG_INFO(logger) BOOST_LOG_SEV(logger, ::BoostLogSeverity::Info)
#define LOG_WARNING(logger) BOOST_LOG_SEV(logger, ::BoostLogSeverity::Warning)
#define LOG_ERROR(logger) BOOST_LOG_SEV(logger, ::BoostLogSeverity::Error)
#define LOG_CRITICAL(logger) BOOST_LOG_SEV(logger, ::BoostLogSeverity::Critical)

#define LOG_TAG(name, value) boost::log::add_value(name, value)
#define LOG_SCOPED_TAG(logger, name, value) \
  BOOST_LOG_SCOPED_LOGGER_TAG(logger, name, value)

#define LOG_NAME(name) boost::log::keywords::channel = (name)

void InitBoostLogging(const std::filesystem::path& path, bool console);
