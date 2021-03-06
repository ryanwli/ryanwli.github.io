---
layout:     post
title:      "Fabric1.1动态新加组织"
subtitle:   "此篇文章介绍了如何在已经存在的hyperledger fabric联盟链中加入新组织，目前fabric1.1的版本还是比较繁琐的，这里写下来以备以后使用"
date:       2018-06-20 12:00:00
author:     "ryan"
header-img: "img/post-bg-02.jpg"
---

# 1.前言

此篇文章介绍了如何在已经存在的hyperledger fabric联盟链中加入新组织，目前fabric1.1的版本还是比较繁琐的，这里写下来以备以后使用。



# 2.步骤

2.1 获取当前联盟链的Channel的PB配置文件：

```bash
#登陆fabric-cli容器
docker exec -it cli bash
#设置orderer节点的CA的文件路径(路径根据自己的部署结构来)
export ORDER_CA_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/mysample.com/orderers/orderer.mysample.com/msp/tlscacerts/tlsca.mysample.com-cert.pem
#获取mychannel的pb配置文件
peer channel fetch config config_block.pb -o orderer.mysample.com:7050 -c mychannel --tls --cafile $ORDER_CA_FILE
```

2.2 编辑crypto-config.yaml, 添加org2的peer节点：

```yaml
PeerOrgs:
  - Name: Org2
    Domain: org2.mysample.com
    CA:
       Country: CN
       Province: Sichuan
       Locality: Chengdu
       OrganizationalUnit: tech
    Template:
      Count: 1
    Users:
      Count: 1
```

2.3 使用cryptogen生成新组织的证书和私钥：

```bash
cryptogen generate --config=./crypto-config.yaml --output ./org2-crypto-config
```

2.4 编辑configtx.yaml，加入新的组织配置：

```yaml
Organizations:
    - &Org2
        Name: Org2MSP
        ID: Org2MSP
        #注意这里的路径是当前文件下面的crypto-config里面peer的org2的msp文件
        MSPDir: crypto-config/peerOrganizations/org2.mysample.com/msp
        AnchorPeers:
            - Host: peer0.org2.mysample.com
              Port: 7051
```

2.5 根据上面新配置的configtx.yaml生成org2的json配置文件(后面会用到)：

```bash
configtxgen -printOrg Org2MSP > ./org2.json
```

2.6 启动configtxlator：

```bash
configtxlator start
```

2.7 将2.1步生成config_block.pb文件decode成json方便后面阅读和编辑：

```bash
curl -X POST --data-binary @config_block.pb http://127.0.0.1:7059/protolator/decode/common.Block > config_block.json
```

2.8 获取channel的json的config区域的json配置：

```bash
#这里需要安装jq，如果没有请自己行安装
jq .data.data[0].payload.data.config config_block.json > config.json
```

2.9 将2.5生成的org2的json放到config.json适合的位置，并保存成updated_config.json，如下：

![add-new-org-01](https://ryanwli.github.io/img/2018/add-new-org-01.png)

2.10 将config.json和编辑后的updated_config.json转成pb格式的文件，并计算出差异生成待更新的pb文件：

```bash
#config.json转config.pb
curl -X POST --data-binary @config.json http://127.0.0.1:7059/protolator/encode/common.Config > config.pb

#update_config.json转update_config.pb
curl -X POST --data-binary @updated_config.json http://127.0.0.1:7059/protolator/encode/common.Config > updated_config.pb

#计算差异，生成待升级的pb文件，注意填写需要新组织加入的通道名称
curl -X POST -F original=@config.pb -F updated=@updated_config.pb http://127.0.0.1:7059/configtxlator/compute/update-from-configs -F channel=mychannel > config_update.pb
```

2.11 将config_update.pb转json，并添加必要外围json格式，生成可以被peer channel update执行的pb文件:

```bash
curl -X POST --data-binary @config_update.pb http://127.0.0.1:7059/protolator/decode/common.ConfigUpdate > config_update.json

#添加外围格式生成格式化后的json文件，注意填写正确channel名
echo '{"payload":{"header":{"channel_header":{"channel_id":"mychannel", "type":2}},"data":{"config_update":'$(cat config_update.json)'}}}' > config_update_as_envelope.json

curl -X POST --data-binary @config_update_as_envelope.json http://127.0.0.1:7059/protolator/encode/common.Envelope > config_update_as_envelope.pb
```

2.12 将带有新org2的通道信息更新到联盟链中

```bash
#登陆fabric-cli容器
docker exec -it cli bash
#设置orderer节点的CA的文件路径(路径根据自己的部署结构来)
export ORDER_CA_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/mysample.com/orderers/orderer.mysample.com/msp/tlscacerts/tlsca.mysample.com-cert.pem
#设置待更新的channel配置文件
export CHANNEL_UPDATED_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/scripts/config_update_as_envelope.pb
#更新配置现有的通道
peer channel update -f $CHANNEL_UPDATED_FILE -o orderer.mysample.com:7050 -c mychannel --tls --cafile $ORDER_CA_FILE
```

> 注意：这里的例子是联盟链中只有1个组织的情况下，直接使用peer channel update就可以了(该命令会自动签名并更新)，如果是2个以上的组织，在更新之前，还需要其他组织对这次的更新内容进行签名，执行命令如下：

```bash
#这里如果有需要的话，要切换到当前签名组织的环境变量
#CORE_PEER_TLS_ROOTCERT_FILE
#CORE_PEER_TLS_KEY_FILE
#CORE_PEER_LOCALMSPID
#CORE_PEER_TLS_CERT_FILE
#CORE_PEER_TLS_ENABLED
#CORE_PEER_MSPCONFIGPATH
#设置orderer节点的CA的文件路径(路径根据自己的部署结构来)
export ORDER_CA_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/crypto/ordererOrganizations/mysample.com/orderers/orderer.mysample.com/msp/tlscacerts/tlsca.mysample.com-cert.pem
#设置待更新的channel配置文件
export CHANNEL_UPDATED_FILE=/opt/gopath/src/github.com/hyperledger/fabric/peer/scripts/config_update_as_envelope.pb
#签名配置现有的通道
peer channel signconfigtx -f $CHANNEL_UPDATED_FILE -o orderer.example.com:7050 --tls --cafile $ORDER_CA_FILE
```

> 另外，上面我们只修改了Channel中Application中的Peer的配置，所以只需要在这个Application中已存在的Peer节点进行签名就可以了，下面是引用官方文档一部分，主要说的是如果需要修改channel里面的其他一些配置，这些配置需要哪些节点的Admin来对这个config_update_as_envelope.pb进行签名。
>
> Once you’ve successfully generated the protobuf file, it’s time to get it signed. To do this, you need to know the relevant policy for whatever it is you’re trying to change.
>
> By default, editing the configuration of:
>
> - **A particular org** (for example, changing anchor peers) requires only the admin signature of that org.
> - **The application** (like who the member orgs are) requires a majority of the application organizations’ admins to sign.
> - **The orderer** requires a majority of the ordering organizations’ admins (of which there are by default only 1).
> - **The top level channel group** requires both the agreement of a majority of application organization admins and orderer organization admins.

2.13 启动部署新组织org2的peer节点，把对应的msp，以及tls拷贝到部署节点进行部署，然后安装链码到对应的peer节点(使用新版本)，这里部署细节就不多述(不清楚看我之前搭建联盟链的文章，链接待加)；

> 注意：在加入新的Peer节点的时候需要这个联盟链的创世块文件，这个文件需要使用下面的命令进行获取：
>
> peer channel fetch 0 first.block -o orderer.mysample.com:7050 -c $CHANNEL_NAME --tls --cafile $ORDERER_CERT_CA

2.14 由于之前背书策略没有org2的peer节点，所以这里还需要安装所有组织的peer上的链码为org2的peer上的链码版本，并且进行升级，升级链码也不累述了，看之前的文章(链接待加)；



# 3.尾语

看着这么多步骤是不是头大？但是熟悉了这个手动的过程是怎么回事，后面就可以自己写脚本一键执行了。