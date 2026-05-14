# Bionic OS

<p align="center">
  <img src="assets/bionic-logo.png" alt="Bionic OS Logo" width="180">
</p>

<p align="center">
  <b>Experimental OS from Scratch</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/status-experimental-orange" alt="Status">
  <img src="https://img.shields.io/badge/architecture-x86%2032--bit-blue" alt="Architecture">
  <img src="https://img.shields.io/badge/boot-GRUB%20%2F%20Multiboot-purple" alt="Boot">
  <img src="https://img.shields.io/badge/language-C%20%2B%20ASM-green" alt="Language">
</p>

---

## About

**Bionic OS** is an experimental operating system built from scratch using **C and x86 Assembly**.

The goal of Bionic is not to become a production-ready operating system, but to explore how an OS works internally: booting, memory management, filesystems, keyboard input, graphics, GUI systems, windows, and user interaction.

It started as a simple text-mode kernel, but has evolved into a small graphical environment with a desktop, window system, RAM filesystem, graphical file explorer, terminal, text editor, and mouse support.

> Bionic is a learning project, a playground, and a personal operating system experiment.

---

## Features

### Kernel

- Custom x86 32-bit kernel
- GRUB / Multiboot boot support
- Global Descriptor Table
- Interrupt Descriptor Table
- IRQ handling
- Keyboard input
- PS/2 mouse input
- Physical memory manager
- Basic heap allocator with `kmalloc` and `kfree`

---

### Memory Management

Bionic includes a simple physical memory manager and a kernel heap.

Current memory features:

- Physical memory detection
- Frame allocation
- `kmalloc`
- `kcalloc`
- `kfree`
- Heap usage information
- RAM-based memory structures

This allows the kernel to dynamically allocate memory for internal structures such as files, folders, windows, GUI state, and other runtime data.

---

### RAMFS

Bionic includes a custom in-memory filesystem called **RAMFS**.

It supports:

- Directories
- Files
- Relative and absolute paths
- File writing
- File preview
- File editing
- Copying
- Moving
- Renaming
- Recursive deletion
- Tree navigation

Example shell commands:

```txt
mkdir /home
mkdir -p /home/user/docs
touch /home/user/docs/notes.txt
write /home/user/docs/notes.txt Hello from Bionic
cat /home/user/docs/notes.txt
ls /home/user/docs
tree /
rename /home/user/docs/notes.txt todo.txt
cp /home/user/docs/todo.txt /home/user/docs/backup.txt
mv /home/user/docs/backup.txt /home/user/backup.txt
rm -r /home/user/docs
