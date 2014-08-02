### crane

A mini linux container.

### features

* Copy on write filesystem (aufs)
* Namespaces (clone)
* Cgroup
* Network Settings
* ...

### example

```bash
crane --name test --image /data/rootfs --bridge=br0 --network="192.168.1.5/24@192.168.1.1#192.168.1.255" --memory=200m --cpuset=0-1
```

### reference

The part of network code copy from [lxc](https://github.com/lxc/lxc) project.

### License

Copyright (c) 2014 kenshin54. Distributed under the MIT License. See LICENSE.txt for further details.
