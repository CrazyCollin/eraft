#include <Kv/server.h>
#include <Kv/raft_server.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>

namespace kvserver
{

Server::Server() {
    this->serverAddress_ = DEFAULT_ADDR;
}

Server::Server(std::string addr, Storage* st) {
    this->serverAddress_ = addr;
    this->st_ = st;
}

Status Server::Raft(ServerContext* context, const raft_serverpb::RaftMessage* request, Done* response) 
{
    std::cout << "start_key: " << request->start_key() << std::endl;
    auto raftStorage = static_cast<RaftStorage*>(this->st_);
    if(raftStorage->Raft(request))
    {
        return Status::OK;
    }
    else
    {
        return Status::CANCELLED;
    }
}

Status Server::RawGet(ServerContext* context, const kvrpcpb::RawGetRequest* request, kvrpcpb::RawGetResponse* response) 
{
    auto reader = this->st_->Reader(request->context());
    auto val = reader->GetFromCF(request->cf(), request->key());
    response->set_value(val);
    return Status::OK;
}

Status Server::RawPut(ServerContext* context, const kvrpcpb::RawPutRequest* request, kvrpcpb::RawPutResponse* response) 
{
    Put* pt = new Put(request->key(), request->value(), request->cf());
    std::vector<Modify> batchs;
    Modify modify(pt, OpType::Put);
    batchs.push_back(modify);
    if(!this->st_->Write(request->context(), batchs))
    {
        return Status::CANCELLED;
    }
    return Status::OK;
}

Status Server::RawDelete(ServerContext* context, const kvrpcpb::RawDeleteRequest* request, kvrpcpb::RawDeleteResponse* response) 
{
    Delete* dt = new Delete(request->key(), request->cf());
    std::vector<Modify> batchs;
    Modify modify(dt, OpType::Delete);
    batchs.push_back(modify);
    if(!this->st_->Write(request->context(), batchs))
    {
        return Status::CANCELLED;
    }
    return Status::OK;
}

Status Server::RawScan(ServerContext* context, const kvrpcpb::RawScanRequest* request, kvrpcpb::RawScanResponse* response) 
{
    
}

Status Server::Snapshot(ServerContext* context, const raft_serverpb::SnapshotChunk* request, Done* response) 
{

}

bool Server::RunLogic() {
    Server service;
    grpc::EnableDefaultHealthCheckService(true);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(this->serverAddress_, grpc::InsecureServerCredentials());

    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    // TODO: print log (server listening on server address)
    std::cout << "server listening on: " << this->serverAddress_ << std::endl;
    server->Wait();
}

Server::~Server() {

}

} // namespace kvserver
