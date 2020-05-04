#include "global.h"

#include <dllentry.h>


Settings settings;

DEFAULT_SETTINGS(settings);
std::unordered_map<uint64_t, bool> inProcess;

void dllenter() { Mod::CommandSupport::GetInstance().AddListener(SIG("loaded"), initCommand); }
void dllexit() {}

void PreInit() {
}
void PostInit() {}

bool VoteRewards::asyncThreadRunning;
rigtorp::MPMCQueue<std::function<void()>> VoteRewards::queue(10);

void VoteRewards::startAsyncThread() {
  asyncThreadRunning = true;
  std::thread thread([]() {
    std::function<void()> cb;
    while (asyncThreadRunning) {
      queue.pop(cb);
      if (cb) cb();
    }
  });
  thread.detach();
}

void VoteRewards::stopAsyncThread() {
  VoteRewards::postAsync([]() { asyncThreadRunning = false; });
}

void VoteRewards::postAsync(std::function<void()> callback) { queue.push(std::move(callback)); }

void ServerStart() { VoteRewards::startAsyncThread(); }

void BeforeUnload() { VoteRewards::stopAsyncThread(); }



