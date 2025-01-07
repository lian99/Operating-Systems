#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include "encdec.h"

#define MODULE_NAME "encdec"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lian Natour, Shatha Maree");

int encdec_open(struct inode *inode, struct file *filp);
int encdec_release(struct inode *inode, struct file *filp);
int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int memory_size = 0;
// Buffers for Caesar and XOR ciphers
char *caesar_buff;
char *xor_buff;

MODULE_PARM(memory_size, "i");

int major = 0;

struct file_operations fops_caesar = {
    .open = encdec_open,
    .release = encdec_release,
    .read = encdec_read_caesar,
    .write = encdec_write_caesar,
    .llseek = NULL,
    .ioctl = encdec_ioctl,
    .owner = THIS_MODULE
};

struct file_operations fops_xor = {
    .open = encdec_open,
    .release = encdec_release,
    .read = encdec_read_xor,
    .write = encdec_write_xor,
    .llseek = NULL,
    .ioctl = encdec_ioctl,
    .owner = THIS_MODULE
};

// Use this structure as your file-object's private data structure
typedef struct {
    unsigned char key;
    int read_state;
} encdec_private_date;

// Module initialization function
int init_module(void)
{
    // Register the device
    major = register_chrdev(major, MODULE_NAME, &fops_caesar);
    if (major < 0) {
        return major;
    }

    // Allocate memory for the Caesar buffer
    caesar_buff = kmalloc(memory_size, GFP_KERNEL);
    if (!caesar_buff) {
        unregister_chrdev(major, MODULE_NAME);
        return -ENOMEM;
    }

    // Allocate memory for the XOR buffer
    xor_buff = kmalloc(memory_size, GFP_KERNEL);
    if (!xor_buff) {
        kfree(caesar_buff);
        unregister_chrdev(major, MODULE_NAME);
        return -ENOMEM;
    }

    return 0;
}

// Module cleanup function
void cleanup_module(void)
{
    // Unregister the device-driver
    unregister_chrdev(major, MODULE_NAME);
    // Free the allocated device buffers using kfree
    if (caesar_buff) {
        kfree(caesar_buff);
    }
    if (xor_buff) {
        kfree(xor_buff);
    }
}

// Open function for the device
int encdec_open(struct inode *inode, struct file *filp)
{
    int minor = MINOR(inode->i_rdev);
    encdec_private_date *data;

    // Set the appropriate file operations based on the minor number
    switch (minor) {
        case 0:
            filp->f_op = &fops_caesar;
            break;
        case 1:
            filp->f_op = &fops_xor;
            break;
        default:
            return -ENODEV;
    }

    // Allocate memory for the private data structure
    data = kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    // Initialize the private data
    data->key = 0;
    data->read_state = ENCDEC_READ_STATE_RAW;
    filp->private_data = data;

    return 0;
}

// Release function for the device
int encdec_release(struct inode *inode, struct file *filp)
{
    // Free the allocated private data if not NULL
    if (filp->private_data) {
        kfree(filp->private_data);
    }
    return 0;
}

// IOCTL function for the device
int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case ENCDEC_CMD_CHANGE_KEY:
            ((encdec_private_date *)filp->private_data)->key = (unsigned char)arg;
            break;
        case ENCDEC_CMD_SET_READ_STATE:
            ((encdec_private_date *)filp->private_data)->read_state = (int)arg;
            break;
        case ENCDEC_CMD_ZERO:
            if (MINOR(inode->i_rdev) == 0) {
                memset(caesar_buff, 0, memory_size);
            } else {
                memset(xor_buff, 0, memory_size);
            }
            break;
        default:
            return -ENOTTY;
    }

    return 0;
}

// Read function for Caesar cipher
ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    encdec_private_date *data = (encdec_private_date *)filp->private_data;
    size_t bytes_to_read, bytes_read;
    char *read_buf;

    // Check if trying to read beyond the buffer
    if (*f_pos >= memory_size)
        return -EINVAL;

    // Calculate the number of bytes to read
    bytes_to_read = count;
    if (bytes_to_read > memory_size - *f_pos)
        bytes_to_read = memory_size - *f_pos;

    read_buf = kmalloc(bytes_to_read, GFP_KERNEL);
    if (!read_buf)
        return -ENOMEM;

    // Copy data from the buffer to read_buf
    memcpy(read_buf, caesar_buff + *f_pos, bytes_to_read);

    // Decrypt data if necessary
    if (data->read_state == ENCDEC_READ_STATE_DECRYPT) {
        size_t i;
        for (i = 0; i < bytes_to_read; i++)
            read_buf[i] = (read_buf[i] - data->key + 128) % 128;
    }

    // Copy data to user space
    if (copy_to_user(buf, read_buf, bytes_to_read)) {
        kfree(read_buf);
        return -EFAULT;
    }

    // Update file position and bytes_read
    *f_pos += bytes_to_read;
    bytes_read = bytes_to_read;

    kfree(read_buf);
    return bytes_read;
}

// Write function for Caesar cipher
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    encdec_private_date *data = (encdec_private_date *)filp->private_data;
    size_t bytes_to_write, bytes_written;
    char *write_buf;

    // Check if trying to write beyond the buffer
    if (*f_pos >= memory_size)
        return -ENOSPC;

    // Calculate the number of bytes to write
    bytes_to_write = count;
    if (bytes_to_write > memory_size - *f_pos)
        bytes_to_write = memory_size - *f_pos;

    write_buf = kmalloc(bytes_to_write, GFP_KERNEL);
    if (!write_buf)
        return -ENOMEM;

    // Copy data from user  to write buffer
    if (copy_from_user(write_buf, buf, bytes_to_write)) {
        kfree(write_buf);
        return -EFAULT;
    }

    // Encrypt data
    size_t i;
    for (i = 0; i < bytes_to_write; i++)
        write_buf[i] = (write_buf[i] + data->key) % 128;

    // Copy data to the buffer
    memcpy(caesar_buff + *f_pos, write_buf, bytes_to_write);

    // Update file position and bytes_written
    *f_pos += bytes_to_write;
    bytes_written = bytes_to_write;

    kfree(write_buf);
    return bytes_written;
}

// Read function for XOR cipher
ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    encdec_private_date *data = (encdec_private_date *)filp->private_data;
    size_t bytes_to_read, bytes_read;
    char *read_buf;

    // Check if trying to read beyond the buffer
    if (*f_pos >= memory_size)
        return -EINVAL;

    // Calculate the number of bytes to read
    bytes_to_read = count;
    if (bytes_to_read > memory_size - *f_pos)
        bytes_to_read = memory_size - *f_pos;

    read_buf = kmalloc(bytes_to_read, GFP_KERNEL);
    if (!read_buf)
        return -ENOMEM;

    // Copy data from the buffer to read_buf
    memcpy(read_buf, xor_buff + *f_pos, bytes_to_read);

    // Decrypt data if necessary
    if (data->read_state == ENCDEC_READ_STATE_DECRYPT) {
        size_t i;
        for (i = 0; i < bytes_to_read; i++)
            read_buf[i] ^= data->key;
    }

    // Copy data to user space
    if (copy_to_user(buf, read_buf, bytes_to_read)) {
        kfree(read_buf);
        return -EFAULT;
    }

    // Update file position and bytes_read
    *f_pos += bytes_to_read;
    bytes_read = bytes_to_read;

    kfree(read_buf);
    return bytes_read;
}

// Write function for XOR cipher
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    encdec_private_date *data = (encdec_private_date *)filp->private_data;
    size_t bytes_to_write, bytes_written;
    char *write_buf;

    // Check if trying to write beyond the buffer
    if (*f_pos >= memory_size)
        return -ENOSPC;

    // Calculate the number of bytes to write
    bytes_to_write = count;
    if (bytes_to_write > memory_size - *f_pos)
        bytes_to_write = memory_size - *f_pos;

    write_buf = kmalloc(bytes_to_write, GFP_KERNEL);
    if (!write_buf)
        return -ENOMEM;

    // Copy data from user space to write_buf
    if (copy_from_user(write_buf, buf, bytes_to_write)) {
        kfree(write_buf);
        return -EFAULT;
    }

    // Encrypt data
    size_t i;
    for (i = 0; i < bytes_to_write; i++)
        write_buf[i] ^= data->key;

    // Copy data to the buffer
   memcpy(xor_buff + *f_pos, write_buf, bytes_to_write);

    // Update file position and bytes_written
    *f_pos += bytes_to_write;
    bytes_written = bytes_to_write;

    kfree(write_buf);
    return bytes_written;
}
