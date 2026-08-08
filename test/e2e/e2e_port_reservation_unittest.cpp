#include "test/e2e/e2e_port_reservation.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <gtest/gtest.h>

#include <set>
#include <utility>
#include <vector>

namespace client::test {
namespace {

// Whether `port` can actually be bound, i.e. the reservation released the probe
// socket it used to find it. A claim the child process cannot bind is worse
// than no claim at all.
bool CanBind(int port) {
  try {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor{
        io_context,
        boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(),
                                       static_cast<unsigned short>(port)}};
    return true;
  } catch (...) {
    return false;
  }
}

TEST(PortReservationTest, DefaultConstructedIsEmpty) {
  PortReservation reservation;
  EXPECT_FALSE(reservation);
  EXPECT_EQ(reservation.port(), 0);
}

TEST(PortReservationTest, ReservedPortIsBindable) {
  const PortReservation reservation = ReserveEphemeralPort();
  ASSERT_TRUE(reservation);
  EXPECT_TRUE(CanBind(reservation.port()))
      << "the reservation must hand the port over free for the launched "
         "process to bind; port "
      << reservation.port();
}

// The property the whole mechanism exists for: while one holder has a port, no
// other holder can take it. The lock is per open handle rather than per
// process, so a second claim inside one process exercises exactly the exclusion
// a second E2E process would hit.
TEST(PortReservationTest, SecondClaimOfHeldPortFails) {
  const PortReservation held = ReserveEphemeralPort();
  ASSERT_TRUE(held);

  const PortReservation second = TryReservePort(held.port());
  EXPECT_FALSE(second)
      << "a port already claimed was handed out again — two concurrent runs "
         "would aim two processes at port "
      << held.port();
}

TEST(PortReservationTest, ReleasingMakesThePortClaimableAgain) {
  int port = 0;
  {
    const PortReservation held = ReserveEphemeralPort();
    ASSERT_TRUE(held);
    port = held.port();
  }

  const PortReservation reclaimed = TryReservePort(port);
  EXPECT_TRUE(reclaimed);
  EXPECT_EQ(reclaimed.port(), port);
}

TEST(PortReservationTest, MoveTransfersTheClaim) {
  PortReservation source = ReserveEphemeralPort();
  ASSERT_TRUE(source);
  const int port = source.port();

  const PortReservation moved = std::move(source);
  EXPECT_FALSE(source);
  EXPECT_EQ(moved.port(), port);
  // The claim moved rather than being dropped along the way.
  EXPECT_FALSE(TryReservePort(port));
}

TEST(PortReservationTest, RejectsPortsOutsideTheValidRange) {
  EXPECT_FALSE(TryReservePort(0));
  EXPECT_FALSE(TryReservePort(-1));
  EXPECT_FALSE(TryReservePort(65536));
}

// What PortPool is: repeated reservations, all held at once. A cluster needs
// well over a dozen ports live simultaneously, so the pool's guarantee is that
// none of them repeats and none is released early.
TEST(PortReservationTest, ConcurrentReservationsAreAllDistinctAndAllHeld) {
  std::set<int> ports;
  std::vector<PortReservation> held;
  for (int i = 0; i < 16; ++i) {
    PortReservation reservation = ReserveEphemeralPort();
    ASSERT_TRUE(reservation);
    const int port = reservation.port();
    EXPECT_TRUE(ports.insert(port).second)
        << "port " << port << " was handed out twice";
    EXPECT_FALSE(TryReservePort(port)) << "port " << port << " is not held";
    held.push_back(std::move(reservation));
  }

  held.clear();
  for (int port : ports)
    EXPECT_TRUE(TryReservePort(port)) << "port " << port << " stayed claimed";
}

}  // namespace
}  // namespace client::test
