---
layout:     post
title:      "Spark Inside(Cluster)"
subtitle:   "(集群篇)Spark inside让普通开发人员可以通过该篇文档的学习可以快速上手Spark"
date:       2017-04-17 12:30:00
author:     "ryan"
header-img: "img/post-bg-01.jpg"
---

# Digest

前一篇主要讲单机模式下的Spark开发基础，这一篇文章主要分享一下Spark在集群里面的运行架构，以及如何使用Standalone方式来使用Spark的集群管理。



# 1. 集群运行时架构

在Spark集群中，有一个节点来协调其他节点的操作，该节点为“驱动器节点”，而其他节点就是“执行器节点”，从名字大概可以了解他们的作用了，先来看一下架构图：

![2017-04-17-spark-runtime-arch](//ryanwli.github.io/img/2017/2017-04-17-spark-runtime-arch.png)

之前第一篇写的独立驱动程序就是在Spark驱动器节点执行，分析出需要多少任务，然后分发给其他执行器节点进行执行。他们之间的协作需要有一个集群管理器来做，Spark内部提供Standalone进群管理器，也可以使用如Hadoop Yarn，以及Apache Mesos来做管理都可以；



# 2. 驱动器与执行器节点

## 2.1 驱动器节点

在前一篇文章中，写的所有Java示例代码都是由spark-submit将这些代码提交到驱动器节点，驱动器节点执行main方法；然后在该进程中主要做两件事情“分析程序并生成任务”和“调度协作其他执行器节点”。

### 2.1.1 分析生成任务

我们再来看看上篇那张图：

![2017-04-13-spark-hdfs-partition](//ryanwli.github.io/img/2017/2017-04-13-spark-hdfs-partition.png)

这张是Spark读取Hdfs文件，进行并行处理的过来；task是spark的最小执行单元，驱动器节点，负责分析提交的程序，并产生若干task；一个task可能是一个RDD操作，也可能是多个RDD操作的合并(spark驱动器进行会进行优化合并，加快执行效率)；最后讲这些任务打包发送到集群里面，分给这些执行器节点进行执行生成RDD分区；这一过程是将我们写的程序逻辑处理流程(这个逻辑处理流程官方术语也叫DAG, Directed Acyclic Graph)转换为物理执行计划的过程；

### 2.1.2 调度执行器节点

执行器节点启动后会向驱动器节点注册自己。这样驱动器节点就可以根据物理执行计划进行各执行器进程间的调度了；执行器节点负责实际计算，并能缓存中间计算结果，有了中间缓存结果，这样驱动器节点就之后后续task应该往哪些之前的执行器节点扔任务。每个执行器节点有多个线程，每个线程负责任务的执行，然后会将执行结果返回给驱动器节点进程；



## 2.2 执行器节点

执行器节点主要也负责两件事情：

第一，它负责执行由驱动器拆出来的task，并将task的执行结果返回给驱动程序；

第二，它还负责提供集群执行的中间缓存(可配，内存还是硬盘，默认内存)，在驱动器程序协调下可以将前后有依赖的task分配给同一个执行器节点，这样可以充分利用之前的中间缓存结果；



# 3. standalone集群模式

## 3.1 配置集群

### 3.1.1 自动启动

1. 将spark包放到每个集群节点里面去；
2. 用ssh-keygen生成密匙，能让驱动器节点服务器能够无密码访问各个节点的服务器；
3. 在驱动节点的conf/slave下面配置各个执行节点的ip或者hostname；
4. 在驱动节点使用sbin/start-all.sh来实现自动启动，若要停止整个使用sbin/stop-all.sh；

### 3.1.2 手动启动

1. 将spark包放到每个集群节点里面去；

2. 启动驱动节点：

   ```sh
   bin/spark-class org.apache.spark.deploy.master.Master；
   ```

3. 启动执行节点：

   ```shell
   bin/spark-class org.apache.spark.deploy.worker.Worker spark://masterip:7077;
   ```

## 3.2 提交应用

使用如下命令可以将Spark独立驱动程序提交到集群中去：

```sh
bin/spark-submit --master spark://masterip:7077 --class xx.xx.xx yourapp.jar
```

### 3.2.1 客户端部署模式

上面使用的是默认的client模式进行提交驱动程序到集群，这个类似linux系统的中的前台交互执行，虽然这种模式也利用了spark集群，但是它的输入，输出都是在控制在进行，这就意味着你提交这个机器的网络和集群网络是内网，否者执行效率会大打折扣；

### 3.2.2 集群部署模式

还有一种方式是集群方式，命令如下：

```sh
bin/spark-submit --master spark://masterip:7077 --deploy-mode cluster --class xx.xx.xx yourapp.jar
```

这种方式是将驱动程序提交到集群中的某个执行器节点，然后再进行spark-submit操作，这样就肯定是再内网执行驱动程序的提交提高了执行效率，这种方式一般使用在产品环境，但是这种方式有一个条件，就是必须将yourapp.jar放到hdfs，或者在每台执行器节点都要放这个jar，否者会提示不能找到该jar的可能；



## 3.3 配置资源用量

bin/spark-submit提交使用集群资源默认是，每个节点只能使用1G内存，但是CPU核心数默认就是无限；我们可以使用如下命令参数来设置该驱动程序所要使用的集群资源；

```sh
bin/spark-submit --master spark://masterip:7077 --executor-memory 2G --executor-cores 2 --total-executor-cores 10 --class xx.xx.xx yourapp.jar
```

一般一个执行节点，只会开启一个执行器进程，对于上面的配置，该执行器进行是一个JVM，声明的JVM队是2G，每个执行器进程启动2个工作线程执行并行处理，total-executor-cores说明该独立驱动程序在集群里面最多使用10个核数，所以该独立驱动程序组多被分配到5个执行节点服务器上面；