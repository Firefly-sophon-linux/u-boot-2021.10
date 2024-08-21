#include <common.h>
#include <command.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <dm.h>
#include <fdtdec.h>

static int do_firefly_upgrade(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
    char tmp_buff[50];
    int err;
    int node;
    struct gpio_desc recovery_gpio;
    u32 value = -1;
    int sub_value = 0;

    // 检查设备树中是否存在 /position 节点
    int pos_node = fdt_path_offset(gd->fdt_blob, "/firefly-position");
    if (pos_node < 0) {
        printf("Error: could not find device tree node for /firefly-position\n");
        return -ENOENT;
    }

   // 获取设备树节点
    node = fdt_path_offset(gd->fdt_blob, "/gpio-keys/gpio12");
    if (node < 0) {
        printf("Error: could not find device tree node for /gpio-keys\n");
        return -ENOENT;
    }
    mdelay(1000);
     printf("Prepare requested GPIO·Recovery·Key\n");
    // 申请 GPIO·Recovery·Key
    err = gpio_request_by_name_nodev(offset_to_ofnode(node), "gpios", 0, &recovery_gpio, GPIOD_IS_IN);
    if (err) {
        printf("Error: could not request GPIO Recovery Key, err = %d\n", err);
        return err;
    }

    // 检测 GPIO 状态
    if (!dm_gpio_is_valid(&recovery_gpio)) {
        printf("Error: invalid GPIO Recovery Key\n");
        dm_gpio_free(NULL, &recovery_gpio);
        return -EINVAL;
    }

    value = dm_gpio_get_value(&recovery_gpio);
    printf("GPIO Recovery Key value: %d\n", value);

    // 释放 GPIO
    dm_gpio_free(NULL, &recovery_gpio);

    // 如果 GPIO 被拉低，则执行特定命令
    if (value == 0) {
        // 运行 firefly_sub_detect 命令，获取子板位置
        run_command("firefly_sub_detect", 0);

        // 从环境变量获取子板位置的数字
        char *position = env_get("firefly_sub_position");
        if (position == NULL) {
            printf("Error: could not detect sub position\n");
            return -1;
        } else {
            printf("position = %s\n", position);

            if (strchr(position, '-') != NULL) {
                char *dash = strchr(position, '-');
                *dash = '\0'; 
                int main = simple_strtol(position + 3, NULL, 10); 
                int sub = simple_strtol(dash + 1, NULL, 10); 

                sub_value = (main - 1) * 8 + sub;
            } else {
                sub_value = simple_strtol(position + 3, NULL, 10); 
            }

            printf("sub_value = %d\n", sub_value);

            // 构造 IP 和 TFTP 服务器地址
            snprintf(tmp_buff, sizeof(tmp_buff), "setenv ipaddr 172.22.%d.0", sub_value);
            run_command(tmp_buff, 0);
            run_command("setenv serverip 172.22.250.0", 0);
        }

        // 设置网络参数
        run_command("setenv gatewayip 172.22.0.1", 0);
        run_command("setenv netmask 255.255.0.0", 0);

        udelay(1000 * 1000);

        printf("setenv ipaddr: %s\n", env_get("ipaddr"));
        printf("setenv gatewayip: %s\n", env_get("gatewayip"));
        printf("setenv serverip: %s\n", env_get("serverip"));
        printf("setenv netmask: %s\n", env_get("netmask"));

        // 网络连接测试
        do {
            udelay(200 * 1000);
            printf("ping server: 172.22.250.0 \n");
            err = run_command("ping 172.22.250.0", 0);
        } while (err != 0);

        // 设置 OTA 路径并下载引导脚本
        sprintf(tmp_buff, "setenv ota_path %s", position);
        run_command(tmp_buff, 0);
        
        do {
            udelay(1000 * 1000);
            snprintf(tmp_buff, sizeof(tmp_buff), "tftp 0x120000000 /$ota_path/boot.scr");
            printf("sub path upgrade = %s\n", tmp_buff);
            err = run_command(tmp_buff, 0);
        } while (err != 0);

        run_command("source 0x120000000", 0);
    }

    return 0;
}

U_BOOT_CMD(
    firefly_upgrade, 3, 1, do_firefly_upgrade,
    "firefly boot image via network using BOOTP/TFTP protocol",
    "[loadAddress] [[hostIPaddr:]bootfilename]"
);
