---
layout:     post
title:      "使用Fabric1.1搭建一个联盟网络"
subtitle:   "在这篇文章中将介绍如果基于k8s搭建一个联盟链步骤"
date:       2018-06-10 12:00:00
author:     "ryan"
header-img: "img/post-bg-03.jpg"
---

# 1.前言

在这篇文章中将介绍如果基于k8s搭建一个联盟链网络，这里写下来以备以后使用。



# 2.部署结构

## 2.1 测试环境的物理部署

![build-union-blockchain-01](/Users/ryan.w.li/Documents/ryan/git/ryanwli.github.io/img/2018/build-union-blockchain-01.jpg)

这个部署只是一个简单的在测试环境的一个部署方案，当然在实际的产品环境中，肯定不可能所有组织都在一个k8s内网体系中，一个org肯定就是一个自己大的内网，通过公共的kafka集群来完成区块的顺序生成的共识，而且kafka Cluster也不会都在一个服务器上，这里仅仅演示。

## 2.2 真实环境的逻辑部署图：

![build-union-blockchain-02](/Users/ryan.w.li/Documents/ryan/git/ryanwli.github.io/img/2018/build-union-blockchain-02.jpg)



# 3.步骤

## 3.1 准备证书密钥等相关文件

k8s的配置文件基本都是用了static pod的形式，因为orderer,peer,kafka,zookeeper都是有状态的应用所有都采用静态Pod，当然还可以采用k8s的StatefulSet类型来部署

#### 3.1.1 配置crypto-config.yaml文件：

```bash
mkdir config && cd config && vim crypto-config.yaml
```

```yaml
OrdererOrgs:
  - Name: org1
    Domain: org1.mysample.cn
    Specs:
      - Hostname: orderer0
        CommonName: orderer0.org1.mysample.cn
      #如果一个组织为了防止点单可以有多个orderer节点
      #- Hostname: orderer1
      #  CommonName: orderer1.org1.mysample.cn
  - Name: org2
    Domain: org2.mysample.cn
    Specs:
      - Hostname: orderer0
        CommonName: orderer0.org2.mysample.cn
  - Name: org3
    Domain: org3.mysample.cn
    Specs:
      - Hostname: orderer0
        CommonName: orderer0.org3.mysample.cn
PeerOrgs:
  - Name: org1
    Domain: org2.mysample.cn
    CA:
       Country: CN
       Province: Sichuan
       Locality: Chengdu
       OrganizationalUnit: org3-ou
    #count代表几个peer
    Template:
      Count: 1
    Users:
      Count: 1
  - Name: org2
    Domain: org2.mysample.cn
    CA:
       Country: CN
       Province: Sichuan
       Locality: Chengdu
       OrganizationalUnit: org2-ou
    Template:
      Count: 1
    Users:
      Count: 1
  - Name: org3
    Domain: org3.mysample.cn
    CA:
       Country: CN
       Province: Sichuan
       Locality: Chengdu
       OrganizationalUnit: org3-ou
    Template:
      Count: 1
    Users:
      Count: 1
```

上面的配置都是一个组织一个orderer, 一个peer，一个usesr，一个admin的配置，只是为了简单部署测试；

#### 3.1.2 使用configtxgen根据上面的配置生成每个角色的证书和密钥

```bash
cryptogen generate --config=./crypto-config.yaml --output ./crypto-config
```

执行完后每个角色都会生成一个msp和tls目录，msp用于标示身份验签，tls用于传输加密数据

#### 3.1.3 在configtx.yaml编写通道相关配置：

```yaml
#Profiles节点为组织下面Organizations/Order/Application
Profiles:
    OrgsOrdererGenesis:
        Orderer:
            <<: *OrdererDefaults
            Organizations:
                - *OrdererOrg1
                - *OrdererOrg2
                - *OrdererOrg3
        Consortiums:
            SampleConsortium:
                Organizations:
                    - *PeerOrg1
                    - *PeerOrg2
                    - *PeerOrg3
    OrgsChannel:
        Consortium: SampleConsortium
        Application:
            <<: *ApplicationDefaults
            Organizations:
                - *PeerOrg1
                - *PeerOrg2
                - *PeerOrg3
                
Organizations:
    - &OrdererOrg1
        Name: OrdererOrg1
        ID: OrdererOrg1MSP
        MSPDir: crypto-config/ordererOrganizations/org1.mysample.com/msp
    - &PeerOrg1
        Name: PeerOrg1MSP
        ID: PeerOrg1MSP
        MSPDir: crypto-config/peerOrganizations/org1.mysample.com/msp
        AnchorPeers:
            - Host: peer0.org1.mysample.com
              Port: 7051
    - &OrdererOrg2
        Name: OrdererOrg2
        ID: OrdererOrg2
        MSPDir: crypto-config/ordererOrganizations/org2.mysample.com/msp
    - &PeerOrg2
        Name: PeerOrg2MSP
        ID: PeerOrg2MSP
        MSPDir: crypto-config/peerOrganizations/org2.mysample.com/msp
        AnchorPeers:
            - Host: peer0.org2.mysample.com
              Port: 7051
    - &OrdererOrg3
        Name: OrdererOrg3
        ID: OrdererOrg3MSP
        MSPDir: crypto-config/ordererOrganizations/org3.mysample.com/msp
    - &PeerOrg3
        Name: PeerOrg3
        ID: PeerOrg3
        MSPDir: crypto-config/peerOrganizations/org3.mysample.com/msp
        AnchorPeers:
            - Host: peer0.org3.mysample.com
              Port: 7051
              
Orderer: &OrdererDefaults
    OrdererType: kafka
    Addresses:
        - orderer0.org1.mysample.com:7050
        - orderer0.org2.mysample.com:7050
        - orderer0.org3.mysample.com:7050
    BatchTimeout: 2s
    BatchSize:
        MaxMessageCount: 10
        AbsoluteMaxBytes: 99 MB
        PreferredMaxBytes: 512 KB
    Kafka:
        Brokers:
            - kafka0.mysample.com:9092
            - kafka1.mysample.com:9092
    Organizations:
Application: &ApplicationDefaults
    Organizations:
```

更多解释：https://github.com/hyperledger/fabric/blob/master/sampleconfig/configtx.yaml

#### 3.1.4 根据3.1.3的配置生成orderer节点的初始化区块

```bash
configtxgen -profile OrgsOrdererGenesis -outputBlock ./orderer.genesis.block
#该文件会在启动orderer节点的时候使用
```

#### 3.1.5 根据3.1.3的配置生成通道文件

```bash
configtxgen -profile OrgsChannel -outputCreateChannelTx ./mychannel.tx -channelID mychannel
```

3.1.6 根据3.1.6的配置生成每个组织peer锚节点配置更新文件()

```bash
configtxgen -profile OrgsChannel -outputAnchorPeersUpdate ./PeerOrg1MSPanchors.tx -channelID mychannel -asOrg PeerOrg1MSP
configtxgen -profile OrgsChannel -outputAnchorPeersUpdate ./PeerOrg2MSPanchors.tx -channelID mychannel -asOrg PeerOrg2MSP
configtxgen -profile OrgsChannel -outputAnchorPeersUpdate ./PeerOrg3MSPanchors.tx -channelID mychannel -asOrg PeerOrg3MSP
#后面创建Channel的时候会用到
```



## 3.2 准备K8S相关文件

K8S的测试环境需要自己搭建的一个DNS服务器，并把对应的orderer/peer节点的域名解析到对应Cluster IP

#### 3.2.1 准备Orderder的Pod文件：

```yaml
#这里只给出一个组织的一个orderer配置，其他依次类推
apiVersion: v1
kind: Pod
metadata:
    name: org1-orderer0
    labels:
        name: org1-orderer0
spec:
    volumes:
        - name: fabric-vol-msp
          hostPath:
            path: /mnt/config/fabric/orderer0/msp
        - name: fabric-vol-tls
          hostPath:
            path: /mnt/config/fabric/orderer0/tls
        - name: fabric-vol-block
          hostPath:
            path: /mnt/config/fabric/orderer0/orderer.genesis.block
        - name: fabric-vol-data
          hostPath:
            path: /mnt/data/fabric/orderer0
    containers:
        - name: org1-orderer0
          image: hyperledger/fabric-orderer:x86_64-1.1.0
          env:
            - name: ORDERER_GENERAL_LISTENADDRESS
              value: "0.0.0.0"
            - name: ORDERER_GENERAL_LOGLEVEL
              value: "DEBUG"
            - name: ORDERER_GENERAL_GENESISMETHOD
              value: "file"
            - name: ORDERER_GENERAL_GENESISFILE
              value: "/var/hyperledger/orderer/orderer.genesis.block"
            - name: ORDERER_GENERAL_LOCALMSPID
              value: "OrdererOrg1MSP"
            - name: ORDERER_GENERAL_LOCALMSPDIR
              value: "/var/hyperledger/orderer/msp"
            - name: ORDERER_GENERAL_TLS_PRIVATEKEY
              value: "/var/hyperledger/orderer/tls/server.key"
            - name: ORDERER_GENERAL_TLS_CERTIFICATE
              value: "/var/hyperledger/orderer/tls/server.crt"
            - name: ORDERER_GENERAL_TLS_ROOTCAS
              value: "[/var/hyperledger/orderer/tls/ca.crt]"
            - name: ORDERER_GENERAL_TLS_ENABLED
              value: "true"
          workingDir: /opt/gopath/src/github.com/hyperledger/fabric
          command: ["orderer"]
          volumeMounts:
            - mountPath: /var/hyperledger/orderer/msp
              name: fabric-vol-msp
            - mountPath: /var/hyperledger/orderer/tls
              name: fabric-vol-tls
            - mountPath: /var/hyperledger/production/orderer
              name: fabric-vol-data
            - mountPath: /var/hyperledger/orderer/orderer.genesis.block
              name: fabric-vol-block
          ports:
            - containerPort: 7050
```

#### 3.2.2 准备Peer的Pod文件：

```yaml
apiVersion: v1
kind: Pod
metadata:
    name: org1-peer0
    labels:
        name: org1-peer0
spec:
    volumes:
        - name: fabric-vol-msp
          hostPath:
            path: /mnt/config/fabric/peer0/msp
        - name: fabric-vol-tls
          hostPath:
            path: /mnt/config/fabric/peer0/tls
        - name: host-vol-run
          hostPath:
            path: /var/run
        - name: fabric-vol-data
          hostPath:
            path: /mnt/data/fabric/peer0
    containers:
        - name: org1-couchdb0
          image: hyperledger/fabric-couchdb
          env:
            - name: COUCHDB_USER
              value: "org1"
            - name: COUCHDB_PASSWORD
              value: "org1123"
          ports:
            - containerPort: 5984
        - name: org1-peer0
          image: hyperledger/fabric-peer:x86_64-1.1.0
          env:
            - name: CORE_PEER_ADDRESSAUTODETECT
              value: "true"
            - name: CORE_PEER_TLS_SERVERHOSTOVERRIDE
              value: "%(hostname).peer"
            - name: CORE_PEER_ID
              value: "peer0.org1.mysample.com"
            - name: CORE_PEER_ADDRESS
              value: "peer0.org1.mysample.com:7051"
            - name: CORE_PEER_GOSSIP_BOOTSTRAP
              value: "peer0.org1.mysample.com:7051"
            - name: CORE_PEER_GOSSIP_EXTERNALENDPOINT
              value: "peer0.org1.mysample.com:7051"
            - name: CORE_PEER_LOCALMSPID
              value: "PeerOrg1MSP"
            - name: CORE_VM_HOSTCONFIG_DNS
              value: "10.68.0.2"
            - name: CORE_VM_ENDPOINT
              value: "unix:///host/var/run/docker.sock"
            - name: CORE_LOGGING_LEVEL
              value: "DEBUG"
            - name: CORE_PEER_TLS_ENABLED
              value: "true"
            - name: CORE_PEER_GOSSIP_USELEADERELECTION
              value: "true"
            - name: CORE_PEER_GOSSIP_ORGLEADER
              value: "false"
            - name: CORE_PEER_PROFILE_ENABLED
              value: "true"
            - name: CORE_PEER_TLS_CERT_FILE
              value: "/etc/hyperledger/fabric/tls/server.crt"
            - name: CORE_PEER_TLS_KEY_FILE
              value: "/etc/hyperledger/fabric/tls/server.key"
            - name: CORE_PEER_TLS_ROOTCERT_FILE
              value: "/etc/hyperledger/fabric/tls/ca.crt"
            - name: CORE_LEDGER_STATE_STATEDATABASE
              value: "CouchDB"
            - name: CORE_LEDGER_STATE_COUCHDBCONFIG_COUCHDBADDRESS
              value: "localhost:5984"
            - name: CORE_LEDGER_STATE_COUCHDBCONFIG_USERNAME
              value: "org1"
            - name: CORE_LEDGER_STATE_COUCHDBCONFIG_PASSWORD
              value: "org1123"
          workingDir: /opt/gopath/src/github.com/hyperledger/fabric/peer
          volumeMounts:
            - mountPath: /etc/hyperledger/fabric/msp
              name: fabric-vol-msp
            - mountPath: /etc/hyperledger/fabric/tls
              name: fabric-vol-tls
            - mountPath: /var/hyperledger/production
              name: fabric-vol-data
            - mountPath: /host/var/run
              name: host-vol-run
          ports:
            - containerPort: 7051
```

#### 3.2.3 准备Kafka的Pod文件：

```yaml
#这里zookeeper和kafka的配置为了方便放到了一个Pod里面，实际生产环境，kafka和zookeeper是分开集群部署的
apiVersion: v1
kind: Pod
metadata:
    name: kafka0
    labels:
        name: kafka0
spec:
    volumes:
        - name: kafka-vol-data
          hostPath:
            path: /mnt/data/kafka0/logs
        - name: zk-vol-snapshot
          hostPath:
            path: /mnt/data/zk0/snapshot
        - name: zk-vol-tran-log
          hostPath:
            path: /mnt/data/zk0/tran-log
    containers:
        - name: zookeeper0
          image: hyperledger/fabric-zookeeper:latest
          env:
            - name: ZOO_MY_ID
              value: "1"
            - name: ZOO_SERVERS
              value: "server.1=localhost:2888:3888"
          volumeMounts:
            - mountPath: /data
              name: zk-vol-snapshot
            - mountPath: /datalog
              name: zk-vol-tran-log
          ports:
            - containerPort: 2181
            - containerPort: 2888
            - containerPort: 3888
        - name: kafka0
          image: hyperledger/fabric-kafka:latest
          env:
            - name: KAFKA_ADVERTISED_HOST_NAME
              value: "kafka0.mysample.cn"
            - name: KAFKA_HEAP_OPTS
              value: "-Xmx256M"
            - name: KAFKA_BROKER_ID
              value: "1"
            - name: KAFKA_MIN_INSYNC_REPLICAS
              value: "1"
            - name: KAFKA_DEFAULT_REPLICATION_FACTOR
              value: "1"
            - name: KAFKA_ZOOKEEPER_CONNECT
              value: "localhost:2181"
            - name: KAFKA_MESSAGE_MAX_BYTES
              value: "103809024"
            - name: KAFKA_REPLICA_FETCH_MAX_BYTES
              value: "103809024"
            - name: KAFKA_UNCLEAN_LEADER_ELECTION_ENABLE
              value: "false"
            - name: KAFKA_LOG_RETENTION_MS
              value: "-1"
          volumeMounts:
            - mountPath: /tmp/kafka-logs
              name: kafka-vol-data
          ports:
            - containerPort: 9092
```

#### 3.2.4 准备cli的Pod文件：

```yaml
apiVersion: v1
kind: Pod
metadata:
    name: cli
    labels:
        name: cli
spec:
    volumes:
        - name: fabric-vol-crypto
          hostPath:
            path: /mnt/config/fabric/cli/crypto
        - name: fabric-vol-chaincode
          hostPath:
            path: /mnt/config/fabric/cli/chaincode
        - name: host-vol-run
          hostPath:
            path: /var/run
        - name: fabric-vol-channel
          hostPath:
            path: /mnt/config/fabric/cli/channel
    containers:
        - name: cli
          image: hyperledger/fabric-tools:latest
          env:
            - name: GOPATH
              value: "/opt/gopath"
            - name: CORE_VM_ENDPOINT
              value: "unix:///host/var/run/docker.sock"
            - name: CORE_LOGGING_LEVEL
              value: "DEBUG"
            - name: CORE_PEER_ID
              value: "cli"
            - name: CORE_PEER_ADDRESS
              value: "peer0.org1.mysample.coom:7051"
            - name: CORE_PEER_TLS_ENABLED
              value: "true"
            - name: CORE_PEER_LOCALMSPID
              value: "PeerOrg1MSP"
            - name: CHANNEL_NAME
              value: "mychannel"
            - name: CORE_PEER_MSPCONFIGPATH
              value: "/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/users/Admin@org1.mysample.com/msp"
            - name: CORE_PEER_TLS_CERT_FILE
              value: "/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/peers/peer0.org1.mysample.com/tls/server.crt"
            - name: CORE_PEER_TLS_KEY_FILE
              value: "/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/peers/peer0.org1.mysample.com/tls/server.key"
            - name: CORE_PEER_TLS_ROOTCERT_FILE
              value: "/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/peers/peer0.org1.mysample.com/tls/ca.crt"
          workingDir: /opt/gopath/src/github.com/hyperledger/fabric/peer
          volumeMounts:
            - mountPath: /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/
              name: fabric-vol-crypto
            - mountPath: /opt/gopath/src/github.com/chaincode
              name: fabric-vol-chaincode
            - mountPath: /opt/gopath/src/github.com/hyperledger/fabric/peer/channel/
              name: fabric-vol-channel
            - mountPath: /host/var/run
              name: host-vol-run
          command: [ "/bin/bash", "-c" ]
          args: [ "tail -f /dev/null" ]
#这里需要将之前生成的mychannel.tx,PeerOrg1MSPanchors.tx,PeerOrg2MSPanchors.tx,PeerOrg3MSPanchors.tx拷贝到/mnt/config/fabric/cli/channel
#还有官方的chaincode_example02拷贝到/mnt/config/fabric/cli/channel
#还有之前的cryptogen产生的crypto-config拷贝到/mnt/config/fabric/cli/crypto
```

## 3.3 启动并初始化网络

#### 3.3.1 创建通道：

```bash
kubeclt exec -it cli-pod-name bash
peer channel create -o orderer0.org1.mysample.com:7050 -c ${CHANNEL_NAME} -f /opt/gopath/src/github.com/hyperledger/fabric/peer/channel-artifacts/mychannel.tx --tls --cafile /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/org1.mysample.com/orderers/orderer0.org1.mysample.com/msp/tlscacerts/tlsca.org1.mysample.com-cert.pem
```

#### 3.3.2 将每个组织的Peer节点加入到通道内：

```bash
#org1-peer0
peer channel join -b ${CHANNEL_NAME}.block

#org2-peer0
#修改msp/rootca路径，mspid和peer地址
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org2.mysample.com/users/Admin\@org2.mysample.com/msp/
CORE_PEER_TLS_ROOTCERT_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org2.mysample.com/peers/peer0.org2.mysample.com/tls/ca.crt
CORE_PEER_LOCALMSPID="Org2MSP"
CORE_PEER_ADDRESS=peer0.org2.mysample.com:7051
peer channel join -b ${CHANNEL_NAME}.block

#org3-peer0
#修改msp/rootca路径，mspid和peer地址
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org3.mysample.com/users/Admin\@org3.mysample.com/msp/
CORE_PEER_TLS_ROOTCERT_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org3.mysample.com/peers/peer0.org3.mysample.com/tls/ca.crt
CORE_PEER_LOCALMSPID="Org2MSP"
CORE_PEER_ADDRESS=peer0.org3.mysample.com:7051
peer channel join -b ${CHANNEL_NAME}.block
```

#### 3.3.3 将之前生成的Peer锚点配置更新到刚才创建的通道内：

```bash
#更新org1的peer锚点配置
CORE_PEER_LOCALMSPID="Org1MSP"
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/users/Admin\@org1.mysample.com/msp/
peer channel update -o orderer0.org1.mysample.com:7050 -c ${CHANNEL_NAME} -f /opt/gopath/src/github.com/hyperledger/fabric/peer/channel-artifacts/PeerOrg1MSPanchors.tx --tls --cafile /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/org1.mysample.com/orderers/orderer0.org1.mysample.com/msp/tlscacerts/tlsca.org1.mysample.com-cert.pem

#更新org2的peer锚点配置
CORE_PEER_LOCALMSPID="Org2MSP"
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org2.mysample.com/users/Admin\@org2.mysample.com/msp/
peer channel update -o orderer0.org2.mysample.com:7050 -c ${CHANNEL_NAME} -f /opt/gopath/src/github.com/hyperledger/fabric/peer/channel-artifacts/PeerOrg2MSPanchors.tx --tls --cafile /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/org2.mysample.com/orderers/orderer0.org2.mysample.com/msp/tlscacerts/tlsca.org2.mysample.com-cert.pem

#更新org3的peer锚点配置
CORE_PEER_LOCALMSPID="Org3MSP"
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org3.mysample.com/users/Admin\@org3.mysample.com/msp/
peer channel update -o orderer0.org3.mysample.com:7050 -c ${CHANNEL_NAME} -f /opt/gopath/src/github.com/hyperledger/fabric/peer/channel-artifacts/PeerOrg3MSPanchors.tx --tls --cafile /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/org3.mysample.com/orderers/orderer0.org3.mysample.com/msp/tlscacerts/tlsca.org3.mysample.com-cert.pem
```

到这里，三个org的联盟链就启动和初始化完了，下面我们会安装链码进行测试。



## 3.4 安装链码，并测试

#### 3.4.1 在每个组织的每个peer安装链码：

```bash
#org1的peer0安装链码
CORE_PEER_LOCALMSPID="Org1MSP"
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/users/Admin\@org1.mysample.com/msp/
CORE_PEER_ADDRESS=peer0.org1.mysample.com:7051
peer chaincode install -n sample_cc -v 1.0 -p github.com/chaincode/chaincode_example02/

#org2的peer0安装链码
CORE_PEER_LOCALMSPID="Org2MSP"
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org2.mysample.com/users/Admin\@org2.mysample.com/msp/
CORE_PEER_ADDRESS=peer0.org2.mysample.com:7051
peer chaincode install -n sample_cc -v 1.0 -p github.com/chaincode/chaincode_example02/

#org3的peer0安装链码
CORE_PEER_LOCALMSPID="Org3MSP"
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org3.mysample.com/users/Admin\@org3.mysample.com/msp/
CORE_PEER_ADDRESS=peer0.org3.mysample.com:7051
peer chaincode install -n sample_cc -v 1.0 -p github.com/chaincode/chaincode_example02/
```

#### 3.4.2 初始化链码：

```bash
#链码的初始化只需要在一个节点进行就可以了，peer的背书策略写的所有peer成员
CORE_PEER_LOCALMSPID="Org1MSP"
CORE_PEER_MSPCONFIGPATH=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/users/Admin\@org1.mysample.com/msp/
CORE_PEER_TLS_ROOTCERT_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/peerOrganizations/org1.mysample.com/peers/peer0.org1.mysample.com/tls/ca.crt
CORE_PEER_ADDRESS=peer0.org1.mysample.com:7051
peer chaincode instantiate -o orderer0.org1.mysample.com:7050 -C ${CHANNEL_NAME} -n sample_cc -v 1.0 -c '{"Args":["init", "a", "100", "b", "200"]}' -P "AND ('Org1MSP.member','Org2MSP.member','Org3MSP.member')" --tls --cafile /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/org1.mysample.com/orderers/orderer0.org1.mysample.com/msp/tlscacerts/tlsca.orderer0.org1.mysample.com-cert.pem
```

#### 3.4.3 Invoke测试：

```bash
peer chaincode invoke -o orderer0.org1.mysample.com:7050 -C ${CHANNEL_NAME} -n sample_cc -c '{"Args":["invoke", "a", "b", "10"]}' --tls --cafile /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/org1.mysample.com/orderers/orderer0.org1.mysample.com/msp/tlscacerts/tlsca.org1.mysample.com-cert.pem
```

#### 3.4.5 query测试：

```bash
peer chaincode query -n sample_cc -C ${CHANNEL_NAME} -c '{"Args":["query","a"]}'
---
Query Result: 90
#一切顺利的话，这里query测试结果应该是90
```

#### 3.4 升级链码:

升级链码和安装比较类似，和安装链码的前面步骤一样，只要初始化步骤改成upgrade，如下：

```bash
peer chaincode upgrade -o orderer0.org1.mysample.com:7050 -C ${CHANNEL_NAME} -n sample_cc -v 1.1 -c '{"Args":["init"]}' -P "AND('PeerOrg1MSP.member','PeerOrg2MSP.member','PeerOrg3MSP.member')" --tls true --cafile /opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/org1.mysample.com/orderers/orderer0.org1.mysample.com/msp/tlscacerts/tlsca.org1.mysample.com-cert.pem

```



# 3.尾语

看着这么多步骤是不是头大，但是熟悉了这个手动的过程是怎么回事，后面就可以自己写脚本一键之行了