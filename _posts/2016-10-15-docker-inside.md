---
layout:     post
title:      "Docker Inside Part 1"
subtitle:   "docker在近2年都非常炙手可热，为了推进公司技术前进以及储备，有了这篇docker的share文档，供愿意尝试docker的开发童鞋入门，后面我也会浅析一下docker如何工作的；抛砖引玉，让童鞋们快进入状态。"
date:       2016-12-28 13:27:00
author:     "ryan"
header-img: "img/post-bg-06.jpg"
---

# 1.前言

docker在近2年都非常炙手可热，为了推进公司技术前进以及储备，有了这篇docker的share文档，供愿意尝试docker的开发童鞋入门，后面我也会浅析一下docker如何工作的；抛砖引玉，让童鞋们快进入状态。



# 2. 集装箱的故事

### 2.1 场景

一些进口产品，需要经历从出口国某地到进口国某地，需要经过产地=>汽车(出口国)=>货车(出口国)=>汽车(出口国)=>货轮=>汽车(进口国)=>火车(进口国)=>汽车(进口国)=>目标市场，集装箱就是为了解决货物传输问题二产生的。

### 2.2 传统老模式

每次从货车到火车到货轮，都是需要把**每件**货物卸下来然后转而装运到其他的运载工具，上面描述的场景要装卸14次才能完成投放目标市场。

### 2.3 集装箱模式

再来看看当今的集装箱模式，每次转运不是装卸每件货物，而是在源头装载封箱一次，在目的地拆箱卸载一次，中间使用机械流程化的直接转运集装箱就可以了，所有的货物都封装在每个规定规格大小的集装箱里面，这些集装箱可以被火车，货车，轮转，甚至大型运输机都可以识别规格尺寸的容器；整个流程只需要2次装卸就可以完成。

### 2.4 解决问题

如果在途时间固定，如果装卸货物的时间能够使用集装箱优化，整个运输时间会得到大幅度的减少，加快运输的效率；有了这个做铺垫，再来看看我们是什么就更容易理解了；



# 2.docker

![what_is_docker](https://ryanwli.github.io/img/2016/20161228_what_is_docker.png)

### 2.1 docker命令&服务程序

docker命令和服务程序，算是上面集装箱故事的装货工人，工具，以及车间，来完成将货物在源头封装成集装箱；

### 2.2 镜像

docker镜像就算是封装好的集装箱了，不同的是真正的集装箱封装的是货物，而镜像封装的是软件；

### 2.3 容器

docker容器，为了避免直接使用镜像而破坏了原有镜像，而使用了镜像的一个分身来运行，这个能够运行的分身镜像就是docker的容器；

### 2.4 解决问题

我们可以把对每台服务器的环境部署叫做运输上的封箱过程，以前的传统模式是，我们新上了一个系统，如果有很多服务器实例需要运行相同的环境和代码，那么我们就要有多少台，重复的部署多少台机器；这个就想集装箱故事中不同运输工具上面都要做一次装卸的操作一样；所以docker使用镜像加上分身容器来达到不同的实例上实现了快速部署，为运维同学提升了很多效率；

除了PRD使用该技术外，我们本地开发也可以使用，比如，我只想临时使用centos6的一个版本测试一下某几个程序，我只需要从一个centos6的镜像运行一个容器，安装测试完后删除容器就可以了；然后我原本的操作系统没有更多清除动作，方便高效；

拿我自己的话来说，自从有了docker，我再也不担心，领导说我们再需要几台oracle server了，因为oracle server在Linux无界面安装真的很恼人，很多依赖需要安装；现在，我只需要一句docker run命令就可以去做别的事情，直等到docker container启动起来，我就可以使用oracle server了；



# 3.运行容器

前面两节都是再告诉你什么是docker，以及docker能解决什么问题，下面的内容，我们将来一些实际的docker知识，来告诉你怎么快速使用它；

### 3.1 环境配置

- 64位CPU；
- Linux 3.8以上内核；
- 存储驱动，一般为Device Mapper，实现多个虚拟逻辑磁盘与物理磁盘的映射[8.1]；
- 内核必须支持并开启cgroup，该功能实现了将linux系统中进程进行按容器分组[8.2]；
- 内核必须支持并开启namespace功能，该用能用来抽象隔离系统资源的，如文件系统，网络访问等等[8.3]；

这些前置环境在官方的centos安装好后都是配置好的，只需要检查一下；

### 3.2 安装Docker

docker的安装非常简单，我们使用的是centos7来做后面的演示，执行如下命令就可以完成安装：

```shell
$ wget https://get.docker.com/builds/Linux/x86_64/docker-latest.tgz
$ tar -xvzf docker-latest.tgz
docker/
docker/docker
docker/docker-containerd
docker/docker-containerd-ctr
docker/docker-containerd-shim
docker/docker-proxy
docker/docker-runc
docker/dockerd
$ mv docker/* /usr/bin/
$ dockerd & 
```

上面的最后一步是启动docker的守护进程；另外，还可以在https://github.com/docker/docker/tree/master/contrib/init下载启动脚本，来更加方便的启动关闭，以及配置开机启动，如下：

```bash
$ cd /etc/init.d/
$ wget https://raw.githubusercontent.com/docker/docker/master/contrib/init/sysvinit-redhat/docker
$ cd /etc/sysconfig/
$ wget https://raw.githubusercontent.com/docker/docker/master/contrib/init/sysvinit-redhat/docker.sysconfig
$ mv docker.sysconfig docker
$ chkconfig --level 2345 docker on
```

到这里安装基本完成，当然你还可以使用yum install，但是使用binary包自己安装，个人觉得心里要踏实些，知道下载的程序部署到了哪个位置。

另外，在windows或者mac os里面安装docker的话，使用docker toolbox，它原理的host了一个linux内核的虚机然后在里面安装守护进程和运行服务。所以可以这样说docker是基于linux内核技术，只要是linux内核，不管你是什么发行版本都可以玩；

### 3.3 Docker的C/S架构

![docker_cs_art](https://ryanwli.github.io/img/2016/20161228_docker_cs_art.png)

上一小节说道的dockerd就是图中守护进程，它复制控制守护进程宿主机中的Docker容器，并提供docker客户端的远程连接；老的版本中docker客户端和守护进程的连接没有认证机制的，所以一般都没有开放远程，或者使用iptables进行访问控制；

直接启动dockerd模式使用的是Unix Domain Socket[8.4]进行宿主机器内的跨进程通信;

当然也可以启用跨网络的docker客户端到服务端守护dockerd的通信，那就需要我们3.2中配置的/etc/sysconfig/docker中的other_args='-H tcp://0.0.0.0:4200'中的配置；客户端连接的时候需要带上host参数，如下：

```shell
$ docker -H tcp://ip:4200
```

如果想每次都省去-H的参数输入，那么就在.bash_profile里面export DOCKER_HOST="tcp://ip:4200";

### 3.4 创建一个Docker

使用下面命令就可以启动一个docker容器了

```shell
$ docker run -i -t centos6 /bin/bash
```

-i/-t告诉容器以stdin和伪tty终端运行，其实就是开启一个交互式的shell运行；

由于模式是从docker官方的registry下载远程镜像无比缓慢，所以我们可以在/etc/sysconfig/docker的参数行里面配置一个阿里云的registry镜像地址；

```
$ other_args='--registry-mirror=http://xxxx.aliyun.com'
```

当run起来一个交互式的容器以后，如果exit，那么这个容器也将推出停止运行；如果想再次启动，以及进入执行下面命令即可：

```shell
#打印出刚才创建的容器列表
$ docker ps -a 
#使用docker start启动容器
$ docker start containerId
#使用docker attach进入容器
$ docker attach containerId
```

如果attach后想退出，但是并不想结束容器的运行，那么请先按CRTL-p再按CRTL-q；

### 3.5 运行守护式容器

上面我们讲到了怎么运行交互式的，现在我们来说说怎么运行守护式的，这种方式也叫后台运行，这种方式必须在启动的时候执行一个前台运行的程序，用以保证容器的处于运行状态，执行命令如下：

```shell
$ docker run -d centos6 /bin/bash -c 'tail -f /dev/null';
#也可以执行一个java程序
$ docker run -d centos6 /bin/bash -c 'java -jar web.jar';
```

-d的使用表示该容器会以后台方式运行，并立即返回一个containerId；-c表示在后台执行"tail -f /dev/null"语句，tail -f是实时获取追加到文件末尾的文字，而/dev/null，是一个“黑洞”，空设备，意思永远也不会有新的输出，而且也会一直让tail执行等待刷新，这样容器就不会退出；当然也可以使用一个要运行的程序一直执行来让程序不会退出。

如果想停止运行容器，可以使用下面命令停止：

```shell
$ docker stop containerId
```

如果想要进入这种守护容器，以前的docker attach就不能用了，这里介绍两种办法：

```shell
$ docker exec -it containerId /bin/bash
#其中的-it是上面介绍-i和-t的组合；
#第二种就是开启ssh了(这里就略过不讲了)，这个官方推荐使用exec来进入，ssh开启后会降低容器安全；
```

# 4.Registry

### 4.1 作用

registry类似开发使用的代码仓库，也类似于maven仓库，不过它存储的是docker的镜像文件；官方有一个registery服务器，类似github.com，官方的registry是http://hub.docker.com/，我们也可以使用官方提供的registry的镜像建立自己的私有镜像仓库，4.2小节将告诉你怎么建立私有镜像仓库服务。

### 4.2 运行

执行下面的命令就可以run一个registry私库起来，是不是部署起来超级方便？这个就是docker强大之处：

```shell
$ docker run -d -p 5000:5000 --restart=always --name registry registry:2
```

-p: 参数表示我们需要把宿主机的5000端口映射到容器的5000端口；

--restart: 表示宿主机器重启后，该容器起会自动重启起来；

--name: 表示容器的名字，默认创建容器的时候会自动生成一个随机名字；

镜像名字后面多了一个:2，表示该尽享的版本号，类似代码版本库中的tag；

在第5节中，我们会结合镜像的创建来说明如何使用私有镜像库registry；



# 5.制作镜像

### 5.1 commit

commit也类似代码版本管理器中的commit，当我们对容器内做了任何修改，比如安装了ssh，或者安装java后，我们下次想再其他机器或者创建新的容器，想用同样的安装了这些程序的容器，就可以将他们封装成镜像，就像把货物装载到集装箱一样，对该容器做完任何修改后，只需要简单执行下面语句，docker就可以帮你完成镜像的制作：

```shell
$ docker commit containerId 192.168.2.230:5000/mycentos:1
```

commit后面带上需要制作成镜像的Id，以及需要上传服务器IP和Port，再加上镜像名称和版本号就可以了；

### 5.2 dockerfile

dockerfile是构建镜像的一个批量处理文档，我们还是给一个实例代码来演示如何使用这个dockerfile的：

```shell
$ mkdir mycentos
$ cd mycentos
$ vi dockerfile
#写入下面命令
FROM centos:6
RUN yum -y install openssh-server
EXPOSE 22
ENTRYPOINT service sshd start && tail -f /dev/null
$ docker build -t "192.168.2.230:5000/mycentos:1" .
```

这个dockerfile文档说明一下：

FROM：表示我需要用那个镜像作为基础镜像进行构建；

RUN: 表示在构建中需要在镜像里面执行的命令；

EXPOSE：表示容器会使用哪些端口，这个仅仅只是描述，起到一个告知作用，不会真打开，需要在使用的时候使用-p参数来打开；

ENTRYPOINT：在从该镜像启动的容器，会自动先运行后面的语句，这样我们就不需要再docker run -d的时候在后面写/bin/bash之类的后续命令了；

docker build -t命令：执行dockerfile的批处理并commit成镜像；

dockerfile批处理文件中还有许多其他的命令，它可以让你写一个批处理脚本就可以完成装箱和封箱的操作，在devOps中dockerfile极其的有用；更多的dockerfile命令，请参见后面的文档链接[8.7]；

### 5.3 share

使用下面命令就可以分享docker镜像了

```shell
$ docker push 192.168.2.230:5000/mycentos:1
```



# 6.docker网络

### 6.1 default docker0

在docker run没有加--net, 默认情况下所创建的docker容器都会在以docker0网桥为网关的网络中，docker默认会使用172.17.0.0/16这个网段，如果该网段存在会在第二位加1(如：172.18.0.0/16)；我们ifconfig打印一下，我只列出相关的信息：

```shell
docker0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 172.17.42.1  netmask 255.255.0.0  broadcast 0.0.0.0
        inet6 fe80::42:ddff:fe0d:e467  prefixlen 64  scopeid 0x20<link>
        ether 02:42:dd:0d:e4:67  txqueuelen 0  (Ethernet)
        RX packets 159036  bytes 60395238 (57.5 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 233746  bytes 270206256 (257.6 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
veth19b130e: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 fe80::8894:feff:fe95:e1b9  prefixlen 64  scopeid 0x20<link>
        ether 8a:94:fe:95:e1:b9  txqueuelen 0  (Ethernet)
        RX packets 38563  bytes 54033478 (51.5 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 42013  bytes 3307140 (3.1 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
veth5464b87: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 fe80::2ccc:8ff:fefd:4270  prefixlen 64  scopeid 0x20<link>
        ether 2e:cc:08:fd:42:70  txqueuelen 0  (Ethernet)
        RX packets 737  bytes 357657 (349.2 KiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 836  bytes 99292 (96.9 KiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

上面的信息，除了docker0这个网桥以外，还有veth*的两个虚拟接口，它和容器是成对存在的，可以把它理解成宿主机连接容器的虚拟网线，如下图：

![docker_network](https://ryanwli.github.io/img/2016/20161228_docker_network.png)

这样，不光实现了容器和宿主机的通信，而且还实现了容器与容器之间通信；

### 6.2 custom network

除了使用基于docker0的默认网络，还可以自己创建一个网络，让需要网络隔离的容器在指定自己创建的网络中运行，这种自定义网络原理和基于docker0的默认网络类似；使用下面命令可以创建一个新网络：

```shell
$ docker network create mynet
#查看创建的自定义网络
$ docker network inspect mynet
```

基于自定义网络运行一个新容器：

```shell
$ docker run -i -t --net=mynet --name=app centos:6 /bin/bash
```

将已有容器加入该自定义网络：

```shell
$ docker network connect mynet containerId/containerName
#将某个容器断开自定义网络：
$ docker network disconnect mynet containerId/containerName
```

### 6.2 docker容器如何与公网通信

容器是如何与宿主机外面的网络进行通信的呢？这个就是docker守护程序操作iptable搞的事情了；回忆一下我们之间如果要让，比如：我们之前创建的私有registry，使用了-p 5000:5000，这样宿主所连接的公网就可以使用5000端口访问私有库的服务了；

我们先来看看外网是如何访问容器的，先看下面的信息：

```shell
$ iptables -t nat -L
Chain PREROUTING (policy ACCEPT)
target     prot opt source               destination
DOCKER     all  --  anywhere             anywhere             ADDRTYPE match dst-type LOCAL

Chain DOCKER (2 references)
target     prot opt source               destination         
DNAT       tcp  --  anywhere             anywhere             tcp dpt:5000 to:172.18.0.3:5000


$ iptables -t filter -L
Chain FORWARD (policy ACCEPT)
target     prot opt source               destination
DOCKER     all  --  anywhere             anywhere 

Chain DOCKER (2 references)
target     prot opt source               destination         
ACCEPT     tcp  --  anywhere             172.18.0.3           tcp dpt:5000
```

上面我只给出了需要的信息，从上面的信息可以看出，PREROUTING的挂接点，进行DNAT的操作，意思就是说在路由之前，DNAT函数将本来请求是宿主机的10.10.101.105:5000这个目标地址转成了172.18.0.3:5000这个地址；然后这个地址再经过iptable的forward转发到了容器内部虚拟网卡上面了；这样就实现了外网访问容器内。

再来看看容器内如何访问外面的，还是先贴路由信息：

```shell
$ iptables -t nat -L
Chain POSTROUTING (policy ACCEPT)
target     prot opt source               destination
MASQUERADE  all  --  172.18.0.0/16        anywhere
```

容器对外网的请求，会先通过基于docker0网桥的虚拟网络，到达网关docker0，然后就会查宿主机的路由表，然后发现上面这条路由信息，该信息做了一个SNAT操作，告诉该包应该将在docker0网络的源地址转成宿主机器的公网地址；这样，在外网看来就是该宿主机发起的请求，隐藏的内网的docker容器；



# 7. 结语

从docker的概念，安装，使用，了解完上面的知识，我们应该能对docker有一个入门，以及比较清晰的认识了；对于个人以及中小型公司上面的知识已经可以docker话所有环境了；如果docker要应用到大型企业规模，还需要借助docker componse，以及docker swarm来编配和管理多样复杂docker容器集群；关于这个docker集群的管理我们将在后续文章中给出；



# 8.参考文献：

8.1 Device Mapper: http://www.ibm.com/developerworks/cn/linux/l-devmapper/

8.2 cgroup: http://www.ibm.com/developerworks/cn/linux/1506_cgroup/index.html

8.3 namespace: https://en.wikipedia.org/wiki/Linux_namespaces

8.4 unix domain socket: https://en.wikipedia.org/wiki/Unix_domain_socket

8.5 docker command: https://docs.docker.com/engine/reference/commandline/

8.6 docker run command: https://docs.docker.com/engine/reference/run/

8.7 dockerfile command: https://docs.docker.com/engine/reference/builder/