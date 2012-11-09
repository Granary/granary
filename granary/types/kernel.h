/*
 * kernel_types.hpp
 *
 *  Created on: 2012-10-01
 *      Author: pag
 *     Version: $Id$
 */

#ifndef DRK_KERNEL_TYPES_HPP_
#define DRK_KERNEL_TYPES_HPP_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DONT_INCLUDE_HEADERS
#   include <stdbool.h>
#else
#   define bool _Bool
#endif

/// special purpose "ignore" type
/// !!! can only be used at the end of another struct; means we don't care about
///     the last field, and we don't know its size, so it's totally unsafe to use
///     it.
typedef void *IGNORE_REST;
#define IFNAMSIZ        16
#define IFALIASZ        256
#define MAX_ADDR_LEN    32

/// general list types

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned int __uint32_t;
typedef unsigned int uint32_t;
typedef uint32_t __u32;
typedef __u32 __wsum;
typedef unsigned char u8;
typedef unsigned long long u64;
typedef unsigned gfp_t;
typedef long unsigned int size_t;

typedef long int __ssize_t ;
typedef __ssize_t ssize_t ;
typedef long int __off64_t ;
typedef __off64_t __loff_t ;
typedef __loff_t loff_t ;

enum dma_data_direction { DMA_BIDIRECTIONAL = 0 , DMA_TO_DEVICE = 1 , DMA_FROM_DEVICE = 2 , DMA_NONE = 3 , } ;

typedef enum {
    PHY_INTERFACE_MODE_MII ,
    PHY_INTERFACE_MODE_GMII ,
    PHY_INTERFACE_MODE_SGMII ,
    PHY_INTERFACE_MODE_TBI ,
    PHY_INTERFACE_MODE_RMII ,
    PHY_INTERFACE_MODE_RGMII ,
    PHY_INTERFACE_MODE_RGMII_ID ,
    PHY_INTERFACE_MODE_RGMII_RXID ,
    PHY_INTERFACE_MODE_RGMII_TXID ,
    PHY_INTERFACE_MODE_RTBI
} phy_interface_t;

typedef struct { volatile int counter ; } atomic_t;
typedef struct { volatile long counter ; } atomic64_t;

typedef struct raw_spinlock { unsigned int slock ; } raw_spinlock_t;
typedef struct { raw_spinlock_t raw_lock ; } spinlock_t;

struct netdev_hw_addr_list { struct list_head list ; int count ; };
struct net_device_stats { unsigned long rx_packets ; unsigned long tx_packets ; unsigned long rx_bytes ; unsigned long tx_bytes ; unsigned long rx_errors ; unsigned long tx_errors ; unsigned long rx_dropped ; unsigned long tx_dropped ; unsigned long multicast ; unsigned long collisions ; unsigned long rx_length_errors ; unsigned long rx_over_errors ; unsigned long rx_crc_errors ; unsigned long rx_frame_errors ; unsigned long rx_fifo_errors ; unsigned long rx_missed_errors ; unsigned long tx_aborted_errors ; unsigned long tx_carrier_errors ; unsigned long tx_fifo_errors ; unsigned long tx_heartbeat_errors ; unsigned long tx_window_errors ; unsigned long rx_compressed ; unsigned long tx_compressed ; } ;


typedef struct pm_message { int event ; } pm_message_t;
struct hlist_node { struct hlist_node * next , **pprev ;};

struct page;

struct firmware {
    size_t size;
    const u8 * data;
    struct page **pages ;
};

struct semaphore { spinlock_t lock ; unsigned int count ; struct list_head wait_list ; };
struct sysfs_dirent;
struct kref { atomic_t refcount; };
struct kobj_type;
struct kset;

struct __wait_queue_head { spinlock_t lock ; struct list_head task_list ; } ;
/*--__wait_queue_head--*/
typedef struct __wait_queue_head wait_queue_head_t ;

enum dpm_state { DPM_INVALID , DPM_ON , DPM_PREPARING , DPM_RESUMING , DPM_SUSPENDING , DPM_OFF , DPM_OFF_IRQ , } ;
/*--dpm_state--*/
enum rpm_status { RPM_ACTIVE = 0 , RPM_RESUMING , RPM_SUSPENDED , RPM_SUSPENDING , } ;
/*--rpm_status--*/
enum rpm_request { RPM_REQ_NONE = 0 , RPM_REQ_IDLE , RPM_REQ_SUSPEND , RPM_REQ_RESUME , } ;
/*--rpm_request--*/

struct timer_list {
    struct list_head entry ;
    unsigned long expires ;
    void ( * function ) ( unsigned long ) ;
    unsigned long data ;
    struct tvec_base * base ;
    void * start_site ;
    char start_comm [ 16 ] ;
    int start_pid ;
} ;

struct pci_dynids { spinlock_t lock ; struct list_head list ; } ;
/*--pci_dynids--*/
typedef unsigned int pci_ers_result_t ;

typedef atomic64_t atomic_long_t;
typedef void ( *work_func_t ) ( struct work_struct *work );

struct work_struct {
    atomic_long_t data;
    struct list_head entry;
    work_func_t func;
};

struct dev_pm_info {
    pm_message_t power_state ;
    unsigned int can_wakeup:1;
    unsigned int should_wakeup:1;
    enum dpm_state status;
    struct list_head entry;
    struct timer_list suspend_timer;
    unsigned long timer_expires;
    struct work_struct work;
    wait_queue_head_t wait_queue;
    spinlock_t lock;
    atomic_t usage_count;
    atomic_t child_count;
    unsigned int disable_depth:3;
    unsigned int ignore_children:1;
    unsigned int idle_notification:1;
    unsigned int request_pending:1;
    unsigned int deferred_resume:1;
    enum rpm_request request;
    enum rpm_status runtime_status;
    int runtime_error;
};

struct kobject {
    const char * name;
    struct list_head entry;
    struct kobject * parent;
    struct kset * kset;
    struct kobj_type * ktype;
    struct sysfs_dirent * sd;
    struct kref kref;
    unsigned int state_initialized:1;
    unsigned int state_in_sysfs:1;
    unsigned int state_add_uevent_sent:1;
    unsigned int state_remove_uevent_sent:1;
    unsigned int uevent_suppress:1;
};

struct dev_archdata { void * acpi_handle ; struct dma_map_ops * dma_ops ; };
typedef unsigned long int __dev_t;
typedef __dev_t dev_t ;

struct klist_node { void * n_klist ; struct list_head n_node ; struct kref n_ref ; };
struct class1;

struct device {
    struct device * parent ;
    struct device_private * p ;
    struct kobject kobj;
    const char * init_name;
    struct device_type * type;
    struct semaphore sem;
    struct bus_type * bus;
    struct device_driver * driver;
    void * platform_data;
    struct dev_pm_info power;
    int numa_node ;
    u64 * dma_mask ;
    u64 coherent_dma_mask ;
    struct device_dma_parameters * dma_parms ;
    struct list_head dma_pools ;
    struct dma_coherent_mem * dma_mem ;
    struct dev_archdata archdata ;
    dev_t devt ;
    spinlock_t devres_lock ;
    struct list_head devres_head ;
    struct klist_node knode_class ;
    struct class1 *class1;
    const struct attribute_group * * groups ;
    void ( * release ) ( struct device * dev ) ;
};
/* The device structure describes a single device. */
#if 0

/// types for device drivers

struct bus_type;
struct module;
struct pci_device_id;
struct pci_dev;
struct netdev_tx_t;
struct sk_buff;
typedef int u16;
struct scatterlist;
struct vlan_group;
struct neigh_parms;
struct ifmap;
struct ifreq;
struct iw_handler_def;
struct iw_public_data;
struct header_ops;
struct netdev_hw_addr_list;
struct dev_addr_list;
struct wireless_dev;
struct netdev_hw_addr_list;
struct netdev_queue;
struct Qdisc;
struct list_head;
struct hlist_node;
struct net_device;
struct netpoll_info;
struct net;
struct net_bridge_port;
struct macvlan_port;
struct garp_port;
struct attribute_group;
struct rtnl_link_ops;
struct dcbnl_rtnl_ops;
/*
 *  Network device statistics. Akin to the 2.0 ether stats but
 *  with byte counters.
 */

#define __ARCH_SPIN_LOCK_UNLOCKED { 1 }

typedef struct {
         volatile unsigned int slock;
} arch_spinlock_t;




//struct lockdep_map;





enum ethtool_phys_id_state {
       ETHTOOL_ID_INACTIVE,
       ETHTOOL_ID_ACTIVE,
       ETHTOOL_ID_ON,
       ETHTOOL_ID_OFF
 };

#endif

struct netdev_queue { struct net_device * dev ; struct Qdisc * qdisc ; unsigned long state ; struct Qdisc * qdisc_sleeping ; spinlock_t _xmit_lock __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ; int xmit_lock_owner ; unsigned long trans_start ; unsigned long tx_bytes ; unsigned long tx_packets ; unsigned long tx_dropped ; } __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;

enum netdev_tx { NETDEV_TX_OK = 0 , NETDEV_TX_BUSY , NETDEV_TX_LOCKED = - 1 , } ;
/*--netdev_tx--*/
typedef enum netdev_tx netdev_tx_t ;
typedef signed long long s64 ;
union ktime { s64 tv64 ; } ;
/*--ktime--*/
typedef union ktime ktime_t ;

typedef unsigned short __u16 ;
typedef __u16 __be16 ;

typedef unsigned char __u8 ;

typedef unsigned int sk_buff_data_t ;

struct sk_buff {
    struct sk_buff * next ;
    struct sk_buff * prev ;
    struct sock * sk ;
    ktime_t tstamp ;
    struct net_device * dev ;
    unsigned long _skb_dst ;
    char cb [ 48 ] ;
    unsigned int len , data_len ;
    __u16 mac_len , hdr_len ;
    union { __wsum csum ; struct { __u16 csum_start ; __u16 csum_offset ; } ; } ;
    __u32 priority ;

    int flags1_begin [ 0 ] ;
    __u8 local_df : 1 , cloned : 1 , ip_summed : 2 , nohdr : 1 , nfctinfo : 3 ;
    __u8 pkt_type : 3 , fclone : 2 , ipvs_property : 1 , peeked : 1 , nf_trace : 1 ;
    __be16 protocol : 16 ;
    int flags1_end [ 0 ] ; ;


    void ( * destructor ) ( struct sk_buff * skb ) ;
    int iif ; __u16 tc_index ; __u16 tc_verd ; int flags2_begin [ 0 ] ; ;
    __u16 queue_mapping : 16 ;
    int flags2_end [ 0 ] ; ;
    __u32 secmark ; __u32 mark ; __u16 vlan_tci ; sk_buff_data_t transport_header ; sk_buff_data_t network_header ;
    sk_buff_data_t mac_header ; sk_buff_data_t tail ; sk_buff_data_t end ; unsigned char * head , * data ; unsigned int truesize ;
    atomic_t users ; } ;

struct net_device_ops {
    int (*ndo_init ) ( struct net_device * dev ) ;
    void (*ndo_uninit ) ( struct net_device * dev ) ;
    int (*ndo_open ) ( struct net_device * dev ) ;
    int (*ndo_stop ) ( struct net_device * dev ) ;
    netdev_tx_t (*ndo_start_xmit ) ( struct sk_buff * skb , struct net_device * dev ) ;
    u16 (*ndo_select_queue ) ( struct net_device * dev , struct sk_buff * skb ) ;
    void (*ndo_change_rx_flags ) ( struct net_device * dev , int flags ) ;
    void (*ndo_set_rx_mode ) ( struct net_device * dev ) ;
    void (*ndo_set_multicast_list ) ( struct net_device * dev ) ;
    int (*ndo_set_mac_address ) ( struct net_device * dev , void * addr ) ;
    int (*ndo_validate_addr ) ( struct net_device * dev ) ;
    int (*ndo_do_ioctl ) ( struct net_device * dev , struct ifreq * ifr , int cmd );
    int (*ndo_set_config ) ( struct net_device * dev , struct ifmap * map );
    int (*ndo_change_mtu ) ( struct net_device * dev , int new_mtu );
    int (*ndo_neigh_setup ) ( struct net_device * dev , struct neigh_parms * );
    void (*ndo_tx_timeout ) ( struct net_device * dev );
    struct net_device_stats * ( * ndo_get_stats ) ( struct net_device * dev );
    void (*ndo_vlan_rx_register ) ( struct net_device * dev , struct vlan_group * grp );
    void (*ndo_vlan_rx_add_vid ) ( struct net_device * dev , unsigned short vid );
    void (*ndo_vlan_rx_kill_vid ) ( struct net_device * dev , unsigned short vid );
    void (*ndo_poll_controller ) ( struct net_device * dev );
    int (*ndo_fcoe_enable ) ( struct net_device * dev );
    int (*ndo_fcoe_disable ) ( struct net_device * dev );
    int (*ndo_fcoe_ddp_setup ) ( struct net_device * dev , u16 xid , struct scatterlist * sgl , unsigned int sgc );
    int (*ndo_fcoe_ddp_done ) ( struct net_device * dev , u16 xid );
};

struct ethtool_ops {
    int ( * get_settings ) ( struct net_device * , struct ethtool_cmd * ) ;
    int ( * set_settings ) ( struct net_device * , struct ethtool_cmd * ) ;
    void ( * get_drvinfo ) ( struct net_device * , struct ethtool_drvinfo * ) ;
    int ( * get_regs_len ) ( struct net_device * ) ;
    void ( * get_regs ) ( struct net_device * , struct ethtool_regs * , void * ) ;
    void ( * get_wol ) ( struct net_device * , struct ethtool_wolinfo * ) ;
    int ( * set_wol ) ( struct net_device * , struct ethtool_wolinfo * ) ;
    u32 ( * get_msglevel ) ( struct net_device * ) ;
    void ( * set_msglevel ) ( struct net_device * , u32 ) ;
    int ( * nway_reset ) ( struct net_device * ) ;
    u32 ( * get_link ) ( struct net_device * ) ;
    int ( * get_eeprom_len ) ( struct net_device * ) ;
    int ( * get_eeprom ) ( struct net_device * , struct ethtool_eeprom * , u8 * ) ;
    int ( * set_eeprom ) ( struct net_device * , struct ethtool_eeprom * , u8 * ) ;
    int ( * get_coalesce ) ( struct net_device * , struct ethtool_coalesce * ) ;
    int ( * set_coalesce ) ( struct net_device * , struct ethtool_coalesce * ) ;
    void ( * get_ringparam ) ( struct net_device * , struct ethtool_ringparam * ) ;
    int ( * set_ringparam ) ( struct net_device * , struct ethtool_ringparam * ) ;
    void ( * get_pauseparam ) ( struct net_device * , struct ethtool_pauseparam * ) ;
    int ( * set_pauseparam ) ( struct net_device * , struct ethtool_pauseparam * ) ;
    u32 ( * get_rx_csum ) ( struct net_device * ) ;
    int ( * set_rx_csum ) ( struct net_device * , u32 ) ;
    u32 ( * get_tx_csum ) ( struct net_device * ) ;
    int ( * set_tx_csum ) ( struct net_device * , u32 ) ;
    u32 ( * get_sg ) ( struct net_device * ) ;
    int ( * set_sg ) ( struct net_device * , u32 ) ;
    u32 ( * get_tso ) ( struct net_device * ) ;
    int ( * set_tso ) ( struct net_device * , u32 ) ;
    void ( * self_test ) ( struct net_device * , struct ethtool_test * , u64 * ) ;
    void ( * get_strings ) ( struct net_device * , u32 stringset , u8 * ) ;
    int ( * phys_id ) ( struct net_device * , u32 ) ;
    void ( * get_ethtool_stats ) ( struct net_device * , struct ethtool_stats * , u64 * ) ;
    int ( * begin ) ( struct net_device * ) ;
    void ( * complete ) ( struct net_device * ) ;
    u32 ( * get_ufo ) ( struct net_device * ) ;
    int ( * set_ufo ) ( struct net_device * , u32 ) ;
    u32 ( * get_flags ) ( struct net_device * ) ;
    int ( * set_flags ) ( struct net_device * , u32 ) ;
    u32 ( * get_priv_flags ) ( struct net_device * ) ;
    int ( * set_priv_flags ) ( struct net_device * , u32 ) ;
    int ( * get_sset_count ) ( struct net_device * , int ) ;
    int ( * self_test_count ) ( struct net_device * ) ;
    int ( * get_stats_count ) ( struct net_device * ) ;
    int ( * get_rxnfc ) ( struct net_device * , struct ethtool_rxnfc * , void * ) ;
    int ( * set_rxnfc ) ( struct net_device * , struct ethtool_rxnfc * ) ;
    int ( * flash_device ) ( struct net_device * , struct ethtool_flash * ) ;
    int ( * reset ) ( struct net_device * , u32 * ) ;
};

struct net_device {
    char name [16];
    struct hlist_node name_hlist ;
    char * ifalias ;
    unsigned long mem_end ;
    unsigned long mem_start ;
    unsigned long base_addr ;
    unsigned int irq ;
    unsigned char if_port ;
    unsigned char dma ;
    unsigned long state ;
    struct list_head dev_list ;
    struct list_head napi_list ;
    unsigned long features ;
    int ifindex ;
    int iflink ;
    struct net_device_stats stats ;
    const struct iw_handler_def * wireless_handlers ;
    struct iw_public_data * wireless_data ;
    const struct net_device_ops * netdev_ops ;
    const struct ethtool_ops * ethtool_ops ;
    const struct header_ops * header_ops ;
    unsigned int flags ;
    unsigned short gflags ;
    unsigned short priv_flags ;
    unsigned short padded ;
    unsigned char operstate ;
    unsigned char link_mode ;
    unsigned mtu ;
    unsigned short type ;
    unsigned short hard_header_len ;
    unsigned short needed_headroom ;
    unsigned short needed_tailroom ;
    struct net_device * master ;
    unsigned char perm_addr [32];
    unsigned char addr_len;
    unsigned short dev_id ;
    struct netdev_hw_addr_list uc ;
    int uc_promisc ;
    spinlock_t addr_list_lock ;
    struct dev_addr_list * mc_list ;
    int mc_count ;
    unsigned int promiscuity ;
    unsigned int allmulti ;
    void * dsa_ptr ;
    void * atalk_ptr ;
    void * ip_ptr ;
    void * dn_ptr ;
    void * ip6_ptr ;
    void * ec_ptr ;
    void * ax25_ptr ;
    struct wireless_dev * ieee80211_ptr ;
    unsigned long last_rx ;
    unsigned char * dev_addr ;
    struct netdev_hw_addr_list dev_addrs ;
    unsigned char broadcast [ 32 ] ;
    struct netdev_queue rx_queue ;
    struct netdev_queue * _tx __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;
    unsigned int num_tx_queues ;
    unsigned int real_num_tx_queues ;
    struct Qdisc * qdisc ;
    unsigned long tx_queue_len ;
    spinlock_t tx_global_lock ;
    unsigned long trans_start ;
    int watchdog_timeo ;
    struct timer_list watchdog_timer ;
    atomic_t refcnt __attribute__ ( ( __aligned__ ( ( 1 << ( 6 ) ) ) ) ) ;
    struct list_head todo_list ;
    struct hlist_node index_hlist ;
    struct net_device * link_watch_next ;
    enum {
        NETREG_UNINITIALIZED = 0 ,
        NETREG_REGISTERED ,
        NETREG_UNREGISTERING ,
        NETREG_UNREGISTERED ,
        NETREG_RELEASED ,
        NETREG_DUMMY
    } reg_state ;
    void ( * destructor ) ( struct net_device * dev ) ;
    struct netpoll_info * npinfo ;
    void * ml_priv ;
    struct net_bridge_port * br_port ;
    struct macvlan_port * macvlan_port ;
    struct garp_port * garp_port ;
    struct device dev ;
    const struct attribute_group * sysfs_groups [ 3 ] ;
    const struct rtnl_link_ops * rtnl_link_ops ;
    unsigned long vlan_features ;
    unsigned int gso_max_size ;
    struct dcbnl_rtnl_ops * dcbnl_ops ;
    unsigned int fcoe_ddp_xid ;
};

struct dev_pm_ops {
    int ( * prepare ) ( struct device * dev ) ;
    void ( * complete ) ( struct device * dev ) ;
    int ( * suspend ) ( struct device * dev ) ;
    int ( * resume ) ( struct device * dev ) ;
    int ( * freeze ) ( struct device * dev ) ;
    int ( * thaw ) ( struct device * dev ) ;
    int ( * poweroff ) ( struct device * dev ) ;
    int ( * restore ) ( struct device * dev ) ;
    int ( * suspend_noirq ) ( struct device * dev ) ;
    int ( * resume_noirq ) ( struct device * dev ) ;
    int ( * freeze_noirq ) ( struct device * dev ) ;
    int ( * thaw_noirq ) ( struct device * dev ) ;
    int ( * poweroff_noirq ) ( struct device * dev ) ;
    int ( * restore_noirq ) ( struct device * dev ) ;
    int ( * runtime_suspend ) ( struct device * dev ) ;
    int ( * runtime_resume ) ( struct device * dev ) ;
    int ( * runtime_idle ) ( struct device * dev ) ;
};


struct device_driver {
    const char *name;
    struct bus_type *bus;
    struct module *owner;
    const char *mod_name;
    bool suppress_bind_attrs;
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    void (*shutdown)(struct device *dev);
    int (*suspend)(struct device *dev, pm_message_t state);
    int (*resume)(struct device *dev);
    const struct attribute_group **groups;
    const struct dev_pm_ops *pm ;
    struct driver_private *p;
};

typedef int pci_power_t ;
/* 1......pci_power_t......*//* 3......pci_power_t......*//* 4......pci_power_t......*//*--pci_power_t--*/
typedef unsigned int pci_channel_state_t ;

enum {
    PCI_STD_RESOURCES ,
    PCI_STD_RESOURCE_END = 5 ,
    PCI_ROM_RESOURCE ,
    PCI_IOV_RESOURCES ,
    PCI_IOV_RESOURCE_END = PCI_IOV_RESOURCES + 6 - 1 ,
    PCI_BRIDGE_RESOURCES ,
    PCI_BRIDGE_RESOURCE_END = PCI_BRIDGE_RESOURCES + 4 - 1 ,
    PCI_NUM_RESOURCES ,
    DEVICE_COUNT_RESOURCE
} ;

struct hlist_head { struct hlist_node * first ; } ;

typedef u64 phys_addr_t ;

typedef phys_addr_t resource_size_t ;

struct resource {
    resource_size_t start ;
    resource_size_t end ;
    const char * name ;
    unsigned long flags ;
    struct resource * parent , * sibling , * child ;
};

struct device_dma_parameters { unsigned int max_segment_size ; unsigned long segment_boundary_mask ; } ;

typedef unsigned short pci_dev_flags_t ;

struct pci_dev {
    struct list_head bus_list ;
    struct pci_bus * bus ;
    struct pci_bus * subordinate ;
    void * sysdata ;
    struct proc_dir_entry * procent ;
    struct pci_slot * slot ;
    unsigned int devfn ;
    unsigned short vendor ;
    unsigned short device ;
    unsigned short subsystem_vendor ;
    unsigned short subsystem_device ;
    unsigned int class_ele ;
    u8 revision ;
    u8 hdr_type ;
    u8 pcie_type ;
    u8 rom_base_reg ;
    u8 pin ;
    struct pci_driver * driver ;
    u64 dma_mask ;
    struct device_dma_parameters dma_parms ;
    pci_power_t current_state ;
    int pm_cap ;
    unsigned int pme_support : 5 ;
    unsigned int d1_support : 1 ;
    unsigned int d2_support : 1 ;
    unsigned int no_d1d2 : 1 ;
    unsigned int wakeup_prepared : 1 ;
    pci_channel_state_t error_state ;
    struct device dev ;
    int cfg_size ;
    unsigned int irq ;
    struct resource resource [ DEVICE_COUNT_RESOURCE ] ;
    unsigned int transparent : 1 ;
    unsigned int multifunction : 1 ;
    unsigned int is_added : 1 ;
    unsigned int is_busmaster : 1 ;
    unsigned int no_msi : 1 ;
    unsigned int block_ucfg_access : 1 ;
    unsigned int broken_parity_status : 1 ;
    unsigned int irq_reroute_variant : 2 ;
    unsigned int msi_enabled : 1 ;
    unsigned int msix_enabled : 1 ;
    unsigned int ari_enabled : 1 ;
    unsigned int is_managed : 1 ;
    unsigned int is_pcie : 1 ;
    unsigned int needs_freset : 1 ;
    unsigned int state_saved : 1 ;
    unsigned int is_physfn : 1 ;
    unsigned int is_virtfn : 1 ;
    unsigned int reset_fn : 1 ;
    unsigned int is_hotplug_bridge : 1 ;
    pci_dev_flags_t dev_flags ;
    atomic_t enable_cnt ;
    u32 saved_config_space [ 16 ] ;
    struct hlist_head saved_cap_space ;
    struct bin_attribute * rom_attr ;
    int rom_attr_enabled ;
    struct bin_attribute * res_attr [ DEVICE_COUNT_RESOURCE ] ;
    struct bin_attribute * res_attr_wc [ DEVICE_COUNT_RESOURCE ] ;
    struct list_head msi_list ;
    struct pci_vpd * vpd ;
    union {
        struct pci_sriov * sriov ;
        struct pci_dev * physfn ;
    } ;
    struct pci_ats * ats ;
};

struct pci_driver {
    struct list_head node ;
    char * name ;
    const struct pci_device_id * id_table ;
    int ( * probe ) ( struct pci_dev * dev , const struct pci_device_id * id ) ;
    void ( * remove ) ( struct pci_dev * dev ) ;
    int ( * suspend ) ( struct pci_dev * dev , pm_message_t state ) ;
    int ( * suspend_late ) ( struct pci_dev * dev , pm_message_t state ) ;
    int ( * resume_early ) ( struct pci_dev * dev ) ;
    int ( * resume ) ( struct pci_dev * dev ) ;
    void ( * shutdown ) ( struct pci_dev * dev ) ;
    struct pci_error_handlers * err_handler ;
    struct device_driver driver ;
    struct pci_dynids dynids ;
};



enum pci_channel_state {
    pci_channel_io_normal = ( pci_channel_state_t ) 1,
    pci_channel_io_frozen = ( pci_channel_state_t ) 2,
    pci_channel_io_perm_failure = ( pci_channel_state_t ) 3,
};

struct pci_error_handlers {
    pci_ers_result_t ( * error_detected ) ( struct pci_dev * dev, enum pci_channel_state error );
    pci_ers_result_t ( * mmio_enabled ) ( struct pci_dev * dev );
    pci_ers_result_t ( * link_reset ) ( struct pci_dev * dev );
    pci_ers_result_t ( * slot_reset ) ( struct pci_dev * dev );
    void ( * resume ) ( struct pci_dev * dev );
};

enum phy_state {
    PHY_DOWN = 0,
    PHY_STARTING ,
    PHY_READY,
    PHY_PENDING,
    PHY_UP,
    PHY_AN,
    PHY_RUNNING,
    PHY_NOLINK,
    PHY_FORCING,
    PHY_CHANGELINK,
    PHY_HALTED,
    PHY_RESUMING
};





struct delayed_work {
    struct work_struct work;
    struct timer_list timer;
};

struct mutex {
    atomic_t count ;
    spinlock_t wait_lock ;
    struct list_head wait_list ;
    struct thread_info * owner ;
};



struct mii_bus {
    const char * name;
    char id [(20-3)];
    void * priv ;
    int ( * read ) ( struct mii_bus * bus , int phy_id , int regnum ) ;
    int ( * write ) ( struct mii_bus * bus , int phy_id , int regnum , u16 val ) ;
    int ( * reset ) ( struct mii_bus * bus ) ;
    struct mutex mdio_lock ;
    struct device * parent ;
    enum {
        MDIOBUS_ALLOCATED = 1 ,
        MDIOBUS_REGISTERED ,
        MDIOBUS_UNREGISTERED ,
        MDIOBUS_RELEASED ,
    } state ;
    struct device dev ;
    struct phy_device * phy_map[32] ;
    u32 phy_mask ;
    int * irq ;
};

struct phy_driver {
    u32 phy_id ;
    char * name ;
    unsigned int phy_id_mask ;
    u32 features ;
    u32 flags ;
    int ( * config_init ) ( struct phy_device * phydev ) ;
    int ( * probe ) ( struct phy_device * phydev ) ;
    int ( * suspend ) ( struct phy_device * phydev ) ;
    int ( * resume ) ( struct phy_device * phydev ) ;
    int ( * config_aneg ) ( struct phy_device * phydev ) ;
    int ( * read_status ) ( struct phy_device * phydev ) ;
    int ( * ack_interrupt ) ( struct phy_device * phydev ) ;
    int ( * config_intr ) ( struct phy_device * phydev ) ;
    int ( * did_interrupt ) ( struct phy_device * phydev ) ;
    void ( * remove ) ( struct phy_device * phydev ) ;
    struct device_driver driver ;
};


struct phy_device {
    struct phy_driver * drv;
    struct mii_bus * bus;
    struct device dev;
    u32 phy_id;
    enum phy_state state;
    u32 dev_flags;
    phy_interface_t interface;
    int addr;
    int speed;
    int duplex;
    int pause;
    int asym_pause;
    int link;
    u32 interrupts;
    u32 supported;
    u32 advertising;
    int autoneg;
    int link_timeout;
    int irq ;
    void * priv ;
    struct work_struct phy_queue ;
    struct delayed_work state_queue ;
    atomic_t irq_disable ;
    struct mutex lock ;
    struct net_device * attached_dev ;
    void ( * adjust_link ) ( struct net_device * dev ) ;
    void ( * adjust_state ) ( struct net_device * dev ) ;
};


struct napi_struct {
    struct list_head poll_list;
    unsigned long state;
    int weight;
    int ( * poll ) ( struct napi_struct * , int );
    spinlock_t poll_lock;
    int poll_owner;
    unsigned int gro_count;
    struct net_device * dev;
    struct list_head dev_list;
    struct sk_buff * gro_list;
    struct sk_buff * skb;
};

enum irqreturn { IRQ_NONE , IRQ_HANDLED , IRQ_WAKE_THREAD , } ;

typedef enum irqreturn irqreturn_t ;

//typedef enum irqreturn irqreturn_t;

typedef int (*poll)(struct napi_struct *, int);
typedef irqreturn_t (*irq_handler_t)(int, void *);
typedef void (*handler)(struct net_device *);


/// kernel-exported function prototypes
extern struct page * alloc_pages_current ( gfp_t gfp , unsigned order );
extern int __pci_register_driver(struct pci_driver *, struct module *, const char *mod_name);
extern int kallsyms_on_each_symbol(int (*fn)(void *, const char *, struct module *, unsigned long), void *data);
extern int pci_enable_device(struct pci_dev *dev);
extern int pci_request_regions(struct pci_dev *pdev, const char *res_name);
extern void pci_set_master(struct pci_dev *dev);
extern void netif_napi_add(struct net_device *dev, struct napi_struct *napi,poll p1, int weight);
extern int pci_set_dma_mask(struct pci_dev *dev, unsigned long mask);
extern int pci_save_state(struct pci_dev *dev);
extern pci_power_t pci_target_state(struct pci_dev *dev);
extern void phy_disconnect(struct phy_device *phydev);
extern struct sk_buff *skb_gso_segment(struct sk_buff *skb, int features);
extern void phy_stop(struct phy_device *phydev);
extern void skb_dma_unmap(struct device *dev, struct sk_buff *skb, enum dma_data_direction dir);
extern int ethtool_op_set_tx_csum(struct net_device *dev, u32 data);
extern int pci_find_capability(struct pci_dev *dev, int cap);
extern int pci_request_regions(struct pci_dev *pdev, const char *res_name);
extern void netif_device_attach(struct net_device *dev);
extern struct pci_dev* pci_get_device(unsigned int vendor, unsigned int device, struct pci_dev *from);
extern struct phy_device * phy_connect(struct net_device *dev, const char *bus_id, void (*handler)(struct net_device *), u32 flags, phy_interface_t interface);
extern int phy_start_aneg(struct phy_device *phydev);
extern int pci_dev_present(const struct pci_device_id *ids);
extern int phy_ethtool_sset(struct phy_device *phydev, struct ethtool_cmd *cmd);
extern bool pci_pme_capable(struct pci_dev *dev, pci_power_t state);
extern int pci_bus_write_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 val);
extern int pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 *val);
extern int pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 *val);
extern int pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 *val);
extern int pci_bus_write_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 val);
extern struct pci_dev * pci_get_slot(struct pci_bus *bus, unsigned int devfn);
extern void netif_device_detach(struct net_device *dev);
extern void pci_dev_put(struct pci_dev *dev);
extern void dev_set_drvdata(struct device *dev, void *data);
extern int ethtool_op_set_tx_csum(struct net_device *dev, u32 data);
extern int pcie_set_readrq(struct pci_dev *dev, int rq);
extern struct pci_dev *pci_get_slot(struct pci_bus *bus, unsigned int devfn);
extern void napi_complete(struct napi_struct *n);
extern int ethtool_op_set_tx_ipv6_csum(struct net_device *dev, u32 data);
extern void release_firmware(const struct firmware *fw);
extern void *dev_get_drvdata(const struct device *dev);
extern int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask);
extern unsigned char *skb_put(struct sk_buff *skb, unsigned int len);
extern void pci_disable_device(struct pci_dev *dev);
extern void pci_disable_msix(struct pci_dev *dev);
extern void msi_remove_pci_irq_vectors(struct pci_dev *dev);
extern void pci_restore_msi_state(struct pci_dev *dev);
extern int param_get_int(char *buffer, struct kernel_param *kp);
extern void netif_carrier_on(struct net_device *dev);
extern void netif_carrier_off(struct net_device *dev);
extern struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t priority);
extern int schedule_work(struct work_struct *work);
extern int cancel_work_sync(struct work_struct *work);
extern int phy_start_aneg(struct phy_device *phydev);
extern void pci_release_regions(struct pci_dev *);
extern void init_timer_key(struct timer_list *timer,   const char *name, struct lock_class_key *key);
extern int pci_enable_wake(struct pci_dev *dev, pci_power_t state, bool enable);
extern int param_set_int(const char *val, struct kernel_param *kp);
extern void mdiobus_unregister(struct mii_bus *bus);
extern struct sk_buff *__netdev_alloc_skb(struct net_device *dev,  unsigned int length, gfp_t gfp_mask);
extern int vlan_gro_receive(struct napi_struct *napi, struct vlan_group *grp, unsigned int vlan_tci, struct sk_buff *skb);
extern void consume_skb(struct sk_buff *skb);
extern void __netif_schedule(struct Qdisc *q);
extern int pci_enable_msi_block(struct pci_dev *dev, unsigned int nvec);
extern int phy_ethtool_sset(struct phy_device *phydev, struct ethtool_cmd *cmd);
extern int phy_mii_ioctl(struct phy_device *phydev,struct mii_ioctl_data *mii_data, int cmd);
extern int register_netdev(struct net_device *dev);
extern void unregister_netdev(struct net_device *dev);
extern int request_firmware(const struct firmware **fw, const char *name,   struct device *device);
extern void pci_disable_msi(struct pci_dev *dev);
extern int eth_validate_addr(struct net_device *dev);
extern int pci_set_power_state(struct pci_dev *dev, pci_power_t state);
extern void  *pci_ioremap_bar(struct pci_dev *pdev, int bar);
extern void pci_unregister_driver(struct pci_driver *dev);
extern int  pskb_expand_head(struct sk_buff *skb, int nhead, int ntail, gfp_t gfp_mask);
extern u16  eth_type_trans(struct sk_buff *skb, struct net_device *dev);
extern void __napi_schedule(struct napi_struct *n);
extern  int ethtool_op_set_sg(struct net_device *dev, u32 data);
extern int  mdiobus_register(struct mii_bus *bus);
extern struct sk_buff *skb_copy_expand(const struct sk_buff *skb,int newheadroom, int newtailroom, gfp_t priority);
extern void dev_kfree_skb_any(struct sk_buff *skb);
extern int  mod_timer(struct timer_list *timer, unsigned long expires);
extern int  dev_close(struct net_device *dev);
extern void free_netdev(struct net_device *dev);
extern void mdiobus_free(struct mii_bus *bus);
extern u32  ethtool_op_get_link(struct net_device *dev);
extern int  pci_restore_state(struct pci_dev *dev);
extern int  pci_enable_msix(struct pci_dev *dev, struct msix_entry *entries, int nvec);
extern int  try_to_del_timer_sync(struct timer_list *timer);
extern int  skb_dma_map(struct device *dev, struct sk_buff *skb, enum dma_data_direction dir);
extern int  ethtool_op_set_tso(struct net_device *dev, u32 data);
extern void phy_start(struct phy_device *phydev);
extern int  pci_request_selected_regions(struct pci_dev *, int, const char *);
extern void pci_release_selected_regions(struct pci_dev *, int);
extern int  pcix_set_mmrbc(struct pci_dev *dev, int mmrbc);
extern int  param_set_invbool(const char *val, struct kernel_param *kp);
extern int  pci_wake_from_d3(struct pci_dev *dev, bool enable);
extern int  pci_enable_device_mem(struct pci_dev *dev);
extern int  ___pskb_trim(struct sk_buff *skb, unsigned int len);
extern void pci_clear_mwi(struct pci_dev *dev);
extern int  dev_open(struct net_device *dev);
extern int  dev_close(struct net_device *dev);
extern int  pci_prepare_to_sleep(struct pci_dev *dev);
extern int  pci_back_from_sleep(struct pci_dev *dev);
extern int  param_set_uint(const char *val, struct kernel_param *kp);
extern int  param_array_set(const char *val, struct kernel_param *kp);
extern int  param_array_get(char *buffer, struct kernel_param *kp);
extern const char *dev_driver_string(const struct device *dev);
extern int  param_get_uint(char *buffer, struct kernel_param *kp);
extern int  pci_select_bars(struct pci_dev *dev, unsigned long flags);
extern int  pci_set_mwi(struct pci_dev *dev);
extern int  pcix_get_mmrbc(struct pci_dev *dev);
extern int  pcix_set_mmrbc(struct pci_dev *dev, int mmrbc);
extern int  netif_receive_skb(struct sk_buff *skb);
extern void free_netdev(struct net_device *dev);
extern struct net_device *alloc_etherdev_mq(int sizeof_priv, unsigned int queue_count);
extern int  del_timer_sync(struct timer_list *timer);
extern unsigned char *__pskb_pull_tail(struct sk_buff *skb, int delta);
extern void skb_trim(struct sk_buff *skb, unsigned int len);
extern u16  csum_ipv6_magic(const struct in6_addr *saddr, const struct in6_addr *daddr, __u32 len, unsigned short proto, __wsum sum);
extern int request_threaded_irq(unsigned int irq, irq_handler_t handler, irq_handler_t thread_fn, unsigned long irqflags, const char *devname, void *dev_id);
extern int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,const char *name, void *dev);
extern void add_timer(struct timer_list *timer);
extern void free_irq(unsigned int, void *);



enum hrtimer_mode { HRTIMER_MODE_ABS = 0x0 , HRTIMER_MODE_REL = 0x1 , HRTIMER_MODE_PINNED = 0x02 , HRTIMER_MODE_ABS_PINNED = 0x02 , HRTIMER_MODE_REL_PINNED = 0x03 , } ;
/*--hrtimer_mode--*/
enum hrtimer_restart { HRTIMER_NORESTART , HRTIMER_RESTART , } ;
/*--hrtimer_restart--*/

void add_timer_on ( struct timer_list * timer , int cpu ) ;
int del_timer ( struct timer_list * timer ) ;
void hrtimer_init_sleeper ( struct hrtimer_sleeper * sl , struct task_struct * task ) ;
void sk_reset_timer ( struct sock * sk , struct timer_list * timer , unsigned long expires ) ;
void sk_stop_timer ( struct sock * sk , struct timer_list * timer ) ;
int hrtimer_start_range_ns ( struct hrtimer * timer , ktime_t tim , unsigned long delta_ns , const enum hrtimer_mode mode ) ;
int hrtimer_start ( struct hrtimer * timer , ktime_t tim , const enum hrtimer_mode mode ) ;
int hrtimer_try_to_cancel ( struct hrtimer * timer ) ;
int hrtimer_cancel ( struct hrtimer * timer ) ;
u64 hrtimer_forward ( struct hrtimer * timer , ktime_t now , ktime_t interval ) ;
int register_timer_hook ( int ( * hook ) ( struct pt_regs * ) ) ;
void unregister_timer_hook ( int ( * hook ) ( struct pt_regs * ) ) ;
int mod_timer_pending ( struct timer_list * timer , unsigned long expires ) ;
int mod_timer_pinned ( struct timer_list * timer , unsigned long expires ) ;

/****************************************************************/
extern int pci_ext_cfg_avail ( struct pci_dev * dev ) ;
extern struct pci_bus *pci_scan_bus_on_node ( int busno , struct pci_ops * ops , int node ) ;
extern struct pci_bus *pci_scan_bus_with_sysdata ( int busno ) ;
extern void x86_pci_root_bus_res_quirks ( struct pci_bus * b ) ;
extern void * pci_iomap ( struct pci_dev * dev , int bar , unsigned long maxlen ) ;
extern void pci_iounmap ( struct pci_dev * dev , void * addr ) ;
int pci_enable_pcie_error_reporting ( struct pci_dev * dev ) ;
int pci_disable_pcie_error_reporting ( struct pci_dev * dev ) ;
int pci_cleanup_aer_uncorrect_error_status ( struct pci_dev * dev ) ;
void pci_no_aer ( void ) ;
int pci_uevent ( struct device * dev , struct kobj_uevent_env * env ) ;
extern unsigned int pci_do_scan_bus ( struct pci_bus * bus ) ;
void pci_restore_msi_state ( struct pci_dev * dev ) ;
int pci_enable_msi_block ( struct pci_dev * dev , unsigned int nvec ) ;
void pci_msi_shutdown ( struct pci_dev * dev ) ;
void pci_disable_msi ( struct pci_dev * dev ) ;
int pci_msix_table_size ( struct pci_dev * dev ) ;
int pci_enable_msix ( struct pci_dev * dev , struct msix_entry * entries , int nvec ) ;
void pci_msix_shutdown ( struct pci_dev * dev ) ;
void pci_disable_msix ( struct pci_dev * dev ) ;
void msi_remove_pci_irq_vectors ( struct pci_dev * dev ) ;
void pci_no_msi ( void ) ;
int pci_msi_enabled ( void ) ;
void pci_msi_init_pci_dev ( struct pci_dev * dev ) ;
int pci_iov_init ( struct pci_dev * dev ) ;
void pci_iov_release ( struct pci_dev * dev ) ;
//int pci_iov_resource_bar ( struct pci_dev * dev , int resno , enum pci_bar_type * type ) ;
int pci_sriov_resource_alignment ( struct pci_dev * dev , int resno ) ;
void pci_restore_iov_state ( struct pci_dev * dev ) ;
int pci_iov_bus_range ( struct pci_bus * bus ) ;
int pci_enable_sriov ( struct pci_dev * dev , int nr_virtfn ) ;
void pci_disable_sriov ( struct pci_dev * dev ) ;
irqreturn_t pci_sriov_migration ( struct pci_dev * dev ) ;
int pci_enable_ats ( struct pci_dev * dev , int ps ) ;
void pci_disable_ats ( struct pci_dev * dev ) ;
int pci_ats_queue_depth ( struct pci_dev * dev ) ;
void pci_setup_cardbus ( struct pci_bus * bus ) ;
extern void pci_bus_size_bridges ( struct pci_bus * bus ) ;
extern void pci_bus_assign_resources ( const struct pci_bus * bus ) ;
void ata_pci_remove_one ( struct pci_dev * pdev ) ;
int pci_test_config_bits ( struct pci_dev * pdev , const struct pci_bits * bits ) ;
void ata_pci_device_do_suspend ( struct pci_dev * pdev , pm_message_t mesg ) ;
int ata_pci_device_do_resume ( struct pci_dev * pdev ) ;
int ata_pci_device_suspend ( struct pci_dev * pdev , pm_message_t mesg ) ;
int ata_pci_device_resume ( struct pci_dev * pdev ) ;
extern int ata_pci_bmdma_clear_simplex ( struct pci_dev * pdev ) ;
extern int ata_pci_bmdma_init ( struct ata_host * host ) ;
extern int ata_pci_sff_init_host ( struct ata_host * host ) ;
extern int ata_pci_sff_prepare_host ( struct pci_dev * pdev , const struct ata_port_info * const * ppi , struct ata_host * * r_host ) ;
extern int ata_pci_sff_activate_host ( struct ata_host * host , irq_handler_t irq_handler , struct scsi_host_template * sht ) ;
extern int ata_pci_sff_init_one ( struct pci_dev * pdev , const struct ata_port_info * const * ppi , struct scsi_host_template * sht , void * host_priv ) ;

int pci_bus_read_config_byte ( struct pci_bus * bus , unsigned int devfn , int pos , u8 * value ) ;
int pci_bus_read_config_word ( struct pci_bus * bus , unsigned int devfn , int pos , u16 * value ) ;
int pci_bus_read_config_dword ( struct pci_bus * bus , unsigned int devfn , int pos , u32 * value ) ;
int pci_bus_write_config_byte ( struct pci_bus * bus , unsigned int devfn , int pos , u8 value ) ;
int pci_bus_write_config_word ( struct pci_bus * bus , unsigned int devfn , int pos , u16 value ) ;
int pci_bus_write_config_dword ( struct pci_bus * bus , unsigned int devfn , int pos , u32 value ) ;
struct pci_ops * pci_bus_set_ops ( struct pci_bus * bus , struct pci_ops * ops ) ;
ssize_t pci_read_vpd ( struct pci_dev * dev , loff_t pos , size_t count , void * buf ) ;
ssize_t pci_write_vpd ( struct pci_dev * dev , loff_t pos , size_t count , const void * buf ) ;
int pci_user_read_config_byte ( struct pci_dev * dev , int pos , u8 * val ) ;
int pci_user_read_config_word ( struct pci_dev * dev , int pos , u16 * val ) ;
int pci_user_read_config_dword ( struct pci_dev * dev , int pos , u32 * val ) ;
int pci_user_write_config_byte ( struct pci_dev * dev , int pos , u8 val ) ;
int pci_user_write_config_word ( struct pci_dev * dev , int pos , u16 val ) ;
int pci_user_write_config_dword ( struct pci_dev * dev , int pos , u32 val ) ;
extern int pci_vpd_pci22_init ( struct pci_dev * dev ) ;
extern int pci_vpd_truncate ( struct pci_dev * dev , size_t size ) ;
extern void pci_block_user_cfg_access ( struct pci_dev * dev ) ;
extern  void pci_unblock_user_cfg_access ( struct pci_dev * dev ) ;
//extern int pci_bus_alloc_resource ( struct pci_bus * bus , struct resource * res , resource_size_t size , resource_size_t align , resource_size_t min , unsigned int type_mask , void ( * alignf ) ( void * , struct resource * , resource_size_t , resource_size_t ) , void * alignf_data ) ;
extern int pci_bus_add_device ( struct pci_dev * dev ) ;
extern int pci_bus_add_child ( struct pci_bus * bus ) ;
extern void pci_bus_add_devices ( const struct pci_bus * bus ) ;
extern void pci_enable_bridges ( struct pci_bus * bus ) ;
extern void pci_walk_bus ( struct pci_bus * top , int ( * cb ) ( struct pci_dev * , void * ) , void * userdata ) ;
extern int no_pci_devices ( void ) ;
extern struct pci_bus *pci_add_new_bus ( struct pci_bus * parent , struct pci_dev * dev , int busnr ) ;
extern int pci_scan_bridge ( struct pci_bus * bus , struct pci_dev * dev , int max , int pass ) ;
extern struct pci_dev * alloc_pci_dev ( void ) ;
extern struct pci_dev *pci_scan_single_device ( struct pci_bus * bus , int devfn ) ;
int pci_scan_slot ( struct pci_bus * bus , int devfn ) ;
extern unsigned int pci_scan_child_bus ( struct pci_bus * bus ) ;
extern struct pci_bus * pci_create_bus ( struct device * parent , int bus , struct pci_ops * ops , void * sysdata ) ;
extern struct pci_bus *pci_scan_bus_parented ( struct device * parent , int bus , struct pci_ops * ops , void * sysdata ) ;
extern unsigned int pci_rescan_bus ( struct pci_bus * bus ) ;
extern void pci_sort_breadthfirst ( void ) ;
void pci_remove_bus ( struct pci_bus * pci_bus ) ;
void pci_remove_bus_device ( struct pci_dev * dev ) ;
void pci_remove_behind_bridge ( struct pci_dev * dev ) ;
void pci_stop_bus_device ( struct pci_dev * dev ) ;
unsigned char pci_bus_max_busnr ( struct pci_bus * bus ) ;
void * pci_ioremap_bar ( struct pci_dev * pdev , int bar ) ;
int pci_find_next_capability ( struct pci_dev * dev , u8 pos , int cap ) ;
int pci_find_capability ( struct pci_dev * dev , int cap ) ;
int pci_bus_find_capability ( struct pci_bus * bus , unsigned int devfn , int cap ) ;
int pci_find_ext_capability ( struct pci_dev * dev , int cap ) ;
int pci_find_next_ht_capability ( struct pci_dev * dev , int pos , int ht_cap ) ;
int pci_find_ht_capability ( struct pci_dev * dev , int ht_cap ) ;
struct resource * pci_find_parent_resource ( const struct pci_dev * dev , struct resource * res ) ;
int pci_set_platform_pm ( struct pci_platform_pm_ops * ops ) ;
void pci_update_current_state ( struct pci_dev * dev , pci_power_t state ) ;
int __pci_complete_power_transition ( struct pci_dev * dev , pci_power_t state ) ;
int pci_set_power_state ( struct pci_dev * dev , pci_power_t state ) ;
pci_power_t pci_choose_state ( struct pci_dev * dev , pm_message_t state ) ;
int pci_save_state ( struct pci_dev * dev ) ;
int pci_restore_state ( struct pci_dev * dev ) ;
int pci_reenable_device ( struct pci_dev * dev ) ;
int pci_enable_device_io ( struct pci_dev * dev ) ;
int pci_enable_device_mem ( struct pci_dev * dev ) ;
int pci_enable_device ( struct pci_dev * dev ) ;
int pcim_enable_device ( struct pci_dev * pdev ) ;
void pcim_pin_device ( struct pci_dev * pdev ) ;
void __attribute__ ( ( weak ) ) pcibios_disable_device ( struct pci_dev * dev ) ;
void pci_disable_enabled_device ( struct pci_dev * dev ) ;
void pci_disable_device ( struct pci_dev * dev ) ;
//int __attribute__ ( ( weak ) ) pcibios_set_pcie_reset_state ( struct pci_dev * dev , enum pcie_reset_state state ) ;
//int pci_set_pcie_reset_state ( struct pci_dev * dev , enum pcie_reset_state state ) ;
bool pci_pme_capable ( struct pci_dev * dev , pci_power_t state ) ;
void pci_pme_active ( struct pci_dev * dev , bool enable ) ;
int pci_enable_wake ( struct pci_dev * dev , pci_power_t state , bool enable ) ;
int pci_wake_from_d3 ( struct pci_dev * dev , bool enable ) ;
pci_power_t pci_target_state ( struct pci_dev * dev ) ;
int pci_prepare_to_sleep ( struct pci_dev * dev ) ;
int pci_back_from_sleep ( struct pci_dev * dev ) ;
void pci_pm_init ( struct pci_dev * dev ) ;
void platform_pci_wakeup_init ( struct pci_dev * dev ) ;
void pci_allocate_cap_save_buffers ( struct pci_dev * dev ) ;
void pci_enable_ari ( struct pci_dev * dev ) ;
u8 pci_swizzle_interrupt_pin ( struct pci_dev * dev , u8 pin ) ;
int pci_get_interrupt_pin ( struct pci_dev * dev , struct pci_dev * * bridge ) ;
u8 pci_common_swizzle ( struct pci_dev * dev , u8 * pinp ) ;
void pci_release_region ( struct pci_dev * pdev , int bar ) ;
int pci_request_region ( struct pci_dev * pdev , int bar , const char * res_name ) ;
int pci_request_region_exclusive ( struct pci_dev * pdev , int bar , const char * res_name ) ;
void pci_release_selected_regions ( struct pci_dev * pdev , int bars ) ;
int __pci_request_selected_regions ( struct pci_dev * pdev , int bars , const char * res_name , int excl ) ;
int pci_request_selected_regions ( struct pci_dev * pdev , int bars , const char * res_name ) ;
int pci_request_selected_regions_exclusive ( struct pci_dev * pdev , int bars , const char * res_name ) ;
void pci_release_regions ( struct pci_dev * pdev ) ;
int pci_request_regions ( struct pci_dev * pdev , const char * res_name ) ;
int pci_request_regions_exclusive ( struct pci_dev * pdev , const char * res_name ) ;
void pci_set_master ( struct pci_dev * dev ) ;
void pci_clear_master ( struct pci_dev * dev ) ;
int pci_set_mwi ( struct pci_dev * dev ) ;
int pci_try_set_mwi ( struct pci_dev * dev ) ;
void pci_clear_mwi ( struct pci_dev * dev ) ;
void pci_intx ( struct pci_dev * pdev , int enable ) ;
void pci_msi_off ( struct pci_dev * dev ) ;
extern int pci_set_dma_max_seg_size ( struct pci_dev * dev , unsigned int size ) ;
extern int pci_set_dma_seg_boundary ( struct pci_dev * dev , unsigned long mask ) ;
extern int __pci_reset_function ( struct pci_dev * dev ) ;
extern int pci_reset_function ( struct pci_dev * dev ) ;
extern int pcix_get_max_mmrbc ( struct pci_dev * dev ) ;
extern int pcie_get_readrq ( struct pci_dev * dev ) ;

enum pci_fixup_pass { pci_fixup_early , pci_fixup_header , pci_fixup_final , pci_fixup_enable , pci_fixup_resume , pci_fixup_suspend , pci_fixup_resume_early , } ;
extern void pci_fixup_device ( enum pci_fixup_pass pass , struct pci_dev * dev ) ;
/*--pci_dynid--*/
int pci_add_dynid ( struct pci_driver * drv , unsigned int vendor , unsigned int device , unsigned int subvendor , unsigned int subdevice , unsigned int class_ele , unsigned int class_mask , unsigned long driver_data ) ;
const struct pci_device_id * pci_match_id ( const struct pci_device_id * ids , struct pci_dev * dev ) ;
struct drv_dev_and_id { struct pci_driver * drv ; struct pci_dev * dev ; const struct pci_device_id * id ; } ;
int __pci_register_driver ( struct pci_driver * drv , struct module * owner , const char * mod_name ) ;
void pci_unregister_driver ( struct pci_driver * drv ) ;
struct pci_driver * pci_dev_driver ( const struct pci_dev * dev ) ;
struct pci_dev * pci_dev_get ( struct pci_dev * dev ) ;
void pci_dev_put ( struct pci_dev * dev ) ;
struct pci_dev * pci_find_upstream_pcie_bridge ( struct pci_dev * pdev ) ;
struct pci_bus * pci_find_bus ( int domain , int busnr ) ;
struct pci_bus * pci_find_next_bus ( const struct pci_bus * from ) ;
struct pci_dev * pci_get_slot ( struct pci_bus * bus , unsigned int devfn ) ;
struct pci_dev * pci_get_bus_and_slot ( unsigned int bus , unsigned int devfn ) ;
struct pci_dev * pci_get_subsys ( unsigned int vendor , unsigned int device , unsigned int ss_vendor , unsigned int ss_device , struct pci_dev * from ) ;
struct pci_dev * pci_get_device ( unsigned int vendor , unsigned int device , struct pci_dev * from ) ;
struct pci_dev * pci_get_class ( unsigned int class_ele , struct pci_dev * from ) ;
int pci_dev_present ( const struct pci_device_id * ids ) ;
int pci_mmap_fits ( struct pci_dev * pdev , int resno , struct vm_area_struct * vma ) ;
int __attribute__ ( ( weak ) ) pcibios_add_platform_entries ( struct pci_dev * dev ) ;
int pci_create_sysfs_dev_files ( struct pci_dev * pdev ) ;
void pci_remove_sysfs_dev_files ( struct pci_dev * pdev ) ;
int pci_enable_rom ( struct pci_dev * pdev ) ;
void pci_disable_rom ( struct pci_dev * pdev ) ;
size_t pci_get_rom_size ( struct pci_dev * pdev , void * rom , size_t size ) ;
void * pci_map_rom ( struct pci_dev * pdev , size_t * size ) ;
void pci_unmap_rom ( struct pci_dev * pdev , void * rom ) ;
void pci_cleanup_rom ( struct pci_dev * pdev ) ;
void pci_update_resource ( struct pci_dev * dev , int resno ) ;
int pci_claim_resource ( struct pci_dev * dev , int resource ) ;
void pci_disable_bridge_window ( struct pci_dev * dev ) ;
int pci_assign_resource ( struct pci_dev * dev , int resno ) ;
void pdev_sort_resources ( struct pci_dev * dev , struct resource_list * head ) ;
int pci_enable_resources ( struct pci_dev * dev , int mask ) ;
//enum pci_lost_interrupt_reason pci_lost_interrupt ( struct pci_dev * pdev ) ;
/*--pci_filp_private--*/
int pci_proc_attach_device ( struct pci_dev * dev ) ;
int pci_proc_detach_device ( struct pci_dev * dev ) ;
int pci_proc_detach_bus ( struct pci_bus * bus ) ;
struct pci_slot * pci_create_slot ( struct pci_bus * parent , int slot_nr , const char * name , struct hotplug_slot * hotplug ) ;
void pci_renumber_slot ( struct pci_slot * slot , int slot_nr ) ;
void pci_destroy_slot ( struct pci_slot * slot ) ;
void pci_hp_create_module_link ( struct pci_slot * pci_slot ) ;
void pci_hp_remove_module_link ( struct pci_slot * pci_slot ) ;
struct pci_dev * pci_find_device ( unsigned int vendor , unsigned int device , struct pci_dev * from ) ;
int __pci_hp_register ( struct hotplug_slot * slot , struct pci_bus * bus , int devnr , const char * name , struct module * owner , const char * mod_name ) ;
int pci_hp_deregister ( struct hotplug_slot * hotplug ) ;
int pci_hp_change_slot_info ( struct hotplug_slot * hotplug , struct hotplug_slot_info * info ) ;
void pci_configure_slot ( struct pci_dev * dev ) ;
int pci_get_hp_params ( struct pci_dev * dev , struct hotplug_params * hpp ) ;
extern int pci_vpd_truncate ( struct pci_dev * dev , size_t size ) ;
extern void pci_block_user_cfg_access ( struct pci_dev * dev ) ;
extern  void pci_unblock_user_cfg_access ( struct pci_dev * dev ) ;

typedef u64 phys_addr_t ;
typedef phys_addr_t resource_size_t ;
extern int pci_bus_alloc_resource ( struct pci_bus * bus , struct resource * res , resource_size_t size , resource_size_t align , resource_size_t min , unsigned int type_mask , void ( * alignf ) ( void * , struct resource * , resource_size_t , resource_size_t ) , void * alignf_data ) ;
extern int pci_bus_add_device ( struct pci_dev * dev ) ;

typedef unsigned int pcie_reset_state_t ;
/* 1......pcie_reset_state_t......*//* 3......pcie_reset_state_t......*//* 4......pcie_reset_state_t......*//*--pcie_reset_state_t--*/
enum pcie_reset_state { pcie_deassert_reset = ( pcie_reset_state_t ) 1 , pcie_warm_reset = ( pcie_reset_state_t ) 2 , pcie_hot_reset = ( pcie_reset_state_t ) 3 } ;
/*--pcie_reset_state--*/
int pci_set_pcie_reset_state ( struct pci_dev * dev , enum pcie_reset_state state ) ;

enum pci_lost_interrupt_reason { PCI_LOST_IRQ_NO_INFORMATION = 0 , PCI_LOST_IRQ_DISABLE_MSI , PCI_LOST_IRQ_DISABLE_MSIX , PCI_LOST_IRQ_DISABLE_ACPI , } ;
/*--pci_lost_interrupt_reason--*/
extern enum pci_lost_interrupt_reason pci_lost_interrupt ( struct pci_dev * pdev ) ;


enum gro_result { GRO_MERGED , GRO_MERGED_FREE , GRO_HELD , GRO_NORMAL , GRO_DROP , } ;
/*--gro_result--*/
typedef enum gro_result gro_result_t ;

extern void napi_gro_flush ( struct napi_struct * napi ) ;
gro_result_t napi_gro_frags ( struct napi_struct * napi ) ;
extern void netif_napi_del ( struct napi_struct * napi ) ;
extern void __napi_schedule ( struct napi_struct * n ) ;
extern void __napi_complete ( struct napi_struct * n ) ;
extern void napi_complete ( struct napi_struct * n ) ;
int register_netdevice ( struct net_device * dev ) ;
void unregister_netdevice ( struct net_device * dev ) ;

int netif_rx ( struct sk_buff * skb ) ;
int netif_rx_ni ( struct sk_buff * skb ) ;
struct sk_buff * __alloc_skb ( unsigned int size , gfp_t gfp_mask , int fclone , int node ) ;
struct sk_buff * __netdev_alloc_skb ( struct net_device * dev , unsigned int length , gfp_t gfp_mask ) ;
struct page * __netdev_alloc_page ( struct net_device * dev , gfp_t gfp_mask ) ;
void netif_notify_peers ( struct net_device * dev ) ;
gro_result_t napi_frags_finish ( struct napi_struct * napi , struct sk_buff * skb , gro_result_t ret ) ;
struct sk_buff * napi_frags_skb ( struct napi_struct * napi ) ;

int register_netdevice_notifier ( struct notifier_block * nb ) ;
int unregister_netdevice_notifier ( struct notifier_block * nb ) ;
struct in_device * inetdev_by_index ( struct net * net , int ifindex ) ;
int netdev_set_master ( struct net_device * slave , struct net_device * master ) ;
gro_result_t napi_skb_finish ( gro_result_t ret , struct sk_buff * skb ) ;
extern gro_result_t napi_gro_receive ( struct napi_struct * napi , struct sk_buff * skb ) ;
extern void napi_reuse_skb ( struct napi_struct * napi , struct sk_buff * skb ) ;
extern struct sk_buff * napi_get_frags ( struct napi_struct * napi ) ;
extern unsigned long netdev_increment_features ( unsigned long all , unsigned long one , unsigned long mask ) ;
int init_dummy_netdev ( struct net_device * dev ) ;
void netdev_features_change ( struct net_device * dev ) ;
void netdev_state_change ( struct net_device * dev ) ;
void netdev_bonding_change ( struct net_device * dev , unsigned long event ) ;
void netdev_rx_csum_fault ( struct net_device * dev ) ;
int netdev_class_create_file ( struct class_attribute * class_attr ) ;
void netdev_class_remove_file ( struct class_attribute * class_attr ) ;
struct net_device * alloc_netdev_mq ( int sizeof_priv , const char * name , void ( * setup ) ( struct net_device * ) , unsigned int queue_count ) ;

/************************************************************************************************/
struct completion { unsigned int done ; wait_queue_head_t wait ; } ;
typedef uint32_t grant_handle_t ;
enum xenbus_state { XenbusStateUnknown = 0 , XenbusStateInitialising = 1 , XenbusStateInitWait = 2 , XenbusStateInitialised = 3 , XenbusStateConnected = 4 , XenbusStateClosing = 5 , XenbusStateClosed = 6 } ;
/*--xenbus_state--*/
typedef uint32_t XENSTORE_RING_IDX ;
/* 1......XENSTORE_RING_IDX......*//* 3......XENSTORE_RING_IDX......*//* 4......XENSTORE_RING_IDX......*//*--XENSTORE_RING_IDX--*/
struct xenstore_domain_interface { char req [ 1024 ] ; char rsp [ 1024 ] ; XENSTORE_RING_IDX req_cons , req_prod ; XENSTORE_RING_IDX rsp_cons , rsp_prod ; } ;
/*--xenstore_domain_interface--*/
struct xenbus_watch { struct list_head list ; const char * node ; void ( * callback ) ( struct xenbus_watch * , const char * * vec , unsigned int len ) ; } ;
/*--xenbus_watch--*/
struct xenbus_device { const char * devicetype ; const char * nodename ; const char * otherend ; int otherend_id ; struct xenbus_watch otherend_watch ; struct device dev ; enum xenbus_state state ; struct completion down ; } ;
/*--xenbus_device--*/
struct xenbus_device_id { char devicetype [ 32 ] ; } ;
/*--xenbus_device_id--*/
struct xenbus_driver { char * name ; struct module * owner ; const struct xenbus_device_id * ids ; int ( * probe ) ( struct xenbus_device * dev , const struct xenbus_device_id * id ) ; void ( * otherend_changed ) ( struct xenbus_device * dev , enum xenbus_state backend_state ) ; int ( * remove ) ( struct xenbus_device * dev ) ; int ( * suspend ) ( struct xenbus_device * dev , pm_message_t state ) ; int ( * resume ) ( struct xenbus_device * dev ) ; int ( * uevent ) ( struct xenbus_device * , char * * , int , char * , int ) ; struct device_driver driver ; int ( * read_otherend_details ) ( struct xenbus_device * dev ) ; int ( * is_ready ) ( struct xenbus_device * dev ) ; } ;
/*--xenbus_driver--*/
struct xenbus_transaction { u32 id ; } ;
/*--xenbus_transaction--*/
enum shutdown_state { SHUTDOWN_INVALID = - 1 , SHUTDOWN_POWEROFF = 0 , SHUTDOWN_SUSPEND = 2 , SHUTDOWN_HALT = 4 , } ;
/*--shutdown_state--*/
const char * xenbus_strstate ( enum xenbus_state state ) ;
int xenbus_watch_path ( struct xenbus_device * dev , const char * path , struct xenbus_watch * watch , void ( * callback ) ( struct xenbus_watch * , const char * * , unsigned int ) ) ;
int xenbus_watch_pathfmt ( struct xenbus_device * dev , struct xenbus_watch * watch , void ( * callback ) ( struct xenbus_watch * , const char * * , unsigned int ) , const char * pathfmt , ... ) ;
int xenbus_switch_state ( struct xenbus_device * dev , enum xenbus_state state ) ;
int xenbus_frontend_closed ( struct xenbus_device * dev ) ;
void xenbus_dev_error ( struct xenbus_device * dev , int err , const char * fmt , ... ) ;
void xenbus_dev_fatal ( struct xenbus_device * dev , int err , const char * fmt , ... ) ;
int xenbus_grant_ring ( struct xenbus_device * dev , unsigned long ring_mfn ) ;
int xenbus_alloc_evtchn ( struct xenbus_device * dev , int * port ) ;
int xenbus_bind_evtchn ( struct xenbus_device * dev , int remote_port , int * port ) ;
int xenbus_free_evtchn ( struct xenbus_device * dev , int port ) ;
int xenbus_map_ring_valloc ( struct xenbus_device * dev , int gnt_ref , void * * vaddr ) ;
int xenbus_map_ring ( struct xenbus_device * dev , int gnt_ref , grant_handle_t * handle , void * vaddr ) ;
int xenbus_unmap_ring_vfree ( struct xenbus_device * dev , void * vaddr ) ;
int xenbus_unmap_ring ( struct xenbus_device * dev , grant_handle_t handle , void * vaddr ) ;
enum xenbus_state xenbus_read_driver_state ( const char * path ) ;
void * xenbus_dev_request_and_reply ( struct xsd_sockmsg * msg ) ;
char * * xenbus_directory ( struct xenbus_transaction t , const char * dir , const char * node , unsigned int * num ) ;
int xenbus_exists ( struct xenbus_transaction t , const char * dir , const char * node ) ;
void * xenbus_read ( struct xenbus_transaction t , const char * dir , const char * node , unsigned int * len ) ;
int xenbus_write ( struct xenbus_transaction t , const char * dir , const char * node , const char * string ) ;
int xenbus_mkdir ( struct xenbus_transaction t , const char * dir , const char * node ) ;
int xenbus_rm ( struct xenbus_transaction t , const char * dir , const char * node ) ;
int xenbus_transaction_start ( struct xenbus_transaction * t ) ;
int xenbus_transaction_end ( struct xenbus_transaction t , int abort ) ;
int register_xenbus_watch ( struct xenbus_watch * watch ) ;
void unregister_xenbus_watch ( struct xenbus_watch * watch ) ;
int xenbus_match ( struct device * _dev , struct device_driver * _drv ) ;
int read_otherend_details ( struct xenbus_device * xendev , char * id_node , char * path_node ) ;
int xenbus_dev_probe ( struct device * _dev ) ;
int xenbus_dev_remove ( struct device * _dev ) ;
int xenbus_register_driver_common ( struct xenbus_driver * drv , struct xen_bus_type * bus , struct module * owner , const char * mod_name ) ;
int __xenbus_register_frontend ( struct xenbus_driver * drv , struct module * owner , const char * mod_name ) ;
void xenbus_unregister_driver ( struct xenbus_driver * drv ) ;
struct xb_find_info { struct xenbus_device * dev ; const char * nodename ; } ;
/*--xb_find_info--*/
struct xenbus_device * xenbus_device_find ( const char * nodename , struct bus_type * bus ) ;
int xenbus_probe_node ( struct xen_bus_type * bus , const char * type , const char * nodename ) ;
int xenbus_probe_devices ( struct xen_bus_type * bus ) ;
void xenbus_dev_changed ( const char * node , struct xen_bus_type * bus ) ;
int register_xenstore_notifier ( struct notifier_block * nb ) ;
void unregister_xenstore_notifier ( struct notifier_block * nb ) ;
void xenbus_probe ( struct work_struct * unused ) ;


//------------------------------------------------------------------------------------------------------------------------

struct mii_if_info { int phy_id ; int advertising ; int phy_id_mask ; int reg_num_mask ; unsigned int full_duplex : 1 ; unsigned int force_media : 1 ; unsigned int supports_gmii : 1 ; struct net_device * dev ; int ( * mdio_read ) ( struct net_device * dev , int phy_id , int location ) ; void ( * mdio_write ) ( struct net_device * dev , int phy_id , int location , int val ) ; } ;

struct phy_fixup { struct list_head list ; char bus_id [ 20 ] ; u32 phy_uid ; u32 phy_uid_mask ; int ( * run ) ( struct phy_device * phydev ) ; } ;
/*--phy_fixup--*/
void phy_print_status ( struct phy_device * phydev ) ;
int phy_clear_interrupt ( struct phy_device * phydev ) ;
int phy_config_interrupt ( struct phy_device * phydev , u32 interrupts ) ;
struct phy_setting { int speed ; int duplex ; u32 setting ; } ;
/*--phy_setting--*/
void phy_sanitize_settings ( struct phy_device * phydev ) ;
int phy_ethtool_sset ( struct phy_device * phydev , struct ethtool_cmd * cmd ) ;
int phy_ethtool_gset ( struct phy_device * phydev , struct ethtool_cmd * cmd ) ;
int phy_mii_ioctl ( struct phy_device * phydev , struct mii_ioctl_data * mii_data , int cmd ) ;
int phy_start_aneg ( struct phy_device * phydev ) ;
void phy_start_machine ( struct phy_device * phydev , void ( * handler ) ( struct net_device * ) ) ;
void phy_stop_machine ( struct phy_device * phydev ) ;
int phy_enable_interrupts ( struct phy_device * phydev ) ;
int phy_disable_interrupts ( struct phy_device * phydev ) ;
int phy_start_interrupts ( struct phy_device * phydev ) ;
int phy_stop_interrupts ( struct phy_device * phydev ) ;
void phy_stop ( struct phy_device * phydev ) ;
void phy_start ( struct phy_device * phydev ) ;
void phy_device_free ( struct phy_device * phydev ) ;
int phy_register_fixup ( const char * bus_id , u32 phy_uid , u32 phy_uid_mask , int ( * run ) ( struct phy_device * ) ) ;
int phy_register_fixup_for_uid ( u32 phy_uid , u32 phy_uid_mask , int ( * run ) ( struct phy_device * ) ) ;
int phy_register_fixup_for_id ( const char * bus_id , int ( * run ) ( struct phy_device * ) ) ;
int phy_scan_fixups ( struct phy_device * phydev ) ;
struct phy_device * phy_device_create ( struct mii_bus * bus , int addr , int phy_id ) ;
int get_phy_id ( struct mii_bus * bus , int addr , u32 * phy_id ) ;
struct phy_device * get_phy_device ( struct mii_bus * bus , int addr ) ;
int phy_device_register ( struct phy_device * phydev ) ;
void phy_prepare_link ( struct phy_device * phydev , void ( * handler ) ( struct net_device * ) ) ;
int phy_connect_direct ( struct net_device * dev , struct phy_device * phydev , void ( * handler ) ( struct net_device * ) , u32 flags , phy_interface_t interface ) ;
struct phy_device * phy_connect ( struct net_device * dev , const char * bus_id , void ( * handler ) ( struct net_device * ) , u32 flags , phy_interface_t interface ) ;
void phy_disconnect ( struct phy_device * phydev ) ;
int phy_attach_direct ( struct net_device * dev , struct phy_device * phydev , u32 flags , phy_interface_t interface ) ;
struct phy_device * phy_attach ( struct net_device * dev , const char * bus_id , u32 flags , phy_interface_t interface ) ;
void phy_detach ( struct phy_device * phydev ) ;
int genphy_config_advert ( struct phy_device * phydev ) ;
int genphy_setup_forced ( struct phy_device * phydev ) ;
int genphy_restart_aneg ( struct phy_device * phydev ) ;
int genphy_config_aneg ( struct phy_device * phydev ) ;
int genphy_update_link ( struct phy_device * phydev ) ;
int genphy_read_status ( struct phy_device * phydev ) ;
int genphy_suspend ( struct phy_device * phydev ) ;
int genphy_resume ( struct phy_device * phydev ) ;
int phy_driver_register ( struct phy_driver * new_driver ) ;
void phy_driver_unregister ( struct phy_driver * drv ) ;
struct phy_device * mdiobus_scan ( struct mii_bus * bus , int addr ) ;


struct mii_bus * mdiobus_alloc ( void ) ;
int mdiobus_register ( struct mii_bus * bus ) ;
void mdiobus_unregister ( struct mii_bus * bus ) ;
void mdiobus_free ( struct mii_bus * bus ) ;
struct phy_device * mdiobus_scan ( struct mii_bus * bus , int addr ) ;
int mdiobus_read ( struct mii_bus * bus , int addr , u16 regnum ) ;
int mdiobus_write ( struct mii_bus * bus , int addr , u16 regnum , u16 val ) ;


//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------

typedef __builtin_va_list __gnuc_va_list ;
typedef __gnuc_va_list va_list ;
enum dpm_order { DPM_ORDER_NONE , DPM_ORDER_DEV_AFTER_PARENT , DPM_ORDER_PARENT_BEFORE_DEV , DPM_ORDER_DEV_LAST , } ;

int device_create_file ( struct device * dev , struct device_attribute * attr ) ;
void device_remove_file ( struct device * dev , struct device_attribute * attr ) ;
int device_create_bin_file ( struct device * dev , struct bin_attribute * attr ) ;
void device_remove_bin_file ( struct device * dev , struct bin_attribute * attr ) ;
int device_schedule_callback_owner ( struct device * dev , void ( * func ) ( struct device * ) , struct module * owner ) ;
void device_initialize ( struct device * dev ) ;
int device_private_init ( struct device * dev ) ;
int device_add ( struct device * dev ) ;
int device_register ( struct device * dev ) ;
void device_del ( struct device * dev ) ;
void device_unregister ( struct device * dev ) ;
int device_for_each_child ( struct device * parent , void * data , int ( * fn ) ( struct device * dev , void * data ) ) ;
struct device * device_find_child ( struct device * parent , void * data , int ( * match ) ( struct device * dev , void * data ) ) ;
extern struct device * device_create_vargs ( struct class_decl * class_data , struct device * parent , dev_t devt , void * drvdata , const char * fmt , va_list args ) ;
extern struct device * device_create ( struct class_decl* class_ele , struct device * parent , dev_t devt , void * drvdata , const char * fmt , ... ) ;
extern void device_destroy ( struct class_decl *class_data , dev_t devt ) ;
extern int device_rename ( struct device * dev , char * new_name ) ;
extern void device_shutdown ( void ) ;
int device_reprobe ( struct device * dev ) ;
int device_bind_driver ( struct device * dev ) ;
int driver_probe_device ( struct device_driver * drv , struct device * dev ) ;
int device_attach ( struct device * dev ) ;
void device_release_driver ( struct device * dev ) ;
extern int device_move ( struct device * dev , struct device * new_parent , enum dpm_order dpm_order ) ;

//-----------------------------------------------------------------------------------------

extern int _spin_trylock ( spinlock_t * lock ) ;
extern unsigned long _spin_lock_irqsave ( spinlock_t * lock ) ;
extern void _spin_lock_irq ( spinlock_t * lock ) ;
extern void _spin_lock_bh ( spinlock_t * lock ) ;
extern void _spin_lock ( spinlock_t * lock ) ;
extern void _spin_unlock_irqrestore ( spinlock_t * lock , unsigned long flags ) ;
extern void _spin_unlock_bh ( spinlock_t * lock ) ;
extern int _spin_trylock_bh ( spinlock_t * lock ) ;

typedef struct { unsigned int lock ; } raw_rwlock_t ;
typedef struct { raw_rwlock_t raw_lock ; } rwlock_t ;

extern void _read_unlock_irqrestore ( rwlock_t * lock , unsigned long flags ) ;
extern void _write_unlock_irqrestore ( rwlock_t * lock , unsigned long flags ) ;
extern unsigned long _read_lock_irqsave ( rwlock_t * lock ) ;
extern unsigned long _write_lock_irqsave ( rwlock_t * lock ) ;

//--------------------------------------------------------------------------------------------------------------------
typedef int proc_handler ( struct ctl_table * ctl , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;

typedef unsigned int __uid_t ;
typedef __uid_t uid_t ;
typedef long int __time_t;
typedef long int __blkcnt_t ;
typedef unsigned long int __fsblkcnt_t ;
typedef __blkcnt_t blkcnt_t ;
typedef __fsblkcnt_t fsblkcnt_t ;
typedef unsigned short umode_t ;
typedef unsigned int __gid_t ;
typedef __gid_t gid_t ;
typedef unsigned fmode_t ;
typedef unsigned long sector_t ;

typedef unsigned long long __u64 ;



typedef __u32 Elf64_Word ;
typedef __u16 Elf64_Half ;
typedef __u64 Elf64_Addr ;
typedef __u64 Elf64_Xword ;
typedef long long qsize_t;

typedef void ( * ctor_fn_t ) ( void ) ;

enum module_state { MODULE_STATE_LIVE , MODULE_STATE_COMING , MODULE_STATE_GOING , } ;
typedef struct elf64_sym { Elf64_Word st_name ; unsigned char st_info ; unsigned char st_other ; Elf64_Half st_shndx ; Elf64_Addr st_value ; Elf64_Xword st_size ; } Elf64_Sym ;

typedef int ( * filldir_t ) ( void * , const char * , int , loff_t , u64 , unsigned ) ;
typedef unsigned int __kernel_uid_t ;
typedef __kernel_uid_t __kernel_uid32_t ;
typedef __kernel_uid32_t qid_t ;

typedef signed long rwsem_count_t ;

struct timespec { __time_t tv_sec ; long int tv_nsec ; } ;
struct rw_semaphore { rwsem_count_t count ; spinlock_t wait_lock ; struct list_head wait_list ; } ;


struct radix_tree_root {
    unsigned int height ;
    gfp_t gfp_mask ;
    struct radix_tree_node *rnode ;
};

struct kmem_cache_order_objects { unsigned long x ; } ;
struct kmem_cache_node {
    spinlock_t list_lock ;
    unsigned long nr_partial ;
    struct list_head partial ;
    atomic_long_t nr_slabs ;
    atomic_long_t total_objects ;
    struct list_head full ;
};

struct kmem_cache {
    unsigned long flags ;
    int size ;
    int objsize ;
    int offset ;
    struct kmem_cache_order_objects oo ;
    struct kmem_cache_node local_node ;
    struct kmem_cache_order_objects max ;
    struct kmem_cache_order_objects min ;
    gfp_t allocflags ;
    int refcount ;
    void ( * ctor ) ( void * ) ;
    int inuse ;
    int align ;
    unsigned long min_partial ;
    const char * name ;
    struct list_head list ;
    struct kobject kobj ;
    int remote_node_defrag_ratio ;
    struct kmem_cache_node * node[(1<<6)] ;
    struct kmem_cache_cpu * cpu_slab[64] ;
};


struct page {
    unsigned long flags ;
    atomic_t _count ;
    union { atomic_t _mapcount ; struct { u16 inuse ; u16 objects ; } ; } ;
    union {
        struct { unsigned long private_data ; struct address_space * mapping ; } ;
        spinlock_t ptl ;
        struct kmem_cache * slab ;
        struct page * first_page ;
    } ;
    union { unsigned long index ; void * freelist ; } ;
    struct list_head lru ;
};

struct prio_tree_root {
    struct prio_tree_node *prio_tree_node ;
    unsigned short index_bits ;
    unsigned short raw ;
};

typedef struct {
    size_t written ;
    size_t count ;
    union { char * buf ; void * data ; } arg ;
    int error ;
} read_descriptor_t ;

struct address_space_operations {
    int ( * writepage ) ( struct page * page , struct writeback_control * wbc ) ;
    int ( * readpage ) ( struct file * , struct page * ) ;
    void ( * sync_page ) ( struct page * ) ;
    int ( * writepages ) ( struct address_space * , struct writeback_control * ) ;
    int ( * set_page_dirty ) ( struct page * page ) ;
    int ( * readpages ) ( struct file * filp , struct address_space * mapping , struct list_head * pages , unsigned nr_pages ) ;
    int ( * write_begin ) ( struct file * , struct address_space * mapping , loff_t pos , unsigned len , unsigned flags , struct page * * pagep , void * * fsdata ) ;
    int ( * write_end ) ( struct file * , struct address_space * mapping , loff_t pos , unsigned len , unsigned copied , struct page * page , void * fsdata ) ;
    sector_t ( * bmap ) ( struct address_space * , sector_t ) ;
    void ( * invalidatepage ) ( struct page * , unsigned long ) ;
    int ( * releasepage ) ( struct page * , gfp_t ) ;
    ssize_t ( * direct_IO ) ( int , struct kiocb * , const struct iovec * iov , loff_t offset , unsigned long nr_segs ) ;
    int ( * get_xip_mem ) ( struct address_space * , unsigned long , int , void * * , unsigned long * ) ;
    int ( * migratepage ) ( struct address_space * , struct page * , struct page * ) ;
    int ( * launder_page ) ( struct page * ) ;
    int ( * is_partially_uptodate ) ( struct page * , read_descriptor_t * , unsigned long ) ;
    int ( * error_remove_page ) ( struct address_space * , struct page * ) ;
};

struct address_space {
    struct inode * host ;
    struct radix_tree_root page_tree ;
    spinlock_t tree_lock ;
    unsigned int i_mmap_writable ;
    struct prio_tree_root i_mmap ;
    struct list_head i_mmap_nonlinear ;
    spinlock_t i_mmap_lock ;
    unsigned int truncate_count ;
    unsigned long nrpages ;
    unsigned long writeback_index ;
    const struct address_space_operations * a_ops ;
    unsigned long flags ;
    struct backing_dev_info * backing_dev_info ;
    spinlock_t private_lock ;
    struct list_head private_list ;
    struct address_space * assoc_mapping ;
} ;

struct mem_dqinfo {
    struct quota_format_type * dqi_format ;
    int dqi_fmt_id ;
    struct list_head dqi_dirty_list ;
    unsigned long dqi_flags ;
    unsigned int dqi_bgrace ;
    unsigned int dqi_igrace ;
    qsize_t dqi_maxblimit ;
    qsize_t dqi_maxilimit ;
    void * dqi_priv ;
};

struct quota_info {
    unsigned int flags ;
    struct mutex dqio_mutex ;
    struct mutex dqonoff_mutex ;
    struct rw_semaphore dqptr_sem ;
    struct inode * files [ 2 ] ;
    struct mem_dqinfo info [ 2 ] ;
    struct quota_format_ops * ops [ 2 ] ;
} ;



struct lock_class_key { } ;

struct file_system_type {
    const char *name;
    int fs_flags;
    int (*get_sb) (struct file_system_type *, int,
               const char *, void *, struct vfsmount *);
    void (*kill_sb) (struct super_block *);
    struct module *owner;
    struct file_system_type * next;
    struct list_head fs_supers;

    struct lock_class_key s_lock_key;
    struct lock_class_key s_umount_key;

    struct lock_class_key i_lock_key;
    struct lock_class_key i_mutex_key;
    struct lock_class_key i_mutex_dir_key;
    struct lock_class_key i_alloc_sem_key;
};

struct super_operations {
    struct inode * (*alloc_inode ) ( struct super_block * sb );
    void ( * destroy_inode ) ( struct inode * ) ;
    void ( * dirty_inode ) ( struct inode * ) ;
    int ( * write_inode ) ( struct inode * , int ) ;
    void ( * drop_inode ) ( struct inode * ) ;
    void ( * delete_inode ) ( struct inode * ) ;
    void ( * put_super ) ( struct super_block * ) ;
    void ( * write_super ) ( struct super_block * ) ;
    int ( * sync_fs ) ( struct super_block * sb , int wait ) ;
    int ( * freeze_fs ) ( struct super_block * ) ;
    int ( * unfreeze_fs ) ( struct super_block * ) ;
    int ( * statfs ) ( struct dentry * , struct kstatfs * ) ;
    int ( * remount_fs ) ( struct super_block * , int * , char * ) ;
    void ( * clear_inode ) ( struct inode * ) ;
    void ( * umount_begin ) ( struct super_block * ) ;
    int ( * show_options ) ( struct seq_file * , struct vfsmount * ) ;
    int ( * show_stats ) ( struct seq_file * , struct vfsmount * ) ;
    ssize_t ( * quota_read ) ( struct super_block * , int , char * , size_t , loff_t ) ;
    ssize_t ( * quota_write ) ( struct super_block * , int , const char * , size_t , loff_t ) ;
    int ( * bdev_try_to_free_page ) ( struct super_block * , struct page * , gfp_t ) ;
};

struct dquot_operations {
    int ( * initialize ) ( struct inode * , int ) ;
    int ( * drop ) ( struct inode * ) ;
    int ( * alloc_space ) ( struct inode * , qsize_t , int ) ;
    int ( * alloc_inode ) ( const struct inode * , qsize_t ) ;
    int ( * free_space ) ( struct inode * , qsize_t ) ;
    int ( * free_inode ) ( const struct inode * , qsize_t ) ;
    int ( * transfer ) ( struct inode * , struct iattr * ) ;
    int ( * write_dquot ) ( struct dquot * ) ;
    struct dquot * ( * alloc_dquot ) ( struct super_block * , int ) ;
    void ( * destroy_dquot ) ( struct dquot * ) ;
    int ( * acquire_dquot ) ( struct dquot * ) ;
    int ( * release_dquot ) ( struct dquot * ) ;
    int ( * mark_dirty ) ( struct dquot * ) ;
    int ( * write_info ) ( struct super_block * , int ) ;
    int ( * reserve_space ) ( struct inode * , qsize_t , int ) ;
    int ( * claim_space ) ( struct inode * , qsize_t ) ;
    void ( * release_rsv ) ( struct inode * , qsize_t ) ;
    qsize_t * ( * get_reserved_space ) ( struct inode * ) ;
};

struct quotactl_ops {
    int ( * quota_on ) ( struct super_block * , int , int , char * , int ) ;
    int ( * quota_off ) ( struct super_block * , int , int ) ;
    int ( * quota_sync ) ( struct super_block * , int ) ;
    int ( * get_info ) ( struct super_block * , int , struct if_dqinfo * ) ;
    int ( * set_info ) ( struct super_block * , int , struct if_dqinfo * ) ;
    int ( * get_dqblk ) ( struct super_block * , int , qid_t , struct if_dqblk * ) ;
    int ( * set_dqblk ) ( struct super_block * , int , qid_t , struct if_dqblk * ) ;
    int ( * get_xstate ) ( struct super_block * , struct fs_quota_stat * ) ;
    int ( * set_xstate ) ( struct super_block * , unsigned int , int ) ;
    int ( * get_xquota ) ( struct super_block * , int , qid_t , struct fs_disk_quota * ) ;
    int ( * set_xquota ) ( struct super_block * , int , qid_t , struct fs_disk_quota * ) ;
};

struct export_operations {
    int ( * encode_fh ) ( struct dentry * de , __u32 * fh , int * max_len , int connectable ) ;
    struct dentry * ( * fh_to_dentry ) ( struct super_block * sb , struct fid * fid , int fh_len , int fh_type ) ;
    struct dentry * ( * fh_to_parent ) ( struct super_block * sb , struct fid * fid , int fh_len , int fh_type ) ;
    int ( * get_name ) ( struct dentry * parent , char * name , struct dentry * child ) ;
    struct dentry * ( * get_parent ) ( struct dentry * child ) ;
};

struct xattr_handler {
    char * prefix ;
    size_t ( * list ) ( struct inode * inode , char * list , size_t list_size , const char * name , size_t name_len ) ;
    int ( * get ) ( struct inode * inode , const char * name , void * buffer , size_t size ) ;
    int ( * set ) ( struct inode * inode , const char * name , const void * buffer , size_t size , int flags ) ;
};

struct rcu_head { struct rcu_head * next ; void ( * func ) ( struct rcu_head * head ) ; } ;
struct qstr { unsigned int hash ; unsigned int len ; const unsigned char * name ; } ;

struct dentry {
    atomic_t d_count ;
    unsigned int d_flags ;
    spinlock_t d_lock ;
    int d_mounted ;
    struct inode * d_inode ;
    struct hlist_node d_hash ;
    struct dentry * d_parent ;
    struct qstr d_name ;
    struct list_head d_lru ;
    union {
        struct list_head d_child ;
        struct rcu_head d_rcu ;
    } d_u ;
    struct list_head d_subdirs ;
    struct list_head d_alias ;
    unsigned long d_time ;
    const struct dentry_operations * d_op ;
    struct super_block * d_sb ;
    void * d_fsdata ;
    unsigned char d_iname[32] ;
};

struct super_block {
    struct list_head s_list ;
    dev_t s_dev ;
    unsigned long s_blocksize ;
    unsigned char s_blocksize_bits ;
    unsigned char s_dirt ;
    loff_t s_maxbytes ;
    struct file_system_type *s_type ;
    const struct super_operations *s_op ;
    const struct dquot_operations *dq_op ;
    const struct quotactl_ops *s_qcop ;
    const struct export_operations *s_export_op ;
    unsigned long s_flags ;
    unsigned long s_magic ;
    struct dentry * s_root ;
    struct rw_semaphore s_umount ;
    struct mutex s_lock ;
    int s_count ;
    int s_need_sync ;
    atomic_t s_active ;
    void * s_security ;
    struct xattr_handler **s_xattr ;
    struct list_head s_inodes ;
    struct hlist_head s_anon ;
    struct list_head s_files ;
    struct list_head s_dentry_lru ;
    int s_nr_dentry_unused ;
    struct block_device * s_bdev ;
    struct backing_dev_info *s_bdi ;
    struct mtd_info *s_mtd ;
    struct list_head s_instances;
    struct quota_info s_dquot ;
    int s_frozen ;
    wait_queue_head_t s_wait_unfrozen ;
    char s_id[32] ;
    void * s_fs_info ;
    fmode_t s_mode ;
    struct mutex s_vfs_rename_mutex ;
    u32 s_time_gran ;
    char *s_subtype ;
    char *s_options ;
};

struct inode_operations {
    int ( * create ) ( struct inode * , struct dentry * , int , struct nameidata * ) ;
    struct dentry * ( * lookup ) ( struct inode * , struct dentry * , struct nameidata * ) ;
    int ( * link ) ( struct dentry * , struct inode * , struct dentry * ) ;
    int ( * unlink ) ( struct inode * , struct dentry * ) ;
    int ( * symlink ) ( struct inode * , struct dentry * , const char * ) ;
    int ( * mkdir ) ( struct inode * , struct dentry * , int ) ;
    int ( * rmdir ) ( struct inode * , struct dentry * ) ;
    int ( * mknod ) ( struct inode * , struct dentry * , int , dev_t ) ;
    int ( * rename ) ( struct inode * , struct dentry * , struct inode * , struct dentry * ) ;
    int ( * readlink ) ( struct dentry * , char * , int ) ;
    void * ( * follow_link ) ( struct dentry * , struct nameidata * ) ;
    void ( * put_link ) ( struct dentry * , struct nameidata * , void * ) ;
    void ( * truncate ) ( struct inode * ) ;
    int ( * permission ) ( struct inode * , int ) ;
    int ( * check_acl ) ( struct inode * , int ) ;
    int ( * setattr ) ( struct dentry * , struct iattr * ) ;
    int ( * getattr ) ( struct vfsmount * mnt , struct dentry * , struct kstat * ) ;
    int ( * setxattr ) ( struct dentry * , const char * , const void * , size_t , int ) ;
    ssize_t ( * getxattr ) ( struct dentry * , const char * , void * , size_t ) ;
    ssize_t ( * listxattr ) ( struct dentry * , char * , size_t ) ;
    int ( * removexattr ) ( struct dentry * , const char * ) ;
    void ( * truncate_range ) ( struct inode * , loff_t , loff_t ) ;
    long ( * fallocate ) ( struct inode * inode , int mode , loff_t offset , loff_t len ) ;
    int ( * fiemap ) ( struct inode * , struct fiemap_extent_info * , u64 start , u64 len ) ;
};

typedef struct files_struct * fl_owner_t ;

struct file_operations {
    struct module * owner ;
    loff_t ( * llseek ) ( struct file * , loff_t , int ) ;
    ssize_t ( * read ) ( struct file * , char * , size_t , loff_t * ) ;
    ssize_t ( * write ) ( struct file * , const char * , size_t , loff_t * ) ;
    ssize_t ( * aio_read ) ( struct kiocb * , const struct iovec * , unsigned long , loff_t ) ;
    ssize_t ( * aio_write ) ( struct kiocb * , const struct iovec * , unsigned long , loff_t ) ;
    int ( * readdir ) ( struct file * , void * , filldir_t ) ;
    unsigned int ( * poll ) ( struct file * , struct poll_table_struct * ) ;
    int ( * ioctl ) ( struct inode * , struct file * , unsigned int , unsigned long ) ;
    long ( * unlocked_ioctl ) ( struct file * , unsigned int , unsigned long ) ;
    long ( * compat_ioctl ) ( struct file * , unsigned int , unsigned long ) ;
    int ( * mmap ) ( struct file * , struct vm_area_struct * ) ;
    int ( * open ) ( struct inode * , struct file * ) ;
    int ( * flush ) ( struct file * , fl_owner_t id ) ;
    int ( * release ) ( struct inode * , struct file * ) ;
    int ( * fsync ) ( struct file * , struct dentry * , int datasync ) ;
    int ( * aio_fsync ) ( struct kiocb * , int datasync ) ;
    int ( * fasync ) ( int , struct file * , int ) ;
    int ( * lock ) ( struct file * , int , struct file_lock * ) ;
    ssize_t ( * sendpage ) ( struct file * , struct page * , int , size_t , loff_t * , int ) ;
    unsigned long ( * get_unmapped_area ) ( struct file * , unsigned long , unsigned long , unsigned long , unsigned long ) ;
    int ( * check_flags ) ( int ) ;
    int ( * flock ) ( struct file * , int , struct file_lock * ) ;
    ssize_t ( * splice_write ) ( struct pipe_inode_info * , struct file * , loff_t * , size_t , unsigned int ) ;
    ssize_t ( * splice_read ) ( struct file * , loff_t * , struct pipe_inode_info * , size_t , unsigned int ) ;
    int ( * setlease ) ( struct file * , long , struct file_lock * * ) ;
};

typedef long __kernel_time_t ;

struct timespec_inode { __kernel_time_t tv_sec ; long tv_nsec ; } ;

struct inode {
    struct hlist_node i_hash ;
    struct list_head i_list ;
    struct list_head i_sb_list ;
    struct list_head i_dentry ;
    unsigned long i_ino ;
    atomic_t i_count ;
    unsigned int i_nlink ;
    uid_t i_uid ;
    gid_t i_gid ;
    dev_t i_rdev ;
    u64 i_version ;
    loff_t i_size ;
    struct timespec_inode i_atime ;
    struct timespec_inode i_mtime ;
    struct timespec_inode i_ctime ;
    blkcnt_t i_blocks ;
    unsigned int i_blkbits ;
    unsigned short i_bytes ;
    umode_t i_mode ;
    spinlock_t i_lock ;
    struct mutex i_mutex ;
    struct rw_semaphore i_alloc_sem ;
    const struct inode_operations * i_op ;
    const struct file_operations * i_fop ;
    struct super_block * i_sb ;
    struct file_lock * i_flock ;
    struct address_space * i_mapping ;
    struct address_space i_data ;
    struct dquot * i_dquot[2] ;
    struct list_head i_devices ;
    union {
        struct pipe_inode_info * i_pipe ;
        struct block_device * i_bdev ;
        struct cdev * i_cdev ;
    };
    __u32 i_generation ;
    __u32 i_fsnotify_mask ;
    struct hlist_head i_fsnotify_mark_entries ;
    struct list_head inotify_watches ;
    struct mutex inotify_mutex ;
    unsigned long i_state ;
    unsigned long dirtied_when ;
    unsigned int i_flags ;
    atomic_t i_writecount ;
    void * i_security ;
    struct posix_acl * i_acl ;
    struct posix_acl * i_default_acl ;
    void * i_private ;
};

typedef unsigned int __mode_t ;
typedef __mode_t mode_t ;

typedef int ctl_handler (struct ctl_table * table , void * oldval , size_t * oldlenp , void * newval , size_t newlen ) ;

struct ctl_table {
    int ctl_name ;
    const char * procname ;
    void * data ;
    int maxlen ;
    mode_t mode ;
    struct ctl_table * child ;
    struct ctl_table * parent ;
    proc_handler * proc_handler ;
    ctl_handler * strategy ;
    void * extra1 ;
    void * extra2 ;
};

typedef __u32 __be32 ;


typedef struct journal_header_s {
    __be32 h_magic ;
    __be32 h_blocktype ;
    __be32 h_sequence ;
}journal_header_t ;

typedef struct journal_superblock_s {
    journal_header_t s_header ;
    __be32 s_blocksize ;
    __be32 s_maxlen ;
    __be32 s_first ;
    __be32 s_sequence ;
    __be32 s_start ;
    __be32 s_errno ;
    __be32 s_feature_compat ;
    __be32 s_feature_incompat ;
    __be32 s_feature_ro_compat ;
    __u8 s_uuid [ 16 ] ;
    __be32 s_nr_users ;
    __be32 s_dynsuper ;
    __be32 s_max_transaction ;
    __be32 s_max_trans_data ;
    __u32 s_padding [ 44 ] ;
    __u8 s_users [ 16 * 48 ] ;
}journal_superblock_t ;

typedef int __pid_t ;
typedef __pid_t pid_t ;
typedef struct transaction_s transaction_t ;
typedef unsigned int tid_t;

struct journal_s {
    unsigned long j_flags ;
    int j_errno ;
    struct buffer_head * j_sb_buffer ;
    journal_superblock_t * j_superblock ;
    int j_format_version ;
    spinlock_t j_state_lock ;
    int j_barrier_count ;
    struct mutex j_barrier ;
    transaction_t * j_running_transaction ;
    transaction_t * j_committing_transaction ;
    transaction_t * j_checkpoint_transactions ;
    wait_queue_head_t j_wait_transaction_locked ;
    wait_queue_head_t j_wait_logspace ;
    wait_queue_head_t j_wait_done_commit ;
    wait_queue_head_t j_wait_checkpoint ;
    wait_queue_head_t j_wait_commit ;
    wait_queue_head_t j_wait_updates ;
    struct mutex j_checkpoint_mutex ;
    unsigned int j_head ;
    unsigned int j_tail ;
    unsigned int j_free ;
    unsigned int j_first ;
    unsigned int j_last ;
    struct block_device * j_dev ;
    int j_blocksize ;
    unsigned int j_blk_offset ;
    struct block_device * j_fs_dev ;
    unsigned int j_maxlen ;
    spinlock_t j_list_lock ;
    struct inode * j_inode ;
    tid_t j_tail_sequence ;
    tid_t j_transaction_sequence ;
    tid_t j_commit_sequence ;
    tid_t j_commit_request ;
    __u8 j_uuid [ 16 ] ;
    struct task_struct * j_task ;
    int j_max_transaction_buffers ;
    unsigned long j_commit_interval ;
    struct timer_list j_commit_timer ;
    spinlock_t j_revoke_lock ;
    struct jbd_revoke_table_s * j_revoke ;
    struct jbd_revoke_table_s * j_revoke_table[2] ;
    struct buffer_head **j_wbuf ;
    int j_wbufsize ;
    pid_t j_last_sync_writer ;
    u64 j_average_commit_time ;
    void * j_private ;
};

extern int proc_nmi_enabled ( struct ctl_table * table , int write , void * buffer , size_t * length , loff_t * ppos ) ;
void proc_sched_show_task ( struct task_struct * p , struct seq_file * m ) ;
void proc_sched_set_task ( struct task_struct * p ) ;
int proc_dostring ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_dointvec ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_dointvec_minmax ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_doulongvec_minmax ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_doulongvec_ms_jiffies_minmax ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_dointvec_jiffies ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_dointvec_userhz_jiffies ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_dointvec_ms_jiffies ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
void create_prof_cpu_mask ( struct proc_dir_entry * root_irq_dir ) ;
int proc_dosoftlockup_thresh ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_dohung_task_timeout_secs ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
int proc_nr_files ( struct ctl_table * table , int write , void * buffer , size_t * lenp , loff_t * ppos ) ;
void pde_users_dec ( struct proc_dir_entry * pde ) ;
struct inode * proc_get_inode ( struct super_block * sb , unsigned int ino , struct proc_dir_entry * de ) ;
int proc_fill_super ( struct super_block * s ) ;
void proc_flush_task ( struct task_struct * task ) ;
struct dentry * proc_pid_lookup ( struct inode * dir , struct dentry * dentry , struct nameidata * nd ) ;

struct dentry * proc_lookup_de ( struct proc_dir_entry * de , struct inode * dir , struct dentry * dentry ) ;
struct dentry * proc_lookup ( struct inode * dir , struct dentry * dentry , struct nameidata * nd ) ;

struct proc_dir_entry * proc_symlink ( const char * name , struct proc_dir_entry * parent , const char * dest ) ;

struct proc_dir_entry * proc_mkdir_mode ( const char * name , mode_t mode , struct proc_dir_entry * parent ) ;
struct proc_dir_entry * proc_net_mkdir ( struct net * net , const char * name , struct proc_dir_entry * parent ) ;
struct proc_dir_entry * proc_mkdir ( const char * name , struct proc_dir_entry * parent ) ;
struct proc_dir_entry * create_proc_entry ( const char * name , mode_t mode , struct proc_dir_entry * parent ) ;
struct proc_dir_entry * proc_create_data ( const char * name , mode_t mode , struct proc_dir_entry * parent , const struct file_operations * proc_fops , void * data ) ;
void free_proc_entry ( struct proc_dir_entry * de ) ;
void remove_proc_entry ( const char * name , struct proc_dir_entry * parent ) ;
int proc_pid_status ( struct seq_file * m , struct pid_namespace * ns , struct pid * pid , struct task_struct * task ) ;
int proc_tid_stat ( struct seq_file * m , struct pid_namespace * ns , struct pid * pid , struct task_struct * task ) ;
int proc_tgid_stat ( struct seq_file * m , struct pid_namespace * ns , struct pid * pid , struct task_struct * task ) ;
int proc_pid_statm ( struct seq_file * m , struct pid_namespace * ns , struct pid * pid , struct task_struct * task ) ;
void proc_tty_register_driver ( struct tty_driver * driver ) ;
void proc_tty_unregister_driver ( struct tty_driver * driver ) ;
void proc_net_remove ( struct net * net , const char * name ) ;
void proc_clear_tty ( struct task_struct * p ) ;
void proc_fork_connector ( struct task_struct * task ) ;
void proc_exec_connector ( struct task_struct * task ) ;
void proc_id_connector ( struct task_struct * task , int which_id ) ;
void proc_sid_connector ( struct task_struct * task ) ;
void proc_exit_connector ( struct task_struct * task ) ;
struct proc_dir_entry * proc_net_fops_create ( struct net * net , const char * name , mode_t mode , const struct file_operations * fops ) ;



int eth_header ( struct sk_buff * skb , struct net_device * dev , unsigned short type , const void * daddr , const void * saddr , unsigned len ) ;
int eth_rebuild_header ( struct sk_buff * skb ) ;
int eth_header_parse ( const struct sk_buff * skb , unsigned char * haddr ) ;
int eth_header_cache ( const struct neighbour * neigh , struct hh_cache * hh ) ;
void eth_header_cache_update ( struct hh_cache * hh , const struct net_device * dev , const unsigned char * haddr ) ;
int eth_mac_addr ( struct net_device * dev , void * p ) ;
int eth_change_mtu ( struct net_device * dev , int new_mtu ) ;
int eth_validate_addr ( struct net_device * dev ) ;

//-------------------------------------------------------------------------------------------



int vfs_statfs ( struct dentry * dentry , struct kstatfs * buf ) ;
loff_t vfs_llseek ( struct file * file , loff_t offset , int origin ) ;
ssize_t vfs_read ( struct file * file , char * buf , size_t count , loff_t * pos ) ;
ssize_t vfs_write ( struct file * file , const char * buf , size_t count , loff_t * pos ) ;
ssize_t vfs_readv ( struct file * file , const struct iovec * vec , unsigned long vlen , loff_t * pos ) ;
ssize_t vfs_writev ( struct file * file , const struct iovec * vec , unsigned long vlen , loff_t * pos ) ;
struct vfsmount * vfs_kern_mount ( struct file_system_type * type , int flags , const char * name , void * data ) ;
int vfs_getattr ( struct vfsmount * mnt , struct dentry * dentry , struct kstat * stat ) ;
int vfs_fstat ( unsigned int fd , struct kstat * stat ) ;
int vfs_fstatat ( int dfd , char * filename , struct kstat * stat , int flag ) ;
int vfs_stat ( char * name , struct kstat * stat ) ;
int vfs_lstat ( char * name , struct kstat * stat ) ;
int vfs_path_lookup ( struct dentry * dentry , struct vfsmount * mnt , const char * name , unsigned int flags , struct nameidata * nd ) ;
int vfs_create ( struct inode * dir , struct dentry * dentry , int mode , struct nameidata * nd ) ;
int vfs_mknod ( struct inode * dir , struct dentry * dentry , int mode , dev_t dev ) ;
int vfs_mkdir ( struct inode * dir , struct dentry * dentry , int mode ) ;
int vfs_rmdir ( struct inode * dir , struct dentry * dentry ) ;
int vfs_unlink ( struct inode * dir , struct dentry * dentry ) ;
int vfs_symlink ( struct inode * dir , struct dentry * dentry , const char * oldname ) ;
int vfs_link ( struct dentry * old_dentry , struct inode * dir , struct dentry * new_dentry ) ;
int vfs_rename ( struct inode * old_dir , struct dentry * old_dentry , struct inode * new_dir , struct dentry * new_dentry ) ;
int vfs_readlink ( struct dentry * dentry , char * buffer , int buflen , const char * link ) ;
int vfs_follow_link ( struct nameidata * nd , const char * link ) ;
int vfs_readdir ( struct file * file , filldir_t filler , void * buf ) ;
//void __attribute__ ( ( __section__ (  ) ) ) __attribute__ ( ( __cold__ ) ) __attribute__ ( ( no_instrument_function ) ) vfs_caches_init_early ( void ) ;
//void __attribute__ ( ( __section__ (  ) ) ) __attribute__ ( ( __cold__ ) ) __attribute__ ( ( no_instrument_function ) ) vfs_caches_init ( unsigned long mempages ) ;
int vfs_setxattr ( struct dentry * dentry , const char * name , const void * value , size_t size , int flags ) ;
ssize_t vfs_getxattr ( struct dentry * dentry , const char * name , void * value , size_t size ) ;
ssize_t vfs_listxattr ( struct dentry * d , char * list , size_t size ) ;
int vfs_removexattr ( struct dentry * dentry , const char * name ) ;
int vfs_fsync_range ( struct file * file , struct dentry * dentry , loff_t start , loff_t end , int datasync ) ;
int vfs_fsync ( struct file * file , struct dentry * dentry , int datasync ) ;
int vfs_quota_sync ( struct super_block * sb , int type ) ;
void vfs_dq_drop ( struct inode * inode ) ;
int vfs_dq_transfer ( struct inode * inode , struct iattr * iattr ) ;
int vfs_quota_disable ( struct super_block * sb , int type , unsigned int flags ) ;
int vfs_quota_off ( struct super_block * sb , int type , int remount ) ;
int vfs_quota_on_path ( struct super_block * sb , int type , int format_id , struct path * path ) ;
int vfs_quota_on ( struct super_block * sb , int type , int format_id , char * name , int remount ) ;
int vfs_quota_enable ( struct inode * inode , int type , int format_id , unsigned int flags ) ;
int vfs_quota_on_mount ( struct super_block * sb , char * qf_name , int format_id , int type ) ;
int vfs_dq_quota_on_remount ( struct super_block * sb ) ;
int vfs_get_dqblk ( struct super_block * sb , int type , qid_t id , struct if_dqblk * di ) ;
int vfs_set_dqblk ( struct super_block * sb , int type , qid_t id , struct if_dqblk * di ) ;
int vfs_get_dqinfo ( struct super_block * sb , int type , struct if_dqinfo * ii ) ;
int vfs_set_dqinfo ( struct super_block * sb , int type , struct if_dqinfo * ii ) ;
int vfs_setlease ( struct file * filp , long arg , struct file_lock * * lease ) ;
int vfs_test_lock ( struct file * filp , struct file_lock * fl ) ;
int vfs_lock_file ( struct file * filp , unsigned int cmd , struct file_lock * fl , struct file_lock * conf ) ;
int vfs_cancel_lock ( struct file * filp , struct file_lock * fl ) ;

typedef __signed__ int __s32 ;

void input_event ( struct input_dev * dev , unsigned int type , unsigned int code , int value ) ;
void input_inject_event ( struct input_handle * handle , unsigned int type , unsigned int code , int value ) ;
int input_grab_device ( struct input_handle * handle ) ;
void input_release_device ( struct input_handle * handle ) ;
int input_filter_device ( struct input_handle * handle ) ;
void input_unfilter_device ( struct input_handle * handle ) ;
int input_open_device ( struct input_handle * handle ) ;
int input_flush_device ( struct input_handle * handle , struct file * file ) ;
void input_close_device ( struct input_handle * handle ) ;
int input_get_keycode ( struct input_dev * dev , int scancode , int * keycode ) ;
int input_set_keycode ( struct input_dev * dev , int scancode , int keycode ) ;
union input_seq_state { struct { unsigned short pos ; bool mutex_acquired ; } ; void * p ; } ;
struct input_dev * input_allocate_device ( void ) ;
void input_free_device ( struct input_dev * dev ) ;
void input_set_capability ( struct input_dev * dev , unsigned int type , unsigned int code ) ;
int input_register_device ( struct input_dev * dev ) ;
void input_unregister_device ( struct input_dev * dev ) ;
int input_register_handler ( struct input_handler * handler ) ;
void input_unregister_handler ( struct input_handler * handler ) ;
int input_register_handle ( struct input_handle * handle ) ;
void input_unregister_handle ( struct input_handle * handle ) ;
int input_event_from_user ( const char * buffer , struct input_event * event ) ;
int input_event_to_user ( char * buffer , const struct input_event * event ) ;
int input_ff_effect_from_user ( const char * buffer , size_t size , struct ff_effect * effect ) ;
int input_ff_upload ( struct input_dev * dev , struct ff_effect * effect , struct file * file ) ;
int input_ff_erase ( struct input_dev * dev , int effect_id , struct file * file ) ;
int input_ff_event ( struct input_dev * dev , unsigned int type , unsigned int code , int value ) ;
int input_ff_create ( struct input_dev * dev , int max_effects ) ;
void input_ff_destroy ( struct input_dev * dev ) ;
void add_input_randomness ( unsigned int type , unsigned int code , unsigned int value ) ;


//-----------------------------------------------------------------



enum sock_shutdown_cmd { sock_shutdown_cmd_SHUT_RD = 0 , sock_shutdown_cmd_SHUT_WR = 1 , sock_shutdown_cmd_SHUT_RDWR = 2 , } ;
typedef void ( * poll_queue_proc ) ( struct file * , wait_queue_head_t * , struct poll_table_struct * ) ;
typedef struct poll_table_struct { poll_queue_proc qproc ; unsigned long key ; } poll_table ;

int sock_map_fd ( struct socket * sock , int flags ) ;
void sock_release ( struct socket * sock ) ;
int sock_tx_timestamp ( struct msghdr * msg , struct sock * sk , union skb_shared_tx * shtx ) ;
int sock_sendmsg ( struct socket * sock , struct msghdr * msg , size_t size ) ;
int sock_recvmsg ( struct socket * sock , struct msghdr * msg , size_t size , int flags ) ;
int sock_create_lite ( int family , int type , int protocol , struct socket * * res ) ;
int sock_wake_async ( struct socket * sock , int how , int band ) ;
int sock_create ( int family , int type , int protocol , struct socket * * res ) ;
int sock_create_kern ( int family , int type , int protocol , struct socket * * res ) ;
int sock_register ( const struct net_proto_family * ops ) ;
void sock_unregister ( int family ) ;
extern int kernel_sock_shutdown ( struct socket * sock , enum sock_shutdown_cmd how ) ;
int sock_queue_rcv_skb ( struct sock * sk , struct sk_buff * skb ) ;
int sock_setsockopt ( struct socket * sock , int level , int optname , char * optval , unsigned int optlen ) ;
int sock_getsockopt ( struct socket * sock , int level , int optname , char * optval , int * optlen ) ;
void sock_wfree ( struct sk_buff * skb ) ;
void sock_rfree ( struct sk_buff * skb ) ;
int sock_i_uid ( struct sock * sk ) ;
unsigned long sock_i_ino ( struct sock * sk ) ;
struct sk_buff * sock_wmalloc ( struct sock * sk , unsigned long size , int force , gfp_t priority ) ;
struct sk_buff * sock_rmalloc ( struct sock * sk , unsigned long size , int force , gfp_t priority ) ;
void * sock_kmalloc ( struct sock * sk , int size , gfp_t priority ) ;
void sock_kfree_s ( struct sock * sk , void * mem , int size ) ;
struct sk_buff * sock_alloc_send_pskb ( struct sock * sk , unsigned long header_len , unsigned long data_len , int noblock , int * errcode ) ;
struct sk_buff * sock_alloc_send_skb ( struct sock * sk , unsigned long size , int noblock , int * errcode ) ;
int sock_no_bind ( struct socket * sock , struct sockaddr * saddr , int len ) ;
int sock_no_connect ( struct socket * sock , struct sockaddr * saddr , int len , int flags ) ;
int sock_no_socketpair ( struct socket * sock1 , struct socket * sock2 ) ;
int sock_no_accept ( struct socket * sock , struct socket * newsock , int flags ) ;
int sock_no_getname ( struct socket * sock , struct sockaddr * saddr , int * len , int peer ) ;
unsigned int sock_no_poll ( struct file * file , struct socket * sock , poll_table * pt ) ;
int sock_no_ioctl ( struct socket * sock , unsigned int cmd , unsigned long arg ) ;
int sock_no_listen ( struct socket * sock , int backlog ) ;
int sock_no_shutdown ( struct socket * sock , int how ) ;
int sock_no_setsockopt ( struct socket * sock , int level , int optname , char * optval , unsigned int optlen ) ;
int sock_no_getsockopt ( struct socket * sock , int level , int optname , char * optval , int * optlen ) ;
int sock_no_sendmsg ( struct kiocb * iocb , struct socket * sock , struct msghdr * m , size_t len ) ;
int sock_no_recvmsg ( struct kiocb * iocb , struct socket * sock , struct msghdr * m , size_t len , int flags ) ;
int sock_no_mmap ( struct file * file , struct socket * sock , struct vm_area_struct * vma ) ;
ssize_t sock_no_sendpage ( struct socket * sock , struct page * page , int offset , size_t size , int flags ) ;
void sock_init_data ( struct socket * sock , struct sock * sk ) ;
int sock_get_timestamp ( struct sock * sk , struct timeval * userstamp ) ;
int sock_get_timestampns ( struct sock * sk , struct timespec * userstamp ) ;
void sock_enable_timestamp ( struct sock * sk , int flag ) ;
int sock_common_getsockopt ( struct socket * sock , int level , int optname , char * optval , int * optlen ) ;
int sock_common_recvmsg ( struct kiocb * iocb , struct socket * sock , struct msghdr * msg , size_t size , int flags ) ;
int sock_common_setsockopt ( struct socket * sock , int level , int optname , char * optval , unsigned int optlen ) ;
void sock_prot_inuse_add ( struct net * net , struct proto * prot , int val ) ;
int sock_prot_inuse_get ( struct net * net , struct proto * prot ) ;




void simple_set_mnt ( struct vfsmount * mnt , struct super_block * sb ) ;
int simple_getattr ( struct vfsmount * mnt , struct dentry * dentry , struct kstat * stat ) ;
int simple_statfs ( struct dentry * dentry , struct kstatfs * buf ) ;
struct dentry * simple_lookup ( struct inode * dir , struct dentry * dentry , struct nameidata * nd ) ;
int simple_sync_file ( struct file * file , struct dentry * dentry , int datasync ) ;
int simple_link ( struct dentry * old_dentry , struct inode * dir , struct dentry * dentry ) ;
int simple_empty ( struct dentry * dentry ) ;
int simple_unlink ( struct inode * dir , struct dentry * dentry ) ;
int simple_rmdir ( struct inode * dir , struct dentry * dentry ) ;
int simple_rename ( struct inode * old_dir , struct dentry * old_dentry , struct inode * new_dir , struct dentry * new_dentry ) ;
int simple_readpage ( struct file * file , struct page * page ) ;
extern int simple_prepare_write ( struct file * file , struct page * page , unsigned from , unsigned to ) ;
int simple_write_begin ( struct file * file , struct address_space * mapping , loff_t pos , unsigned len , unsigned flags , struct page * * pagep , void * * fsdata ) ;
int simple_write_end ( struct file * file , struct address_space * mapping , loff_t pos , unsigned len , unsigned copied , struct page * page , void * fsdata ) ;
int simple_fill_super ( struct super_block * s , unsigned long magic , struct tree_descr * files ) ;
int simple_pin_fs ( struct file_system_type * type , struct vfsmount * * mount , int * count ) ;
void simple_release_fs ( struct vfsmount * * mount , int * count ) ;
ssize_t simple_read_from_buffer ( void * to , size_t count , loff_t * ppos , const void * from , size_t available ) ;
void simple_transaction_set ( struct file * file , size_t n ) ;
char * simple_transaction_get ( struct file * file , const char * buf , size_t size ) ;
ssize_t simple_transaction_read ( struct file * file , char * buf , size_t size , loff_t * pos ) ;
int simple_transaction_release ( struct inode * inode , struct file * file ) ;
int simple_attr_open ( struct inode * inode , struct file * file , int ( * get ) ( void * , u64 * ) , int ( * set ) ( void * , u64 ) , const char * fmt ) ;
int simple_attr_release ( struct inode * inode , struct file * file ) ;
ssize_t simple_attr_read ( struct file * file , char * buf , size_t len , loff_t * ppos ) ;
ssize_t simple_attr_write ( struct file * file , const char * buf , size_t len , loff_t * ppos ) ;
extern int simple_fsync ( struct file * file , struct dentry * dentry , int datasync ) ;
extern unsigned long simple_strtoul ( const char * cp , char * * endp , unsigned int base ) ;
extern long simple_strtol ( const char * cp , char * * endp , unsigned int base ) ;
extern unsigned long long simple_strtoull ( const char * cp , char * * endp , unsigned int base ) ;
extern long long simple_strtoll ( const char * cp , char * * endp , unsigned int base ) ;


int sata_down_spd_limit ( struct ata_link * link , u32 spd_limit ) ;
int sata_set_spd ( struct ata_link * link ) ;
int sata_link_debounce ( struct ata_link * link , const unsigned long * params , unsigned long deadline ) ;
int sata_link_resume ( struct ata_link * link , const unsigned long * params , unsigned long deadline ) ;
int sata_link_hardreset ( struct ata_link * link , const unsigned long * timing , unsigned long deadline , bool * online , int ( * check_ready ) ( struct ata_link * ) ) ;
extern int sata_std_hardreset ( struct ata_link * link , unsigned int * class_ele , unsigned long deadline ) ;
int sata_scr_valid ( struct ata_link * link ) ;
int sata_scr_read ( struct ata_link * link , int reg , u32 * val ) ;
int sata_scr_write ( struct ata_link * link , int reg , u32 val ) ;
int sata_scr_write_flush ( struct ata_link * link , int reg , u32 val ) ;
int sata_link_init_spd ( struct ata_link * link ) ;
int sata_async_notification ( struct ata_port * ap ) ;
extern int sata_sff_hardreset ( struct ata_link * link , unsigned int * class_ele , unsigned long deadline ) ;
extern int sata_pmp_qc_defer_cmd_switch ( struct ata_queued_cmd * qc ) ;
extern void sata_pmp_error_handler ( struct ata_port * ap ) ;


struct dentry * debugfs_create_file ( const char * name , mode_t mode , struct dentry * parent , void * data , const struct file_operations * fops ) ;
struct dentry * debugfs_create_dir ( const char * name , struct dentry * parent ) ;
struct dentry * debugfs_create_symlink ( const char * name , struct dentry * parent , const char * target ) ;
void debugfs_remove ( struct dentry * dentry ) ;
void debugfs_remove_recursive ( struct dentry * dentry ) ;
struct dentry * debugfs_rename ( struct dentry * old_dir , struct dentry * old_dentry , struct dentry * new_dir , const char * new_name ) ;
bool debugfs_initialized ( void ) ;
struct dentry * debugfs_create_u8 ( const char * name , mode_t mode , struct dentry * parent , u8 * value ) ;
struct dentry * debugfs_create_u16 ( const char * name , mode_t mode , struct dentry * parent , u16 * value ) ;
struct dentry * debugfs_create_u32 ( const char * name , mode_t mode , struct dentry * parent , u32 * value ) ;
struct dentry * debugfs_create_u64 ( const char * name , mode_t mode , struct dentry * parent , u64 * value ) ;
struct dentry * debugfs_create_x8 ( const char * name , mode_t mode , struct dentry * parent , u8 * value ) ;
struct dentry * debugfs_create_x16 ( const char * name , mode_t mode , struct dentry * parent , u16 * value ) ;
struct dentry * debugfs_create_x32 ( const char * name , mode_t mode , struct dentry * parent , u32 * value ) ;
struct dentry * debugfs_create_size_t ( const char * name , mode_t mode , struct dentry * parent , size_t * value ) ;
struct dentry * debugfs_create_bool ( const char * name , mode_t mode , struct dentry * parent , u32 * value ) ;
struct dentry * debugfs_create_blob ( const char * name , mode_t mode , struct dentry * parent , struct debugfs_blob_wrapper * blob ) ;



void * pnp_alloc ( long size ) ;
int pnp_register_protocol ( struct pnp_protocol * protocol ) ;
void pnp_unregister_protocol ( struct pnp_protocol * protocol ) ;
void pnp_free_resource ( struct pnp_resource * pnp_res ) ;
void pnp_free_resources ( struct pnp_dev * dev ) ;
struct pnp_dev * pnp_alloc_dev ( struct pnp_protocol * protocol , int id , char * pnpid ) ;
int __pnp_add_device ( struct pnp_dev * dev ) ;
int pnp_add_device ( struct pnp_dev * dev ) ;
void __pnp_remove_device ( struct pnp_dev * dev ) ;
struct pnp_card * pnp_alloc_card ( struct pnp_protocol * protocol , int id , char * pnpid ) ;
int pnp_add_card ( struct pnp_card * card ) ;
void pnp_remove_card ( struct pnp_card * card ) ;
int pnp_add_card_device ( struct pnp_card * card , struct pnp_dev * dev ) ;
void pnp_remove_card_device ( struct pnp_dev * dev ) ;
struct pnp_dev * pnp_request_card_device ( struct pnp_card_link * clink , const char * id , struct pnp_dev * from ) ;
void pnp_release_card_device ( struct pnp_dev * dev ) ;
int pnp_register_card_driver ( struct pnp_card_driver * drv ) ;
void pnp_unregister_card_driver ( struct pnp_card_driver * drv ) ;
int compare_pnp_id ( struct pnp_id * pos , const char * id ) ;
int pnp_device_attach ( struct pnp_dev * pnp_dev ) ;
void pnp_device_detach ( struct pnp_dev * pnp_dev ) ;
int pnp_register_driver ( struct pnp_driver * drv ) ;
void pnp_unregister_driver ( struct pnp_driver * drv ) ;
struct pnp_id * pnp_add_id ( struct pnp_dev * dev , char * id ) ;
struct pnp_option * pnp_build_option ( struct pnp_dev * dev , unsigned long type , unsigned int option_flags ) ;
//int pnp_register_irq_resource ( struct pnp_dev * dev , unsigned int option_flags , pnp_irq_mask_t * map , unsigned char flags ) ;
int pnp_register_dma_resource ( struct pnp_dev * dev , unsigned int option_flags , unsigned char map , unsigned char flags ) ;
int pnp_register_port_resource ( struct pnp_dev * dev , unsigned int option_flags , resource_size_t min , resource_size_t max , resource_size_t align , resource_size_t size , unsigned char flags ) ;
int pnp_register_mem_resource ( struct pnp_dev * dev , unsigned int option_flags , resource_size_t min , resource_size_t max , resource_size_t align , resource_size_t size , unsigned char flags ) ;
void pnp_free_options ( struct pnp_dev * dev ) ;
int pnp_check_port ( struct pnp_dev * dev , struct resource * res ) ;
int pnp_check_mem ( struct pnp_dev * dev , struct resource * res ) ;
int pnp_check_irq ( struct pnp_dev * dev , struct resource * res ) ;
int pnp_check_dma ( struct pnp_dev * dev , struct resource * res ) ;
unsigned long pnp_resource_type ( struct resource * res ) ;
struct resource * pnp_get_resource ( struct pnp_dev * dev , unsigned long type , unsigned int num ) ;
struct pnp_resource * pnp_add_irq_resource ( struct pnp_dev * dev , int irq , int flags ) ;
struct pnp_resource * pnp_add_dma_resource ( struct pnp_dev * dev , int dma , int flags ) ;
struct pnp_resource * pnp_add_io_resource ( struct pnp_dev * dev , resource_size_t start , resource_size_t end , int flags ) ;
struct pnp_resource * pnp_add_mem_resource ( struct pnp_dev * dev , resource_size_t start , resource_size_t end , int flags ) ;
int pnp_possible_config ( struct pnp_dev * dev , int type , resource_size_t start , resource_size_t size ) ;
int pnp_range_reserved ( resource_size_t start , resource_size_t end ) ;
void pnp_init_resources ( struct pnp_dev * dev ) ;
int pnp_auto_config_dev ( struct pnp_dev * dev ) ;
int pnp_start_dev ( struct pnp_dev * dev ) ;
int pnp_stop_dev ( struct pnp_dev * dev ) ;
int pnp_activate_dev ( struct pnp_dev * dev ) ;
int pnp_disable_dev ( struct pnp_dev * dev ) ;
int pnp_is_active ( struct pnp_dev * dev ) ;
void pnp_eisa_id_to_string ( u32 id , char * str ) ;
char * pnp_resource_type_name ( struct resource * res ) ;
void dbg_pnp_show_resources ( struct pnp_dev * dev , char * desc ) ;
char *pnp_option_priority_name ( struct pnp_option * option ) ;
void dbg_pnp_show_option ( struct pnp_dev * dev , struct pnp_option * option ) ;
void pnp_fixup_device ( struct pnp_dev * dev ) ;

enum dmi_field { DMI_NONE , DMI_BIOS_VENDOR , DMI_BIOS_VERSION , DMI_BIOS_DATE , DMI_SYS_VENDOR , DMI_PRODUCT_NAME , DMI_PRODUCT_VERSION , DMI_PRODUCT_SERIAL , DMI_PRODUCT_UUID , DMI_BOARD_VENDOR , DMI_BOARD_NAME , DMI_BOARD_VERSION , DMI_BOARD_SERIAL , DMI_BOARD_ASSET_TAG , DMI_CHASSIS_VENDOR , DMI_CHASSIS_TYPE , DMI_CHASSIS_VERSION , DMI_CHASSIS_SERIAL , DMI_CHASSIS_ASSET_TAG , DMI_STRING_MAX , } ;
/*--dmi_field--*/
int dmi_check_system ( const struct dmi_system_id * list ) ;
const struct dmi_system_id * dmi_first_match ( const struct dmi_system_id * list ) ;
const char * dmi_get_system_info ( int field ) ;
int dmi_name_in_serial ( const char * str ) ;
int dmi_name_in_vendors ( const char * str ) ;
const struct dmi_device * dmi_find_device ( int type , const char * name , const struct dmi_device * from ) ;
bool dmi_get_date ( int field , int * yearp , int * monthp , int * dayp ) ;
int dmi_walk ( void ( * decode ) ( const struct dmi_header * , void * ) , void * private_data ) ;
bool dmi_match ( enum dmi_field f , const char * str ) ;

/*
struct module {
    enum module_state state ;
    struct list_head list ;
    char name [ ( 64 - sizeof ( unsigned long ) ) ] ;
    struct module_kobject mkobj ;
    struct module_attribute * modinfo_attrs ;
    const char * version ;
    const char * srcversion ;
    struct kobject * holders_dir ;
    const struct kernel_symbol * syms ;
    const unsigned long * crcs ;
    unsigned int num_syms ;
    struct kernel_param * kp ;
    unsigned int num_kp ;
    unsigned int num_gpl_syms ;
    const struct kernel_symbol * gpl_syms ;
    const unsigned long * gpl_crcs ;
    const struct kernel_symbol * unused_syms ;
    const unsigned long * unused_crcs ;
    unsigned int num_unused_syms ;
    unsigned int num_unused_gpl_syms ;
    const struct kernel_symbol * unused_gpl_syms ;
    const unsigned long * unused_gpl_crcs ;
    const struct kernel_symbol * gpl_future_syms ;
    const unsigned long * gpl_future_crcs ;
    unsigned int num_gpl_future_syms ;
    unsigned int num_exentries ;
    struct exception_table_entry * extable ;
    int ( * init ) ( void ) ;
    void * module_init ;
    void * module_core ;
    unsigned int init_size , core_size ;
    unsigned int init_text_size , core_text_size ;
    struct mod_arch_specific arch ;
    unsigned int taints ;
    unsigned num_bugs ;
    struct list_head bug_list ;
    struct bug_entry * bug_table ;
    Elf64_Sym * symtab , * core_symtab ;
    unsigned int num_symtab , core_num_syms ;
    char * strtab , * core_strtab ;
    struct module_sect_attrs * sect_attrs ;
    struct module_notes_attrs * notes_attrs ;
    void * percpu ;
    char * args ;
    struct tracepoint * tracepoints ;
    unsigned int num_tracepoints ;
    const char **trace_bprintk_fmt_start ;
    unsigned int num_trace_bprintk_fmt ;
    struct ftrace_event_call * trace_events ;
    unsigned int num_trace_events ;
    unsigned long * ftrace_callsites ;
    unsigned int num_ftrace_callsites ;
    struct list_head modules_which_use_me ;
    struct task_struct * waiter ;
    void ( * exit ) ( void ) ;
    char * refptr ;
    ctor_fn_t * ctors ;
    unsigned int num_ctors ;
};

*/


typedef void (*bh_end_io_t)( struct buffer_head * bh , int uptodate );



typedef struct { char * from ; char * to ; } substring_t ;

typedef int ( *get_block_t)( struct inode * inode , sector_t iblock , struct buffer_head * bh_result , int create ) ;

typedef int ( * read_actor_t ) ( read_descriptor_t * , struct page * , unsigned long , unsigned long ) ;
typedef void ( *dio_iodone_t ) ( struct kiocb * iocb , loff_t offset , ssize_t bytes , void * private_data ) ;

typedef struct {
    unsigned long int __val[(1024/(8*sizeof(unsigned long int)))];
} __sigset_t ;

typedef __sigset_t sigset_t ;

typedef int ( * notifier ) ( void * priv );
typedef signed int s32 ;

struct buffer_head {
    unsigned long b_state ;
    struct buffer_head * b_this_page ;
    struct page * b_page ;
    sector_t b_blocknr ;
    size_t b_size ;
    char * b_data ;
    struct block_device * b_bdev ;
    bh_end_io_t b_end_io ;
    void * b_private ;
    struct list_head b_assoc_buffers ;
    struct address_space * b_assoc_map ;
    atomic_t b_count ;
};









typedef void (*ctor)( void * );
typedef int (*fill_super)(struct super_block * , void * , int );

struct mb_cache_op { int ( * free ) ( struct mb_cache_entry * , gfp_t );} ;

extern int get_sb_bdev ( struct file_system_type * fs_type , int flags , const char * dev_name , void * data , fill_super fill_super_var , struct vfsmount * mnt );

extern struct kmem_cache * kmem_cache_create ( const char * name , size_t size , size_t align , unsigned long flags , ctor ctor_var ) ;
extern struct mb_cache * mb_cache_create ( const char * name , struct mb_cache_op * cache_op , size_t entry_size , int indexes_count , int bucket_bits );

extern void module_layout ( struct module * mod , struct modversion_info * ver , struct kernel_param * kp , struct kernel_symbol * ks , struct tracepoint * tp ) ;
extern int register_filesystem(struct file_system_type * fs);
extern int unregister_filesystem(struct file_system_type * fs);

//extern int sb_min_blocksize(struct super_block *sb, int size);

extern struct buffer_head * __bread ( struct block_device * bdev , sector_t block , unsigned size );
extern int bdev_read_only(struct block_device *bdev);

extern struct posix_acl *
posix_acl_alloc(int count, gfp_t flags);
extern int
posix_acl_equiv_mode(const struct posix_acl *acl, mode_t *mode_p);


typedef struct handle_s handle_t;
typedef struct journal_s journal_t;


extern handle_t *jbd2_journal_start ( journal_t * journal , int nblocks ) ;
/*--jbd2_journal_start--*/
extern int jbd2_journal_extend ( handle_t * handle , int nblocks ) ;
/*--jbd2_journal_extend--*/
extern int jbd2_journal_restart ( handle_t * handle , int nblocks ) ;
/*--jbd2_journal_restart--*/
extern void jbd2_journal_lock_updates ( journal_t * journal ) ;
/*--jbd2_journal_lock_updates--*/
extern void jbd2_journal_unlock_updates ( journal_t * journal ) ;
/*--jbd2_journal_unlock_updates--*/
extern int jbd2_journal_get_write_access ( handle_t * handle , struct buffer_head * bh ) ;
/*--jbd2_journal_get_write_access--*/
extern int jbd2_journal_get_create_access ( handle_t * handle , struct buffer_head * bh ) ;
/*--jbd2_journal_get_create_access--*/
extern int jbd2_journal_get_undo_access ( handle_t * handle , struct buffer_head * bh ) ;
/*--jbd2_journal_get_undo_access--*/
extern void jbd2_journal_set_triggers ( struct buffer_head * bh , struct jbd2_buffer_trigger_type * type ) ;
/*--jbd2_journal_set_triggers--*/
extern void jbd2_buffer_commit_trigger ( struct journal_head * jh , void * mapped_data , struct jbd2_buffer_trigger_type * triggers ) ;
/*--jbd2_buffer_commit_trigger--*/
extern void jbd2_buffer_abort_trigger ( struct journal_head * jh , struct jbd2_buffer_trigger_type * triggers ) ;
/*--jbd2_buffer_abort_trigger--*/
extern int jbd2_journal_dirty_metadata ( handle_t * handle , struct buffer_head * bh ) ;
/*--jbd2_journal_dirty_metadata--*/
extern void jbd2_journal_release_buffer ( handle_t * handle , struct buffer_head * bh ) ;
/*--jbd2_journal_release_buffer--*/
extern int jbd2_journal_forget ( handle_t * handle , struct buffer_head * bh ) ;
/*--jbd2_journal_forget--*/
extern int jbd2_journal_stop ( handle_t * handle ) ;
/*--jbd2_journal_stop--*/
extern int jbd2_journal_force_commit ( journal_t * journal ) ;
/*--__jbd2_journal_unfile_buffer--*/
extern void jbd2_journal_unfile_buffer ( journal_t * journal , struct journal_head * jh ) ;
/*--jbd2_journal_unfile_buffer--*/
extern int jbd2_journal_try_to_free_buffers ( journal_t * journal , struct page * page , gfp_t gfp_mask ) ;
/*--jbd2_journal_try_to_free_buffers--*/
extern void jbd2_journal_invalidatepage ( journal_t * journal , struct page * page , unsigned long offset ) ;
/*--jbd2_journal_invalidatepage--*/
extern void jbd2_journal_file_buffer ( struct journal_head * jh , transaction_t * transaction , int jlist ) ;
/*--jbd2_journal_file_buffer--*/
void jbd2_journal_refile_buffer ( journal_t * journal , struct journal_head * jh ) ;
/*--jbd2_journal_refile_buffer--*/
int jbd2_journal_file_inode ( handle_t * handle , struct jbd2_inode * jinode ) ;
/*--jbd2_journal_file_inode--*/
int jbd2_journal_begin_ordered_truncate ( journal_t * journal , struct jbd2_inode * jinode , loff_t new_size ) ;
/*--jbd2_jtypedef unsigned int tid_t ;ournal_begin_ordered_truncate--*/
void jbd2_journal_commit_transaction ( journal_t * journal ) ;
/*--jbd2_journal_commit_transaction--*/
int jbd2_journal_recover ( journal_t * journal ) ;
/*--jbd2_journal_recover--*/
int jbd2_journal_skip_recovery ( journal_t * journal ) ;
/*--jbd2_journal_skip_recovery--*/
int jbd2_log_do_checkpoint ( journal_t * journal ) ;
/*--jbd2_log_do_checkpoint--*/
int jbd2_cleanup_journal_tail ( journal_t * journal ) ;
/*--jbd2_cleanup_journal_tail--*/


void jbd2_journal_destroy_revoke_caches ( void ) ;
/*--jbd2_journal_destroy_revoke_caches--*/
int jbd2_journal_init_revoke_caches ( void ) ;
/*--jbd2_journal_init_revoke_caches--*/
int jbd2_journal_init_revoke ( journal_t * journal , int hash_size ) ;
/*--jbd2_journal_init_revoke--*/
void jbd2_journal_destroy_revoke ( journal_t * journal ) ;
/*--jbd2_journal_destroy_revoke--*/
int jbd2_journal_revoke ( handle_t * handle , unsigned long long blocknr , struct buffer_head * bh_in ) ;
/*--jbd2_journal_revoke--*/
int jbd2_journal_cancel_revoke ( handle_t * handle , struct journal_head * jh ) ;
/*--jbd2_journal_cancel_revoke--*/
void jbd2_journal_switch_revoke_table ( journal_t * journal ) ;
/*--jbd2_journal_switch_revoke_table--*/
void jbd2_journal_write_revoke_records ( journal_t * journal , transaction_t * transaction , int write_op ) ;
/*--jbd2_journal_write_revoke_records--*/
int jbd2_journal_set_revoke ( journal_t * journal , unsigned long long blocknr , tid_t sequence ) ;
/*--jbd2_journal_set_revoke--*/
int jbd2_journal_test_revoke ( journal_t * journal , unsigned long long blocknr , tid_t sequence ) ;
/*--jbd2_journal_test_revoke--*/
void jbd2_journal_clear_revoke ( journal_t * journal ) ;
/*--jbd2_journal_clear_revoke--*/
struct posix_acl *posix_acl_alloc(int count, gfp_t flags);

int jbd2_journal_write_metadata_buffer ( transaction_t * transaction , struct journal_head * jh_in , struct journal_head * * jh_out , unsigned long long blocknr ) ;
/*--jbd2_journal_write_metadata_buffer--*/
int jbd2_log_start_commit ( journal_t * journal , tid_t tid ) ;
/*--jbd2_log_start_commit--*/
int jbd2_journal_force_commit_nested ( journal_t * journal ) ;
/*--jbd2_journal_force_commit_nested--*/
int jbd2_journal_start_commit ( journal_t * journal , tid_t * ptid ) ;
/*--jbd2_journal_start_commit--*/
int jbd2_log_wait_commit ( journal_t * journal , tid_t tid ) ;
/*--jbd2_log_wait_commit--*/
int jbd2_journal_next_log_block ( journal_t * journal , unsigned long long * retp ) ;
/*--jbd2_journal_next_log_block--*/
int jbd2_journal_bmap ( journal_t * journal , unsigned long blocknr , unsigned long long * retp ) ;
/*--jbd2_journal_bmap--*/
struct journal_head * jbd2_journal_get_descriptor_buffer ( journal_t * journal ) ;
/*--jbd2_journal_get_descriptor_buffer--*/
struct jbd2_stats_proc_session { journal_t * journal ; struct transaction_stats_s * stats ; int start ; int max ; } ;
/*--jbd2_stats_proc_session--*/
journal_t * jbd2_journal_init_dev ( struct block_device * bdev , struct block_device * fs_dev , unsigned long long start , int len , int blocksize ) ;
/*--jbd2_journal_init_dev--*/
journal_t * jbd2_journal_init_inode ( struct inode * inode ) ;
/*--jbd2_journal_init_inode--*/
void jbd2_journal_update_superblock ( journal_t * journal , int wait ) ;
/*--jbd2_journal_update_superblock--*/
int jbd2_journal_load ( journal_t * journal ) ;
/*--jbd2_journal_load--*/
int jbd2_journal_destroy ( journal_t * journal ) ;
/*--jbd2_journal_destroy--*/
int jbd2_journal_check_used_features ( journal_t * journal , unsigned long compat , unsigned long ro , unsigned long incompat ) ;
/*--jbd2_journal_check_used_features--*/
int jbd2_journal_check_available_features ( journal_t * journal , unsigned long compat , unsigned long ro , unsigned long incompat ) ;
/*--jbd2_journal_check_available_features--*/
int jbd2_journal_set_features ( journal_t * journal , unsigned long compat , unsigned long ro , unsigned long incompat ) ;
/*--jbd2_journal_set_features--*/
void jbd2_journal_clear_features ( journal_t * journal , unsigned long compat , unsigned long ro , unsigned long incompat ) ;
/*--jbd2_journal_clear_features--*/
int jbd2_journal_update_format ( journal_t * journal ) ;
/*--jbd2_journal_update_format--*/
int jbd2_journal_flush ( journal_t * journal ) ;
/*--jbd2_journal_flush--*/
int jbd2_journal_wipe ( journal_t * journal , int write ) ;
/*--jbd2_journal_wipe--*/

void jbd2_journal_abort ( journal_t * journal , int errno ) ;
/*--jbd2_journal_abort--*/
int jbd2_journal_errno ( journal_t * journal ) ;
/*--jbd2_journal_errno--*/
int jbd2_journal_clear_err ( journal_t * journal ) ;
/*--jbd2_journal_clear_err--*/
void jbd2_journal_ack_err ( journal_t * journal ) ;
/*--jbd2_journal_ack_err--*/
int jbd2_journal_blocks_per_page ( struct inode * inode ) ;
/*--jbd2_journal_blocks_per_page--*/
extern struct journal_head * jbd2_journal_add_journal_head ( struct buffer_head * bh ) ;
/*--jbd2_journal_add_journal_head--*/
extern struct journal_head * jbd2_journal_grab_journal_head ( struct buffer_head * bh ) ;
/*--jbd2_journal_grab_journal_head--*/
extern void jbd2_journal_remove_journal_head ( struct buffer_head * bh ) ;
/*--jbd2_journal_remove_journal_head--*/
extern void jbd2_journal_put_journal_head ( struct journal_head * jh ) ;
/*--jbd2_journal_put_journal_head--*/
extern void jbd2_journal_init_jbd_inode ( struct jbd2_inode * jinode , struct inode * inode ) ;
/*--jbd2_journal_init_jbd_inode--*/
extern void jbd2_journal_release_jbd_inode ( journal_t * journal , struct jbd2_inode * jinode ) ;
/*--jbd2_journal_release_jbd_inode--*/
extern const char * jbd2_dev_to_name ( dev_t device ) ;
/*--jbd2_dev_to_name--*/

typedef void ( *smbus_alarm_callback)( void * context );

int acpi_smbus_read ( struct acpi_smb_hc * hc , u8 protocol , u8 address , u8 command , u8 * data ) ;
/*--acpi_smbus_read--*/
int acpi_smbus_write ( struct acpi_smb_hc * hc , u8 protocol , u8 address , u8 command , u8 * data , u8 length ) ;
/*--acpi_smbus_write--*/
int acpi_smbus_register_callback ( struct acpi_smb_hc * hc , smbus_alarm_callback callback , void * context ) ;
/*--acpi_smbus_register_callback--*/
int acpi_smbus_unregister_callback ( struct acpi_smb_hc * hc ) ;
/*--acpi_smbus_unregister_callback--*/

extern int inode_permission(struct inode *inode, int mask);


typedef int __clockid_t ;
typedef __clockid_t clockid_t ;

struct posix_acl_entry { short e_tag ; unsigned short e_perm ; unsigned int e_id ; } ;

struct posix_acl {
    atomic_t a_refcount ;
    unsigned int a_count ;
    struct posix_acl_entry a_entries [0];
};

extern int posix_get_coarse_res ( const clockid_t which_clock , struct timespec * tp ) ;

extern int posix_timer_event ( struct k_itimer * timr , int si_private ) ;
/*--posix_timer_event--*/
extern void register_posix_clock ( const clockid_t clock_id , struct k_clock * new_clock ) ;
/*--register_posix_clock--*/
extern int do_posix_clock_nosettime ( const clockid_t clockid , struct timespec * tp ) ;
/*--do_posix_clock_nosettime--*/
extern int do_posix_clock_nonanosleep ( const clockid_t clock , int flags , struct timespec * t , struct timespec * r ) ;
/*--do_posix_clock_nonanosleep--*/
extern int posix_cpu_clock_getres ( const clockid_t which_clock , struct timespec * tp ) ;
/*--posix_cpu_clock_getres--*/
extern int posix_cpu_clock_set ( const clockid_t which_clock , const struct timespec * tp ) ;
/*--posix_cpu_clock_set--*/
extern int posix_cpu_clock_get ( const clockid_t which_clock , struct timespec * tp ) ;
/*--posix_cpu_clock_get--*/
extern int posix_cpu_timer_create ( struct k_itimer * new_timer ) ;
/*--posix_cpu_timer_create--*/
extern int posix_cpu_timer_del ( struct k_itimer * timer ) ;
/*--posix_cpu_timer_del--*/
extern void posix_cpu_timers_exit ( struct task_struct * tsk ) ;
/*--posix_cpu_timers_exit--*/
extern void posix_cpu_timers_exit_group ( struct task_struct * tsk ) ;
/*--posix_cpu_timers_exit_group--*/
extern int posix_cpu_timer_set ( struct k_itimer * timer , int flags , struct itimerspec * new_data , struct itimerspec * old );
/*--posix_cpu_timer_set--*/
extern void posix_cpu_timer_get ( struct k_itimer * timer , struct itimerspec * itp ) ;
/*--posix_cpu_timer_get--*/
extern void posix_cpu_timer_schedule ( struct k_itimer * timer ) ;
/*--posix_cpu_timer_schedule--*/
extern void run_posix_cpu_timers ( struct task_struct * tsk ) ;
/*--run_posix_cpu_timers--*/
extern int posix_cpu_nsleep ( const clockid_t which_clock , int flags , struct timespec * rqtp , struct timespec * rmtp ) ;
/*--posix_cpu_nsleep--*/
extern long posix_cpu_nsleep_restart ( struct restart_block * restart_block ) ;

extern int posix_acl_access_exists ( struct inode * inode ) ;
/*--posix_acl_access_exists--*/
extern int posix_acl_default_exists ( struct inode * inode ) ;
/*--posix_acl_default_exists--*/
extern void posix_test_lock ( struct file * filp , struct file_lock * fl ) ;
/*--posix_test_lock--*/
extern int posix_lock_file ( struct file * filp , struct file_lock * fl , struct file_lock * conflock ) ;
/*--posix_lock_file--*/
extern int posix_lock_file_wait ( struct file * filp , struct file_lock * fl ) ;
/*--posix_lock_file_wait--*/
extern int posix_unblock_lock ( struct file * filp , struct file_lock * waiter ) ;
/*--posix_unblock_lock--*/
extern struct posix_acl * posix_acl_alloc ( int count , gfp_t flags ) ;
/*--posix_acl_alloc--*/
extern struct posix_acl * posix_acl_clone ( const struct posix_acl * acl , gfp_t flags ) ;
/*--posix_acl_clone--*/
extern int posix_acl_valid ( const struct posix_acl * acl ) ;
/*--posix_acl_valid--*/
extern int posix_acl_equiv_mode ( const struct posix_acl * acl , mode_t * mode_p ) ;
/*--posix_acl_equiv_mode--*/
extern struct posix_acl * posix_acl_from_mode ( mode_t mode , gfp_t flags ) ;
/*--posix_acl_from_mode--*/
extern int posix_acl_permission ( struct inode * inode , const struct posix_acl * acl , int want ) ;
/*--posix_acl_permission--*/
extern int posix_acl_create_masq ( struct posix_acl * acl , mode_t * mode_p ) ;
/*--posix_acl_create_masq--*/
extern int posix_acl_chmod_masq ( struct posix_acl * acl , mode_t mode ) ;
/*--posix_acl_chmod_masq--*/
extern struct posix_acl * posix_acl_from_xattr ( void * value , size_t size ) ;
/*--posix_acl_from_xattr--*/
extern int posix_acl_to_xattr ( struct posix_acl * acl , void * buffer , size_t size ) ;
/*--posix_acl_to_xattr--*/

typedef struct __wait_queue wait_queue_t ;

typedef int ( * wait_queue_func_t ) ( wait_queue_t * wait , unsigned mode , int flags , void * key ) ;

struct __wait_queue {
    unsigned int flags ;
    void * private_data ;
    wait_queue_func_t func ;
    struct list_head task_list ;
};

struct block_device {
    dev_t bd_dev ;
    struct inode * bd_inode ;
    struct super_block * bd_super ;
    int bd_openers ;
    struct mutex bd_mutex ;
    struct list_head bd_inodes ;
    void * bd_holder ;
    int bd_holders ;
    struct list_head bd_holder_list ;
    struct block_device * bd_contains ;
    unsigned bd_block_size ;
    struct hd_struct * bd_part ;
    unsigned bd_part_count ;
    int bd_invalidated ;
    struct gendisk * bd_disk ;
    struct list_head bd_list ;
    unsigned long bd_private ;
    int bd_fsfreeze_count ;
    struct mutex bd_fsfreeze_mutex ;
};

struct load_weight { unsigned long weight , inv_weight ; } ;
struct rb_node { unsigned long rb_parent_color ; struct rb_node * rb_right ; struct rb_node * rb_left ; };


struct sched_entity {
    struct load_weight load ;
    struct rb_node run_node ;
    struct list_head group_node ;
    unsigned int on_rq ;
    u64 exec_start ;
    u64 sum_exec_runtime ;
    u64 vruntime ;
    u64 prev_sum_exec_runtime ;
    u64 last_wakeup ;
    u64 avg_overlap ;
    u64 nr_migrations ;
    u64 start_runtime ;
    u64 avg_wakeup ;
    u64 avg_running ;
    u64 wait_start ;
    u64 wait_max ;
    u64 wait_count ;
    u64 wait_sum ;
    u64 iowait_count ;
    u64 iowait_sum ;
    u64 sleep_start ;
    u64 sleep_max ;
    s64 sum_sleep_runtime ;
    u64 block_start ;
    u64 block_max ;
    u64 exec_max ;
    u64 slice_max ;
    u64 nr_migrations_cold ;
    u64 nr_failed_migrations_affine ;
    u64 nr_failed_migrations_running ;
    u64 nr_failed_migrations_hot ;
    u64 nr_forced_migrations ;
    u64 nr_forced2_migrations ;
    u64 nr_wakeups ;
    u64 nr_wakeups_sync ;
    u64 nr_wakeups_migrate ;
    u64 nr_wakeups_local ;
    u64 nr_wakeups_remote ;
    u64 nr_wakeups_affine ;
    u64 nr_wakeups_affine_attempts ;
    u64 nr_wakeups_passive ;
    u64 nr_wakeups_idle ;
    struct sched_entity * parent ;
    struct cfs_rq * cfs_rq ;
    struct cfs_rq * my_q ;
};

struct sched_rt_entity {
    struct list_head run_list ;
    unsigned long timeout ;
    unsigned int time_slice ;
    int nr_cpus_allowed ;
    struct sched_rt_entity * back ;
    struct sched_rt_entity * parent ;
    struct rt_rq * rt_rq ;
    struct rt_rq * my_q ;
} ;
struct latency_record { unsigned long backtrace [ 12 ] ; unsigned int count ; unsigned long time ; unsigned long max ; } ;
typedef struct cpumask { unsigned long bits [ ( ( ( 64 ) + ( 8 * sizeof ( long ) ) - 1 ) / ( 8 * sizeof ( long ) ) ) ] ; } cpumask_t ;

struct prop_local_single { unsigned long events ; unsigned long period ; int shift ; spinlock_t lock ; } ;
typedef struct { unsigned long bits [ ( ( ( ( 1 << 6 ) ) + ( 8 * sizeof ( long ) ) - 1 ) / ( 8 * sizeof ( long ) ) ) ] ; } nodemask_t ;
typedef unsigned long cputime_t ;

typedef union sigval { int sival_int ; void * sival_ptr ; } sigval_t ;

typedef long int __clock_t ;

typedef struct siginfo {
    int si_signo ; int si_errno ; int si_code ;
    union { int _pad [ ( ( 128 / sizeof ( int ) ) - 4 ) ] ;
    struct { __pid_t si_pid ; __uid_t si_uid ; } _kill ;
    struct { int si_tid ; int si_overrun ; sigval_t si_sigval ; } _timer ;
    struct { __pid_t si_pid ; __uid_t si_uid ; sigval_t si_sigval ; } _rt ;
    struct { __pid_t si_pid ; __uid_t si_uid ; int si_status ; __clock_t si_utime ; __clock_t si_stime ; } _sigchld ;
    struct { void * si_addr ; } _sigfault ;
    struct { long int si_band ; int si_fd ; } _sigpoll ; } _sifields ;
}siginfo_t ;

struct task_io_accounting { u64 rchar ; u64 wchar ; u64 syscr ; u64 syscw ; u64 read_bytes ; u64 write_bytes ; u64 cancelled_write_bytes ; } ;

struct plist_head { struct list_head prio_list ; struct list_head node_list ; } ;

typedef struct { int mode ; } seccomp_t ;

struct sigpending { struct list_head list ; sigset_t signal ; } ;

struct desc_struct {
    union {
        struct {
            unsigned int a ;
            unsigned int b ;
        } ;
        struct {
            u16 limit0 ;
            u16 base0 ;
            unsigned base1 : 8 , type : 4 , s : 1 , dpl : 2 , p : 1 ;
            unsigned limit : 4 , avl : 1 , l : 1 , d : 1 , g : 1 , base2 : 8 ;
        } ;
    } ;
} __attribute__ ( ( packed ) ) ;

struct thread_struct {
    struct desc_struct tls_array [ 3 ] ;
    unsigned long sp0 ;
    unsigned long sp ;
    unsigned long usersp ; unsigned short es ; unsigned short ds ; unsigned short fsindex ;
    unsigned short gsindex ; unsigned long fs ; unsigned long gs ; unsigned long debugreg0 ; unsigned long debugreg1 ;
    unsigned long debugreg2 ; unsigned long debugreg3 ; unsigned long debugreg6 ; unsigned long debugreg7 ;
    unsigned long cr2 ; unsigned long trap_no ; unsigned long error_code ; union thread_xstate * xstate ;
    unsigned long * io_bitmap_ptr ; unsigned long iopl ; unsigned io_bitmap_max ; unsigned long debugctlmsr ;
    struct ds_context * ds_ctx ;
} ;

struct sched_info { unsigned long pcount ; unsigned long long run_delay ; unsigned long long last_arrival , last_queued ; unsigned int bkl_count ; } ;

struct sysv_sem { struct sem_undo_list * undo_list ; } ;

struct plist_node { int prio ; struct plist_head plist ; } ;

enum pid_type { PIDTYPE_PID , PIDTYPE_PGID , PIDTYPE_SID , PIDTYPE_MAX } ;

struct pid_link { struct hlist_node node ; struct pid * pid ; } ;

struct task_cputime { cputime_t utime ; cputime_t stime ; unsigned long long sum_exec_runtime ; } ;

struct task_struct {
    volatile long state ;
    void * stack ;
    atomic_t usage ;
    unsigned int flags ;
    unsigned int ptrace ;
    int lock_depth ;
    int prio , static_prio , normal_prio ;
    unsigned int rt_priority ;
    const struct sched_class * sched_class ;
    struct sched_entity se ;
    struct sched_rt_entity rt ;
    struct hlist_head preempt_notifiers ;
    unsigned char fpu_counter ;
    unsigned int btrace_seq ;
    unsigned int policy ;
    cpumask_t cpus_allowed ;
    struct sched_info sched_info ;
    struct list_head tasks ;
    struct plist_node pushable_tasks ;
    struct mm_struct * mm , * active_mm ;
    int exit_state ;
    int exit_code , exit_signal ;
    int pdeath_signal ;
    unsigned int personality ;
    unsigned did_exec : 1 ;
    unsigned in_execve : 1 ;
    unsigned in_iowait : 1 ;
    unsigned sched_reset_on_fork : 1 ;
    pid_t pid ;
    pid_t tgid ;
    unsigned long stack_canary ;
    struct task_struct * real_parent ;
    struct task_struct * parent ;
    struct list_head children ;
    struct list_head sibling ;
    struct task_struct * group_leader ;
    struct list_head ptraced ;
    struct list_head ptrace_entry ;
    struct bts_context * bts ;
    struct pid_link pids [ PIDTYPE_MAX ] ;
    struct list_head thread_group ;
    struct completion * vfork_done ;
    int * set_child_tid ;
    int * clear_child_tid ;
    cputime_t utime , stime , utimescaled , stimescaled ;
    cputime_t gtime ;
    cputime_t prev_utime , prev_stime ;
    unsigned long nvcsw , nivcsw ;
    struct timespec start_time ;
    struct timespec real_start_time ;
    unsigned long min_flt , maj_flt ;
    struct task_cputime cputime_expires ;
    struct list_head cpu_timers [ 3 ] ;
    const struct cred * real_cred ;
    const struct cred * cred ;
    struct mutex cred_guard_mutex ;
    struct cred * replacement_session_keyring ;
    char comm [ 16 ] ;
    int link_count , total_link_count ;
    struct sysv_sem sysvsem ;
    unsigned long last_switch_count ;
    struct thread_struct thread ;
    struct fs_struct * fs ;
    struct files_struct * files ;
    struct nsproxy * nsproxy ;
    struct signal_struct * signal ;
    struct sighand_struct * sighand ;
    sigset_t blocked , real_blocked ;
    sigset_t saved_sigmask ;
    struct sigpending pending ; unsigned long sas_ss_sp ; size_t sas_ss_size ;
    int ( * notifier ) ( void * priv ) ;
    void * notifier_data ;
    sigset_t * notifier_mask ;
    struct audit_context * audit_context ; uid_t loginuid ;
    unsigned int sessionid ;
    seccomp_t seccomp ; u32 parent_exec_id ; u32 self_exec_id ;
    spinlock_t alloc_lock ; struct irqaction * irqaction ;
    spinlock_t pi_lock ; struct plist_head pi_waiters ;
    struct rt_mutex_waiter * pi_blocked_on ; void * journal_info ;
    struct bio * bio_list , * * bio_tail ; struct reclaim_state * reclaim_state ;
    struct backing_dev_info * backing_dev_info ;
    struct io_context * io_context ;
    unsigned long ptrace_message ; siginfo_t * last_siginfo ;
    struct task_io_accounting ioac ;
    u64 acct_rss_mem1 ; u64 acct_vm_mem1 ;
    cputime_t acct_timexpd ;
    nodemask_t mems_allowed ;
    int cpuset_mem_spread_rotor ;
    struct css_set * cgroups ;
    struct list_head cg_list ;
    struct robust_list_head * robust_list ;
    struct compat_robust_list_head * compat_robust_list ;
    struct list_head pi_state_list ;
    struct futex_pi_state * pi_state_cache ;
    struct perf_event_context * perf_event_ctxp ;
    struct mutex perf_event_mutex ;
    struct list_head perf_event_list ;
    struct mempolicy * mempolicy ;
    short il_next ;
    atomic_t fs_excl ;
    struct rcu_head rcu ;
    struct pipe_inode_info * splice_pipe ;
    struct prop_local_single dirties ;
    int latency_record_count ;
    struct latency_record latency_record [ 32 ] ;
    unsigned long timer_slack_ns ;
    unsigned long default_timer_slack_ns ;
    struct list_head * scm_work_list ;
    int curr_ret_stack ;
    struct ftrace_ret_stack * ret_stack ;
    unsigned long long ftrace_timestamp ;
    atomic_t trace_overrun ;
    atomic_t tracing_graph_pause ;
    unsigned long trace ;
    unsigned long trace_recursion ; } ;

struct kthread_create_info {
    int ( * threadfn ) ( void * data ) ;
    void * data ;
    struct task_struct * result ;
    struct completion done ;
    struct list_head list ;
};

extern void mb_cache_entry_release ( struct mb_cache_entry * ce ) ;
extern void get_random_bytes ( void * buf , int nbytes ) ;
extern void kmem_cache_free ( struct kmem_cache * s , void * x ) ;
extern void __wait_on_buffer ( struct buffer_head * bh ) ;
extern void warn_slowpath_null ( const char * file , int line ) ;

extern void set_bh_page ( struct buffer_head * bh , struct page * page , unsigned long offset );
extern void __init_waitqueue_head ( wait_queue_head_t * q , struct lock_class_key * key );
extern ktime_t ktime_get ( void );
extern int wake_bit_function ( wait_queue_t * wait , unsigned mode , int sync , void * arg );
extern void __lock_page ( struct page * page );
extern void ll_rw_block ( int rw , int nr , struct buffer_head * bhs[ ]) ;
extern void end_buffer_write_sync ( struct buffer_head * bh , int uptodate );
extern int autoremove_wake_function ( wait_queue_t * wait , unsigned mode , int sync , void * key );
extern void put_page ( struct page * page );
extern void down_read(struct rw_semaphore *sem);
extern void __lock_buffer ( struct buffer_head * bh ) ;

extern void unlock_page ( struct page * page );

extern void __brelse ( struct buffer_head * buf ) ;
extern wait_queue_head_t * bit_waitqueue ( void * word , int bit ) ;
extern void free_buffer_head ( struct buffer_head * bh );
extern const char * bdevname ( struct block_device * bdev , char * buf );
extern int sync_blockdev ( struct block_device * bdev );
extern int try_to_free_buffers ( struct page * page );
extern struct kmem_cache * kmem_cache_create ( const char * name , size_t size , size_t align , unsigned long flags , void (*ctor)(void *));
extern void free_pages ( unsigned long addr , unsigned int order );
extern void __stack_chk_fail ( void );
extern unsigned long __phys_addr ( unsigned long x );
extern void schedule(void) ;
extern void refrigerator(void);
extern int schedule_hrtimeout ( ktime_t * expires , const enum hrtimer_mode mode ) ;
extern void __wake_up ( wait_queue_head_t * q , unsigned int mode , int nr_exclusive , void * key ) ;
extern unsigned long round_jiffies_up ( unsigned long j );

typedef int ( * threadfn ) ( void *data );

extern struct task_struct *kthread_create(threadfn thread_fun, void *data, char namefmt[]) ;
extern void iput ( struct inode * inode );
extern void kfree ( void * x );
extern int wake_up_process ( struct task_struct * p );
extern struct buffer_head * alloc_buffer_head ( gfp_t gfp_flags );
extern int submit_bh ( int rw , struct buffer_head * bh );
extern unsigned long __get_free_pages ( gfp_t gfp_mask , unsigned int order );

extern void unlock_buffer ( struct buffer_head * bh );
extern int __cond_resched_lock ( spinlock_t * lock );
extern void prepare_to_wait ( wait_queue_head_t * q , wait_queue_t * wait , int state );
extern sector_t bmap ( struct inode * inode , sector_t block );

extern void * kmem_cache_alloc ( struct kmem_cache * s , gfp_t gfpflags );
extern void kmem_cache_destroy ( struct kmem_cache * s );
extern void wake_up_bit ( void * word , int bit );

extern int dquot_commit ( struct dquot * dquot ) ;
/*--dquot_commit--*/
extern int dquot_commit_info ( struct super_block * sb , int type ) ;
/*--dquot_commit_info--*/


extern int sync_dirty_buffer ( struct buffer_head * bh );

extern void __bforget ( struct buffer_head * bh );
extern void * __kmalloc ( size_t size , gfp_t flags );

extern void finish_wait ( wait_queue_head_t * q , wait_queue_t * wait ) ;
extern struct buffer_head *__getblk(struct block_device *bdev, sector_t block, unsigned size);

extern struct buffer_head * __find_get_block ( struct block_device * bdev , sector_t block , unsigned size );
extern void yield ( void ) ;
extern void mark_buffer_dirty ( struct buffer_head * bh );


extern int dquot_commit ( struct dquot * dquot ) ;
/*--dquot_commit--*/
extern int dquot_commit_info ( struct super_block * sb , int type ) ;
/*--dquot_commit_info--*/


/*--dquot_operations--*/
enum { _DQUOT_USAGE_ENABLED = 0 , _DQUOT_LIMITS_ENABLED , _DQUOT_SUSPENDED , _DQUOT_STATE_FLAGS } ;

extern int dquot_mark_dquot_dirty ( struct dquot * dquot ) ;
/*--dquot_mark_dquot_dirty--*/
extern int dquot_acquire ( struct dquot * dquot ) ;
/*--dquot_acquire--*/
extern int dquot_commit ( struct dquot * dquot ) ;
/*--dquot_commit--*/
extern int dquot_release ( struct dquot * dquot ) ;
/*--dquot_release--*/
extern void dquot_destroy ( struct dquot * dquot ) ;
/*--dquot_destroy--*/
typedef int ( *fn_scan ) ( struct dquot * dquot , unsigned long priv );

extern int dquot_scan_active ( struct super_block * sb , fn_scan fn, unsigned long priv ) ;
/*--dquot_scan_active--*/
extern struct dquot * dquot_alloc ( struct super_block * sb , int type ) ;
/*--dquot_alloc--*/
extern int dquot_initialize ( struct inode * inode , int type ) ;
/*--dquot_initialize--*/
extern int dquot_drop ( struct inode * inode ) ;
/*--dquot_drop--*/
extern int __dquot_alloc_space ( struct inode * inode , qsize_t number , int warn , int reserve ) ;
/*--__dquot_alloc_space--*/
extern int dquot_alloc_space ( struct inode * inode , qsize_t number , int warn ) ;
/*--dquot_alloc_space--*/
extern int dquot_reserve_space ( struct inode * inode , qsize_t number , int warn ) ;
/*--dquot_reserve_space--*/
extern int dquot_alloc_inode ( const struct inode * inode , qsize_t number ) ;
/*--dquot_alloc_inode--*/
extern int dquot_claim_space ( struct inode * inode , qsize_t number ) ;
/*--dquot_claim_space--*/
extern int __dquot_free_space ( struct inode * inode , qsize_t number , int reserve ) ;
/*--__dquot_free_space--*/
extern int dquot_free_space ( struct inode * inode , qsize_t number ) ;
/*--dquot_free_space--*/
extern void dquot_release_reserved_space ( struct inode * inode , qsize_t number ) ;
/*--dquot_release_reserved_space--*/
extern int dquot_free_inode ( const struct inode * inode , qsize_t number ) ;
/*--dquot_free_inode--*/
extern int dquot_transfer ( struct inode * inode , struct iattr * iattr ) ;
/*--dquot_transfer--*/
extern int dquot_commit_info ( struct super_block * sb , int type ) ;
/*--dquot_commit_info--*/


//--------------------------------------------------------------------------------

typedef __u32 b_blocknr_t;

struct journal_head {
    struct buffer_head * b_bh ;
    int b_jcount ;
    unsigned b_jlist ;
    unsigned b_modified ;
    char * b_frozen_data ;
    char * b_committed_data ;
    transaction_t * b_transaction ;
    transaction_t * b_next_transaction ;
    struct journal_head * b_tnext , * b_tprev ;
    transaction_t * b_cp_transaction ;
    struct journal_head * b_cpnext , * b_cpprev ;
    struct jbd2_buffer_trigger_type * b_triggers ;
    struct jbd2_buffer_trigger_type * b_frozen_triggers ;
};

extern handle_t *journal_start ( journal_t * journal , int nblocks ) ;
extern int journal_extend ( handle_t * handle , int nblocks ) ;
extern int journal_restart ( handle_t * handle , int nblocks ) ;
extern void journal_lock_updates ( journal_t * journal ) ;
extern void journal_unlock_updates ( journal_t * journal ) ;
extern int journal_get_write_access ( handle_t * handle , struct buffer_head * bh ) ;
extern int journal_get_create_access ( handle_t * handle , struct buffer_head * bh ) ;
extern int journal_get_undo_access ( handle_t * handle , struct buffer_head * bh ) ;
extern int journal_dirty_data ( handle_t * handle , struct buffer_head * bh ) ;
extern int journal_dirty_metadata ( handle_t * handle , struct buffer_head * bh ) ;
extern void journal_release_buffer ( handle_t * handle , struct buffer_head * bh ) ;
extern int journal_forget ( handle_t * handle , struct buffer_head * bh ) ;
extern int journal_stop ( handle_t * handle ) ;
extern int journal_force_commit ( journal_t * journal ) ;

extern void journal_unfile_buffer ( journal_t * journal , struct journal_head * jh ) ;
extern int journal_try_to_free_buffers ( journal_t * journal , struct page * page , gfp_t gfp_mask ) ;
extern void journal_invalidatepage ( journal_t * journal , struct page * page , unsigned long offset ) ;

extern void journal_file_buffer ( struct journal_head * jh , transaction_t * transaction , int jlist ) ;

extern void journal_refile_buffer ( journal_t * journal , struct journal_head * jh ) ;
extern void journal_commit_transaction ( journal_t * journal ) ;
extern int journal_recover ( journal_t * journal ) ;
extern int journal_skip_recovery ( journal_t * journal ) ;

extern void journal_destroy_revoke_caches ( void ) ;
extern int journal_init_revoke_caches ( void ) ;
extern int journal_init_revoke ( journal_t * journal , int hash_size ) ;
extern void journal_destroy_revoke ( journal_t * journal ) ;
extern int journal_revoke ( handle_t * handle , unsigned int blocknr , struct buffer_head * bh_in ) ;
extern int journal_cancel_revoke ( handle_t * handle , struct journal_head * jh ) ;
extern void journal_switch_revoke_table ( journal_t * journal ) ;
extern void journal_write_revoke_records ( journal_t * journal , transaction_t * transaction , int write_op ) ;
extern int journal_set_revoke ( journal_t * journal , unsigned int blocknr , tid_t sequence ) ;
extern int journal_test_revoke ( journal_t * journal , unsigned int blocknr , tid_t sequence ) ;
extern void journal_clear_revoke ( journal_t * journal ) ;
extern int journal_write_metadata_buffer ( transaction_t * transaction , struct journal_head * jh_in , struct journal_head **jh_out , unsigned int blocknr ) ;

extern int journal_force_commit_nested ( journal_t * journal ) ;
extern int journal_start_commit ( journal_t * journal , tid_t * ptid ) ;

extern int journal_next_log_block ( journal_t * journal , unsigned int * retp ) ;
extern int journal_bmap ( journal_t * journal , unsigned int blocknr , unsigned int * retp ) ;
extern struct journal_head *journal_get_descriptor_buffer ( journal_t * journal ) ;
extern journal_t * journal_init_dev ( struct block_device * bdev , struct block_device * fs_dev , int start , int len , int blocksize ) ;
extern journal_t * journal_init_inode ( struct inode * inode ) ;
extern int journal_create ( journal_t * journal ) ;
extern void journal_update_superblock ( journal_t * journal , int wait ) ;
extern int journal_load ( journal_t * journal ) ;
extern int journal_destroy ( journal_t * journal ) ;
extern int journal_check_used_features ( journal_t * journal , unsigned long compat , unsigned long ro , unsigned long incompat ) ;
extern int journal_check_available_features ( journal_t * journal , unsigned long compat , unsigned long ro , unsigned long incompat ) ;
extern int journal_set_features ( journal_t * journal , unsigned long compat , unsigned long ro , unsigned long incompat ) ;
extern int journal_update_format ( journal_t * journal ) ;
extern int journal_flush ( journal_t * journal ) ;
extern int journal_wipe ( journal_t * journal , int write ) ;
extern void journal_abort ( journal_t * journal , int errno ) ;
extern int journal_errno ( journal_t * journal ) ;
extern int journal_clear_err ( journal_t * journal ) ;
extern void journal_ack_err ( journal_t * journal ) ;
extern int journal_blocks_per_page ( struct inode * inode ) ;
extern struct journal_head * journal_add_journal_head ( struct buffer_head * bh ) ;
extern struct journal_head * journal_grab_journal_head ( struct buffer_head * bh ) ;
extern void journal_remove_journal_head ( struct buffer_head * bh ) ;
extern void journal_put_journal_head ( struct journal_head * jh ) ;

extern size_t journal_tag_bytes ( journal_t * journal ) ;
extern int journal_release ( struct reiserfs_transaction_handle * th , struct super_block * sb ) ;
extern int journal_release_error ( struct reiserfs_transaction_handle * th , struct super_block * sb ) ;
extern int journal_init ( struct super_block * sb , const char * j_dev_name , int old_format , unsigned int commit_max_age ) ;
extern int journal_transaction_should_end ( struct reiserfs_transaction_handle * th , int new_alloc ) ;
extern int journal_join_abort ( struct reiserfs_transaction_handle * th , struct super_block * sb , unsigned long nblocks ) ;
extern int journal_begin ( struct reiserfs_transaction_handle * th , struct super_block * sb , unsigned long nblocks ) ;
extern int journal_mark_dirty ( struct reiserfs_transaction_handle * th , struct super_block * sb , struct buffer_head * bh ) ;
extern int journal_end ( struct reiserfs_transaction_handle * th , struct super_block * sb , unsigned long nblocks ) ;
extern int journal_end_sync ( struct reiserfs_transaction_handle * th , struct super_block * sb , unsigned long nblocks ) ;
extern int journal_mark_freed ( struct reiserfs_transaction_handle * th , struct super_block * sb , b_blocknr_t blocknr ) ;

extern void iget_failed ( struct inode * inode );
extern void page_zero_new_buffers ( struct page * page , unsigned from , unsigned to );
extern struct mb_cache_entry * mb_cache_entry_find_next ( struct mb_cache_entry * prev , int index , struct block_device * bdev , unsigned int key ) ;
extern void mb_cache_shrink ( struct block_device * bdev );
extern const char * __bdevname ( dev_t dev , char * buffer );
extern struct buffer_head * __bread ( struct block_device * bdev , sector_t block , unsigned size );
extern struct inode * iget_locked ( struct super_block * sb , unsigned long ino );
extern void down_read ( struct rw_semaphore * sem ) ;
extern unsigned long get_seconds ( void );
extern void truncate_inode_pages ( struct address_space * mapping , loff_t lstart );
extern int filemap_flush ( struct address_space * mapping );



extern int block_write_full_page_endio ( struct page * page , get_block_t  get_block , struct writeback_control * wbc , bh_end_io_t handler );

extern int generic_block_fiemap ( struct inode * inode , struct fiemap_extent_info * fieinfo , u64 start , u64 len , get_block_t  get_block );
extern int block_prepare_write ( struct page * page , unsigned from , unsigned to , get_block_t  get_block ) ;
extern int block_page_mkwrite ( struct vm_area_struct * vma , struct vm_fault * vmf , get_block_t get_block ) ;
extern int block_truncate_page ( struct address_space * mapping , loff_t from , get_block_t  get_block ) ;
extern int block_is_partially_uptodate ( struct page * page , read_descriptor_t * desc , unsigned long from ) ;
extern int sdio_set_block_size ( struct sdio_func * func , unsigned blksz );
extern int block_commit_write ( struct page * page , unsigned from , unsigned to );
extern int block_write_end ( struct file * file , struct address_space * mapping , loff_t pos , unsigned len , unsigned copied , struct page * page , void * fsdata ) ;
extern void kill_block_super ( struct super_block * sb );
extern s32 i2c_smbus_read_block_data ( struct i2c_client * client , u8 command , u8 * values );
extern int block_write_full_page ( struct page * page , get_block_t  get_block , struct writeback_control * wbc ) ;
extern s32 i2c_smbus_write_block_data ( struct i2c_client * client , u8 command , u8 length , const u8 * values ) ;
extern int block_write_begin ( struct file * file , struct address_space * mapping , loff_t pos , unsigned len , unsigned flags , struct page * * pagep , void * * fsdata , get_block_t  get_block ) ;
extern int block_fsync ( struct file * filp , struct dentry * dentry , int datasync ) ;
extern s32 i2c_smbus_read_i2c_block_data ( struct i2c_client * client , u8 command , u8 length , u8 * values ) ;
extern void block_sync_page ( struct page * page ) ;
extern sector_t generic_block_bmap ( struct address_space * mapping , sector_t block , get_block_t  get_block ) ;
extern void block_all_signals ( notifier notifier_var, void * priv , sigset_t * mask ) ;

extern int __generic_block_fiemap ( struct inode * inode , struct fiemap_extent_info * fieinfo , u64 start , u64 len , get_block_t  get_block );
extern int block_read_full_page ( struct page * page , get_block_t  get_block ) ;
extern s32 i2c_smbus_write_i2c_block_data ( struct i2c_client * client , u8 command , u8 length , const u8 * values ) ;
extern void block_invalidatepage ( struct page * page , unsigned long offset ) ;
//extern ssize_t generic_getxattr ( struct dentry * dentry , const char * name , void * buffer , size_t size ) ;




enum generic_types { GT_DIR , GT_PIPE , GT_SOCK } ;
struct generic_type { const char * type ; mode_t mode ; } ;
extern void generic_processor_info ( int apicid , int version ) ;
extern int generic_get_free_region ( unsigned long base , unsigned long size , int replace_reg ) ;
extern int generic_validate_add_page ( unsigned long base , unsigned long size , unsigned int type ) ;
extern int generic_ptrace_peekdata ( struct task_struct * tsk , long addr , long data ) ;
extern int generic_ptrace_pokedata ( struct task_struct * tsk , long addr , long data ) ;
extern void generic_smp_call_function_interrupt ( void ) ;
extern void generic_smp_call_function_single_interrupt ( void ) ;
extern int generic_segment_checks ( const struct iovec * iov , unsigned long * nr_segs , size_t * count , int access_flags ) ;
extern ssize_t generic_file_aio_read ( struct kiocb * iocb , const struct iovec * iov , unsigned long nr_segs , loff_t pos ) ;
extern int generic_file_mmap ( struct file * file , struct vm_area_struct * vma ) ;
extern int generic_file_readonly_mmap ( struct file * file , struct vm_area_struct * vma ) ;
extern ssize_t generic_file_direct_write ( struct kiocb * iocb , const struct iovec * iov , unsigned long * nr_segs , loff_t pos , loff_t * ppos , size_t count , size_t ocount ) ;
extern ssize_t generic_file_buffered_write ( struct kiocb * iocb , const struct iovec * iov , unsigned long nr_segs , loff_t pos , loff_t * ppos , size_t count , ssize_t written ) ;
extern ssize_t generic_file_aio_write ( struct kiocb * iocb , const struct iovec * iov , unsigned long nr_segs , loff_t pos ) ;
extern int generic_writepages ( struct address_space * mapping , struct writeback_control * wbc ) ;
extern int generic_error_remove_page ( struct address_space * mapping , struct page * page ) ;

extern int generic_access_phys ( struct vm_area_struct * vma , unsigned long addr , void * buf , int len , int write ) ;
extern int generic_file_open ( struct inode * inode , struct file * filp ) ;
extern loff_t generic_file_llseek_unlocked ( struct file * file , loff_t offset , int origin ) ;
extern loff_t generic_file_llseek ( struct file * file , loff_t offset , int origin ) ;
extern void generic_shutdown_super ( struct super_block * sb ) ;
extern void generic_fillattr ( struct inode * inode , struct kstat * stat ) ;
extern void * generic_pipe_buf_map ( struct pipe_inode_info * pipe , struct pipe_buffer * buf , int atomic ) ;
extern void generic_pipe_buf_unmap ( struct pipe_inode_info * pipe , struct pipe_buffer * buf , void * map_data ) ;
extern int generic_pipe_buf_steal ( struct pipe_inode_info * pipe , struct pipe_buffer * buf ) ;
extern void generic_pipe_buf_get ( struct pipe_inode_info * pipe , struct pipe_buffer * buf ) ;
extern int generic_pipe_buf_confirm ( struct pipe_inode_info * info , struct pipe_buffer * buf ) ;
extern void generic_pipe_buf_release ( struct pipe_inode_info * pipe , struct pipe_buffer * buf ) ;
extern int generic_permission ( struct inode * inode , int mask , int ( * check_acl ) ( struct inode * inode , int mask ) ) ;
extern int generic_readlink ( struct dentry * dentry , char * buffer , int buflen ) ;
extern int generic_block_fiemap ( struct inode * inode , struct fiemap_extent_info * fieinfo , u64 start , u64 len , get_block_t  get_block ) ;
extern void generic_delete_inode ( struct inode * inode ) ;
extern int generic_detach_inode ( struct inode * inode ) ;
extern void generic_drop_inode ( struct inode * inode ) ;
extern int generic_show_options ( struct seq_file * m , struct vfsmount * mnt ) ;
extern ssize_t generic_getxattr ( struct dentry * dentry , char * name , void * buffer , size_t size ) ;
extern ssize_t generic_listxattr ( struct dentry * dentry , char * buffer , size_t buffer_size ) ;
extern int generic_setxattr ( struct dentry * dentry , char * name , void * value , size_t size , int flags ) ;
extern int generic_removexattr ( struct dentry * dentry , char * name ) ;
extern ssize_t generic_read_dir ( struct file * filp , char * buf , size_t siz , loff_t * ppos ) ;

typedef struct inode * ( * get_inode ) ( struct super_block * sb , u64 ino , u32 gen );

extern struct dentry * generic_fh_to_dentry ( struct super_block * sb , struct fid * fid , int fh_len , int fh_type , get_inode get_node_data ) ;
extern struct dentry * generic_fh_to_parent ( struct super_block * sb , struct fid * fid , int fh_len , int fh_type , get_inode get_node_data ) ;
extern ssize_t generic_file_splice_read ( struct file * in , loff_t * ppos , struct pipe_inode_info * pipe , size_t len , unsigned int flags ) ;
extern ssize_t generic_file_splice_write ( struct pipe_inode_info * pipe , struct file * out , loff_t * ppos , size_t len , unsigned int flags ) ;
extern ssize_t generic_splice_sendpage ( struct pipe_inode_info * pipe , struct file * out , loff_t * ppos , size_t len , unsigned int flags ) ;
extern int generic_write_sync ( struct file * file , loff_t pos , loff_t count ) ;
extern int generic_write_end ( struct file * file , struct address_space * mapping , loff_t pos , unsigned len , unsigned copied , struct page * page , void * fsdata ) ;
extern int generic_cont_expand_simple ( struct inode * inode , loff_t size ) ;
extern sector_t generic_block_bmap ( struct address_space * mapping , sector_t block , get_block_t  get_block ) ;

extern int generic_setlease ( struct file * filp , long arg , struct file_lock * * flp ) ;

extern int nobh_write_end ( struct file * file , struct address_space * mapping , loff_t pos , unsigned len , unsigned copied , struct page * page , void * fsdata ) ;

struct generic_bl_info {
    const char * name ;
    int max_intensity ;
    int default_intensity ;
    int limit_mask ;
    void ( * set_bl_intensity ) ( int intensity ) ;
    void ( * kick_battery ) ( void ) ;
};





extern size_t generic_acl_list ( struct inode * inode,
        struct generic_acl_operations * ops, int type , char * list , size_t list_size ) ;

extern int generic_acl_get ( struct inode * inode , struct generic_acl_operations * ops , int type , void * buffer , size_t size ) ;
extern int generic_acl_set ( struct inode * inode , struct generic_acl_operations * ops , int type , const void * value , size_t size ) ;
extern int generic_acl_init ( struct inode * inode , struct inode * dir , struct generic_acl_operations * ops ) ;
extern int generic_acl_chmod ( struct inode * inode , struct generic_acl_operations * ops ) ;
extern void generic_unplug_device ( struct request_queue * q ) ;
extern void generic_make_request ( struct bio * bio ) ;

extern ssize_t __generic_file_aio_write ( struct kiocb * iocb , const struct iovec * iov , unsigned long nr_segs , loff_t * ppos ) ;
extern int __generic_block_fiemap ( struct inode * inode , struct fiemap_extent_info * fieinfo , u64 start , u64 len , get_block_t  get_block ) ;
extern void invalidate_bdev ( struct block_device * bdev ) ;
extern void up_read ( struct rw_semaphore * sem );




extern void __mark_inode_dirty ( struct inode * inode , int flags );
extern int security_inode_init_security ( struct inode * inode , struct inode * dir , char **name , void * * value , size_t * len ) ;
/*--security_inode_init_security--*/
extern int set_blocksize ( struct block_device * bdev , int size ) ;
/*--set_blocksize--*/

extern int sb_set_blocksize ( struct super_block * sb , int size ) ;
/*--sb_set_blocksize--*/

extern void clear_inode ( struct inode * inode ) ;
extern void __percpu_counter_add ( struct percpu_counter * fbc , s64 amount , s32 batch ) ;
/*--__percpu_counter_add--*/

extern struct mb_cache_entry * mb_cache_entry_get ( struct mb_cache * cache , struct block_device * bdev , sector_t block ) ;
/*--mb_cache_entry_get--*/

extern void bd_release ( struct block_device * bdev ) ;
/*--bd_release--*/
extern void bd_release_from_disk ( struct block_device * bdev , struct gendisk * disk ) ;
/*--bd_release_from_disk--*/

extern void path_put ( struct path * path ) ;
extern int set_page_dirty ( struct page * page ) ;
extern int bh_uptodate_or_lock ( struct buffer_head * bh ) ;

extern int sb_min_blocksize ( struct super_block * sb , int size ) ;
extern void init_special_inode ( struct inode * inode , umode_t mode , dev_t rdev ) ;
extern int blkdev_issue_flush ( struct block_device * bdev , sector_t * error_sector ) ;
extern void lock_super ( struct super_block * sb ) ;
extern int __set_page_dirty_nobuffers ( struct page * page ) ;
extern int __page_symlink ( struct inode * inode , const char * symname , int len , int nofs ) ;
extern struct timespec current_kernel_time ( void );
extern void * memscan ( void * addr , int c , size_t size ) ;
extern int is_bad_inode ( struct inode * inode ) ;
extern void __init_rwsem ( struct rw_semaphore * sem , const char * name , struct lock_class_key * key );
extern void percpu_counter_destroy ( struct percpu_counter * fbc );
extern int inode_setattr ( struct inode * inode , struct iattr * attr ) ;
extern int blkdev_put ( struct block_device * bdev , fmode_t mode ) ;
extern void end_buffer_read_sync ( struct buffer_head * bh , int uptodate ) ;
extern int mnt_want_write ( struct vfsmount * mnt ) ;
extern void mnt_drop_write ( struct vfsmount * mnt ) ;
extern int log_wait_commit ( journal_t * journal , tid_t tid ) ;

extern ssize_t __blockdev_direct_IO ( int rw , struct kiocb * iocb , struct inode * inode , struct block_device * bdev , const struct iovec * iov , loff_t offset , unsigned long nr_segs , get_block_t get_block , dio_iodone_t end_io , int dio_lock_type ) ;
extern void __inode_add_bytes ( struct inode * inode , loff_t bytes ) ;
extern void inode_add_bytes ( struct inode * inode , loff_t bytes ) ;
extern ssize_t do_sync_read ( struct file * filp , char * buf , size_t len , loff_t * ppos ) ;
extern void d_instantiate ( struct dentry * entry , struct inode * inode ) ;
extern int seq_puts ( struct seq_file * m , const char * s ) ;
extern int capable ( int cap ) ;
extern struct mb_cache_entry * mb_cache_entry_alloc ( struct mb_cache * cache , gfp_t gfp_flags ) ;
extern __u32 half_md4_transform ( __u32 buf [ 4 ] , __u32 const in [ 8 ] ) ;
extern ssize_t do_sync_write ( struct file * filp , const char * buf , size_t len , loff_t * ppos ) ;
extern char * match_strdup ( substring_t * s ) ;


extern void page_put_link ( struct dentry * dentry , struct nameidata * nd , void * cookie );
extern int bd_claim ( struct block_device * bdev , void * holder ) ;
extern struct page * grab_cache_page_write_begin ( struct address_space * mapping , unsigned long index , unsigned flags ) ;


extern void mb_cache_destroy ( struct mb_cache * cache ) ;
extern int inode_change_ok ( const struct inode * inode , struct iattr * attr ) ;

extern int mpage_readpage ( struct page * page , get_block_t get_block ) ;

int nobh_write_begin ( struct file * file , struct address_space * mapping , loff_t pos , unsigned len , unsigned flags , struct page * * pagep , void * * fsdata , get_block_t  get_block ) ;



extern int log_start_commit ( journal_t * journal , tid_t tid );



struct match_token { int token ; const char * pattern ; } ;
/*--match_token--*/
typedef struct match_token match_table_t[] ;
extern int match_token ( char * s , const match_table_t table , substring_t args[]) ;
extern s64 __percpu_counter_sum ( struct percpu_counter * fbc ) ;

extern void unlock_super ( struct super_block * sb ) ;
/*--unlock_super--*/

extern int redirty_page_for_writepage ( struct writeback_control * wbc , struct page * page ) ;
/*--redirty_page_for_writepage--*/
struct inode * new_inode ( struct super_block * sb ) ;
struct inode_smack * new_inode_smack ( char * smack ) ;


void rb_insert_color ( struct rb_node * node , struct rb_root * root ) ;
void rb_erase ( struct rb_node * node , struct rb_root * root ) ;

struct rb_node * rb_first ( const struct rb_root * root ) ;
struct rb_node * rb_last ( const struct rb_root * root ) ;
struct rb_node * rb_next ( const struct rb_node * node ) ;
struct rb_node * rb_prev ( const struct rb_node * node ) ;
void rb_replace_node ( struct rb_node * victim , struct rb_node * new_data , struct rb_root * root ) ;

extern void inode_sub_bytes ( struct inode * inode , loff_t bytes ) ;

extern struct mb_cache_entry * mb_cache_entry_find_first ( struct mb_cache * cache , int index , struct block_device * bdev , unsigned int key ) ;
extern struct page * find_or_create_page ( struct address_space * mapping , unsigned long index , gfp_t gfp_mask ) ;
extern int nobh_writepage ( struct page * page , get_block_t  get_block , struct writeback_control * wbc ) ;
extern int __percpu_counter_init ( struct percpu_counter * fbc , s64 amount , struct lock_class_key * key );

extern void * page_follow_link_light ( struct dentry * dentry , struct nameidata * nd ) ;
extern void mb_cache_entry_free ( struct mb_cache_entry * ce ) ;
extern unsigned long find_next_zero_bit ( const unsigned long * addr , unsigned long size , unsigned long offset ) ;
extern int mb_cache_entry_insert ( struct mb_cache_entry * ce , struct block_device * bdev , sector_t block , unsigned int keys [ ] ) ;
extern void create_empty_buffers ( struct page * page , unsigned long blocksize , unsigned long b_state ) ;
extern int bh_submit_read ( struct buffer_head * bh ) ;

extern void make_bad_inode ( struct inode * inode ) ;
extern void inode_init_once ( struct inode * inode ) ;
extern int insert_inode_locked ( struct inode * inode ) ;
extern struct dentry * d_alloc_root ( struct inode * root_inode ) ;
extern struct dentry * d_obtain_alias ( struct inode * inode ) ;

extern int match_int ( substring_t * s , int * result ) ;
extern struct dentry * d_splice_alias ( struct inode * inode , struct dentry * dentry ) ;
extern struct block_device * open_by_devnum ( dev_t dev , fmode_t mode ) ;

extern void init_buffer(struct buffer_head *, bh_end_io_t *, void *);


//void print_hex_dump ( const char * level , const char * prefix_str ,
//          int prefix_type , int rowsize , int groupsize , const void * buf ,
    //                                              size_t len , bool ascii ) ;
void print_hex_dump_bytes ( const char * prefix_str , int prefix_type ,
                                                const void * buf , size_t len ) ;

extern void unlock_new_inode ( struct inode * inode ) ;
extern int kern_path ( const char * name , unsigned int flags , struct path * path ) ;


extern void down_write ( struct rw_semaphore * sem ) ;
extern void up_write ( struct rw_semaphore * sem ) ;

extern void __mutex_init ( struct mutex * lock , const char * name , struct lock_class_key * key ) ;

extern void page_cache_sync_readahead ( struct address_space * mapping, struct file_ra_state *ra,
                struct file * filp , unsigned long offset , unsigned long req_size ) ;

extern int buffer_migrate_page ( struct address_space * mapping ,
                                struct page * newpage , struct page * page ) ;
extern int mpage_readpages ( struct address_space * mapping ,
        struct list_head * pages , unsigned nr_pages , get_block_t get_block ) ;

int mpage_writepages ( struct address_space * mapping , struct writeback_control * wbc , get_block_t get_block ) ;
int nobh_truncate_page ( struct address_space * mapping , loff_t from , get_block_t  get_block ) ;

extern int vprintk ( const char * fmt , va_list args );


extern void unblock_all_signals ( void );
extern int _cond_resched ( void ) ;
extern int current_umask ( void );
extern void dump_stack ( void ) ;
extern void lock_kernel ( void ) ;
extern void unlock_kernel ( void ) ;

extern int in_group_p ( gid_t grp ) ;

extern void disable_irq ( unsigned int irq ) ;
extern void enable_irq ( unsigned int irq ) ;
extern void flush_scheduled_work ( void ) ;
extern void *vmalloc ( unsigned long size ) ;
extern void msleep ( unsigned int msecs ) ;
extern u32 ethtool_op_get_sg ( struct net_device * dev ) ;
extern void pm_qos_remove_requirement ( int pm_qos_class , char * name ) ;
extern int pm_qos_update_requirement ( int pm_qos_class , char * name , s32 new_value ) ;
extern int pm_qos_add_requirement ( int pm_qos_class , char * name , s32 value ) ;
extern unsigned long msleep_interruptible ( unsigned int msecs ) ;
extern int net_ratelimit ( void ) ;
extern u32 ethtool_op_get_tso ( struct net_device * dev ) ;
extern void dev_kfree_skb_irq ( struct sk_buff * skb ) ;
extern void mutex_unlock ( struct mutex * lock ) ;
extern unsigned long round_jiffies ( unsigned long j ) ;
extern void synchronize_irq ( unsigned int irq ) ;
extern void * ioremap_nocache ( resource_size_t phys_addr , unsigned long size ) ;
extern void remove_irq ( unsigned int irq , struct irqaction * act ) ;
extern void iounmap ( void * addr ) ;
extern void mutex_lock ( struct mutex * lock ) ;
extern void __udelay ( unsigned long usecs ) ;
extern void vfree ( void * addr ) ;
extern struct thread_info *current_thread_info(void);
extern void mcount(void);
extern void __const_udelay(unsigned long xloops);
//extern size_t strlen ( char * s );
//extern int strncmp ( char * cs , char * ct , size_t count ) ;
//extern char * strsep ( char **s , char * ct );
//extern int memcmp ( void * cs , void * ct , size_t count ) ;
//extern void * memmove ( void * dest , void * src , size_t count ) ;
//extern int strcmp ( char * str1 , char * str2 ) ;

typedef struct { unsigned long seg ; } mm_segment_t ;

struct restart_block { long ( * fn ) ( struct restart_block * ) ; union { struct { unsigned long arg0 , arg1 , arg2 , arg3 ; } ; struct { u32 * uaddr ; u32 val ; u32 flags ; u32 bitset ; u64 time ; u32 * uaddr2 ; } futex ; struct { clockid_t index ; struct timespec * rmtp ; struct compat_timespec * compat_rmtp ; u64 expires ; } nanosleep ; struct { struct pollfd * ufds ; int nfds ; int has_timeout ; unsigned long tv_sec ; unsigned long tv_nsec ; } poll ; } ; } ;

struct thread_info {
    struct task_struct * task ;
    struct exec_domain * exec_domain ;
    __u32 flags ; __u32 status ;
    __u32 cpu ;
    int preempt_count ;
    mm_segment_t addr_limit ;
    struct restart_block restart_block ;
    void * sysenter_return ;
    int uaccess_err ;
} ;

#ifdef __cplusplus
}
#endif

#endif /* DRK_KERNEL_TYPES_HPP_ */
