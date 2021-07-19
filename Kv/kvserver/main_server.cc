// @file Main.cc
// @author Colin
// This module impl kv main class.
// 

#include <Kv/server.h>
#include <Kv/config.h>
#include <Kv/raft_store.h>
#include <Kv/raft_server.h>
#include <Logger/Logger.h>

int main(int argc, char *argv[]) {

    // make conf
    std::shared_ptr<kvserver::Config> conf = std::make_shared<kvserver::Config>();
    conf->PrintConfigToConsole();

    // start raft store
    kvserver::RaftStorage* storage = new kvserver::RaftStorage(conf);
    storage->Start();
    Logger::GetInstance()->INFO("test log");

    // start rpc service server
    kvserver::Server svr(conf->storeAddr_, storage);
    svr.RunLogic();

    return 0;
}
