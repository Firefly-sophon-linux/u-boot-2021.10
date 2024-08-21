#include <common.h>
#include <command.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h> 

static int do_firefly_sub_detect(struct cmd_tbl *cmdtp, int flag, int argc,
                                 char *const argv[])
{
    int node, ret;
    struct gpio_desc *gpiods; // 动态分配的 GPIO 数组
    u32 value = 0;
    int i;
    u8 *position; // 动态分配的位置数组

    // 获取设备树节点
    node = fdt_path_offset(gd->fdt_blob, "/firefly-position");
    if (node < 0) {
        printf("Error: could not find device tree node for /firefly-position\n");
        return -ENOENT;
    }

    // 读取 firefly-used-num 属性
    ret = fdtdec_get_int(gd->fdt_blob, node, "firefly-used-num", -1);
    if (ret < 0) {
        printf("Error: could not get firefly-used-num\n");
        return -EINVAL;
    }
    int num_gpios = ret;
    printf("firefly-used-num: %d\n", num_gpios);

    // 动态分配 GPIO 和位置数组
    gpiods = malloc(sizeof(struct gpio_desc) * num_gpios);
    position = malloc(sizeof(u8) * num_gpios);

    if (!gpiods || !position) {
        printf("Error: could not allocate memory\n");
        free(gpiods);
        free(position);
        return -ENOMEM;
    }

    // 请求 GPIO 和读取值
    for (i = 0; i < num_gpios; i++) {
        char prop_name[32];
        snprintf(prop_name, sizeof(prop_name), "firefly-bit%d-gpio", i);
        printf("Prepare requested GPIO %s\n", prop_name);
        mdelay(100);
        // 请求 GPIO
        ret = gpio_request_by_name_nodev(offset_to_ofnode(node), prop_name, 0, &gpiods[i], GPIOD_IS_IN);
        if (ret) {
            printf("Error: could not request GPIO %s , ret = %d \n", prop_name, ret);
            goto cleanup;
        }

        // 成功申请 GPIO，打印信息
        printf("Successfully requested GPIO %s\n", prop_name);

        // 读取 GPIO 值
        if (!dm_gpio_is_valid(&gpiods[i])) {
            printf("Error: invalid GPIO %s\n", prop_name);
            goto cleanup;
        }

        position[i] = dm_gpio_get_value(&gpiods[i]);
    }

    // 计算最终的 value
    for (i = 0; i < num_gpios; i++) {
        value += position[i] << i;
    }

    // 构造 firefly_sub_position 的值
    char buf[32];
    if (value <= 8) {
        sprintf(buf, "sub%02d", value);
    } else {
        int major = (value - 1) / 8 + 1;
        int minor = (value - 1) % 8 + 1;
        sprintf(buf, "sub%d-%02d", major, minor);
    }
    printf("Result: %s\n", buf);

    // 如果需要，将结果存储为环境变量
    env_set("firefly_sub_position", buf);

cleanup:
    // 释放 GPIO 和动态分配的内存
    for (i = 0; i < num_gpios; i++) {
        if (dm_gpio_is_valid(&gpiods[i])) {
            dm_gpio_free(NULL, &gpiods[i]);
        }
    }
    free(gpiods);
    free(position);

    return ret ? ret : 0;
}

U_BOOT_CMD(
    firefly_sub_detect, 1, 0, do_firefly_sub_detect,
    "Detect position using GPIO from device tree",
    " - Detects position based on GPIO inputs defined in the device tree"
);
