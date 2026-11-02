#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace client::test {

// A minimal IEC 60870-5-104 controlling station -- a dispatch centre -- that a
// cluster test points at a tier's IEC 104 slave link to send single commands
// and read their confirmations.
//
// Raw APDUs over a socket rather than the iec60870 library: the harness is an
// INTERFACE library linked by the Qt client's suite and the root sweep, neither
// of which consumes that product, and a dispatch centre needs four frame kinds.
// The link parameters are the E2E fixture's (configuration-fixture.sql): common
// address 2 bytes, cause of transmission 2 bytes, information object address
// 3 bytes. IEC 60870-5-104 §5 (APCI) and IEC 60870-5-101 §7.3.2.1 (C_SC_NA_1).
class Iec104Master {
 public:
  // The confirmation the slave sent for a command.
  struct Confirmation {
    // Cause of transmission: 7 activation confirmation, 9 deactivation
    // confirmation.
    int cause = 0;
    // The P/N bit: the slave refused the command.
    bool negative = false;
  };

  Iec104Master();
  ~Iec104Master();

  Iec104Master(const Iec104Master&) = delete;
  Iec104Master& operator=(const Iec104Master&) = delete;

  // Connects to 127.0.0.1:`port` and starts data transfer (STARTDT act/con).
  // False, with `error()` set, if either does not happen within `timeout`.
  bool Connect(int port, std::chrono::milliseconds timeout);

  // Sends a single command (C_SC_NA_1) to `ioa` at common address
  // `common_address` -- with `select` the select step of two-stage control,
  // and with `deactivate` its release (cause 8) -- and waits for the matching
  // confirmation. nullopt, with `error()` set, if none arrives within
  // `timeout`; monitoring traffic in between is acknowledged and skipped.
  std::optional<Confirmation> SingleCommand(unsigned common_address,
                                            unsigned ioa,
                                            bool value,
                                            bool select,
                                            bool deactivate,
                                            std::chrono::milliseconds timeout);

  const std::string& error() const { return error_; }

 private:
  using Clock = std::chrono::steady_clock;

  bool Write(const std::vector<std::uint8_t>& apdu, Clock::time_point deadline);
  bool ReadExact(std::uint8_t* data,
                 std::size_t size,
                 Clock::time_point deadline);
  // Reads one APDU (start byte and length stripped), answering TESTFR and
  // acknowledging I-frames as it goes.
  std::optional<std::vector<std::uint8_t>> ReadApdu(Clock::time_point deadline);

  boost::asio::io_context io_context_;
  boost::asio::ip::tcp::socket socket_;
  std::uint16_t send_sequence_ = 0;
  std::uint16_t receive_sequence_ = 0;
  std::string error_;
};

}  // namespace client::test
