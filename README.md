HTTPSCWS
========

简介
-----

HTTPSCWS是一个基于HTTP协议简易的中文分词系统，采用SCWS，它可以将输入的文本字符串根据设定好的选项切割后以数组形式返回每一个词汇。
它为中文而编写，暂时支持 utf8 字符集，适当的修改词典后也可以支持非中文的多字节语言切词（如日文、韩文等）。
除分词外，还提供一个简单的关键词汇统计功能，它内置了一个简单的算法来排序。

需求
-----

本扩展需要 scws-1.x.x 的支持。

安装
-----

```shell
# 先安装libevent
$ yum install libevent-devel

# 安装scws
$ wget http://www.xunsearch.com/scws/down/scws-1.2.2.tar.bz2
$ tar xf scws-1.2.2.tar.bz2
$ cd scws-1.2.2
$ ./configure
$ make
$ make install
$ cp -R etc /etc/scws

# 安装scws词库
$ wget http://www.xunsearch.com/scws/down/scws-dict-chs-utf8.tar.bz2
$ tar xf scws-dict-chs-utf8.tar.bz2
$ mv dict.utf8.xdb /etc/scws

# 安装httpscws
$ cd httpscws
$ cmake .
$ make
$ make install
```

启动服务
--------

```shell
$ /usr/local/httpscws/httpscws -d -l 127.0.0.1 -x /etc/scws/ -i /var/run/httpscws.pid
```

作者
----

Ivan Lam *&lt;ivan.lin.1985@gmail.com&gt;*
