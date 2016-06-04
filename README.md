# Linux VM Hacks

This repo is a place for experimental code and patches targeting linux 4.6
designed to help me better understand the linux VM subsystem.

This project's sister repo, [linux-vm-notes][vm-notes] contains notes on the
subsystem.

## Hacks

### pagetables

Pagetables is a kernel module which outputs the precise PGD/PUD/PMD/PTE page
table contents for a given process.

It differs from `/proc/<pid>/pagemap` ([doc page][page-map]) in that pagemap is
vastly more useful :) it allows a process to map between virtual and physical
pages without reference to individual page tables whereas `pagetables` is
designed to expose these details.

Additionally, `pagetables` exposes kernel mappings.

#### Usage

__WARNING:__ Don't use this with a kernel you care about. It's experimental and
I've probably made horrific mistakes which will result in data/hair/firstborn
loss.

Compilation should be as simple as `cd pagetables; make`. Then run `sudo insmod
pagetables.ko` to insert the module, preferably in a VM.

1. The current process's PGD is exposed raw at
   `/sys/kernel/debug/pagetables/pgd`.

2. A specific PGD entry can be selected by
   `/sys/kernel/debug/pagetables/pgdindex`, which will cause reads of
   `/sys/kernel/debug/pagetables/pud` to read the PUD page referred to by
   `pgd[pgdindex]`.

3. A specific PUD entry can be selected by
   `/sys/kernel/debug/pagetables/pudindex`, which will cause reads of
   `/sys/kernel/debug/pagetables/pmd` to read the PMD page referred to by
   `pud[pudindex]`.

4. A specific PMD entry can be selected by
   `/sys/kernel/debug/pagetables/pmdindex`, which will cause reads of
   `/sys/kernel/debug/pagetables/pte` to read the PTE page referred to by
   `pmd[pmdindex]`.

There is a userland tool included which outputs page table entries using this
interface, simply run `sudo ./pagetables` from the `pagetables` directory.

## License

All code here is licensed under the [GPL v2][gpl-v2] license to remain
compatible with the kernel itself.

[vm-notes]:https://github.com/lorenzo-stoakes/linux-vm-notes
[page-map]:https://github.com/torvalds/linux/blob/v4.6/Documentation/vm/pagemap.txt
[gpl-v2]:http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
