#pragma once

#include <yaml.h>
#include <string>
#include <map>
#include <base/log.h>
#include "base/playerdb.h"
#include <Actor/Actor.h>
#include <Actor/Player.h>
#include <mods/Economy.h>
#include <Item/ItemStack.h>
#include <Core/NBT.h>
#include <Core/DataIO.h>
#include <mods/CommandSupport.h>

#include <Packet/TransferPacket.h>
#include <Packet/TextPacket.h>

#include <Core/ServerInstance.h>
#include <Command/CommandContext.h>
#include <Core/MCRESULT.h>
#include <Command/MinecraftCommands.h>

#include <Core/json.h>

#include <thread>
#include "MPMCQueue.h"
#include <curl/curl.h>
#include <boost/algorithm/string.hpp>

struct Settings {
  std::string token;
  bool executeCommands;
  std::vector<std::string> commands;
  int money;
  std::string link;

  template <typename IO> static inline bool io(IO f, Settings &settings, YAML::Node &node) {
    return f(settings.token, node["TOKEN"]) && f(settings.money, node["moneyReward"]) &&
           f(settings.link, node["link"]) && f(settings.executeCommands, node["executeCommands"]) &&
           f(settings.commands, node["commands"]);
  }
};

DEF_LOGGER("VR");

extern Settings settings;

extern std::unordered_map<uint64_t, bool> inProcess;

class VoteRewards {
private:
  static bool asyncThreadRunning;
  static rigtorp::MPMCQueue<std::function<void()>> queue;

public:
  VoteRewards(std::string const &playerName);

  ~VoteRewards();

  static void startAsyncThread();

  static void stopAsyncThread();

  static void postAsync(std::function<void()> callback);
};

extern void initCommand(CommandRegistry *registry);

