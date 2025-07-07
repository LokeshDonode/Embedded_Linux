// Booting a bare minimum linux using QEMU
# Command to install Qemu
$ sudo apt-get install qemu-system-x86

# Utilities
# Command to install build essentials
$ sudo apt-get install build-essential

$ sudo apt-get install lzop flex bison libncurses-dev

# Download Linux Kernel and BusyBox
# Create workspace directory and cd to workspace directory
$ mkdir ~/workspace_kernel_mini && cd ~/workspace_kernel_mini  ( && or ;; for running both the commands)

# Downloading latest Linux Kernel 
The official Linux Kernel Archives lists the current stable release as 6.15.5, published on July 6, 2025. You can download the tarball from:
linux-6.15.5.tar.gz (stable release)
$ wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.15.5.tar.gz  

# Downloading latest Busybox -- he latest official BusyBox release is version 1.37.0, which was released on September 27, 2024
$ wget https://busybox.net/downloads/busybox-1.37.0.tar.bz2

**Creating Bare minimum Linux**
$ cd ~/workspace_kernel_mini

# extracting the kernel source
$ tar -xvf /linux-6.15.5.tar.gz

# change directory to extracted linux source directory
$ cd linux-6.15.5/

# Configure smallest possible kernel
$ make allnoconfig     
>> "This will create .config file setting values to 'n' as much as possible.
>> For more refer Documentation/admin-guide/README.rst at kernel.org"

# Customization
$ make menuconfig 

 >> This will open configuration options. Now set following options …
> > Option 1: Host name
General setup > Default hostname  “bare-mini-linux-kernel"
![image](https://github.com/user-attachments/assets/0d9f6a9a-f8d1-4014-8294-6d132326d7c2)

> > Option 2: Enable 64 bit support
64-bit Kernel
![image](https://github.com/user-attachments/assets/06146677-4075-41f7-84b3-b45bea4b40d8)

> > Option 3: Enable support for RAM disk
General Setup > Initial RAM filesystem and RAM disk (initramfs/initrd) support
![image](https://github.com/user-attachments/assets/3e36d2e2-ff3d-43a5-8d3f-e264a7c71d2e)

> > Option 4: Enable printk support
General Setup > Configure standard kernel features (expert users) > Enable support for printk
![image](https://github.com/user-attachments/assets/006a0079-e4b6-4bc5-86c5-c444115e1144)

> > Option 5: Ensure Gzip Kernel compression
General Setup >kernel compression mode (Gzip)
![image](https://github.com/user-attachments/assets/454e67c0-c856-4176-8267-046515028165)

> > Option 6: ELF binary and script
Executable file formats > Kernel support for ELF binaries
Executable file formats > Kernel support for scripts starting with #!
![image](https://github.com/user-attachments/assets/20b3839a-1952-4cd6-87da-4c25000229f9)

> > Option 7: Enable devtmpfs
Device Driver > Generic Driver Options > Maintain a devtmpfs filesystem to mount at /dev
Device Driver > Generic Driver Options > Automount devtmpfs at /dev, after the kernel mounted the rootfs
![image](https://github.com/user-attachments/assets/d696cd37-88bc-4c36-9b26-a8e6c77fb61f)

> >Option 8: Enable TTY
Device Driver > Character devices > Enable TTY
![image](https://github.com/user-attachments/assets/55da0f3c-9e2f-401b-9670-bc1d1636a1eb)

> > Option 9: Enable Serial Drivers
Device Driver > Character devices > Serial Drivers  > 8250/16550 and compatible serial support
Device Driver > Character devices > Serial Drivers  > Console on 8250/16550 and compatible serial port
![image](https://github.com/user-attachments/assets/d1bef354-c024-4f3e-9429-da5991c876d0)

> > Option 10: Pseudo filesystems
File systems > Pseudo filesystems > /proc file system support
File systems > Pseudo filesystems > /sysfs file system support
![image](https://github.com/user-attachments/assets/a4d5faa2-439c-4a6b-aba3-842dd4b4d1e1)

Now, save and close configuration window. This will save .config file.
# We can review .config file …
$ gedit bare-mini-linux-kernel.config


# Building Linux Kernel
$ make -j4  ( -j4 runs 4 threads in 4 cores of the processor to speed up the process of compilation)

>> To check cores in your PC
$ nproc

# Enter to boot directory in linux kernel
$ /home/lokesh/workspace_kernel_mini/linux-6.15.5/arch/x86/boot  (will take a while)

Depending on the internet speed and PC configuration, this will take some time. 
Once build is complete, you can verify presence of kernel.

# Path "$pwd" 
example : ~/workspace_kernel_mini/linux-6.15.5/arch/x86/boot
$ ls -sh bzImage 
1.6M bzImage

Now we have Linux kernel available. Next is creating initram disk.

**Creating Initramfs**

# change directory to workspace directory
$ cd ~/workspace_kernel_mini

# extracting the Busybox source tree
$  tar -xvf busybox-1.37.0.tar.bz2 

# change directory to busybox
$ cd busybox-1.37.0/

# Customize busybox 
$ make menuconfig

This will start configuration menu for BusyBox. We need only one setting.
Settings > Build static binary (no shared libs)
![image](https://github.com/user-attachments/assets/e46b7ff1-da61-40fa-9bb1-db767a5983ff)

Now, exit and save.
It is time to build busybox.

# Build
$ make -j4

# Install  
$ make install
This will install binaries in “./_install” directory

# change directory to workspace directory
$  cd ~/workspace_kernel_mini

# creating mini_linux directory and cd to mini_linux
$ mkdir mini_linux && cd mini_linux

# its time to create required directory. We need etc, proc, sys and dev directory.
$ mkdir -p etc proc sys dev

# Coping all busybox installed files to ~/workspace_kernel_mini/mini_linux
$  cp -a ~/workspace_kernel_mini/busybox-1.37.0/_install/* .

Create init script in “mini_linux” directory.
This is the content of init script.
/**
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
cat <<!
boot took $(cut -d' ' -f1 /proc/uptime) seconds
exec /bin/sh
**/

# check the init file 
$ cat init

# It is time to make init file executable. Give executable permission to init file
$  chmod +x init

# Creating initramfs as cpio archieve
$ find . -print0 | cpio --null -ov --format=newc | gzip -9 > initramfs.cpio.gz

**Note :** Description of “find . -print0 | cpio --null -ov --format=newc | gzip -9 > initramfs.cpio.gz”

find . -print 0 : this will create a list of all files. print the full file name on the standard output, followed by a null character
cpio --null -ov --format=newc: copy file to archive, use newc (SVR4) archive format , -o = create , -v verbosly list all files processed
gzip -9 > initramfs.cpio.gz : creating gzip file, with compression speed 9.
initramfs.cpio.gz should be created.

**Booting Mini Linux in QEMU**

Now we have, Linux kernel
~/workspace_kernel_mini/linux-6.15.5/arch/x86/boot/bzImage

And ramfs ..
~/workspace_kernel_mini/mini_linux/initramfs.cpio.gz

# it is time to start QEMU and booting our mini Linux.
$ qemu-system-x86_64 \
-kernel ~/workspace_kernel_mini/linux-6.15.5/arch/x86/boot/bzImage \
-initrd ~/workspace_kernel_mini/mini_linux/initramfs.cpio.gz \
-append "init=/bin/sh console=ttyS0"  -nographic

Command

Description

qemu-system-x86_64

Command to start QEMU for system.

-kernel ~/workspace_kernel_mini/Linux
-6.15.5/arch/x8664/boot/bzImage \

“-kernel” option to provide path of bzImage.

-initrd ~/workspace_kernel_mini/mini_linux/initramfs.cpio.gz \

“-initrd” option to provide initramfs file.

-append "init=/bin/sh console=ttyS0" 

“-append” provide command line argument for Linux kernel.

-nographic

We don’t want to use graphics.


# Review of bare minimum Linux Kernel running in QEMU
Now we can try multiple commands to review our bare minimum Linux.
$ uname -a

$ top
>> top command display all the process running in Linux. You can see we have /bin/sh which is bourne shell . 
Process ID (PID) of bourne shell is 1. and second process is kthreadd, process Id is 2. 
kthreadd is the kernel thread started after init and all kernel threads are descendant of kthreadd.

$ cat /proc/cmdline
> This command shows command line argument give to the kernel.

$ cat /proc/cpuinfo
>> you can review parameter related to CPU.  Model name is QEMU Virtual CPU

$ dmesg
>> This command shows all kernel messages which were routed to ttyS0.
You should able to find following information in kernel messages.












