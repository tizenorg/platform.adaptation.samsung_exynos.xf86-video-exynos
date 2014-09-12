#include "../../src/sec.h"

typedef int bool;

struct udev_list_node 
{
        struct udev_list_node *next, *prev;
};

struct udev_list {
        struct udev *udev;
        struct udev_list_node node;
        struct udev_list_entry **entries;
        unsigned int entries_cur;
        unsigned int entries_max;
        bool unique;
};

struct udev_enumerate {
        struct udev *udev;
        int refcount;
        struct udev_list sysattr_match_list;
        struct udev_list sysattr_nomatch_list;
        struct udev_list subsystem_match_list;
        struct udev_list subsystem_nomatch_list;
        struct udev_list sysname_match_list;
        struct udev_list properties_match_list;
        struct udev_list tags_match_list;
        struct udev_device *parent_match;
        struct udev_list devices_list;
        struct syspath *devices;
        unsigned int devices_cur;
        unsigned int devices_max;
        bool devices_uptodate:1;
        bool match_is_initialized;
};

struct udev 
{
        int refcount;
        void (*log_fn)(struct udev *udev,
                       int priority, const char *file, int line, const char *fn,
                       const char *format, va_list args);
        void *userdata;
        char *sys_path;
        char *dev_path;
        char *rules_path[4];
        unsigned long long rules_path_ts[4];
        int rules_path_count;
        char *run_path;
        struct udev_list properties_list;
        int log_priority;
};



struct udev *udev_new(void)
{
	return NULL;
}

struct udev_enumerate *udev_enumerate_new(struct udev *udev)
{
	return NULL;
}

int udev_enumerate_add_match_subsystem(struct udev_enumerate *udev_enumerate, const char *subsystem)
{
	return 0;
}

int udev_enumerate_add_match_sysname(struct udev_enumerate *udev_enumerate, const char *sysname)
{
	return 0;
}

int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate)
{
		return 0;
}

struct udev_list_entry *udev_enumerate_get_list_entry(struct udev_enumerate *udev_enumerate)
{
	return NULL;
}

const char *udev_list_entry_get_name(struct udev_list_entry *list_entry)
{
	return NULL;
}	

struct udev_device *udev_device_new_from_syspath(struct udev *udev, const char *syspath)
{
	return NULL;
}

const char *udev_device_get_property_value(struct udev_device *udev_device, const char *key)
{
	return NULL;
}

void udev_device_unref(struct udev_device *udev_device)
{

}

void udev_enumerate_unref(struct udev_enumerate *udev_enumerate)
{

}

void udev_unref(struct udev *udev)
{

}

struct udev_list_entry *udev_list_entry_get_next(struct udev_list_entry *list_entry)
{
	return NULL;
}

const char *udev_device_get_devnode(struct udev_device *udev_device)
{
	return NULL;
}

struct udev_monitor *udev_monitor_new_from_netlink(struct udev *udev, const char *name)
{
	return NULL;
}

int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor *udev_monitor,
                                                    const char *subsystem, const char *devtype)
{
	return 0;
}


int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
	return 0;
}

void udev_monitor_unref(struct udev_monitor *udev_monitor)
{

}

int udev_monitor_get_fd(struct udev_monitor *udev_monitor)
{
	return 0;
}

struct udev *udev_monitor_get_udev(struct udev_monitor *udev_monitor)
{
	return NULL;
}

struct udev_device *udev_monitor_receive_device(struct udev_monitor *udev_monitor)
{
	return NULL;
}

dev_t udev_device_get_devnum(struct udev_device *udev_device)
{
	return 0;
}
































































