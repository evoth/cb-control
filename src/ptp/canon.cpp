#include "canon.h"

#include <chrono>

void CanonPTPCamera::openSession() {
  PTP::openSession();

  mesg(CanonOperationCode::EOSSetRemoteMode, {0x01});
  mesg(CanonOperationCode::EOSSetEventMode, {0x01});

  eosEventThread = std::jthread([this](std::stop_token stoken) {
    while (true) {
      if (stoken.stop_requested())
        return;
      recv(CanonOperationCode::EOSGetEvent);
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  });
}

void CanonPTPCamera::closeSession() {
  if (eosEventThread.joinable()) {
    eosEventThread.request_stop();
    eosEventThread.join();
  }

  PTP::closeSession();
}

void CanonPTPCamera::releaseShutter() {
  mesg(CanonOperationCode::EOSRemoteRelease);
}