#include "canon.h"

#include <chrono>

void CanonPTP::openSession() {
  PTPExtension::openSession();

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

void CanonPTP::closeSession() {
  if (eosEventThread.joinable()) {
    eosEventThread.request_stop();
    eosEventThread.join();
  }

  PTPExtension::closeSession();
}