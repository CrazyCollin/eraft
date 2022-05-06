// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <eraftio/raft_messagepb.pb.h>
#include <network/raft_bootstrap.h>
#include <network/raft_encode_assistant.h>
#include <spdlog/spdlog.h>
#include <storage/write_batch.h>

#include <memory>

namespace network {

BootHelper *BootHelper::instance_ = nullptr;

uint64_t BootHelper::gCounter_ = 0;

BootHelper::BootHelper() {}

BootHelper::~BootHelper() {}

bool BootHelper::IsRangeEmpty(
    std::shared_ptr<storage::StorageEngineInterface> db, std::string startKey,
    std::string endKey) {
  // TODO: checkout is range empty in db
  return true;
}

//
// Save clusterId and storeId to storage engine
//
// Key: StoreIdent {0x01, 0x02}
// Value:
// message StoreIdent {
//     uint64 cluster_id = 1;
//     uint64 store_id = 2;
//     string addr = 3;
// }
//

bool BootHelper::DoBootstrapStore(std::shared_ptr<DBEngines> engines,
                                  uint64_t clusterId, uint64_t storeId,
                                  std::string storeAddr) {
  std::shared_ptr<raft_messagepb::StoreIdent> storeIdent(
      new raft_messagepb::StoreIdent());
  // if (!IsRangeEmpty(engines->kvDB_, "",
  //                   RaftEncodeAssistant::GetInstance()->MaxKeyStr())) {
  //   SPDLOG_ERROR("kv db is not empty");
  //   return false;
  // }

  // if (!IsRangeEmpty(engines->raftDB_, "",
  //                   RaftEncodeAssistant::GetInstance()->MaxKeyStr())) {
  // }

  storeIdent->set_cluster_id(clusterId);
  storeIdent->set_store_id(storeId);
  storeIdent->set_addr(storeAddr);

  if (RaftEncodeAssistant::GetInstance()->PutMessageToEngine(
          engines->kvDB_,
          RaftEncodeAssistant::GetInstance()->StoreIdentKeyStr(),
          *storeIdent) == storage::EngOpStatus::OK) {
    SPDLOG_DEBUG("put StoreIdent key succ");
    return true;
  }
  return false;
}

uint64_t BootHelper::AllocID() {
  gCounter_++;
  return gCounter_;
}

//
// make region from peerAddrMaps
// and call PrepareBoostrapCluster
//
std::pair<std::shared_ptr<metapb::Region>, bool> BootHelper::PrepareBootstrap(
    std::shared_ptr<DBEngines> engines, std::string storeAddr,
    std::map<std::string, int> peerAddrMaps) {
  std::shared_ptr<metapb::Region> region = std::make_shared<metapb::Region>();
  for (auto item : peerAddrMaps) {
    if (item.first == storeAddr) {
      region->mutable_region_epoch()->set_version(kInitEpochVer);
      region->mutable_region_epoch()->set_conf_ver(kInitEpoceConfVer);
      auto addPeer = region->add_peers();
      addPeer->set_id(item.second);
      addPeer->set_store_id(item.second);
      addPeer->set_addr(item.first);
      region->set_id(1);
      region->set_start_key("");
      region->set_end_key("");
      continue;
    }
    auto addNewPeer = region->add_peers();
    addNewPeer->set_id(item.second);
    addNewPeer->set_store_id(item.second);
    addNewPeer->set_addr(item.first);
  }
  if (!PrepareBoostrapCluster(engines, region)) {
    return std::make_pair(region, false);
  }
  return std::make_pair(region, true);
}

//
// write RegionLocalState,
// write InitialApplyState,
// write InitialRaftState to engine
//
bool BootHelper::PrepareBoostrapCluster(
    std::shared_ptr<DBEngines> engines,
    std::shared_ptr<metapb::Region> region) {
  raft_messagepb::RegionLocalState *state =
      new raft_messagepb::RegionLocalState();
  state->set_allocated_region(region.get());
  RaftEncodeAssistant::GetInstance()->PutMessageToEngine(
      engines->kvDB_,
      RaftEncodeAssistant::GetInstance()->PrepareBootstrapKeyStr(), *state);
  RaftEncodeAssistant::GetInstance()->PutMessageToEngine(
      engines->kvDB_,
      RaftEncodeAssistant::GetInstance()->RegionStateKey(region->id()), *state);
  storage::WriteBatch kvWB;
  WriteInitialApplyState(kvWB, region->id());
  engines->kvDB_->PutWriteBatch(kvWB);
  storage::WriteBatch raftWB;
  WriteInitialRaftState(raftWB, region->id());
  engines->raftDB_->PutWriteBatch(raftWB);
  SPDLOG_DEBUG("do prepare boostrap cluster successful!");
  return true;
}

//
//  write init apply state to storage writebatch
//
//  applyState [index:5 applied_index:5 term: 5]
//
void BootHelper::WriteInitialApplyState(storage::WriteBatch &kvWB,
                                        uint64_t regionId) {
  std::shared_ptr<raft_messagepb::RaftApplyState> applyState =
      std::make_shared<raft_messagepb::RaftApplyState>();
  raft_messagepb::RaftTruncatedState *truncatedState =
      new raft_messagepb::RaftTruncatedState();
  applyState->set_applied_index(
      RaftEncodeAssistant::GetInstance()->kRaftInitLogIndex);
  truncatedState->set_index(
      RaftEncodeAssistant::GetInstance()->kRaftInitLogIndex);
  truncatedState->set_term(
      RaftEncodeAssistant::GetInstance()->kRaftInitLogTerm);
  applyState->set_allocated_truncated_state(truncatedState);
  std::string val = applyState->SerializeAsString();
  SPDLOG_DEBUG("write init apply state : " + applyState->DebugString() + "to kvWB");
  kvWB.Put(RaftEncodeAssistant::GetInstance()->ApplyStateKey(regionId), val);
}

//
// write initial raft state to stroage writebatch
//
// raftState: [last_index: 5]
//
void BootHelper::WriteInitialRaftState(storage::WriteBatch &raftWB,
                                       uint64_t regionId) {
  std::shared_ptr<raft_messagepb::RaftLocalState> raftState =
      std::make_shared<raft_messagepb::RaftLocalState>();
  raftState->set_last_index(
      RaftEncodeAssistant::GetInstance()->kRaftInitLogIndex);
  // TODO: set hard state
  eraftpb::HardState* hardState = new eraftpb::HardState();
  hardState->set_term(RaftEncodeAssistant::GetInstance()->kRaftInitLogTerm);
  hardState->set_commit(RaftEncodeAssistant::GetInstance()->kRaftInitLogIndex);
  raftState->set_allocated_hard_state(hardState);
  std::string val = raftState->SerializeAsString();
  SPDLOG_DEBUG("write init raft state : " + raftState->DebugString() + "to raftWB");
  raftWB.Put(RaftEncodeAssistant::GetInstance()->RaftStateKey(regionId), val);
}

BootHelper *BootHelper::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new BootHelper();
  }
  return instance_;
}

}  // namespace network
