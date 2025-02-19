// Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"flag"
	"fmt"
	"net"
	"net/http"
	_ "net/http/pprof"
	"os"
	"os/signal"
	"strings"
	"syscall"

	"github.com/eraft-io/eraft/pkg/log"
	meta_svr "github.com/eraft-io/eraft/pkg/metaserver"
	pb "github.com/eraft-io/eraft/pkg/protocol"
	"google.golang.org/grpc/reflection"

	"google.golang.org/grpc"
)

var nodeId = flag.Int("id", 0, "input this meta server node id")
var nodeAddrs = flag.String("addrs", "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", "input meta server node addrs")
var peerAddrs = flag.String("peers", "wellwood-metaserver-0.wellwood-metaserver:8088,wellwood-metaserver-1.wellwood-metaserver:8089,wellwood-metaserver-2.wellwood-metaserver:8090", "input meta server node peers")
var dataDir = flag.String("data_path", "./data", "input meta server data path")
var debugAddrs = flag.String("debug_addrs", ":18088,:18089,:18090", "input block server debug addrs")
var enableDebug = flag.Bool("enable_debug", false, "if the server enable debug")

func main() {
	flag.Parse()

	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)

	metaSvrPeersMap := make(map[int]string)
	nodePeersArr := strings.Split(*peerAddrs, ",")
	for i, addr := range nodePeersArr {
		metaSvrPeersMap[i] = addr
	}
	nodeAddrsMap := make(map[int]string)
	nodeAddrsArr := strings.Split(*nodeAddrs, ",")
	for i, addr := range nodeAddrsArr {
		nodeAddrsMap[i] = addr
	}
	metaServer := meta_svr.MakeMetaServer(metaSvrPeersMap, *nodeId, *dataDir)
	svr := grpc.NewServer()
	pb.RegisterRaftServiceServer(svr, metaServer)
	pb.RegisterMetaServiceServer(svr, metaServer)
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)
	go func() {
		sig := <-sigs
		fmt.Printf("recived sig %d \n", sig)
		metaServer.StopAppling()
		os.Exit(-1)
	}()
	reflection.Register(svr)
	lis, err := net.Listen("tcp", nodeAddrsMap[*nodeId])
	if err != nil {
		log.MainLogger.Error().Msgf("meta server failed to listen: %v", err)
		return
	}
	if *enableDebug {
		debugSvrPeersMap := make(map[int]string)
		for i, addr := range strings.Split(*debugAddrs, ",") {
			debugSvrPeersMap[i] = addr
		}
		go func() {
			if err := http.ListenAndServe(debugSvrPeersMap[*nodeId], nil); err != nil {
				log.MainLogger.Error().Msgf("block server debug failed to: %v", err)
			}
			os.Exit(0)
		}()
	}
	log.MainLogger.Info().Msgf("meta server success listen on: %s", nodeAddrsMap[*nodeId])
	if err := svr.Serve(lis); err != nil {
		log.MainLogger.Error().Msgf("meta server failed to serve: %v", err)
		return
	}
}
