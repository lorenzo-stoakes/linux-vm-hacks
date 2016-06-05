# Linux VM Hacks

This repo is a place for experimental code and patches targeting linux 4.6
designed to help me better understand the linux VM subsystem.

This project's sister repo, [linux-vm-notes][vm-notes] contains notes on the
subsystem.

## pagetables

Pagetables is a kernel module which outputs the precise PGD/PUD/PMD/PTE page
table contents for a given process.

It differs from `/proc/<pid>/pagemap` ([doc page][page-map]) in that pagemap is
vastly more useful :) it allows a process to map between virtual and physical
pages without reference to individual page tables whereas `pagetables` is
designed to expose these details.

Additionally, `pagetables` exposes kernel mappings.

### Building

```
$ cd pagetables
$ make
```

### Usage

__WARNING:__ Don't use this with a kernel you care about. It's experimental and
I've probably made horrific mistakes which will result in data/hair/firstborn
loss.

__EVEN MORE SERIOUS WARNING:__ This module is a security nightmare and exposes
sensitive data, including kernel mappings and the mappings of any specified
process. You've been warned!

```
$ cd pagetables
$ sudo insmod pagetables.ko
$ sudo ./pagetables <pid, defaults to pagetables itself>
```

## License

All code here is licensed under [GPL v2][gpl-v2] to remain compatible with the
kernel itself.

[vm-notes]:https://github.com/lorenzo-stoakes/linux-vm-notes
[page-map]:https://github.com/torvalds/linux/blob/v4.6/Documentation/vm/pagemap.txt
[gpl-v2]:http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
