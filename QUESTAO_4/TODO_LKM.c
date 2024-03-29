/**
 * @file   TODO_LKM.c
 * @author Luigi Muller
 * @date   10 June 2019
 * @version 0.1
 * @brief   A character driver to support a task list that will run as an LKM. 
 * This code is based on Derek Molloy's code.
 * @see http://www.derekmolloy.ie/ for the base code.
 */

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>        // Required for the copy to user function
#include <linux/list.h>           // Required for task list
#include <linux/string.h>         // Required for string functions
#include <linux/slab.h>           // Required for kmalloc function

#define  DEVICE_NAME "TODO_LIST"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "todo"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Luigi Muller");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for to do list");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static int majorNumber;                    ///< Stores the device number -- determined automatically
static char message[1200] = {0};           ///< Memory for the string that is passed from userspace
static short size_of_message;              ///< Used to remember the size of the string stored
static int numberOpens = 0;                ///< Counts the number of times the device is opened
static struct class *ebbcharClass = NULL;  ///< The device-driver class struct pointer
static struct device *ebbcharDevice = NULL;///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
static int dev_open(struct inode *, struct file *);

static int dev_release(struct inode *, struct file *);

static ssize_t dev_read(struct file *, char *, size_t, loff_t *);

//static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset);
/**
 * @brief The structure will contain the description of the task to be done and a struct from the list.
*/
struct todo_list {
    char descricao[256];
    struct list_head list;
};

struct todo_list todo_head;

/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops ={
                .open = dev_open,
                .read = dev_read,
                .write = dev_write,
                .release = dev_release,
        };

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init todo_init(void) {
    printk(KERN_INFO "TODO LKM: iniciando o TODO list em LKM\n");

    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "TODO LKM: falhou em registrar um major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "TODO LKM: major number registrado como %d\n", majorNumber);

    // Register the device class
    ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(ebbcharClass)) {                // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "TODO LKM: falha em registrar a classe do dispositivo\n");
        return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "TODO LKM: classe do dispositivo registrado corretamente\n");

    // Register the device driver
    ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(ebbcharDevice)) {               // Clean up if there is an error
        class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "TODO LKM: falha em criar o dispositivo\n");
        return PTR_ERR(ebbcharDevice);
    }
    printk(KERN_INFO "TODO LKM: dispositivo criado corretamente\n"); // Made it! device was initialized

    INIT_LIST_HEAD( &todo_head.list ); //inicia a lista

    printk(KERN_INFO "TODO LKM: todo list head foi inicializado\n"); // todo_head was initialized

    return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit todo_exit(void) {
	struct list_head *pos, *q;
    struct todo_list *tmp;

    device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
    class_unregister(ebbcharClass);                          // unregister the device class
    class_destroy(ebbcharClass);                             // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number

    list_for_each_safe(pos, q, &todo_head.list ){
        tmp = list_entry(pos, struct todo_list, list);
        printk(KERN_INFO "TODO LKM: apagando recado: %s\n", tmp->descricao);
        list_del(pos);
        kfree(tmp);
    } //Deleting list nodes
    printk(KERN_INFO "TODO LKM: se nao aparecer mensagem de recado apagado entao a lista esta vazia\n");
    list_for_each_safe(pos, q, &todo_head.list ){
        tmp = list_entry(pos, struct todo_list, list);
        printk(KERN_INFO "TODO LKM: apagando recado: %s\n", tmp->descricao);
        list_del(pos);
        kfree(tmp);
    } //Trying to delete list nodes
    printk(KERN_INFO "TODO LKM: saindo de boas do kernel!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep) {
    numberOpens++;
    printk(KERN_INFO "TODO LKM: dispositivo foi aberto %d vez(es)\n", numberOpens);
    return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;
    struct list_head *pos;
    struct todo_list *tmp;

    list_for_each(pos, &todo_head.list ){

        tmp = list_entry(pos, struct todo_list, list);

        strcat(message, tmp->descricao);
        strcat(message, "\n");
        size_of_message = strlen(message);
    }//Creates a text with all tasks in the task list
    
    error_count = copy_to_user(buffer, message, size_of_message); // copy_to_user has the format ( * to, *from, size) and returns 0 on success

    if (error_count == 0) {            // if true then have success
        printk(KERN_INFO "TODO LKM: enviado %d caracteres para o usuario\n", size_of_message);
        return (size_of_message = 0);  // clear the position to the start and return 0
    } else {
        printk(KERN_ALERT "TODO LKM: falha ao enviar %d caracteres para o usuario\n", error_count);
        return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
    }
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    struct todo_list *tmp;

    tmp = kmalloc(sizeof(struct todo_list), GFP_ATOMIC); //Memory allocation
    if(tmp){
    	strcpy(tmp->descricao, "Abrir porta");           //Add activity description
    	list_add_tail( &tmp->list, &todo_head.list);     //Add the nodes
    } else {
    	printk(KERN_ALERT "TODO LKM: uma tarefa nao teve espaco alocado\n");
    }

    tmp = kmalloc(sizeof(struct todo_list), GFP_ATOMIC); //Memory allocation
    if(tmp){
    	strcpy(tmp->descricao, "Beber cafe");            //Add activity description
    	list_add_tail( &tmp->list, &todo_head.list);     //Add the nodes
    } else {
    	printk(KERN_ALERT "TODO LKM: uma tarefa nao teve espaco alocado\n");
    }
    
    tmp = kmalloc(sizeof(struct todo_list), GFP_ATOMIC); //Memory allocation
    if(tmp){
    	strcpy(tmp->descricao, "Comer lasanha");         //Add activity description
    	list_add_tail( &tmp->list, &todo_head.list);     //Add the nodes
    } else {
    	printk(KERN_ALERT "TODO LKM: uma tarefa nao teve espaco alocado\n");
    }

    tmp = kmalloc(sizeof(struct todo_list), GFP_ATOMIC); //Memory allocation
    if(tmp){
    	strcpy(tmp->descricao, "Dormir");                //Add activity description
    	list_add_tail( &tmp->list, &todo_head.list);     //Add the nodes
    } else {
    	printk(KERN_ALERT "TODO LKM: uma tarefa nao teve espaco alocado\n");
    }

    tmp = kmalloc(sizeof(struct todo_list), GFP_ATOMIC); //Memory allocation
    if(tmp){
    	strcpy(tmp->descricao, "Estudar");               //Add activity description
    	list_add_tail( &tmp->list, &todo_head.list);     //Add the nodes
    } else {
    	printk(KERN_ALERT "TODO LKM: uma tarefa nao teve espaco alocado\n");
    }
    
    return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "TODO LKM: dispositivo fechado corretamente\n");
    return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(todo_init);
module_exit(todo_exit);