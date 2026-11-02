#include "test/e2e/e2e_iec104_master.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <thread>

namespace client::test {

namespace {

constexpr std::uint8_t kStartByte = 0x68;
// U-format control octets (IEC 60870-5-104 §5.3).
constexpr std::uint8_t kStartDtAct = 0x07;
constexpr std::uint8_t kStartDtCon = 0x0B;
constexpr std::uint8_t kTestFrAct = 0x43;
constexpr std::uint8_t kTestFrCon = 0x83;

constexpr std::uint8_t kSingleCommand = 45;  // C_SC_NA_1
constexpr int kCauseActivation = 6;
constexpr int kCauseDeactivation = 8;
constexpr int kCauseActivationCon = 7;
constexpr int kCauseDeactivationCon = 9;
constexpr std::uint8_t kNegativeBit = 0x40;

std::vector<std::uint8_t> UFrame(std::uint8_t control) {
  return {kStartByte, 4, control, 0, 0, 0};
}

std::vector<std::uint8_t> SFrame(std::uint16_t receive_sequence) {
  return {kStartByte,
          4,
          0x01,
          0,
          static_cast<std::uint8_t>(receive_sequence << 1),
          static_cast<std::uint8_t>(receive_sequence >> 7)};
}

}  // namespace

Iec104Master::Iec104Master() : socket_{io_context_} {}

Iec104Master::~Iec104Master() {
  boost::system::error_code ec;
  socket_.close(ec);
}

bool Iec104Master::Connect(int port, std::chrono::milliseconds timeout) {
  const Clock::time_point deadline = Clock::now() + timeout;
  const boost::asio::ip::tcp::endpoint endpoint{
      boost::asio::ip::make_address("127.0.0.1"),
      static_cast<unsigned short>(port)};

  // The slave may not be listening yet: retry until the deadline.
  while (true) {
    boost::system::error_code ec = boost::asio::error::would_block;
    socket_.async_connect(endpoint,
                          [&ec](boost::system::error_code e) { ec = e; });
    io_context_.restart();
    io_context_.run_until(deadline);
    if (ec == boost::asio::error::would_block) {
      socket_.close();
      io_context_.restart();
      io_context_.run();
      error_ = "timed out connecting to port " + std::to_string(port);
      return false;
    }
    if (!ec)
      break;
    socket_.close();
    if (Clock::now() >= deadline) {
      error_ = "cannot connect to port " + std::to_string(port) + ": " +
               ec.message();
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
  }

  if (!Write(UFrame(kStartDtAct), deadline))
    return false;
  while (true) {
    std::optional<std::vector<std::uint8_t>> apdu = ReadApdu(deadline);
    if (!apdu) {
      error_ = "no STARTDT confirmation: " + error_;
      return false;
    }
    if (apdu->size() >= 4 && (*apdu)[0] == kStartDtCon)
      return true;
  }
}

std::optional<Iec104Master::Confirmation> Iec104Master::SingleCommand(
    unsigned common_address,
    unsigned ioa,
    bool value,
    bool select,
    bool deactivate,
    std::chrono::milliseconds timeout) {
  const Clock::time_point deadline = Clock::now() + timeout;
  const int cause = deactivate ? kCauseDeactivation : kCauseActivation;
  // SCO: bit 0 the state, bit 7 select/execute (IEC 60870-5-101 §7.2.6.15).
  const std::uint8_t sco =
      static_cast<std::uint8_t>((value ? 0x01 : 0x00) | (select ? 0x80 : 0x00));
  const std::vector<std::uint8_t> asdu{
      kSingleCommand,
      1,  // VSQ: one object
      static_cast<std::uint8_t>(cause),
      0,  // originator address
      static_cast<std::uint8_t>(common_address),
      static_cast<std::uint8_t>(common_address >> 8),
      static_cast<std::uint8_t>(ioa),
      static_cast<std::uint8_t>(ioa >> 8),
      static_cast<std::uint8_t>(ioa >> 16),
      sco};
  std::vector<std::uint8_t> apdu{
      kStartByte,
      static_cast<std::uint8_t>(4 + asdu.size()),
      static_cast<std::uint8_t>(send_sequence_ << 1),
      static_cast<std::uint8_t>(send_sequence_ >> 7),
      static_cast<std::uint8_t>(receive_sequence_ << 1),
      static_cast<std::uint8_t>(receive_sequence_ >> 7)};
  apdu.insert(apdu.end(), asdu.begin(), asdu.end());
  ++send_sequence_;
  if (!Write(apdu, deadline))
    return std::nullopt;

  const int wanted_cause =
      deactivate ? kCauseDeactivationCon : kCauseActivationCon;
  while (true) {
    std::optional<std::vector<std::uint8_t>> frame = ReadApdu(deadline);
    if (!frame) {
      error_ = "no confirmation for IOA " + std::to_string(ioa) + ": " + error_;
      return std::nullopt;
    }
    // An I-frame: 4 control octets, then the ASDU above.
    const std::vector<std::uint8_t>& f = *frame;
    if (f.size() < 4 + asdu.size() || (f[0] & 0x01) != 0 ||
        f[4] != kSingleCommand)
      continue;
    const unsigned frame_ioa = f[10] | (f[11] << 8) | (f[12] << 16);
    const int frame_cause = f[6] & 0x3F;
    if (frame_ioa != ioa || frame_cause != wanted_cause)
      continue;
    return Confirmation{.cause = frame_cause,
                        .negative = (f[6] & kNegativeBit) != 0};
  }
}

bool Iec104Master::Write(const std::vector<std::uint8_t>& apdu,
                         Clock::time_point deadline) {
  boost::system::error_code ec = boost::asio::error::would_block;
  boost::asio::async_write(
      socket_, boost::asio::buffer(apdu),
      [&ec](boost::system::error_code e, std::size_t) { ec = e; });
  io_context_.restart();
  io_context_.run_until(deadline);
  if (ec == boost::asio::error::would_block) {
    socket_.cancel();
    io_context_.restart();
    io_context_.run();
    error_ = "write timed out";
    return false;
  }
  if (ec) {
    error_ = "write failed: " + ec.message();
    return false;
  }
  return true;
}

bool Iec104Master::ReadExact(std::uint8_t* data,
                             std::size_t size,
                             Clock::time_point deadline) {
  boost::system::error_code ec = boost::asio::error::would_block;
  boost::asio::async_read(
      socket_, boost::asio::buffer(data, size),
      [&ec](boost::system::error_code e, std::size_t) { ec = e; });
  io_context_.restart();
  io_context_.run_until(deadline);
  if (ec == boost::asio::error::would_block) {
    socket_.cancel();
    io_context_.restart();
    io_context_.run();
    error_ = "read timed out";
    return false;
  }
  if (ec) {
    error_ = "read failed: " + ec.message();
    return false;
  }
  return true;
}

std::optional<std::vector<std::uint8_t>> Iec104Master::ReadApdu(
    Clock::time_point deadline) {
  while (true) {
    std::uint8_t header[2];
    if (!ReadExact(header, sizeof(header), deadline))
      return std::nullopt;
    if (header[0] != kStartByte || header[1] < 4) {
      error_ = "malformed APDU header";
      return std::nullopt;
    }
    std::vector<std::uint8_t> apdu(header[1]);
    if (!ReadExact(apdu.data(), apdu.size(), deadline))
      return std::nullopt;

    if (apdu[0] == kTestFrAct) {
      if (!Write(UFrame(kTestFrCon), deadline))
        return std::nullopt;
      continue;
    }
    if ((apdu[0] & 0x01) == 0) {
      // An I-frame: acknowledge it at once, so the slave's send window
      // (k = 12) never fills with monitoring traffic the test ignores.
      ++receive_sequence_;
      if (!Write(SFrame(receive_sequence_), deadline))
        return std::nullopt;
    }
    return apdu;
  }
}

}  // namespace client::test
