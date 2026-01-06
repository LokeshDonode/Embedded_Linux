#include<linux/module.h>  // Needed for all modules
#include<linux/kernel.h>  // Needed for KERN_INFO
#include<linux/init.h>   // Needed for macros

// init Function
static int __init my_hello_init(void){

    printk(KERN_INFO "Hello World: Module Loaded successfully !\n");
    return 0;
}

// Exit Function
static init __exit my_hello_exit(void){
    printk(KERN_INFO "Goodbye World: Module Unloaded.\n");
}

// Registering the entry exit points
module_init(my_hello_init);
module_exit(my_hello_exit);

// Module Licensing
MODULE_LICENSE("GPL");
MODULE_AUTHOR("lokeshdonode@1994@gmail.com");
MODULE_DESCRIPTION("A Simple hello world module");

// make
// sudo insmod hello_driver.ko
// dmesg | tail
// sudo rmmod hello_driver