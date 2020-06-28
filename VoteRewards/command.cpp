#include "global.h"

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *) userp)->append((char *) contents, size * nmemb);
  return size * nmemb;
}

class VoteCommand : public Command {
public:
  VoteCommand() {}

  void execute(CommandOrigin const &origin, CommandOutput &output) {
    if (origin.getOriginType() != CommandOriginType::Player) {
      output.error("commands.generic.error.invalidPlayer");
      return;
    }
    auto source = (Player *) origin.getEntity();
    auto &db    = Mod::PlayerDatabase::GetInstance();
    auto it     = db.Find(source);
    if (!it) return;
    if (inProcess.count(it->xuid)) {
      output.error("commands.vote.process");
      return;
    }
    inProcess[it->xuid] = true;
    VoteRewards::postAsync([playerName = it->name, Token = settings.token, votelink = settings.link]() {
      CURL *curl_handle;
      CURLcode res;

      std::string resu;

      curl_global_init(CURL_GLOBAL_ALL);

      curl_handle = curl_easy_init();
      std::string name = playerName;
      std::replace(name.begin(), name.end(), ' ', '+');
      std::string link =
          "https://minecraftpocket-servers.com/api/?object=votes&element=claim&key=" + Token + "&username=" + name;
      curl_easy_setopt(curl_handle, CURLOPT_URL, link.c_str());
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &resu);
      curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

      res = curl_easy_perform(curl_handle);

      if (res != CURLE_OK) {
        LOGE("curl_easy_perform() failed: %s") % curl_easy_strerror(res);
        curl_easy_cleanup(curl_handle);
      } else {
        curl_easy_cleanup(curl_handle);
        if (resu == "0") {
          LocateService<ServerInstance>()->queueForServerThread([playerName, votelink = votelink]() {
            auto it = Mod::PlayerDatabase::GetInstance().Find(playerName);
            if (!it) return;
            auto pk = TextPacket::createTranslatedMessageWithParams(
                "commands.vote.notvoted", {votelink});
            it->player->sendNetworkPacket(pk);
            inProcess.erase(it->xuid);
          });
          return;
        } else if (resu == "1") {
          std::string resu1;

          curl_global_init(CURL_GLOBAL_ALL);
          curl_handle = curl_easy_init();

          std::string link = "https://minecraftpocket-servers.com/api/";
          curl_easy_setopt(curl_handle, CURLOPT_URL, link.c_str());
          curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
          curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &resu1);
          curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
          curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
          // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
          std::string data = "action=post&object=votes&element=claim&key=" + Token + "&username=" + name;
          curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.c_str());

          res = curl_easy_perform(curl_handle);
          if (res != CURLE_OK) {
            LOGE("curl_easy_perform() failed: %s") % curl_easy_strerror(res);
          } else {
            LocateService<ServerInstance>()->queueForServerThread([playerName]() {
              auto it = Mod::PlayerDatabase::GetInstance().Find(playerName);
              if (!it) return;
              auto pk = TextPacket::createTextPacket<TextPacketType::SystemMessage>("commands.vote.voted");
              it->player->sendNetworkPacket(pk);
              if (settings.executeCommands) {
                for (std::string s : settings.commands) {
                  auto originCommand = std::make_unique<Mod::CustomCommandOrigin>();
                  // originCommand->allowSelectorExpansion = false;
                  std::string command = s;
                  boost::replace_all(command, "%name%", it->name);
                  auto value = Mod::CommandSupport::GetInstance().ExecuteCommand(std::move(originCommand), command);
                }
              }
              Mod::Economy::UpdateBalance(it->player, settings.money, "vote");
              inProcess.erase(it->xuid);
            });
            return;
          }
          curl_easy_cleanup(curl_handle);
        } else if (resu == "2") {
          LocateService<ServerInstance>()->queueForServerThread([playerName]() {
            auto it = Mod::PlayerDatabase::GetInstance().Find(playerName);
            if (!it) return;
            auto pk = TextPacket::createTextPacket<TextPacketType::SystemMessage>("commands.vote.alreadyclaimed");
            it->player->sendNetworkPacket(pk);
            inProcess.erase(it->xuid);
          });
          return;
        }
      }
      LocateService<ServerInstance>()->queueForServerThread([playerName]() {
        auto it = Mod::PlayerDatabase::GetInstance().Find(playerName);
        if (!it) return;
        inProcess.erase(it->xuid);
      });
      return;
    });
  }

  static void setup(CommandRegistry *registry) {
    using namespace commands;
    registry->registerCommand("vote", "commands.vote.description", CommandPermissionLevel::Any, CommandFlagCheat, CommandFlagNone);
    registry->registerOverload<VoteCommand>("vote");
  }
};

void initCommand(CommandRegistry *registry) { VoteCommand::setup(registry); }