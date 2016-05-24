# Linux VM Hacks

This repo is a place for experimental code and patches targeting linux 4.6
designed to help me better understand the linux VM subsystem.

This project's sister repo, [linux-vm-notes][vm-notes] contains notes on the
subsystem.

## Hacks

### Tables

__Work in Progress__: Currently only the PGD is exposed.

Tables is a kernel module which outputs the precise PGD/PUD/PMD/PTE page table
contents for a given process.

It differs from `/proc/<pid>/pagemap` ([doc page][page-map]) in that pagemap is
vastly more useful :) it allows a process to map between virtual and physical
pages without reference to individual page tables whereas `tables` is designed
to expose these details.

#### Usage

__WARNING:__ Don't use this with a kernel you care about. It's experimental and
I've probably made horrific mistakes which will result in data/hair/firstborn
loss.

Compilation should be as simple as `cd tables; make`. Then run `sudo insmod
tables.ko` to insert the module, preferably in a VM.

The current process's PGD is exposed raw at `/sys/kernel/debug/tables/pgd`.

There is a userland tool included which outputs PGD entries using this
interface, simply run `sudo ./tables` from the `tables` directory.

## License

All code here is licensed under the [GPL v2][gpl-v2] license to remain
compatible with the kernel itself.

[vm-notes]:https://github.com/lorenzo-stoakes/linux-vm-notes
[page-map]:https://github.com/torvalds/linux/blob/v4.6/Documentation/vm/pagemap.txt
[gpl-v2]:http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
