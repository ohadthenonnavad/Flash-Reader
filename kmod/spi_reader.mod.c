#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xcbae5412, "__const_udelay" },
	{ 0x67628f51, "msleep" },
	{ 0xefbce01e, "pci_get_domain_bus_and_slot" },
	{ 0xcb687552, "pci_read_config_dword" },
	{ 0x97dd6ca9, "ioremap" },
	{ 0xe8213e80, "_printk" },
	{ 0xc8006703, "filp_open" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xa62b1cc9, "kmalloc_caches" },
	{ 0xd1f07d8f, "__kmalloc_cache_noprof" },
	{ 0xbf2f841f, "kernel_write" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0x79a9dee7, "filp_close" },
	{ 0x12ad300e, "iounmap" },
	{ 0x17163423, "pci_dev_put" },
	{ 0xacac6336, "memcpy_fromio" },
	{ 0x78de09ff, "pci_enable_device" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xc368a5e1, "param_ops_ullong" },
	{ 0xc368a5e1, "param_ops_charp" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xd272d446, "__fentry__" },
	{ 0xab006604, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xcbae5412,
	0x67628f51,
	0xefbce01e,
	0xcb687552,
	0x97dd6ca9,
	0xe8213e80,
	0xc8006703,
	0xbd03ed67,
	0xa62b1cc9,
	0xd1f07d8f,
	0xbf2f841f,
	0xcb8b6ec6,
	0x79a9dee7,
	0x12ad300e,
	0x17163423,
	0xacac6336,
	0x78de09ff,
	0xd272d446,
	0xc368a5e1,
	0xc368a5e1,
	0xd272d446,
	0xd272d446,
	0xab006604,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"__const_udelay\0"
	"msleep\0"
	"pci_get_domain_bus_and_slot\0"
	"pci_read_config_dword\0"
	"ioremap\0"
	"_printk\0"
	"filp_open\0"
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"kernel_write\0"
	"kfree\0"
	"filp_close\0"
	"iounmap\0"
	"pci_dev_put\0"
	"memcpy_fromio\0"
	"pci_enable_device\0"
	"__stack_chk_fail\0"
	"param_ops_ullong\0"
	"param_ops_charp\0"
	"__x86_return_thunk\0"
	"__fentry__\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "8AF42063B7785C5FA57E1A3");
