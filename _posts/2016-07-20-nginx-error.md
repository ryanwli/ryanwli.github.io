---
layout:     post
title:      "nginx反向代理的一个权限错误"
subtitle:   "今天copy了一台机器上的nginx到另外一台，导致在另外一台反向代理出来的站点很多静态资源出现“err_incomplete_chunked_encoding”的错误；"
date:       2016-07-19 12:00:00
author:     "ryan"
header-img: "img/post-bg-06.jpg"
---


###导致操作
今天copy了一台机器上的nginx到另外一台，导致在另外一台反向代理出来的站点很多静态资源出现“err_incomplete_chunked_encoding”的错误；

###分析原因
这个错误提示不怎么明显，导致这个错误原因在这个场景下面是因为nginx缓存之前代理tomcat输出的静态资源到proxy_temp目录下面，来提高吞吐量，但是是copy过来的该目录的权限是root权限，所以导致nginx运行的http用户无法访问，最后导致输出到客户端显示上面那个错误，添加一下权限就行了；

###解决办法
>chown -R http:www /usr/local/nginx/proxy_temp
